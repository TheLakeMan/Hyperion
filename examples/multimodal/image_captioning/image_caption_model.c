/**
 * @file image_caption_model.c
 * @brief Implementation of image captioning using multimodal model in Hyperion
 */

#include "image_caption_model.h"
#include "../../../core/memory.h"
#include "../../../models/text/tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Internal structure for image captioning model
 */
struct HyperionImageCaptionModel {
    HyperionMultimodalModel *model;        /* Underlying multimodal model */
    HyperionTokenizer       *tokenizer;    /* Tokenizer for text generation */
    int                    imageWidth;   /* Input image width */
    int                    imageHeight;  /* Input image height */
    int                    maxTokens;    /* Maximum tokens for caption */
    int                    textEmbedDim; /* Text embedding dimension */
    bool                   useSIMD;      /* Whether SIMD is enabled */
};

/**
 * Create an image captioning model
 */
HyperionImageCaptionModel *hyperionImageCaptionModelCreate(const HyperionImageCaptionConfig *config)
{
    if (!config) {
        fprintf(stderr, "Invalid configuration\n");
        return NULL;
    }

    /* Allocate model structure */
    HyperionImageCaptionModel *model =
        (HyperionImageCaptionModel *)malloc(sizeof(HyperionImageCaptionModel));
    if (!model) {
        fprintf(stderr, "Failed to allocate image captioning model\n");
        return NULL;
    }

    /* Initialize model structure */
    memset(model, 0, sizeof(HyperionImageCaptionModel));
    model->imageWidth   = config->imageWidth;
    model->imageHeight  = config->imageHeight;
    model->maxTokens    = config->maxTokenLength;
    model->textEmbedDim = config->textEmbedDim;
    model->useSIMD      = config->useSIMD;

    /* Load tokenizer */
    if (config->vocabFile) {
        model->tokenizer = hyperionTokenizerCreate(config->vocabFile);
        if (!model->tokenizer) {
            fprintf(stderr, "Failed to create tokenizer from %s\n", config->vocabFile);
            free(model);
            return NULL;
        }
    }
    else {
        fprintf(stderr, "No vocabulary file specified\n");
        free(model);
        return NULL;
    }

    /* Configure multimodal model parameters */
    HyperionMultimodalModelParams mmParams;
    memset(&mmParams, 0, sizeof(HyperionMultimodalModelParams));

    /* Set model type and fusion method */
    mmParams.modelType    = HYPERION_MULTIMODAL_CROSS_ATTN;
    mmParams.fusionMethod = HYPERION_FUSION_ATTENTION;
    mmParams.fusionDim    = config->fusionDim;
    mmParams.numLayers    = 2; /* Attention followed by fusion */

    /* Configure modalities */
    mmParams.numModalities   = 2; /* Text and image */
    mmParams.modalityConfigs = (HyperionModalityConfig *)malloc(2 * sizeof(HyperionModalityConfig));
    if (!mmParams.modalityConfigs) {
        fprintf(stderr, "Failed to allocate modality configurations\n");
        hyperionTokenizerFree(model->tokenizer);
        free(model);
        return NULL;
    }

    /* Configure text modality */
    mmParams.modalityConfigs[0].modality              = HYPERION_MODALITY_TEXT;
    mmParams.modalityConfigs[0].config.text.maxTokens = config->maxTokenLength;
    mmParams.modalityConfigs[0].config.text.embedDim  = config->textEmbedDim;

    /* Configure image modality */
    mmParams.modalityConfigs[1].modality              = HYPERION_MODALITY_IMAGE;
    mmParams.modalityConfigs[1].config.image.width    = config->imageWidth;
    mmParams.modalityConfigs[1].config.image.height   = config->imageHeight;
    mmParams.modalityConfigs[1].config.image.channels = 3; /* RGB */

    /* Set remaining parameters */
    mmParams.weightsFile     = config->weightsFile;
    mmParams.useQuantization = config->useQuantization;
    mmParams.useSIMD         = config->useSIMD;
    mmParams.customParams    = NULL;

    /* Create multimodal model */
    model->model = hyperionMultimodalModelCreate(&mmParams);
    if (!model->model) {
        fprintf(stderr, "Failed to create multimodal model\n");
        free(mmParams.modalityConfigs);
        hyperionTokenizerFree(model->tokenizer);
        free(model);
        return NULL;
    }

    /* Clean up */
    free(mmParams.modalityConfigs);

    return model;
}

/**
 * Free an image captioning model
 */
void hyperionImageCaptionModelFree(HyperionImageCaptionModel *model)
{
    if (!model) {
        return;
    }

    /* Free multimodal model */
    if (model->model) {
        hyperionMultimodalModelFree(model->model);
    }

    /* Free tokenizer */
    if (model->tokenizer) {
        hyperionTokenizerFree(model->tokenizer);
    }

    /* Free model structure */
    free(model);
}

/**
 * Generate a caption for an image
 */
bool hyperionImageCaptionGenerate(HyperionImageCaptionModel *model, const char *imagePath,
                                char *caption, int maxLength)
{
    if (!model || !imagePath || !caption || maxLength <= 0) {
        return false;
    }

    /* Load image */
    HyperionImage *image = hyperionImageLoadFromFile(imagePath);
    if (!image) {
        fprintf(stderr, "Failed to load image from %s\n", imagePath);
        return false;
    }

    /* Generate caption */
    bool success = hyperionImageCaptionGenerateFromImage(model, image, caption, maxLength);

    /* Clean up */
    hyperionImageFree(image);

    return success;
}

/**
 * Generate a caption for an image directly from image data
 */
bool hyperionImageCaptionGenerateFromImage(HyperionImageCaptionModel *model, const HyperionImage *image,
                                         char *caption, int maxLength)
{
    if (!model || !image || !caption || maxLength <= 0) {
        return false;
    }

    /* Preprocess image if needed */
    HyperionImage *processedImage = NULL;
    if (image->width != model->imageWidth || image->height != model->imageHeight) {
        processedImage = hyperionImageResize(image, model->imageWidth, model->imageHeight);
        if (!processedImage) {
            fprintf(stderr, "Failed to resize image\n");
            return false;
        }
    }
    else {
        processedImage = hyperionImageCopy(image);
        if (!processedImage) {
            fprintf(stderr, "Failed to copy image\n");
            return false;
        }
    }

    /* Prepare multimodal input */
    HyperionMultimodalInput mmInput;
    if (!hyperionMultimodalInputInit(&mmInput)) {
        fprintf(stderr, "Failed to initialize multimodal input\n");
        hyperionImageFree(processedImage);
        return false;
    }

    /* Set image input */
    mmInput.imageInput = processedImage;

    /* Initialize with start token - assuming we have a [START] token */
    int startToken = hyperionTokenizerEncode(model->tokenizer, "[START]", 7);
    if (startToken < 0) {
        /* If no specific start token, try using first token */
        startToken = 0;
    }

    int *tokens = (int *)malloc(sizeof(int));
    if (!tokens) {
        fprintf(stderr, "Failed to allocate token buffer\n");
        hyperionMultimodalInputFree(&mmInput, false);
        hyperionImageFree(processedImage);
        return false;
    }
    tokens[0]          = startToken;
    mmInput.textInput  = tokens;
    mmInput.textLength = 1;

    /* Prepare multimodal output */
    HyperionMultimodalOutput mmOutput;
    memset(&mmOutput, 0, sizeof(HyperionMultimodalOutput));
    if (!hyperionMultimodalOutputInit(&mmOutput, model->textEmbedDim, 1,
                                    hyperionTokenizerGetVocabSize(model->tokenizer), 0)) {
        fprintf(stderr, "Failed to initialize multimodal output\n");
        free(tokens);
        hyperionMultimodalInputFree(&mmInput, false);
        hyperionImageFree(processedImage);
        return false;
    }

    /* Process input */
    if (!hyperionMultimodalModelProcess(model->model, &mmInput, &mmOutput)) {
        fprintf(stderr, "Failed to process multimodal input\n");
        hyperionMultimodalOutputFree(&mmOutput);
        free(tokens);
        hyperionMultimodalInputFree(&mmInput, false);
        hyperionImageFree(processedImage);
        return false;
    }

    /* Generate caption using a simple greedy decoding */
    int captionTokens[256]; /* Maximum token buffer */
    captionTokens[0] = startToken;
    int numTokens    = 1;
    int endToken     = hyperionTokenizerEncode(model->tokenizer, "[END]", 5);
    if (endToken < 0) {
        endToken = hyperionTokenizerGetVocabSize(model->tokenizer) - 1; /* Default end token */
    }

    /* Generate tokens one by one */
    for (int i = 1; i < model->maxTokens && i < 256; i++) {
        /* Get logits from output */
        float *logits = mmOutput.textLogits;
        if (!logits) {
            break;
        }

        /* Find the token with maximum probability (greedy decoding) */
        int   maxToken = 0;
        float maxProb  = logits[0];
        for (int j = 1; j < hyperionTokenizerGetVocabSize(model->tokenizer); j++) {
            if (logits[j] > maxProb) {
                maxProb  = logits[j];
                maxToken = j;
            }
        }

        /* Add token to generated sequence */
        captionTokens[numTokens++] = maxToken;

        /* Check if we generated an end token */
        if (maxToken == endToken) {
            break;
        }

        /* Update input with generated token */
        free(tokens);
        tokens = (int *)malloc(numTokens * sizeof(int));
        if (!tokens) {
            break;
        }
        memcpy(tokens, captionTokens, numTokens * sizeof(int));
        mmInput.textInput  = tokens;
        mmInput.textLength = numTokens;

        /* Process updated input */
        hyperionMultimodalOutputFree(&mmOutput);
        if (!hyperionMultimodalOutputInit(&mmOutput, model->textEmbedDim, 1,
                                        hyperionTokenizerGetVocabSize(model->tokenizer), 0)) {
            break;
        }
        if (!hyperionMultimodalModelProcess(model->model, &mmInput, &mmOutput)) {
            break;
        }
    }

    /* Decode tokens to text */
    char *decodedText = hyperionTokenizerDecode(model->tokenizer, captionTokens, numTokens);
    if (decodedText) {
        /* Copy to output buffer, ensuring we don't exceed maxLength */
        strncpy(caption, decodedText, maxLength - 1);
        caption[maxLength - 1] = '\0';
        free(decodedText);
    }
    else {
        /* Fallback handling if decoding fails */
        strncpy(caption, "Failed to decode caption", maxLength - 1);
        caption[maxLength - 1] = '\0';
    }

    /* Clean up */
    hyperionMultimodalOutputFree(&mmOutput);
    free(tokens);
    hyperionMultimodalInputFree(&mmInput, false);
    hyperionImageFree(processedImage);

    return true;
}

/**
 * Get model's memory usage statistics
 */
bool hyperionImageCaptionModelGetMemoryUsage(const HyperionImageCaptionModel *model,
                                           size_t *weightMemory, size_t *activationMemory)
{
    if (!model || !weightMemory || !activationMemory) {
        return false;
    }

    /* Get memory usage from multimodal model */
    return hyperionMultimodalModelGetMemoryUsage(model->model, weightMemory, activationMemory);
}

/**
 * Enable or disable SIMD acceleration
 */
bool hyperionImageCaptionModelEnableSIMD(HyperionImageCaptionModel *model, bool enable)
{
    if (!model) {
        return false;
    }

    model->useSIMD = enable;
    return hyperionMultimodalModelEnableSIMD(model->model, enable);
}
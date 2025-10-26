/**
 * @file visual_qa_model.c
 * @brief Implementation of visual question answering using multimodal model in Hyperion
 */

#include "visual_qa_model.h"
#include "../../../core/memory.h"
#include "../../../models/text/tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Internal structure for visual QA model
 */
struct HyperionVisualQAModel {
    HyperionMultimodalModel *model;             /* Underlying multimodal model */
    HyperionTokenizer       *tokenizer;         /* Tokenizer for text processing */
    int                    imageWidth;        /* Input image width */
    int                    imageHeight;       /* Input image height */
    int                    maxQuestionLength; /* Maximum tokens for question */
    int                    maxAnswerLength;   /* Maximum tokens for answer */
    int                    textEmbedDim;      /* Text embedding dimension */
    bool                   useSIMD;           /* Whether SIMD is enabled */
};

/**
 * Create a visual question answering model
 */
HyperionVisualQAModel *hyperionVisualQAModelCreate(const HyperionVisualQAConfig *config)
{
    if (!config) {
        fprintf(stderr, "Invalid configuration\n");
        return NULL;
    }

    /* Allocate model structure */
    HyperionVisualQAModel *model = (HyperionVisualQAModel *)malloc(sizeof(HyperionVisualQAModel));
    if (!model) {
        fprintf(stderr, "Failed to allocate visual QA model\n");
        return NULL;
    }

    /* Initialize model structure */
    memset(model, 0, sizeof(HyperionVisualQAModel));
    model->imageWidth        = config->imageWidth;
    model->imageHeight       = config->imageHeight;
    model->maxQuestionLength = config->maxQuestionLength;
    model->maxAnswerLength   = config->maxAnswerLength;
    model->textEmbedDim      = config->textEmbedDim;
    model->useSIMD           = config->useSIMD;

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
    mmParams.numLayers    = 2; /* Cross-attention followed by fusion */

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
    mmParams.modalityConfigs[0].config.text.maxTokens = config->maxQuestionLength;
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
 * Free a visual QA model
 */
void hyperionVisualQAModelFree(HyperionVisualQAModel *model)
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
 * Answer a question about an image
 */
bool hyperionVisualQAGenerateAnswer(HyperionVisualQAModel *model, const char *imagePath,
                                  const char *question, char *answer, int maxLength)
{
    if (!model || !imagePath || !question || !answer || maxLength <= 0) {
        return false;
    }

    /* Load image */
    HyperionImage *image = hyperionImageLoadFromFile(imagePath);
    if (!image) {
        fprintf(stderr, "Failed to load image from %s\n", imagePath);
        return false;
    }

    /* Generate answer */
    bool success = hyperionVisualQAGenerateAnswerFromImage(model, image, question, answer, maxLength);

    /* Clean up */
    hyperionImageFree(image);

    return success;
}

/**
 * Answer a question about an image directly from image data
 */
bool hyperionVisualQAGenerateAnswerFromImage(HyperionVisualQAModel *model, const HyperionImage *image,
                                           const char *question, char *answer, int maxLength)
{
    if (!model || !image || !question || !answer || maxLength <= 0) {
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

    /* Tokenize the question */
    int *questionTokens = NULL;
    int  questionLength = 0;

    if (question && *question) {
        /* Convert question to tokens */
        questionTokens = hyperionTokenizerEncodeText(model->tokenizer, question, &questionLength);
        if (!questionTokens || questionLength == 0) {
            fprintf(stderr, "Failed to tokenize question: %s\n", question);
            hyperionImageFree(processedImage);
            return false;
        }

        /* Ensure question length doesn't exceed max */
        if (questionLength > model->maxQuestionLength) {
            questionLength = model->maxQuestionLength;
        }
    }
    else {
        fprintf(stderr, "Empty question\n");
        hyperionImageFree(processedImage);
        return false;
    }

    /* Prepare multimodal input */
    HyperionMultimodalInput mmInput;
    if (!hyperionMultimodalInputInit(&mmInput)) {
        fprintf(stderr, "Failed to initialize multimodal input\n");
        free(questionTokens);
        hyperionImageFree(processedImage);
        return false;
    }

    /* Set image input */
    mmInput.imageInput = processedImage;

    /* Set text input (question) */
    mmInput.textInput  = questionTokens;
    mmInput.textLength = questionLength;

    /* Prepare multimodal output */
    HyperionMultimodalOutput mmOutput;
    memset(&mmOutput, 0, sizeof(HyperionMultimodalOutput));
    if (!hyperionMultimodalOutputInit(&mmOutput, model->textEmbedDim, 1,
                                    hyperionTokenizerGetVocabSize(model->tokenizer), 0)) {
        fprintf(stderr, "Failed to initialize multimodal output\n");
        free(questionTokens);
        hyperionMultimodalInputFree(&mmInput, false);
        hyperionImageFree(processedImage);
        return false;
    }

    /* Process input */
    if (!hyperionMultimodalModelProcess(model->model, &mmInput, &mmOutput)) {
        fprintf(stderr, "Failed to process multimodal input\n");
        hyperionMultimodalOutputFree(&mmOutput);
        free(questionTokens);
        hyperionMultimodalInputFree(&mmInput, false);
        hyperionImageFree(processedImage);
        return false;
    }

    /* Generate answer using greedy decoding */
    int answerTokens[256]; /* Maximum token buffer */
    int numTokens = 0;

    /* Start token handling - assuming we have a [START] token */
    int startToken = hyperionTokenizerEncode(model->tokenizer, "[START]", 7);
    if (startToken < 0) {
        /* If no specific start token, try using first token */
        startToken = 0;
    }
    answerTokens[numTokens++] = startToken;

    /* End token handling */
    int endToken = hyperionTokenizerEncode(model->tokenizer, "[END]", 5);
    if (endToken < 0) {
        endToken = hyperionTokenizerGetVocabSize(model->tokenizer) - 1; /* Default end token */
    }

    /* Generate tokens one by one */
    int *generationTokens = (int *)malloc(sizeof(int) * (questionLength + numTokens));
    if (!generationTokens) {
        fprintf(stderr, "Failed to allocate tokens for generation\n");
        hyperionMultimodalOutputFree(&mmOutput);
        free(questionTokens);
        hyperionMultimodalInputFree(&mmInput, false);
        hyperionImageFree(processedImage);
        return false;
    }

    /* Copy initial tokens */
    memcpy(generationTokens, questionTokens, questionLength * sizeof(int));
    memcpy(generationTokens + questionLength, answerTokens, numTokens * sizeof(int));

    /* Update multimodal input */
    free(mmInput.textInput);
    mmInput.textInput  = generationTokens;
    mmInput.textLength = questionLength + numTokens;

    /* Generate answer tokens */
    for (int i = 0; i < model->maxAnswerLength && numTokens < 256; i++) {
        /* Process updated input */
        hyperionMultimodalOutputFree(&mmOutput);
        if (!hyperionMultimodalOutputInit(&mmOutput, model->textEmbedDim, 1,
                                        hyperionTokenizerGetVocabSize(model->tokenizer), 0)) {
            break;
        }
        if (!hyperionMultimodalModelProcess(model->model, &mmInput, &mmOutput)) {
            break;
        }

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
            }S
        }

        /* Add token to generated sequence */
        answerTokens[numTokens++] = maxToken;

        /* Check if we generated an end token */
        if (maxToken == endToken) {
            break;
        }

        /* Reallocate token buffer for next iteration */
        int *newTokens = (int *)malloc(sizeof(int) * (questionLength + numTokens));
        if (!newTokens) {
            break;
        }

        /* Copy tokens */
        memcpy(newTokens, questionTokens, questionLength * sizeof(int));
        memcpy(newTokens + questionLength, answerTokens, numTokens * sizeof(int));

        /* Update input */
        free(mmInput.textInput);
        mmInput.textInput  = newTokens;
        mmInput.textLength = questionLength + numTokens;
    }

    /* Decode answer tokens to text (skip the initial start token) */
    char *decodedText = hyperionTokenizerDecode(model->tokenizer, answerTokens + 1, numTokens - 1);
    if (decodedText) {
        /* Copy to output buffer, ensuring we don't exceed maxLength */
        strncpy(answer, decodedText, maxLength - 1);
        answer[maxLength - 1] = '\0';
        free(decodedText);
    }
    else {
        /* Fallback handling if decoding fails */
        strncpy(answer, "Failed to decode answer", maxLength - 1);
        answer[maxLength - 1] = '\0';
    }

    /* Clean up */
    hyperionMultimodalOutputFree(&mmOutput);
    free(questionTokens);
    free(mmInput.textInput); /* This is now using generationTokens */
    hyperionMultimodalInputFree(&mmInput, false);
    hyperionImageFree(processedImage);

    return true;
}

/**
 * Get model's memory usage statistics
 */
bool hyperionVisualQAModelGetMemoryUsage(const HyperionVisualQAModel *model, size_t *weightMemory,
                                       size_t *activationMemory)
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
bool hyperionVisualQAModelEnableSIMD(HyperionVisualQAModel *model, bool enable)
{
    if (!model) {
        return false;
    }

    model->useSIMD = enable;
    return hyperionMultimodalModelEnableSIMD(model->model, enable);
}
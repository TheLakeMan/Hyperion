# Hyperion Multimodal Capabilities

This document provides an overview of the multimodal capabilities in Hyperion, explaining how to use the API and providing examples of common use cases.

## Overview

Hyperion's multimodal system enables models to process and combine information from different modalities, such as text and images, to perform tasks that require understanding both types of data simultaneously. This is achieved through:

1. **Modality Encoders** - Process each input modality separately
2. **Fusion Methods** - Combine features from different modalities
3. **Cross-Attention** - Allow interactions between modalities
4. **Output Generation** - Produce unified outputs based on fused representations

## Supported Modalities

Currently, Hyperion supports the following modalities:

- **Text** - Text sequences processed as tokens
- **Image** - Image data processed using convolutional networks
- **Audio** - (Partial support, with full integration coming soon)

## Fusion Methods

Hyperion provides several methods to combine features from different modalities:

- **Concatenation** - Simple concatenation of feature vectors
- **Addition** - Element-wise addition of feature vectors
- **Multiplication** - Element-wise multiplication of feature vectors
- **Attention-based Fusion** - Using attention mechanisms to weight different modality features
- **Cross-Attention** - More sophisticated attention between different modalities

## API Overview

The multimodal API consists of the following components:

### Core Structures

- `HyperionMultimodalModel` - The main model structure
- `HyperionMultimodalInput` - Container for multimodal inputs
- `HyperionMultimodalOutput` - Container for multimodal outputs
- `HyperionModalityConfig` - Configuration for each modality
- `HyperionMultimodalModelParams` - Parameters for model creation

### Main Functions

- `hyperionMultimodalModelCreate()` - Create a new multimodal model
- `hyperionMultimodalModelProcess()` - Process multimodal inputs
- `hyperionMultimodalModelFree()` - Free a multimodal model
- `hyperionMultimodalInputInit()` - Initialize multimodal input structure
- `hyperionMultimodalOutputInit()` - Initialize multimodal output structure

### Fusion Operations

- `hyperionFusionConcat()` - Concatenation-based fusion
- `hyperionFusionAdd()` - Addition-based fusion
- `hyperionFusionMultiply()` - Multiplication-based fusion
- `hyperionFusionAttention()` - Attention-based fusion
- `hyperionFusionCrossAttention()` - Cross-attention between two modalities
- `hyperionFusionProject()` - Project features to a common dimension

## Example Use Cases

### Image Captioning

Image captioning uses a multimodal model to generate text descriptions of images. The process involves:

1. Encoding the image to extract visual features
2. Using these features as context for text generation
3. Generating tokens sequentially, conditioned on the image

See the example in `examples/multimodal/image_captioning/` for a complete implementation.

Usage example:

```c
// Configure image captioning model
HyperionImageCaptionConfig config;
memset(&config, 0, sizeof(config));
config.imageWidth = 224;
config.imageHeight = 224;
config.maxTokenLength = 64;
config.textEmbedDim = 512;
config.imageFeatureDim = 1024;
config.fusionDim = 512;
config.useQuantization = false;
config.useSIMD = true;
config.weightsFile = "path/to/weights.bin";
config.vocabFile = "path/to/vocab.txt";

// Create model
HyperionImageCaptionModel *model = hyperionImageCaptionModelCreate(&config);

// Generate caption
char caption[512];
bool success = hyperionImageCaptionGenerate(model, "path/to/image.jpg", caption, 512);
printf("Caption: %s\n", caption);

// Free resources
hyperionImageCaptionModelFree(model);
```

### Visual Question Answering (VQA)

VQA models answer questions about images by combining visual and textual understanding. The process involves:

1. Encoding both the image and the question text
2. Using cross-attention to relate text questions to image regions
3. Generating an answer based on the fused representation

See the example in `examples/multimodal/visual_qa/` for a complete implementation.

Usage example:

```c
// Configure visual QA model
HyperionVisualQAConfig config;
memset(&config, 0, sizeof(config));
config.imageWidth = 224;
config.imageHeight = 224;
config.maxQuestionLength = 64;
config.maxAnswerLength = 64;
config.textEmbedDim = 512;
config.imageFeatureDim = 1024;
config.fusionDim = 512;
config.useQuantization = false;
config.useSIMD = true;
config.weightsFile = "path/to/weights.bin";
config.vocabFile = "path/to/vocab.txt";

// Create model
HyperionVisualQAModel *model = hyperionVisualQAModelCreate(&config);

// Generate answer to a question about an image
char answer[512];
const char *question = "What color is the car?";
bool success = hyperionVisualQAGenerateAnswer(
    model, "path/to/image.jpg", question, answer, 512
);
printf("Question: %s\nAnswer: %s\n", question, answer);

// Free resources
hyperionVisualQAModelFree(model);
```

## Performance Considerations

### Memory Usage

Multimodal models typically require more memory than single-modality models due to:

1. Multiple feature extractors (one per modality)
2. Fusion and cross-attention mechanisms
3. Multiple activation buffers

You can reduce memory usage by:

- Using 4-bit quantization (set `useQuantization = true`)
- Reducing the model dimensions (`fusionDim`, `textEmbedDim`, etc.)
- Processing smaller inputs (lower resolution images, shorter text)

### Acceleration

Hyperion provides SIMD acceleration for multimodal operations:

- Set `useSIMD = true` when creating models
- Ensure your platform supports SSE2/AVX/AVX2 instructions
- Use `hyperionMultimodalModelEnableSIMD()` to toggle SIMD at runtime
- Check memory alignment for optimal performance

## Testing

We provide a comprehensive test suite for multimodal capabilities:

- `tests/test_multimodal.c` - Tests for model creation, fusion methods, etc.
- Run with `hyperion_tests multimodal` or the standalone executable `multimodal_test`

## Future Directions

Upcoming enhancements to the multimodal system include:

- Full audio modality support
- Improved multimodal attention mechanisms
- More fusion methods
- Support for video and time-series data
- More examples and pre-trained models
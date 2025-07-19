# Models API Reference

## Overview

The Models API provides functions for loading, managing, and using AI models in Hyperion, including text generation, image processing, and multimodal capabilities.

## Model Management

### `hyperion_load_model()`
```c
HyperionModel* hyperion_load_model(const char* path);
```
Loads a model from a file.

**Parameters:**
- `path`: Path to the model file

**Returns:**
- Pointer to loaded model or NULL on failure

**Example:**
```c
HyperionModel* model = hyperion_load_model("model.tmai");
if (!model) {
    printf("Error: %s\n", hyperion_get_error());
    return 1;
}
```

### `hyperion_free_model()`
```c
void hyperion_free_model(HyperionModel* model);
```
Frees a loaded model.

**Parameters:**
- `model`: Pointer to model to free

## Text Generation

### `hyperion_generate_text()`
```c
void hyperion_generate_text(HyperionModel* model, const char* prompt, char* output, size_t max_length, int max_tokens);
```
Generates text based on a prompt.

**Parameters:**
- `model`: Pointer to model
- `prompt`: Input prompt
- `output`: Buffer to store generated text
- `max_length`: Maximum output length
- `max_tokens`: Maximum number of tokens to generate

**Example:**
```c
const char* prompt = "Hyperion is";
char output[256];
hyperion_generate_text(model, prompt, output, sizeof(output), 50);
printf("Generated: %s\n", output);
```

### `hyperion_generate_text_with_config()`
```c
void hyperion_generate_text_with_config(HyperionModel* model, const char* prompt, char* output, size_t max_length, const HyperionGenerationConfig* config);
```
Generates text with custom configuration.

**Parameters:**
- `model`: Pointer to model
- `prompt`: Input prompt
- `output`: Buffer to store generated text
- `max_length`: Maximum output length
- `config`: Generation configuration

**Example:**
```c
HyperionGenerationConfig config = {
    .temperature = 0.7f,
    .top_k = 40,
    .top_p = 0.9f
};
hyperion_generate_text_with_config(model, prompt, output, sizeof(output), &config);
```

## Image Processing

### `hyperion_process_image()`
```c
void hyperion_process_image(HyperionModel* model, const HyperionImage* image, HyperionImage* output);
```
Processes an image using a model.

**Parameters:**
- `model`: Pointer to model
- `image`: Input image
- `output`: Output image

**Example:**
```c
HyperionImage input, output;
// Load input image
hyperion_process_image(model, &input, &output);
// Use processed image
```

## Multimodal Operations

### `hyperion_process_multimodal()`
```c
void hyperion_process_multimodal(HyperionModel* model, const HyperionMultimodalInput* input, HyperionMultimodalOutput* output);
```
Processes multimodal input (text and image).

**Parameters:**
- `model`: Pointer to model
- `input`: Multimodal input
- `output`: Multimodal output

**Example:**
```c
HyperionMultimodalInput input = {
    .text = "Describe this image",
    .image = &image
};
HyperionMultimodalOutput output;
hyperion_process_multimodal(model, &input, &output);
printf("Description: %s\n", output.text);
```

## Data Types

### `HyperionModel`
```c
typedef struct {
    void* internal;
    HyperionModelType type;
    size_t parameter_count;
} HyperionModel;
```
Model structure.

### `HyperionGenerationConfig`
```c
typedef struct {
    float temperature;
    int top_k;
    float top_p;
    int max_tokens;
} HyperionGenerationConfig;
```
Text generation configuration.

### `HyperionImage`
```c
typedef struct {
    int width;
    int height;
    int channels;
    uint8_t* data;
} HyperionImage;
```
Image structure.

### `HyperionMultimodalInput`
```c
typedef struct {
    const char* text;
    const HyperionImage* image;
} HyperionMultimodalInput;
```
Multimodal input structure.

### `HyperionMultimodalOutput`
```c
typedef struct {
    char* text;
    HyperionImage* image;
} HyperionMultimodalOutput;
```
Multimodal output structure.

### `HyperionModelType`
```c
typedef enum {
    HYPERION_MODEL_TEXT,
    HYPERION_MODEL_IMAGE,
    HYPERION_MODEL_MULTIMODAL
} HyperionModelType;
```
Model type enumeration.

## Best Practices

1. Check model loading success
2. Configure generation parameters appropriately
3. Handle memory management properly
4. Use appropriate model types
5. Monitor performance metrics
6. Clean up resources

## Common Patterns

### Text Generation
```c
// Load model
HyperionModel* model = hyperion_load_model("text_model.tmai");
if (!model) {
    printf("Error: %s\n", hyperion_get_error());
    return 1;
}

// Configure generation
HyperionGenerationConfig config = {
    .temperature = 0.7f,
    .top_k = 40,
    .top_p = 0.9f
};

// Generate text
const char* prompt = "Hyperion is";
char output[256];
hyperion_generate_text_with_config(model, prompt, output, sizeof(output), &config);
printf("Generated: %s\n", output);

// Clean up
hyperion_free_model(model);
```

### Image Processing
```c
// Load model
HyperionModel* model = hyperion_load_model("image_model.tmai");
if (!model) {
    printf("Error: %s\n", hyperion_get_error());
    return 1;
}

// Process image
HyperionImage input, output;
// Load input image
hyperion_process_image(model, &input, &output);
// Use processed image

// Clean up
hyperion_free_model(model);
```

### Multimodal Processing
```c
// Load model
HyperionModel* model = hyperion_load_model("multimodal_model.tmai");
if (!model) {
    printf("Error: %s\n", hyperion_get_error());
    return 1;
}

// Process multimodal input
HyperionMultimodalInput input = {
    .text = "Describe this image",
    .image = &image
};
HyperionMultimodalOutput output;
hyperion_process_multimodal(model, &input, &output);
printf("Description: %s\n", output.text);

// Clean up
hyperion_free_model(model);
```

## Related Documentation

- [Core API](core.md)
- [Memory Management API](memory.md)
- [Performance Tools API](performance.md)
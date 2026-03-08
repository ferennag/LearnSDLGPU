#include "core/shader.h"

#include <SDL3/SDL_log.h>

SDL_GPUShader *Shader_Load(SDL_GPUDevice *device,
                           const char *name,
                           int numSamplers,
                           int numStorageBuffers,
                           int numUniformBuffers,
                           int numStorageTextures) {
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderFormat selectedFormat = SDL_GPU_SHADERFORMAT_INVALID;
    SDL_GPUShaderStage stage = SDL_GPU_SHADERSTAGE_VERTEX;
    const char *entryPoint = "main";

    char fullPath[256] = {0};

    if (SDL_strstr(name, ".vert")) {
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
    } else if (SDL_strstr(name, ".frag")) {
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    } else {
        SDL_Log("Unknown shader stage: %s", name);
        return NULL;
    }

    if (formats & SDL_GPU_SHADERFORMAT_MSL) {
        selectedFormat = SDL_GPU_SHADERFORMAT_MSL;
        entryPoint = "main0";
        SDL_snprintf(fullPath, 256, "assets/shaders/compiled/msl/%s.msl", name);
    }

    if (selectedFormat == SDL_GPU_SHADERFORMAT_INVALID) {
        return NULL;
    }

    size_t codeSize = 0;
    uint8_t *code = SDL_LoadFile(fullPath, &codeSize);

    SDL_GPUShaderCreateInfo shaderCreateInfo = {};
    shaderCreateInfo.entrypoint = entryPoint;
    shaderCreateInfo.code = code;
    shaderCreateInfo.code_size = codeSize;
    shaderCreateInfo.format = selectedFormat;
    shaderCreateInfo.stage = stage;
    shaderCreateInfo.num_samplers = numSamplers;
    shaderCreateInfo.num_storage_buffers = numStorageBuffers;
    shaderCreateInfo.num_storage_textures = numStorageTextures;
    shaderCreateInfo.num_uniform_buffers = numUniformBuffers;

    SDL_GPUShader *shader = SDL_CreateGPUShader(device, &shaderCreateInfo);
    SDL_free(code);

    if (shader == NULL) {
        SDL_Log("Failed to load shader: %s - %s", name, SDL_GetError());
        return NULL;
    }

    return shader;
}

#pragma once

#include <SDL3/SDL_gpu.h>

typedef enum TextureType {
    TEXTURE_TYPE_BASE_COLOR,
    TEXTURE_TYPE_DIFFUSE,
    TEXTURE_TYPE_EMISSIVE,
    TEXTURE_TYPE_METALLIC,
    TEXTURE_TYPE_ROUGHNESS,
    TEXTURE_TYPE_NORMAL_MAP,
    TEXTURE_TYPE_SPECULAR_MAP,
    TEXTURE_TYPE_UNKNOWN,
} TextureType;

typedef struct Texture {
    TextureType type;
    SDL_GPUTexture *gpuTexture;
} Texture;

bool TextureManager_Init(SDL_GPUDevice *device);

void TextureManager_Shutdown();

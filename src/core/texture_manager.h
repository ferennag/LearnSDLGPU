#pragma once

#include <SDL3/SDL_gpu.h>
#include <cgltf.h>

typedef enum TextureType {
    TEXTURE_TYPE_BASE_COLOR,
    TEXTURE_TYPE_DIFFUSE,
    TEXTURE_TYPE_EMISSIVE,
    TEXTURE_TYPE_METALLIC_ROUGHNESS,
    TEXTURE_TYPE_SPECULAR_GLOSSINESS,
    TEXTURE_TYPE_AMBIENT_OCCLUSION,
    TEXTURE_TYPE_NORMAL_MAP,
    TEXTURE_TYPE_UNKNOWN,
} TextureType;

typedef struct Texture {
    TextureType type;
    SDL_GPUTexture *gpuTexture;

    unsigned char *pixels;
    int width, height, channels;
} Texture;

bool TextureManager_Init(SDL_GPUDevice *device);

void TextureManager_Shutdown();

Texture *TextureManager_LoadTexture(TextureType textureType, cgltf_texture_view *texureView);

bool TextureManager_UploadTexture(SDL_GPUCopyPass *copyPass, Texture *texture);

SDL_GPUTexture *TextureManager_GetFallbackTexture();

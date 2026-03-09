#include "core/texture_manager.h"

#include <SDL3/SDL.h>

#include "core/memory.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

typedef struct TextureManager {
    SDL_GPUDevice *device;

    unsigned int numTextures;
    unsigned int texturesCapacity;
    Texture *textures;
} TextureManager;

static TextureManager gTextureManager;

bool TextureManager_Init(SDL_GPUDevice *device) {
    gTextureManager.device = device;
    return true;
}

void TextureManager_AddTexture(Texture *texture) {
    if (gTextureManager.textures == NULL) {
        gTextureManager.texturesCapacity = 16;
        gTextureManager.numTextures = 0;
        gTextureManager.textures = Memory_AllocateArray(gTextureManager.texturesCapacity, sizeof(Texture));
    } else if (gTextureManager.numTextures >= gTextureManager.texturesCapacity) {
        gTextureManager.texturesCapacity *= 2;
        gTextureManager.textures =
            Memory_ReallocateArray(gTextureManager.textures, gTextureManager.texturesCapacity, sizeof(Texture));
    }

    gTextureManager.textures[gTextureManager.numTextures++] = *texture;
}

void TextureManager_ReleaseTexture(Texture *texture) {
    assert(texture != NULL);

    if (texture->gpuTexture != NULL) {
        SDL_ReleaseGPUTexture(gTextureManager.device, texture->gpuTexture);
    }
}

void TextureManager_Shutdown() {
    if (gTextureManager.textures != NULL) {
        for (int i = 0; i < gTextureManager.numTextures; ++i) {
            TextureManager_ReleaseTexture(&gTextureManager.textures[i]);
        }

        Memory_Free(gTextureManager.textures);
    }
}

// TODO: Steps needed:
// 1. Query all texture types we are interested in from the material
// 2. For each texture type, load the texture data, uncompress with stbi if needed
// 3. Create GPU textures and upload the pixel data to the GPU
// 4. Store the textures in a cache, so we only need to create each texture once

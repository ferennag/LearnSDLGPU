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
    if (texture == NULL) {
        return;
    }

    if (texture->pixels != NULL) {
        Memory_Free(texture->pixels);
    }

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
Texture *TextureManager_LoadTexture(TextureType textureType, cgltf_texture_view *texureView) {
    if (!texureView || !texureView->texture) {
        return NULL;
    }

    Texture *texture = Memory_Allocate(sizeof(Texture));
    texture->type = textureType;

    if (texureView->texture->image->buffer_view != NULL) {
        SDL_Log("Loading embedded texture: %s", texureView->texture->image->name);
        cgltf_buffer_view *bv = texureView->texture->image->buffer_view;
        const unsigned char *bytes = (unsigned char *)bv->buffer->data + bv->offset;

        int width, height, channels;
        unsigned char *pixels = stbi_load_from_memory(bytes, bv->buffer->size, &width, &height, &channels, 4);
        texture->pixels = Memory_AllocateArray(width * height, sizeof(unsigned char));
        texture->width = width;
        texture->height = height;
        texture->channels = 4;
        memcpy(texture->pixels, pixels, width * height);
        stbi_image_free(pixels);
        SDL_Log("Loading successful %s - %dx%dx%d",
                texureView->texture->image->name,
                texture->width,
                texture->height,
                texture->channels);
    }

    TextureManager_AddTexture(texture);

    return texture;
}

bool TextureManager_UploadTexture(SDL_GPUCopyPass *copyPass, Texture *texture) {
    if (texture == NULL) {
        return true;
    }

    if (texture->gpuTexture == NULL) {
        SDL_GPUTextureCreateInfo createInfo = {0};
        createInfo.type = SDL_GPU_TEXTURETYPE_2D;
        createInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        createInfo.width = texture->width;
        createInfo.height = texture->height;
        createInfo.layer_count_or_depth = 1;
        createInfo.num_levels = 1;
        createInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        SDL_GPUTexture *gpuTexture = SDL_CreateGPUTexture(gTextureManager.device, &createInfo);
        if (gpuTexture == NULL) {
            SDL_Log("Failed to create GPU texture: %s", SDL_GetError());
            return false;
        }

        texture->gpuTexture = gpuTexture;

        SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo = {0};
        transferBufferCreateInfo.size = texture->width * texture->height;
        transferBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        SDL_GPUTransferBuffer *transferBuffer =
            SDL_CreateGPUTransferBuffer(gTextureManager.device, &transferBufferCreateInfo);
        if (!transferBuffer) {
            SDL_Log("Failed to create GPU transfer buffer: %s", SDL_GetError());
            return false;
        }

        unsigned char *transferData = SDL_MapGPUTransferBuffer(gTextureManager.device, transferBuffer, false);
        memcpy(transferData, texture->pixels, texture->width * texture->height);
        SDL_UnmapGPUTransferBuffer(gTextureManager.device, transferBuffer);

        SDL_GPUTextureTransferInfo transferInfo = {0};
        transferInfo.pixels_per_row = texture->width;
        transferInfo.rows_per_layer = texture->height;
        transferInfo.transfer_buffer = transferBuffer;
        SDL_GPUTextureRegion destination = {0};
        destination.texture = texture->gpuTexture;
        destination.w = texture->width;
        destination.h = texture->height;

        SDL_UploadToGPUTexture(copyPass, &transferInfo, &destination, false);
        SDL_ReleaseGPUTransferBuffer(gTextureManager.device, transferBuffer);
        SDL_Log("Texture successfully uploaded");
    }

    return true;
}

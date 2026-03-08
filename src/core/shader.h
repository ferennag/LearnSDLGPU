#pragma once

#include <SDL3/SDL_gpu.h>

SDL_GPUShader *Shader_Load(SDL_GPUDevice *device,
                           const char *name,
                           int numSamplers,
                           int numStorageBuffers,
                           int numUniformBuffers,
                           int numStorageTextures);

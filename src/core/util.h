#pragma once

#include <SDL3/SDL_gpu.h>

SDL_GPUSampleCount GetHighestSupportedSampleCount(SDL_GPUDevice *device, SDL_Window *window);

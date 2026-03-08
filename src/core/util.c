#include "core/util.h"

#include <SDL3/SDL_log.h>

SDL_GPUSampleCount GetHighestSupportedSampleCount(SDL_GPUDevice *device, SDL_Window *window) {
    SDL_GPUTextureFormat format = SDL_GetGPUSwapchainTextureFormat(device, window);
    SDL_GPUSampleCount requestedSampleCounts[] = {
        SDL_GPU_SAMPLECOUNT_8,
        SDL_GPU_SAMPLECOUNT_4,
        SDL_GPU_SAMPLECOUNT_2,
        SDL_GPU_SAMPLECOUNT_1,
    };

    for (int i = 0; i < (sizeof(requestedSampleCounts) / sizeof(requestedSampleCounts[0])); ++i) {
        if (SDL_GPUTextureSupportsSampleCount(device, format, requestedSampleCounts[i])) {
            SDL_Log("Highest supported sample count (MSAA) = %dx", 2 << requestedSampleCounts[i]);
            return requestedSampleCounts[i];
        }
    }

    return 0;
}

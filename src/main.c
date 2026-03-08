#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

SDL_GPUShader *ShaderLoad(SDL_GPUDevice *device,
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

int main(int argc, char **argv) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Error initializing SDL: %s", SDL_GetError());
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("Learn SDL GPU",
                                          1024,
                                          768,
                                          SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    if (!window) {
        SDL_Log("Error creating window: %s", SDL_GetError());
        return -2;
    }

    SDL_GPUDevice *device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL, true, NULL);
    if (!device) {
        SDL_Log("Error creating GPU device: %s", SDL_GetError());
        return -3;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_Log("Failed to claim window for GPU device: %s", SDL_GetError());
        return -4;
    }

    SDL_GPUShader *basicVertexShader = ShaderLoad(device, "basic.vert", 0, 0, 0, 0);
    SDL_GPUShader *basicFragmentShader = ShaderLoad(device, "basic.frag", 0, 0, 0, 0);
    if (!basicVertexShader || !basicFragmentShader) {
        return -5;
    }

    SDL_GPUGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
        .target_info =
            {
                .num_color_targets = 1,
                .color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
                    .format = SDL_GetGPUSwapchainTextureFormat(device, window),
                }},
            },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = basicVertexShader,
        .fragment_shader = basicFragmentShader,
        .rasterizer_state =
            {
                .fill_mode = SDL_GPU_FILLMODE_FILL,
            },
    };

    SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(device, &graphicsPipelineCreateInfo);
    if (!pipeline) {
        SDL_Log("Failed to create graphics pipeline: %s", SDL_GetError());
        return -6;
    }

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    running = false;
                    break;
                }
                case SDL_EVENT_KEY_DOWN: {
                    switch (event.key.key) {
                        case SDLK_ESCAPE: {
                            running = false;
                            break;
                        }
                    }
                    break;
                }
            }
        }

        SDL_GPUCommandBuffer *commandBuffer = SDL_AcquireGPUCommandBuffer(device);
        if (!commandBuffer) {
            SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
            break;
        }

        SDL_GPUTexture *swapchainTexture = NULL;
        if (SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, NULL, NULL)) {
            SDL_GPUColorTargetInfo color = {0};
            color.texture = swapchainTexture;
            color.clear_color = (SDL_FColor){0.1f, 0.1, 0.2f};
            color.load_op = SDL_GPU_LOADOP_CLEAR;
            color.store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(commandBuffer, &color, 1, NULL);
            SDL_BindGPUGraphicsPipeline(pass, pipeline);
            SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
            SDL_EndGPURenderPass(pass);
        }
        SDL_SubmitGPUCommandBuffer(commandBuffer);
    }

    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_Quit();
    return 0;
}

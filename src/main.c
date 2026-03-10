#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cglm/cglm.h>
#include <stddef.h>

#include "core/memory.h"
#include "core/model.h"
#include "core/shader.h"
#include "core/texture_manager.h"
#include "core/util.h"

// TODO: projection matrix should not be part of the frame UBO, no need to send it to the GPU every frame
typedef struct FrameUbo {
    mat4 projection;
    mat4 view;
    mat4 model;
} FrameUbo;

SDL_GPUTexture *CreateRenderTargetTexture(SDL_GPUDevice *device, int sampleCount, int width, int height) {
    SDL_GPUTextureCreateInfo createInfo = {0};
    createInfo.type = SDL_GPU_TEXTURETYPE_2D;
    createInfo.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
    createInfo.width = width;
    createInfo.height = height;
    createInfo.layer_count_or_depth = 1;
    createInfo.num_levels = 1;
    createInfo.sample_count = sampleCount;
    createInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    SDL_GPUTexture *texture = SDL_CreateGPUTexture(device, &createInfo);
    return texture;
}

SDL_GPUTexture *CreateDepthTexture(SDL_GPUDevice *device, int sampleCount, int width, int height) {
    SDL_GPUTextureCreateInfo createInfo = {0};
    createInfo.type = SDL_GPU_TEXTURETYPE_2D;
    createInfo.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    createInfo.width = width;
    createInfo.height = height;
    createInfo.layer_count_or_depth = 1;
    createInfo.num_levels = 1;
    createInfo.sample_count = sampleCount;
    createInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    SDL_GPUTexture *texture = SDL_CreateGPUTexture(device, &createInfo);
    return texture;
}

int main(int argc, char **argv) {
    if (!Memory_Init()) {
        SDL_Log("Failed to initialize memory subsystem!");
        return -1;
    }

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

    if (!TextureManager_Init(device)) {
        SDL_Log("Failed to initialize texture manager!");
        return -5;
    }

    Model *car = NULL;
    if (!Model_Load("porsche2", &car)) {
        return -5;
    }

    if (!Model_UploadToGPU(car, device)) {
        return -5;
    }

    SDL_GPUShader *basicVertexShader = Shader_Load(device, "basic.vert", 0, 0, 2, 0);
    SDL_GPUShader *basicFragmentShader = Shader_Load(device, "basic.frag", 1, 0, 0, 0);
    if (!basicVertexShader || !basicFragmentShader) {
        return -5;
    }

    SDL_GPUTexture *colorTargetTexture = NULL;
    SDL_GPUTexture *depthTexture = NULL;

    SDL_GPUGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
        .target_info =
            {
                .num_color_targets = 1,
                .has_depth_stencil_target = true,
                .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
                .color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
                    .format = SDL_GetGPUSwapchainTextureFormat(device, window),
                }},
            },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_input_state =
            {
                .num_vertex_buffers = 1,
                .vertex_buffer_descriptions =
                    (SDL_GPUVertexBufferDescription[]){
                        {
                            .slot = 0,
                            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                            .instance_step_rate = 0,
                            .pitch = sizeof(ModelVertex),
                        },
                    },
                .num_vertex_attributes = 3,
                .vertex_attributes =
                    (SDL_GPUVertexAttribute[]){
                        {
                            .buffer_slot = 0,
                            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                            .location = 0,
                            .offset = 0,
                        },
                        {
                            .buffer_slot = 0,
                            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                            .location = 1,
                            .offset = offsetof(ModelVertex, normal),
                        },
                        {
                            .buffer_slot = 0,
                            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                            .location = 2,
                            .offset = offsetof(ModelVertex, texCoord),
                        },

                    },
            },
        .vertex_shader = basicVertexShader,
        .fragment_shader = basicFragmentShader,
        .depth_stencil_state =
            {
                .enable_depth_test = true,
                .enable_depth_write = true,
                .compare_op = SDL_GPU_COMPAREOP_LESS,
            },
        .rasterizer_state =
            {
                .fill_mode = SDL_GPU_FILLMODE_FILL,
                .cull_mode = SDL_GPU_CULLMODE_BACK,
                .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
            },
        .multisample_state =
            {
                .enable_alpha_to_coverage = false,
                .enable_mask = false,
                .sample_mask = 0,
                .sample_count = GetHighestSupportedSampleCount(device, window),
            },
    };

    SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(device, &graphicsPipelineCreateInfo);
    if (!pipeline) {
        SDL_Log("Failed to create graphics pipeline: %s", SDL_GetError());
        return -6;
    }

    SDL_GPUSamplerCreateInfo samplerCreateInfo = {0};
    samplerCreateInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    samplerCreateInfo.min_filter = SDL_GPU_FILTER_LINEAR;
    samplerCreateInfo.enable_anisotropy = true;
    samplerCreateInfo.max_anisotropy = 8;
    SDL_GPUSampler *sampler = SDL_CreateGPUSampler(device, &samplerCreateInfo);
    if (!sampler) {
        SDL_Log("Failed to create texture sampler: %s", SDL_GetError());
        return -6;
    }

    SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

    FrameUbo frameUbo = {};
    int windowWidth, windowHeight;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    glm_perspective(glm_rad(75.0f), (float)windowWidth / (float)windowHeight, 0.1f, 1000.0f, frameUbo.projection);
    glm_mat4_identity(frameUbo.view);
    glm_mat4_identity(frameUbo.model);
    glm_lookat((vec3){0.0f, 1.0f, 5.0f}, (vec3){0.0f, 0.0f, 0.0f}, (vec3){0.0f, 1.0f, 0.0f}, frameUbo.view);

    bool running = true;
    Uint64 previousFrame = SDL_GetTicks();

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
                case SDL_EVENT_WINDOW_RESIZED: {
                    glm_perspective(glm_rad(75.0f),
                                    (float)event.window.data1 / (float)event.window.data2,
                                    0.1f,
                                    1000.0f,
                                    frameUbo.projection);
                    break;
                }
            }
        }

        Uint64 currentFrame = SDL_GetTicks();
        float delta = (currentFrame - previousFrame) / 1000.0f;
        previousFrame = currentFrame;

        SDL_GPUCommandBuffer *commandBuffer = SDL_AcquireGPUCommandBuffer(device);
        if (!commandBuffer) {
            SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
            break;
        }

        // Update uniforms
        glm_rotate(frameUbo.model, glm_rad(10.0f * delta), (vec3){0.0f, 1.0f, 0.0f});
        SDL_PushGPUVertexUniformData(commandBuffer, 0, &frameUbo, sizeof(frameUbo));

        SDL_GPUTexture *swapchainTexture = NULL;
        unsigned int w, h;
        if (SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, &w, &h)) {
            if (!colorTargetTexture) {
                colorTargetTexture =
                    CreateRenderTargetTexture(device, GetHighestSupportedSampleCount(device, window), w, h);
            }
            if (!depthTexture) {
                depthTexture = CreateDepthTexture(device, GetHighestSupportedSampleCount(device, window), w, h);
            }

            SDL_GPUColorTargetInfo color = {0};
            color.texture = colorTargetTexture;
            color.clear_color = (SDL_FColor){0.1f, 0.1, 0.2f};
            color.load_op = SDL_GPU_LOADOP_CLEAR;
            color.store_op = SDL_GPU_STOREOP_RESOLVE_AND_STORE;
            color.resolve_texture = swapchainTexture;

            SDL_GPUDepthStencilTargetInfo depth = {0};
            depth.load_op = SDL_GPU_LOADOP_CLEAR;
            depth.store_op = SDL_GPU_STOREOP_STORE;
            depth.clear_depth = 1.0f;
            depth.texture = depthTexture;

            SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(commandBuffer, &color, 1, &depth);
            SDL_BindGPUGraphicsPipeline(pass, pipeline);
            Model_Render(car, commandBuffer, pass, sampler);

            SDL_EndGPURenderPass(pass);
        }
        SDL_SubmitGPUCommandBuffer(commandBuffer);
    }

    Model_Destroy(car, device);
    TextureManager_Shutdown();
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_Quit();
    Memory_Shutdown();
    return 0;
}

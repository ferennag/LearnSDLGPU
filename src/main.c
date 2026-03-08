#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cglm/cglm.h>
#include <stddef.h>

#include "core/shader.h"
#include "core/util.h"

typedef struct Vertex {
    float position[3];
    float normal[3];
} Vertex;

// TODO: projection matrix should not be part of the frame UBO, no need to send it to the GPU every frame
typedef struct FrameUbo {
    mat4 projection;
    mat4 view;
    mat4 model;
} FrameUbo;

static Vertex vertices[] = {
    // Front face (+Z)
    {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},

    {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    //
    // Back face (-Z)
    {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},

    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},

    // Left face (-X)
    {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}},
    {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}},
    {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}},

    {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}},
    {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}},
    {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}},

    // Right face (+X)
    {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},

    {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},

    // Top face (+Y)
    {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},

    {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},

    // Bottom face (-Y)
    {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}},
    {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}},

    {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}},
    {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}},
};

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

    SDL_GPUShader *basicVertexShader = Shader_Load(device, "basic.vert", 0, 0, 1, 0);
    SDL_GPUShader *basicFragmentShader = Shader_Load(device, "basic.frag", 0, 0, 0, 0);
    if (!basicVertexShader || !basicFragmentShader) {
        return -5;
    }

    SDL_GPUBufferCreateInfo bufferCreateInfo = {
        .size = sizeof(vertices),
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
    };

    SDL_GPUBuffer *vertexBuffer = SDL_CreateGPUBuffer(device, &bufferCreateInfo);
    SDL_GPUTransferBuffer *vertexTransferBuffer =
        SDL_CreateGPUTransferBuffer(device,
                                    &(SDL_GPUTransferBufferCreateInfo){
                                        .size = sizeof(vertices),
                                        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                    });

    if (!vertexBuffer || !vertexTransferBuffer) {
        SDL_Log("Failed create vertex buffer: %s", SDL_GetError());
        return -6;
    }

    Vertex *transferData = SDL_MapGPUTransferBuffer(device, vertexTransferBuffer, false);
    memcpy(transferData, vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(device, vertexTransferBuffer);

    SDL_GPUCommandBuffer *copyBuffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(copyBuffer);
    SDL_UploadToGPUBuffer(copyPass,
                          &(SDL_GPUTransferBufferLocation){
                              .transfer_buffer = vertexTransferBuffer,
                              .offset = 0,
                          },
                          &(SDL_GPUBufferRegion){
                              .buffer = vertexBuffer,
                              .offset = 0,
                              .size = sizeof(vertices),
                          },
                          false);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(copyBuffer);
    SDL_ReleaseGPUTransferBuffer(device, vertexTransferBuffer);

    SDL_GPUGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
        .target_info =
            {
                .num_color_targets = 1,
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
                            .pitch = sizeof(Vertex),
                        },
                    },
                .num_vertex_attributes = 2,
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
                            .offset = offsetof(Vertex, normal),
                        },

                    },
            },
        .vertex_shader = basicVertexShader,
        .fragment_shader = basicFragmentShader,
        .depth_stencil_state =
            {
                .enable_depth_test = true,
                .compare_op = SDL_GPU_COMPAREOP_GREATER_OR_EQUAL,
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

    SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

    FrameUbo frameUbo = {};
    int windowWidth, windowHeight;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    glm_perspective(glm_rad(75.0f), (float)windowWidth / (float)windowHeight, 0.1f, 1000.0f, frameUbo.projection);
    glm_mat4_identity(frameUbo.view);
    glm_mat4_identity(frameUbo.model);
    glm_lookat((vec3){0.0f, 0.0f, 5.0f}, (vec3){0.0f, 0.0f, 0.0f}, (vec3){0.0f, 1.0f, 0.0f}, frameUbo.view);

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
        glm_rotate(frameUbo.model, glm_rad(100.0f * delta), (vec3){1.0f, 1.0f, 0.0f});
        SDL_PushGPUVertexUniformData(commandBuffer, 0, &frameUbo, sizeof(frameUbo));

        SDL_GPUTexture *swapchainTexture = NULL;
        if (SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, NULL, NULL)) {
            SDL_GPUColorTargetInfo color = {0};
            color.texture = swapchainTexture;
            color.clear_color = (SDL_FColor){0.1f, 0.1, 0.2f};
            color.load_op = SDL_GPU_LOADOP_CLEAR;
            color.store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(commandBuffer, &color, 1, NULL);
            SDL_BindGPUGraphicsPipeline(pass, pipeline);
            SDL_BindGPUVertexBuffers(pass, 0, &(SDL_GPUBufferBinding){.buffer = vertexBuffer, .offset = 0}, 1);
            SDL_DrawGPUPrimitives(pass, (sizeof(vertices) / sizeof(Vertex)), 1, 0, 0);
            SDL_EndGPURenderPass(pass);
        }
        SDL_SubmitGPUCommandBuffer(commandBuffer);
    }

    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_Quit();
    return 0;
}

#pragma once

#include <SDL3/SDL_gpu.h>
#include <cglm/mat4.h>
#include <cglm/vec3.h>

#include "core/texture_manager.h"

typedef struct ModelVertex {
    vec3 position;
    vec3 normal;
    vec2 texCoord;
} ModelVertex;

typedef struct ModelMesh {
    unsigned int numVertices;
    ModelVertex *vertices;
    unsigned int numIndices;
    unsigned int *indices;

    SDL_GPUBuffer *vertexBuffer;
    size_t vertexBufferSize;

    SDL_GPUBuffer *indexBuffer;
    size_t indexBufferSize;

    mat4 transformation;
    unsigned int materialId;
} ModelMesh;

typedef enum ModelMaterialType {
    MATERIAL_PBR,
} ModelMaterialType;

typedef struct ModelMaterial {
    unsigned int id;
    ModelMaterialType type;

    Texture *baseColor;
    Texture *metallicRoughness;
    Texture *normal;
} ModelMaterial;

typedef struct Model {
    unsigned int meshCapacity;
    unsigned int numMeshes;
    ModelMesh *meshes;

    unsigned int numMaterials;
    ModelMaterial *materials;
} Model;

typedef struct ModelMeshUbo {
    mat4 meshTransform;
} ModelMeshUbo;

bool Model_Load(const char *name, Model **out);

void Model_Destroy(Model *model, SDL_GPUDevice *device);

bool Model_UploadToGPU(Model *model, SDL_GPUDevice *device);

void Model_Render(Model *model,
                  SDL_GPUCommandBuffer *commandBuffer,
                  SDL_GPURenderPass *renderPass,
                  SDL_GPUSampler *sampler);

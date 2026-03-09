#include "core/model.h"

#include <SDL3/SDL.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cglm/cglm.h>

#include "core/memory.h"

bool Model_Load(const char *name, Model **out) {
    char fullPath[255] = {0};
    // TODO: Need to support multiple file formats (.gltf, .obj, .glb, etc)
    SDL_snprintf(fullPath, 255, "assets/models/%s/%s.glb", name, name);

    // TODO: I am using aiProcess_PreTransformVertices here to transform all Meshes based on the transformations
    // This is okay for now, but maybe not okay for future use.
    // scene.mRootNode has the scene Graph with all the transformations.
    const struct aiScene *scene = aiImportFile(fullPath,
                                               aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_SortByPType |
                                                   aiProcess_ValidateDataStructure | aiProcess_PreTransformVertices);
    if (scene == NULL) {
        SDL_Log("Failed to load model %s from path %s", name, fullPath);
        return false;
    }

    SDL_Log("Loaded model %s:", name);
    SDL_Log("\t- number of meshes %d", scene->mNumMeshes);
    SDL_Log("\t- number of materials %d", scene->mNumMaterials);
    SDL_Log("\t- number of animations %d", scene->mNumAnimations);
    SDL_Log("\t- number of cameras %d", scene->mNumCameras);
    SDL_Log("\t- number of lights %d", scene->mNumLights);
    SDL_Log("\t- number of textures %d", scene->mNumTextures);
    SDL_Log("\t- number of skeletons %d", scene->mNumSkeletons);

    Model *result = Memory_Allocate(sizeof(Model));
    result->numMeshes = scene->mNumMeshes;
    result->meshes = Memory_AllocateArray(scene->mNumMeshes, sizeof(ModelMesh));

    size_t totalVertices = 0;
    size_t totalIndices = 0;
    for (int i = 0; i < scene->mNumMeshes; ++i) {
        struct aiMesh *mesh = scene->mMeshes[i];
        SDL_Log("\t- Mesh: '%s'", mesh->mName.data);
        totalVertices += mesh->mNumVertices;

        ModelMesh *modelMesh = &result->meshes[i];
        modelMesh->numVertices = mesh->mNumVertices;
        modelMesh->vertices = Memory_AllocateArray(mesh->mNumVertices, sizeof(ModelVertex));
        modelMesh->numIndices = 0;
        for (int v = 0; v < mesh->mNumVertices; ++v) {
            modelMesh->vertices[v].position[0] = mesh->mVertices[v].x;
            modelMesh->vertices[v].position[1] = mesh->mVertices[v].y;
            modelMesh->vertices[v].position[2] = mesh->mVertices[v].z;

            modelMesh->vertices[v].normal[0] = mesh->mNormals[v].x;
            modelMesh->vertices[v].normal[1] = mesh->mNormals[v].y;
            modelMesh->vertices[v].normal[2] = mesh->mNormals[v].z;
        }

        for (int f = 0; f < mesh->mNumFaces; ++f) {
            modelMesh->numIndices += mesh->mFaces[f].mNumIndices;
        }
        totalIndices += modelMesh->numIndices;

        modelMesh->indices = Memory_AllocateArray(modelMesh->numIndices, sizeof(unsigned int));
        int index = 0;
        for (int f = 0; f < mesh->mNumFaces; ++f) {
            for (int i = 0; i < mesh->mFaces[f].mNumIndices; ++i) {
                modelMesh->indices[index++] = mesh->mFaces[f].mIndices[i];
            }
        }
    }

    SDL_Log("\t- number of vertices %zu", totalVertices);
    SDL_Log("\t- number of indices %zu", totalIndices);
    aiReleaseImport(scene);

    *out = result;
    return true;
}

void Model_Destroy(Model *model, SDL_GPUDevice *device) {
    for (int i = 0; i < model->numMeshes; ++i) {
        if (model->meshes[i].vertexBuffer != NULL) {
            SDL_ReleaseGPUBuffer(device, model->meshes[i].vertexBuffer);
        }

        if (model->meshes[i].indexBuffer != NULL) {
            SDL_ReleaseGPUBuffer(device, model->meshes[i].indexBuffer);
        }

        Memory_Free(model->meshes[i].indices);
        Memory_Free(model->meshes[i].vertices);
    }

    Memory_Free(model->meshes);
    Memory_Free(model);
}

bool Model_UploadToGPU(Model *model, SDL_GPUDevice *device) {
    // TODO: copypass should probably come as an input, so we can copy multiple things at the same time efficiently
    SDL_GPUCommandBuffer *copyBuffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(copyBuffer);

    for (int i = 0; i < model->numMeshes; ++i) {
        ModelMesh *mesh = &model->meshes[i];
        if (!mesh->vertexBuffer) {
            mesh->vertexBufferSize = sizeof(mesh->vertices[0]) * mesh->numVertices;

            SDL_GPUBufferCreateInfo bufferCreateInfo = {
                .size = mesh->vertexBufferSize,
                .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            };

            SDL_GPUBuffer *vertexBuffer = SDL_CreateGPUBuffer(device, &bufferCreateInfo);
            if (!vertexBuffer) {
                SDL_Log("Failed create vertex buffer: %s", SDL_GetError());
                return false;
            }

            mesh->vertexBuffer = vertexBuffer;
        }

        if (!mesh->indexBuffer) {
            mesh->indexBufferSize = sizeof(unsigned int) * mesh->numIndices;

            SDL_GPUBufferCreateInfo bufferCreateInfo = {
                .size = mesh->indexBufferSize,
                .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            };

            SDL_GPUBuffer *indexBuffer = SDL_CreateGPUBuffer(device, &bufferCreateInfo);
            if (!indexBuffer) {
                SDL_Log("Failed create index buffer: %s", SDL_GetError());
                return false;
            }

            mesh->indexBuffer = indexBuffer;
        }

        SDL_GPUTransferBuffer *vertexTransferBuffer =
            SDL_CreateGPUTransferBuffer(device,
                                        &(SDL_GPUTransferBufferCreateInfo){
                                            .size = mesh->vertexBufferSize,
                                            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                        });

        ModelVertex *transferData = SDL_MapGPUTransferBuffer(device, vertexTransferBuffer, false);
        memcpy(transferData, mesh->vertices, mesh->vertexBufferSize);
        SDL_UnmapGPUTransferBuffer(device, vertexTransferBuffer);

        SDL_GPUTransferBuffer *indexTransferBuffer =
            SDL_CreateGPUTransferBuffer(device,
                                        &(SDL_GPUTransferBufferCreateInfo){
                                            .size = mesh->indexBufferSize,
                                            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                        });

        unsigned int *indexTransferData = SDL_MapGPUTransferBuffer(device, indexTransferBuffer, false);
        memcpy(indexTransferData, mesh->indices, mesh->indexBufferSize);
        SDL_UnmapGPUTransferBuffer(device, indexTransferBuffer);

        SDL_UploadToGPUBuffer(copyPass,
                              &(SDL_GPUTransferBufferLocation){
                                  .transfer_buffer = vertexTransferBuffer,
                                  .offset = 0,
                              },
                              &(SDL_GPUBufferRegion){
                                  .buffer = mesh->vertexBuffer,
                                  .offset = 0,
                                  .size = mesh->vertexBufferSize,
                              },
                              false);

        SDL_UploadToGPUBuffer(copyPass,
                              &(SDL_GPUTransferBufferLocation){
                                  .transfer_buffer = indexTransferBuffer,
                                  .offset = 0,
                              },
                              &(SDL_GPUBufferRegion){
                                  .buffer = mesh->indexBuffer,
                                  .offset = 0,
                                  .size = mesh->indexBufferSize,
                              },
                              false);
        SDL_ReleaseGPUTransferBuffer(device, vertexTransferBuffer);
        SDL_ReleaseGPUTransferBuffer(device, indexTransferBuffer);
    }

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(copyBuffer);
    return true;
}

void Model_Render(Model *model, SDL_GPURenderPass *renderPass) {
    for (int i = 0; i < model->numMeshes; ++i) {
        ModelMesh *mesh = &model->meshes[i];
        SDL_BindGPUVertexBuffers(renderPass, 0, &(SDL_GPUBufferBinding){.buffer = mesh->vertexBuffer, .offset = 0}, 1);
        SDL_BindGPUIndexBuffer(renderPass,
                               &(SDL_GPUBufferBinding){.buffer = mesh->indexBuffer, .offset = 0},
                               SDL_GPU_INDEXELEMENTSIZE_32BIT);
        SDL_DrawGPUIndexedPrimitives(renderPass, mesh->numIndices, 1, 0, 0, 0);
    }
}

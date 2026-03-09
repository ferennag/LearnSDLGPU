#include "core/model.h"

#include <SDL3/SDL.h>
#include <cglm/cglm.h>
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include "core/memory.h"
#include "core/texture_manager.h"

void Model_ParseNode(cgltf_node *node, Model *out) {
    if (node->mesh != NULL) {
        ModelMesh modelMesh = {0};

        cgltf_mesh *mesh = node->mesh;
        SDL_Log("\t\tMesh: %s", mesh->name);

        for (int p = 0; p < mesh->primitives_count; ++p) {
            cgltf_primitive *primitive = &mesh->primitives[p];
            if (primitive->type != cgltf_primitive_type_triangles) {
                SDL_LogError(0, "Primitive type not supported: %d", primitive->type);
                continue;
            }

            SDL_Log("\t\t\tPrimitive triangles: %d, attributes %zu, extensions %zu, targets %zu",
                    p,
                    primitive->attributes_count,
                    primitive->extensions_count,
                    primitive->targets_count);

            if (primitive->indices) {
                SDL_Log("\t\t\tPrimitive indices: %zu", primitive->indices->count);
            }

            cgltf_accessor *position = NULL;
            cgltf_accessor *normal = NULL;
            cgltf_accessor *tangent = NULL;
            cgltf_accessor *texcoord = NULL;
            cgltf_accessor *color = NULL;

            for (int a = 0; a < primitive->attributes_count; ++a) {
                cgltf_attribute *attribute = &primitive->attributes[a];
                switch (attribute->type) {
                    case cgltf_attribute_type_position:
                        position = attribute->data;
                        break;
                    case cgltf_attribute_type_normal:
                        normal = attribute->data;
                        break;
                    case cgltf_attribute_type_tangent:
                        tangent = attribute->data;
                        break;
                    case cgltf_attribute_type_texcoord:
                        texcoord = attribute->data;
                        break;
                    case cgltf_attribute_type_color:
                        color = attribute->data;
                        break;
                    default:
                        SDL_LogError(0, "Attribute type not supported: %d", attribute->type);
                        break;
                }
            }

            // We need at least positions
            if (!position) {
                SDL_LogError(0, "Position data required!");
                continue;
            }

            modelMesh.numVertices = position->count;
            SDL_Log("\t\t\tVertices: %zu", position->count);
            ModelVertex *vertices = Memory_AllocateArray(position->count, sizeof(ModelVertex));
            modelMesh.vertices = vertices;

            for (int i = 0; i < position->count; ++i) {
                cgltf_float tmp[3];
                if (!cgltf_accessor_read_float(position, i, tmp, 3)) {
                    break;
                }

                vertices[i].position[0] = tmp[0];
                vertices[i].position[1] = tmp[1];
                vertices[i].position[2] = tmp[2];

                if (normal != NULL) {
                    if (!cgltf_accessor_read_float(normal, i, tmp, 3)) {
                        break;
                    }

                    vertices[i].normal[0] = tmp[0];
                    vertices[i].normal[1] = tmp[1];
                    vertices[i].normal[2] = tmp[2];
                }
            }

            if (primitive->indices && primitive->indices->count > 0) {
                modelMesh.numIndices = primitive->indices->count;
                modelMesh.indices = Memory_AllocateArray(primitive->indices->count, sizeof(unsigned int));
                cgltf_accessor_unpack_indices(primitive->indices,
                                              modelMesh.indices,
                                              sizeof(unsigned int),
                                              primitive->indices->count);
            }

            cgltf_node_transform_world(node, (cgltf_float *)modelMesh.transformation);

            SDL_Log("\t\t\tMesh successfully created!");
            out->meshes[out->numMeshes++] = modelMesh;
        }
    }

    if (node->children_count > 0) {
        for (int c = 0; c < node->children_count; ++c) {
            cgltf_node *child = node->children[c];
            Model_ParseNode(child, out);
        }
    }
}

void Model_ParseScene(cgltf_scene *scene, Model *out) {
    for (int n = 0; n < scene->nodes_count; ++n) {
        cgltf_node *node = scene->nodes[n];
        SDL_Log("\tNode %d: %s", n, node->name);
        Model_ParseNode(node, out);
    }
}

void Model_ParseScenes(cgltf_data *data, Model *out) {
    SDL_Log("Buffers: %zu", data->buffers_count);

    for (int s = 0; s < data->scenes_count; ++s) {
        cgltf_scene scene = data->scenes[s];
        SDL_Log("Scene %d: %s", s, scene.name);

        Model_ParseScene(&scene, out);
    }
}

void Model_ParseMaterials(cgltf_data *data, Model *out) {
    out->materials = Memory_AllocateArray(data->materials_count, sizeof(ModelMaterial));
    out->numMaterials = data->materials_count;

    for (int m = 0; m < data->materials_count; ++m) {
        ModelMaterial result = {0};
        cgltf_material *material = &data->materials[m];

        if (material->has_pbr_metallic_roughness) {
            result.baseColor = TextureManager_LoadTexture(TEXTURE_TYPE_BASE_COLOR,
                                                          &material->pbr_metallic_roughness.base_color_texture);
            result.metallicRoughness =
                TextureManager_LoadTexture(TEXTURE_TYPE_METALLIC_ROUGHNESS,
                                           &material->pbr_metallic_roughness.metallic_roughness_texture);
        }

        result.normal = TextureManager_LoadTexture(TEXTURE_TYPE_NORMAL_MAP, &material->normal_texture);
        out->materials[m] = result;
    }
}

bool Model_Load(const char *name, Model **out) {
    char fullPath[255] = {0};
    // TODO: Need to support multiple file formats (.gltf, .obj, .glb, etc)
    SDL_snprintf(fullPath, 255, "assets/models/%s/%s.glb", name, name);

    cgltf_options options = {0};
    cgltf_data *data = NULL;
    if (cgltf_parse_file(&options, fullPath, &data) != cgltf_result_success) {
        SDL_Log("Failed to load model: %s", fullPath);
        return false;
    }

    if (cgltf_validate(data) != cgltf_result_success) {
        SDL_Log("CGLTF validation failed: %s", fullPath);
        cgltf_free(data);
        return false;
    }

    if (cgltf_load_buffers(&options, data, fullPath) != cgltf_result_success) {
        SDL_Log("CGLTF buffer loading failed: %s", fullPath);
        cgltf_free(data);
        return false;
    }

    SDL_Log("Loaded %s with %zu scenes", fullPath, data->scenes_count);
    Model *result = Memory_Allocate(sizeof(Model));
    result->meshes = Memory_AllocateArray(data->meshes_count, sizeof(ModelMesh));
    result->meshCapacity = data->meshes_count;
    result->numMeshes = 0;
    Model_ParseScenes(data, result);
    Model_ParseMaterials(data, result);

    *out = result;
    cgltf_free(data);
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

    if (model->materials != NULL) {
        Memory_Free(model->materials);
    }

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

    SDL_Log("Uploading textures");
    for (int i = 0; i < model->numMaterials; ++i) {
        ModelMaterial *material = &model->materials[i];
        TextureManager_UploadTexture(copyPass, material->baseColor);
        TextureManager_UploadTexture(copyPass, material->metallicRoughness);
        TextureManager_UploadTexture(copyPass, material->normal);
    }

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(copyBuffer);
    return true;
}

void Model_Render(Model *model, SDL_GPUCommandBuffer *commandBuffer, SDL_GPURenderPass *renderPass) {
    for (int i = 0; i < model->numMeshes; ++i) {
        ModelMesh *mesh = &model->meshes[i];
        ModelMeshUbo meshUbo = {0};
        glm_mat4_copy(mesh->transformation, meshUbo.meshTransform);

        SDL_PushGPUVertexUniformData(commandBuffer, 1, &meshUbo, sizeof(meshUbo));
        SDL_BindGPUVertexBuffers(renderPass, 0, &(SDL_GPUBufferBinding){.buffer = mesh->vertexBuffer, .offset = 0}, 1);
        SDL_BindGPUIndexBuffer(renderPass,
                               &(SDL_GPUBufferBinding){.buffer = mesh->indexBuffer, .offset = 0},
                               SDL_GPU_INDEXELEMENTSIZE_32BIT);
        SDL_DrawGPUIndexedPrimitives(renderPass, mesh->numIndices, 1, 0, 0, 0);
    }
}

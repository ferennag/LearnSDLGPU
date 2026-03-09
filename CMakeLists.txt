cmake_minimum_required(VERSION 4.0)
project(learn_sdl_gpu LANGUAGES C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 23)
include(FetchContent)

set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL
    GIT_TAG release-3.4.0
)
FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(cglm
    GIT_REPOSITORY https://github.com/recp/cglm
    GIT_TAG v0.9.6
)
FetchContent_MakeAvailable(cglm)

FetchContent_Declare(assimp
    GIT_REPOSITORY https://github.com/assimp/assimp
    GIT_TAG v6.0.4
)
FetchContent_MakeAvailable(assimp)

file(GLOB_RECURSE SOURCES src/*.c src/*.h)

add_executable(${PROJECT_NAME} ${SOURCES})
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wpedantic -g)
target_include_directories(${PROJECT_NAME} PRIVATE src/ ${PROJECT_SOURCE_DIR}/lib/shadercross/include)
target_link_libraries(${PROJECT_NAME} PRIVATE cglm SDL3::SDL3 ${PROJECT_SOURCE_DIR}/lib/shadercross/libSDL3_shadercross.a assimp)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets)

set(SHADER_SOURCE_DIR "${PROJECT_SOURCE_DIR}/assets/shaders/sources")
set(SHADER_OUTPUT_DIR "${PROJECT_SOURCE_DIR}/assets/shaders/compiled")
set(SHADER_OUTPUT_SPIRV_DIR "${SHADER_OUTPUT_DIR}/spirv")
set(SHADER_OUTPUT_MSL_DIR   "${SHADER_OUTPUT_DIR}/msl")
set(SHADER_OUTPUT_DXIL_DIR  "${SHADER_OUTPUT_DIR}/dxil")
set(SHADERCROSS_EXECUTABLE
    "${PROJECT_SOURCE_DIR}/lib/shadercross/shadercross"
    CACHE FILEPATH "Path to shadercross executable"
)

if(NOT EXISTS "${SHADERCROSS_EXECUTABLE}")
    message(FATAL_ERROR "shadercross not found: ${SHADERCROSS_EXECUTABLE}")
endif()

file(GLOB_RECURSE HLSL_SHADERS CONFIGURE_DEPENDS
    "${SHADER_SOURCE_DIR}/*.hlsl.*"
)

set(ALL_SHADER_OUTPUTS)
foreach(HLSL_FILE IN LISTS HLSL_SHADERS)
    # Path relative to shader source root, so subdirectories are preserved
    file(RELATIVE_PATH REL_PATH "${SHADER_SOURCE_DIR}" "${HLSL_FILE}")

    # Example:
    #   REL_PATH = post/fxaa.vert.hlsl
    #   STEM     = post/fxaa.vert
    get_filename_component(REL_DIR  "${REL_PATH}" DIRECTORY)
    get_filename_component(FILE_NAME "${REL_PATH}" NAME)
    string(REGEX REPLACE "\\.hlsl" "" STEM "${REL_PATH}")

    # Outputs
    set(SPIRV_OUT "${SHADER_OUTPUT_SPIRV_DIR}/${STEM}.spv")
    set(MSL_OUT   "${SHADER_OUTPUT_MSL_DIR}/${STEM}.msl")
    set(DXIL_OUT  "${SHADER_OUTPUT_DXIL_DIR}/${STEM}.dxil")

    # Make sure output subdirectories exist before running shadercross
    get_filename_component(SPIRV_OUT_DIR "${SPIRV_OUT}" DIRECTORY)
    get_filename_component(MSL_OUT_DIR   "${MSL_OUT}" DIRECTORY)
    get_filename_component(DXIL_OUT_DIR  "${DXIL_OUT}" DIRECTORY)

    # Vulkan / SPIR-V
    add_custom_command(
        OUTPUT "${SPIRV_OUT}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SPIRV_OUT_DIR}"
        COMMAND "${SHADERCROSS_EXECUTABLE}"
                "${HLSL_FILE}"
                --dest SPIRV
                --output "${SPIRV_OUT}"
        DEPENDS "${HLSL_FILE}"
        COMMENT "Compiling shader to SPIR-V: ${REL_PATH}"
        VERBATIM
    )

    # Metal / MSL
    add_custom_command(
        OUTPUT "${MSL_OUT}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${MSL_OUT_DIR}"
        COMMAND "${SHADERCROSS_EXECUTABLE}"
                "${HLSL_FILE}"
                --dest MSL
                --output "${MSL_OUT}"
        DEPENDS "${HLSL_FILE}"
        COMMENT "Compiling shader to MSL: ${REL_PATH}"
        VERBATIM
    )

    # DirectX / DXIL
    add_custom_command(
        OUTPUT "${DXIL_OUT}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DXIL_OUT_DIR}"
        COMMAND "${SHADERCROSS_EXECUTABLE}"
                "${HLSL_FILE}"
                --dest DXIL
                --output "${DXIL_OUT}"
        DEPENDS "${HLSL_FILE}"
        COMMENT "Compiling shader to DXIL: ${REL_PATH}"
        VERBATIM
    )

    list(APPEND ALL_SHADER_OUTPUTS
        "${SPIRV_OUT}"
        "${MSL_OUT}"
        "${DXIL_OUT}"
    )
endforeach()

add_custom_target(compiled_shaders ALL DEPENDS ${ALL_SHADER_OUTPUTS})
add_dependencies(${PROJECT_NAME} compiled_shaders)

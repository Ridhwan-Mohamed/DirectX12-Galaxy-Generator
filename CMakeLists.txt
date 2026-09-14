cmake_minimum_required(VERSION 3.25)

project(GravityForge
    VERSION 0.1.0
    LANGUAGES CXX
)

add_executable(GravityForge WIN32
    src/main.cpp
    src/app/Application.cpp
    src/app/Application.h
    src/app/Window.cpp
    src/app/Window.h

    src/core/Log.cpp
    src/core/Log.h
    src/core/Win32Error.h
    src/core/HResult.h 
    src/graphics/d3d12/D3D12Context.h 
    src/graphics/d3d12/D3D12Context.cpp
    src/graphics/frame/FrameResource.h 
    src/graphics/frame/FrameResource.cpp
    src/graphics/pipeline/BasicPipeline.h 
    src/graphics/pipeline/BasicPipeline.cpp
    src/graphics/mesh/mesh.h
    src/graphics/mesh/mesh.cpp
    src/scene/camera.h
    src/scene/camera.cpp
    src/graphics/texture/Rgba8Image.h
    src/graphics/texture/Rgba8Image.cpp 
    src/graphics/texture/Texture2D.h
    src/graphics/texture/Texture2D.cpp
    src/core/ComApartment.h
    src/graphics/texture/WicImageLoader.h
    src/graphics/texture/WicImageLoader.cpp
    src/graphics/material/Material.h
    src/graphics/mesh/MeshData.h 
    src/graphics/mesh/PrimitiveMeshes.h
    src/graphics/mesh/PrimitiveMeshes.cpp
    src/scene/ReactorScene.h
    src/scene/ReactorScene.cpp
    src/graphics/pipeline/PresentationPipeline.h
    src/graphics/pipeline/PresentationPipeline.cpp
    src/simulation/ParticleState.h
    src/simulation/ParticleSystem.h
    src/simulation/ParticleSystem.cpp
    src/graphics/pipeline/ParticleSimulationPipeline.h
    src/graphics/pipeline/ParticleSimulationPipeline.cpp
    src/graphics/pipeline/ParticleDebugPipeline.h
    src/graphics/pipeline/ParticleDebugPipeline.cpp
    src/graphics/pipeline/ParticleBillboardPipeline.h
    src/graphics/pipeline/ParticleBillboardPipeline.cpp
    src/scene/DebrisField.h
    src/scene/DebrisField.cpp
    src/graphics/pipeline/DebrisCullingPipeline.h
    src/graphics/pipeline/DebrisCullingPipeline.cpp
    src/graphics/pipeline/BloomExtractPipeline.h
    src/graphics/pipeline/BloomExtractPipeline.cpp
    src/graphics/pipeline/BloomBlurPipeline.h
    src/graphics/pipeline/BloomBlurPipeline.cpp 
    src/simulation/GalaxyParameters.h
)

find_program(DXC_EXECUTABLE
    NAMES
        dxc.exe
        dxc
)

if(NOT DXC_EXECUTABLE)
    set(WINDOWS_KITS_BIN
        "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin"
    )

    file(GLOB DXC_CANDIDATES
        "${WINDOWS_KITS_BIN}/*/x64/dxc.exe"
    )

    list(
        SORT DXC_CANDIDATES
        COMPARE NATURAL
        ORDER DESCENDING
    )

    if(DXC_CANDIDATES)
        list(
            GET DXC_CANDIDATES
            0
            DXC_EXECUTABLE
        )
    endif()
endif()

if(NOT DXC_EXECUTABLE)
    message(FATAL_ERROR
        "DXC was not found. Install a Windows 10/11 SDK "
        "with DirectX Shader Compiler support."
    )
endif()

message(STATUS
    "Using DXC: ${DXC_EXECUTABLE}"
)

set(DEBRIS_COMMON_SHADER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/DebrisCommon.hlsli"
)

set(BASIC_SHADER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/Basic.hlsl"
)

set(GENERATED_SHADER_DIRECTORY
    "${CMAKE_CURRENT_BINARY_DIR}/generated/shaders"
)

set(EMISSIVE_SHADER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/Emissive.hlsl"
)

set(HDR_PRESENT_SHADER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/HdrPresent.hlsl"
)

set(HDR_PRESENT_VERTEX_SHADER_HEADER
    "${GENERATED_SHADER_DIRECTORY}/HdrPresentVS.h"
)

set(DEBRIS_SHADER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/Debris.hlsl"
)

set(DEBRIS_VERTEX_SHADER_HEADER
    "${GENERATED_SHADER_DIRECTORY}/DebrisVS.h"
)

set(DEBRIS_PIXEL_SHADER_HEADER
    "${GENERATED_SHADER_DIRECTORY}/DebrisPS.h"
)

set(PARTICLE_SIM_SHADER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/ParticleSim.hlsl"
)

set(PARTICLE_SIM_COMPUTE_SHADER_HEADER
    "${GENERATED_SHADER_DIRECTORY}/ParticleSimCS.h"
)

set(PARTICLE_DEBUG_SHADER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/ParticleDebug.hlsl"
)

set(PARTICLE_DEBUG_VERTEX_SHADER_HEADER
    "${GENERATED_SHADER_DIRECTORY}/ParticleDebugVS.h"
)

set(PARTICLE_DEBUG_PIXEL_SHADER_HEADER
    "${GENERATED_SHADER_DIRECTORY}/ParticleDebugPS.h"
)

set(PARTICLE_BILLBOARD_SHADER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/ParticleBillboard.hlsl"
)

set(PARTICLE_BILLBOARD_VERTEX_SHADER_HEADER
    "${GENERATED_SHADER_DIRECTORY}/ParticleBillboardVS.h"
)

set(PARTICLE_BILLBOARD_PIXEL_SHADER_HEADER
    "${GENERATED_SHADER_DIRECTORY}/ParticleBillboardPS.h"
)

set(HDR_PRESENT_PIXEL_SHADER_HEADER
    "${GENERATED_SHADER_DIRECTORY}/HdrPresentPS.h"
)

set(EMISSIVE_PIXEL_SHADER_HEADER
    "${GENERATED_SHADER_DIRECTORY}/EmissivePS.h"
)

set(BASIC_VERTEX_SHADER_HEADER
    "${GENERATED_SHADER_DIRECTORY}/BasicVS.h"
)

set(BASIC_PIXEL_SHADER_HEADER
    "${GENERATED_SHADER_DIRECTORY}/BasicPS.h"
)

set(DEBRIS_CULL_SHADER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/DebrisCull.hlsl"
)

set(DEBRIS_CULL_COMPUTE_SHADER_HEADER
    "${GENERATED_SHADER_DIRECTORY}/DebrisCullCS.h"
)

set(BLOOM_EXTRACT_SHADER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/BloomExtract.hlsl"
)

set(BLOOM_EXTRACT_VERTEX_SHADER_HEADER
    "${GENERATED_SHADER_DIRECTORY}/BloomExtractVS.h"
)

set(BLOOM_EXTRACT_PIXEL_SHADER_HEADER
    "${GENERATED_SHADER_DIRECTORY}/BloomExtractPS.h"
)

set(BLOOM_BLUR_SHADER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/BloomBlur.hlsl"
)

set(BLOOM_BLUR_VERTEX_SHADER_HEADER
    "${GENERATED_SHADER_DIRECTORY}/BloomBlurVS.h"
)

set(BLOOM_BLUR_PIXEL_SHADER_HEADER
    "${GENERATED_SHADER_DIRECTORY}/BloomBlurPS.h"
)

add_custom_command(
    OUTPUT
        "${BLOOM_BLUR_VERTEX_SHADER_HEADER}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${GENERATED_SHADER_DIRECTORY}"

    COMMAND
        "${DXC_EXECUTABLE}"
        -T vs_6_0
        -E VSMain
        -HV 2021
        -Zi
        -Qembed_debug
        -WX
        -Fh "${BLOOM_BLUR_VERTEX_SHADER_HEADER}"
        -Vn g_BloomBlurVertexShader
        "${BLOOM_BLUR_SHADER_SOURCE}"

    DEPENDS
        "${BLOOM_BLUR_SHADER_SOURCE}"

    COMMENT
        "Compiling bloom blur vertex shader"

    VERBATIM
)

add_custom_command(
    OUTPUT
        "${BLOOM_BLUR_PIXEL_SHADER_HEADER}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${GENERATED_SHADER_DIRECTORY}"

    COMMAND
        "${DXC_EXECUTABLE}"
        -T ps_6_0
        -E PSMain
        -HV 2021
        -Zi
        -Qembed_debug
        -WX
        -Fh "${BLOOM_BLUR_PIXEL_SHADER_HEADER}"
        -Vn g_BloomBlurPixelShader
        "${BLOOM_BLUR_SHADER_SOURCE}"

    DEPENDS
        "${BLOOM_BLUR_SHADER_SOURCE}"

    COMMENT
        "Compiling bloom blur pixel shader"

    VERBATIM
)

add_custom_command(
    OUTPUT
        "${BLOOM_EXTRACT_VERTEX_SHADER_HEADER}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${GENERATED_SHADER_DIRECTORY}"

    COMMAND
        "${DXC_EXECUTABLE}"
        -T vs_6_0
        -E VSMain
        -HV 2021
        -Zi
        -Qembed_debug
        -WX
        -Fh "${BLOOM_EXTRACT_VERTEX_SHADER_HEADER}"
        -Vn g_BloomExtractVertexShader
        "${BLOOM_EXTRACT_SHADER_SOURCE}"

    DEPENDS
        "${BLOOM_EXTRACT_SHADER_SOURCE}"

    COMMENT
        "Compiling bloom extraction vertex shader"

    VERBATIM
)

add_custom_command(
    OUTPUT
        "${BLOOM_EXTRACT_PIXEL_SHADER_HEADER}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${GENERATED_SHADER_DIRECTORY}"

    COMMAND
        "${DXC_EXECUTABLE}"
        -T ps_6_0
        -E PSMain
        -HV 2021
        -Zi
        -Qembed_debug
        -WX
        -Fh "${BLOOM_EXTRACT_PIXEL_SHADER_HEADER}"
        -Vn g_BloomExtractPixelShader
        "${BLOOM_EXTRACT_SHADER_SOURCE}"

    DEPENDS
        "${BLOOM_EXTRACT_SHADER_SOURCE}"

    COMMENT
        "Compiling bloom extraction pixel shader"

    VERBATIM
)

add_custom_command(
    OUTPUT
        "${DEBRIS_CULL_COMPUTE_SHADER_HEADER}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${GENERATED_SHADER_DIRECTORY}"

    COMMAND
        "${DXC_EXECUTABLE}"
        -T cs_6_0
        -E CSMain
        -HV 2021
        -Zi
        -Qembed_debug
        -WX
        -Fh "${DEBRIS_CULL_COMPUTE_SHADER_HEADER}"
        -Vn g_DebrisCullComputeShader
        "${DEBRIS_CULL_SHADER_SOURCE}"

    DEPENDS
        "${DEBRIS_CULL_SHADER_SOURCE}"
        "${DEBRIS_COMMON_SHADER_SOURCE}"

    COMMENT
        "Compiling debris culling compute shader"

    VERBATIM
)

add_custom_command(
    OUTPUT
        "${DEBRIS_VERTEX_SHADER_HEADER}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${GENERATED_SHADER_DIRECTORY}"

    COMMAND
        "${DXC_EXECUTABLE}"
        -T vs_6_0
        -E VSMain
        -HV 2021
        -Zi
        -Qembed_debug
        -WX
        -Fh "${DEBRIS_VERTEX_SHADER_HEADER}"
        -Vn g_DebrisVertexShader
        "${DEBRIS_SHADER_SOURCE}"

    DEPENDS
        "${DEBRIS_SHADER_SOURCE}"
        "${DEBRIS_COMMON_SHADER_SOURCE}"

    COMMENT
        "Compiling debris vertex shader"

    VERBATIM
)

add_custom_command(
    OUTPUT
        "${DEBRIS_PIXEL_SHADER_HEADER}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${GENERATED_SHADER_DIRECTORY}"

    COMMAND
        "${DXC_EXECUTABLE}"
        -T ps_6_0
        -E PSMain
        -HV 2021
        -Zi
        -Qembed_debug
        -WX
        -Fh "${DEBRIS_PIXEL_SHADER_HEADER}"
        -Vn g_DebrisPixelShader
        "${DEBRIS_SHADER_SOURCE}"

    DEPENDS
        "${DEBRIS_SHADER_SOURCE}"
        "${DEBRIS_COMMON_SHADER_SOURCE}"

    COMMENT
        "Compiling debris pixel shader"

    VERBATIM
)

add_custom_command(
    OUTPUT
        "${PARTICLE_BILLBOARD_VERTEX_SHADER_HEADER}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${GENERATED_SHADER_DIRECTORY}"

    COMMAND
        "${DXC_EXECUTABLE}"
        -T vs_6_0
        -E VSMain
        -HV 2021
        -Zi
        -Qembed_debug
        -WX
        -Fh "${PARTICLE_BILLBOARD_VERTEX_SHADER_HEADER}"
        -Vn g_ParticleBillboardVertexShader
        "${PARTICLE_BILLBOARD_SHADER_SOURCE}"

    DEPENDS
        "${PARTICLE_BILLBOARD_SHADER_SOURCE}"

    COMMENT
        "Compiling particle billboard vertex shader"

    VERBATIM
)

add_custom_command(
    OUTPUT
        "${PARTICLE_BILLBOARD_PIXEL_SHADER_HEADER}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${GENERATED_SHADER_DIRECTORY}"

    COMMAND
        "${DXC_EXECUTABLE}"
        -T ps_6_0
        -E PSMain
        -HV 2021
        -Zi
        -Qembed_debug
        -WX
        -Fh "${PARTICLE_BILLBOARD_PIXEL_SHADER_HEADER}"
        -Vn g_ParticleBillboardPixelShader
        "${PARTICLE_BILLBOARD_SHADER_SOURCE}"

    DEPENDS
        "${PARTICLE_BILLBOARD_SHADER_SOURCE}"

    COMMENT
        "Compiling particle billboard pixel shader"

    VERBATIM
)

add_custom_command(
    OUTPUT
        "${PARTICLE_DEBUG_VERTEX_SHADER_HEADER}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${GENERATED_SHADER_DIRECTORY}"

    COMMAND
        "${DXC_EXECUTABLE}"
        -T vs_6_0
        -E VSMain
        -HV 2021
        -Zi
        -Qembed_debug
        -WX
        -Fh "${PARTICLE_DEBUG_VERTEX_SHADER_HEADER}"
        -Vn g_ParticleDebugVertexShader
        "${PARTICLE_DEBUG_SHADER_SOURCE}"

    DEPENDS
        "${PARTICLE_DEBUG_SHADER_SOURCE}"

    COMMENT
        "Compiling particle debug vertex shader"

    VERBATIM
)

add_custom_command(
    OUTPUT
        "${PARTICLE_DEBUG_PIXEL_SHADER_HEADER}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${GENERATED_SHADER_DIRECTORY}"

    COMMAND
        "${DXC_EXECUTABLE}"
        -T ps_6_0
        -E PSMain
        -HV 2021
        -Zi
        -Qembed_debug
        -WX
        -Fh "${PARTICLE_DEBUG_PIXEL_SHADER_HEADER}"
        -Vn g_ParticleDebugPixelShader
        "${PARTICLE_DEBUG_SHADER_SOURCE}"

    DEPENDS
        "${PARTICLE_DEBUG_SHADER_SOURCE}"

    COMMENT
        "Compiling particle debug pixel shader"

    VERBATIM
)

add_custom_command(
    OUTPUT
        "${PARTICLE_SIM_COMPUTE_SHADER_HEADER}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${GENERATED_SHADER_DIRECTORY}"

    COMMAND
        "${DXC_EXECUTABLE}"
        -T cs_6_0
        -E CSMain
        -HV 2021
        -Zi
        -Qembed_debug
        -WX
        -Fh "${PARTICLE_SIM_COMPUTE_SHADER_HEADER}"
        -Vn g_ParticleSimComputeShader
        "${PARTICLE_SIM_SHADER_SOURCE}"

    DEPENDS
        "${PARTICLE_SIM_SHADER_SOURCE}"

    COMMENT
        "Compiling particle simulation compute shader"

    VERBATIM
)

add_custom_command(
    OUTPUT
        "${HDR_PRESENT_VERTEX_SHADER_HEADER}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${GENERATED_SHADER_DIRECTORY}"

    COMMAND
        "${DXC_EXECUTABLE}"
        -T vs_6_0
        -E VSMain
        -HV 2021
        -Zi
        -Qembed_debug
        -WX
        -Fh "${HDR_PRESENT_VERTEX_SHADER_HEADER}"
        -Vn g_HdrPresentVertexShader
        "${HDR_PRESENT_SHADER_SOURCE}"

    DEPENDS
        "${HDR_PRESENT_SHADER_SOURCE}"

    COMMENT
        "Compiling HDR presentation vertex shader"

    VERBATIM
)

add_custom_command(
    OUTPUT
        "${HDR_PRESENT_PIXEL_SHADER_HEADER}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${GENERATED_SHADER_DIRECTORY}"

    COMMAND
        "${DXC_EXECUTABLE}"
        -T ps_6_0
        -E PSMain
        -HV 2021
        -Zi
        -Qembed_debug
        -WX
        -Fh "${HDR_PRESENT_PIXEL_SHADER_HEADER}"
        -Vn g_HdrPresentPixelShader
        "${HDR_PRESENT_SHADER_SOURCE}"

    DEPENDS
        "${HDR_PRESENT_SHADER_SOURCE}"

    COMMENT
        "Compiling HDR presentation pixel shader"

    VERBATIM
)

add_custom_command(
    OUTPUT
        "${EMISSIVE_PIXEL_SHADER_HEADER}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${GENERATED_SHADER_DIRECTORY}"

    COMMAND
        "${DXC_EXECUTABLE}"
        -T ps_6_0
        -E PSMain
        -HV 2021
        -Zi
        -Qembed_debug
        -WX
        -Fh "${EMISSIVE_PIXEL_SHADER_HEADER}"
        -Vn g_EmissivePixelShader
        "${EMISSIVE_SHADER_SOURCE}"

    DEPENDS
        "${EMISSIVE_SHADER_SOURCE}"

    COMMENT
        "Compiling emissive pixel shader"

    VERBATIM
)

add_custom_command(
    OUTPUT
        "${BASIC_VERTEX_SHADER_HEADER}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${GENERATED_SHADER_DIRECTORY}"

    COMMAND
        "${DXC_EXECUTABLE}"
        -T vs_6_0
        -E VSMain
        -HV 2021
        -Zi
        -Qembed_debug
        -WX
        -Fh "${BASIC_VERTEX_SHADER_HEADER}"
        -Vn g_BasicVertexShader
        "${BASIC_SHADER_SOURCE}"

    DEPENDS
        "${BASIC_SHADER_SOURCE}"

    COMMENT
        "Compiling Basic vertex shader"

    VERBATIM
)

add_custom_command(
    OUTPUT
        "${BASIC_PIXEL_SHADER_HEADER}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${GENERATED_SHADER_DIRECTORY}"

    COMMAND
        "${DXC_EXECUTABLE}"
        -T ps_6_0
        -E PSMain
        -HV 2021
        -Zi
        -Qembed_debug
        -WX
        -Fh "${BASIC_PIXEL_SHADER_HEADER}"
        -Vn g_BasicPixelShader
        "${BASIC_SHADER_SOURCE}"

    DEPENDS
        "${BASIC_SHADER_SOURCE}"

    COMMENT
        "Compiling Basic pixel shader"

    VERBATIM
)

add_custom_target(GravityForgeShaders
    DEPENDS
        "${BASIC_VERTEX_SHADER_HEADER}"
        "${BASIC_PIXEL_SHADER_HEADER}"
        "${EMISSIVE_PIXEL_SHADER_HEADER}"
        "${HDR_PRESENT_VERTEX_SHADER_HEADER}"
        "${HDR_PRESENT_PIXEL_SHADER_HEADER}"
        "${PARTICLE_SIM_COMPUTE_SHADER_HEADER}"
        "${PARTICLE_DEBUG_VERTEX_SHADER_HEADER}"
        "${PARTICLE_DEBUG_PIXEL_SHADER_HEADER}"
        "${PARTICLE_BILLBOARD_VERTEX_SHADER_HEADER}"
        "${PARTICLE_BILLBOARD_PIXEL_SHADER_HEADER}"
        "${DEBRIS_VERTEX_SHADER_HEADER}"
        "${DEBRIS_PIXEL_SHADER_HEADER}"
        "${DEBRIS_CULL_COMPUTE_SHADER_HEADER}"
        "${BLOOM_EXTRACT_VERTEX_SHADER_HEADER}"
        "${BLOOM_EXTRACT_PIXEL_SHADER_HEADER}"
        "${BLOOM_BLUR_VERTEX_SHADER_HEADER}"
        "${BLOOM_BLUR_PIXEL_SHADER_HEADER}"
)

add_dependencies(
    GravityForge
    GravityForgeShaders
)

set_source_files_properties(
    "${BASIC_SHADER_SOURCE}"
    "${EMISSIVE_SHADER_SOURCE}"
    "${HDR_PRESENT_SHADER_SOURCE}"
    "${PARTICLE_SIM_SHADER_SOURCE}"
    "${PARTICLE_DEBUG_SHADER_SOURCE}"
    "${PARTICLE_BILLBOARD_SHADER_SOURCE}"
    "${DEBRIS_SHADER_SOURCE}"
    "${DEBRIS_CULL_SHADER_SOURCE}"
    "${DEBRIS_COMMON_SHADER_SOURCE}"
    "${BLOOM_EXTRACT_SHADER_SOURCE}"
    "${BLOOM_BLUR_SHADER_SOURCE}"
    PROPERTIES
        HEADER_FILE_ONLY TRUE
)

target_sources(GravityForge PRIVATE
    "${BASIC_SHADER_SOURCE}"
    "${EMISSIVE_SHADER_SOURCE}"
    "${HDR_PRESENT_SHADER_SOURCE}"
    "${PARTICLE_SIM_SHADER_SOURCE}"
    "${PARTICLE_DEBUG_SHADER_SOURCE}"
    "${PARTICLE_BILLBOARD_SHADER_SOURCE}"
    "${DEBRIS_CULL_SHADER_SOURCE}"
    "${BLOOM_EXTRACT_SHADER_SOURCE}"
    "${BLOOM_BLUR_SHADER_SOURCE}"
)

target_compile_features(GravityForge PRIVATE cxx_std_20)

target_compile_definitions(GravityForge PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    GF_ASSET_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/assets"

    $<$<CONFIG:Debug>:USE_PIX>
    $<$<CONFIG:RelWithDebInfo>:USE_PIX>
)

target_include_directories(GravityForge PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${GENERATED_SHADER_DIRECTORY}"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/WinPixEventRuntime/Include/WinPixEventRuntime"
)

target_link_libraries(GravityForge PRIVATE
    d3d12
    dxgi
    ole32
    windowscodecs
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/WinPixEventRuntime/bin/x64/WinPixEventRuntime.lib"
)

add_custom_command(
    TARGET GravityForge
    POST_BUILD

    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different

        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/WinPixEventRuntime/bin/x64/WinPixEventRuntime.dll"

        "$<TARGET_FILE_DIR:GravityForge>"
)

if(MSVC)
    target_compile_options(GravityForge PRIVATE
        /W4
        /permissive-
        /EHsc
        /utf-8
        /Zc:__cplusplus
    )
endif()

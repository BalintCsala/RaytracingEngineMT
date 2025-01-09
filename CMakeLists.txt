cmake_minimum_required(VERSION 3.21)
project(dipterv_rt)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pedantic")

add_link_options(-static -static-libgcc -static-libstdc++)

include(FetchContent)

# GLM
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
)
FetchContent_MakeAvailable(glm)

# GLFW
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw
)
FetchContent_GetProperties(glfw)
if (NOT glfw_POPULATED)
    FetchContent_Populate(glfw)

    set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
    set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
    set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
    set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")

    add_subdirectory(${glfw_SOURCE_DIR} ${glfw_BINARY_DIR})
endif ()

# JSON

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

# Vulkan

find_package(Vulkan REQUIRED)

add_executable(${PROJECT_NAME}
        main.cpp
        VulkanContext.cpp
        Window.cpp 
        Buffer.cpp 
        AccelerationStructure.cpp
        Image.cpp
        DescriptorSetAllocator.cpp
        Model.cpp
        TextureCache.cpp
        Scene.cpp
        Scene.h
        ComputePass.cpp
        ComputePass.h
        util.h
        ProcessingPipeline.cpp
        ProcessingPipeline.h
        expression_parsing.h
        Pass.cpp
        Pass.h
        RaytracePass.cpp
        RaytracePass.h
        Definitions.cpp
)

include_directories(${PROJECT_NAME} ${Vulkan_INCLUDE_DIRS})
include_directories(${PROJECT_NAME} ${GLFW_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} glfw)
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES})
target_link_libraries(${PROJECT_NAME} glm::glm)
target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)

target_include_directories(${PROJECT_NAME} PUBLIC thirdparty)

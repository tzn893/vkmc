cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 17)

project(vkmc)

add_compile_definitions(VKMC_ROOT_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(general-vulkan)
add_subdirectory(flecs)
add_subdirectory(spdlog)

add_subdirectory(src)
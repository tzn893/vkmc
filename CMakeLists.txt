cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 17)


project(vkmc)

add_subdirectory(general-vulkan)
add_subdirectory(flecs)

add_subdirectory(src)
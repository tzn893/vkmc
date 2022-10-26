add_executable(vkmc main.cpp common.h)

target_include_directories(vkmc 
    PRIVATE
    ${CMAKE_SOURCE_DIR}/flecs/include 
    ${CMAKE_SOURCE_DIR}/general-vulkan/src
    ${CMAKE_SOURCE_DIR}/spdlog/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(math)
add_subdirectory(parallel)
add_subdirectory(renderer)
add_subdirectory(terrian)
add_subdirectory(world)

target_link_libraries(vkmc gvk flecs spdlog)
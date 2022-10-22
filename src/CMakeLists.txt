add_executable(vkmc main.cpp)

target_include_directories(vkmc 
    PRIVATE
    ${CMAKE_SOURCE_DIR}/flecs/include 
    ${CMAKE_SOURCE_DIR}/general-vulkan/src
)

target_link_libraries(vkmc gvk flecs)
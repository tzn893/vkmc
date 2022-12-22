add_executable(vkmc main.cpp common.h)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

target_include_directories(vkmc 
    PRIVATE
    ${CMAKE_SOURCE_DIR}/flecs/include 
    ${CMAKE_SOURCE_DIR}/general-vulkan/src
    ${CMAKE_SOURCE_DIR}/spdlog/include
    ${CMAKE_SOURCE_DIR}/xxHash
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(alloc)
add_subdirectory(math)
add_subdirectory(parallel)
add_subdirectory(renderer)
add_subdirectory(terrian)
add_subdirectory(world)
add_subdirectory(ecs)
add_subdirectory(util)

target_link_libraries(vkmc gvk flecs spdlog xxhash)


 add_custom_command(TARGET vkmc POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:flecs>  $<TARGET_FILE_DIR:vkmc>
   COMMAND_EXPAND_LISTS
   )
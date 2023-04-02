file(GLOB RENDERER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

target_sources(vkmc PRIVATE 
		   ${RENDERER_FILE} 
		   ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vkmc_shader_common.h
		   ${CMAKE_CURRENT_SOURCE_DIR}/shaders/skybox_shader_common.h
)
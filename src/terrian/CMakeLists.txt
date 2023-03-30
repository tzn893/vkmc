file(GLOB WORLD_FILE ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

target_sources(vkmc PRIVATE ${WORLD_FILE} 
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/terrian_shader_common.h
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/terrian_shader_fragment.h
)
#math only contains hpp files
file(GLOB MATH_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB MATH_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/*.h")


target_sources(vkmc PRIVATE ${MATH_SOURCE} ${MATH_HEADER})
cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

project("lapwing")

add_executable(${PROJECT_NAME} "src/lapwing.cpp" "src/reader.cpp" "src/reader.h" "src/endian.h"
        "src/endian.cpp" "src/hasher.h" "src/hasher.cpp" "src/writer.cpp"  "src/writer.h"
        "src/file_utils.h" "src/file_utils.cpp" "src/lz4.h" "src/lz4.c")

add_subdirectory(libraries/header_libs)
add_subdirectory(libraries/glm)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:lapwing> ${CMAKE_CURRENT_SOURCE_DIR}/../resources/
)

target_include_directories(${PROJECT_NAME}
	PUBLIC include/
  PUBLIC libraries/header_libs/include
	PUBLIC libraries/glm/include
)

target_link_libraries(${PROJECT_NAME}
	PUBLIC glm
)

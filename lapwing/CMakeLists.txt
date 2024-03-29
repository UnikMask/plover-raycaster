cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
project("lapwing")

add_executable(${PROJECT_NAME} "src/lapwing.cpp" "src/reader.cpp" "src/reader.h" "src/endian.h"
        "src/endian.cpp" "src/hasher.h" "src/hasher.cpp" "src/writer.cpp"  "src/writer.h"
        "src/file_utils.h" "src/file_utils.cpp" "src/lz4.h" "src/lz4.c" "src/vox.h" "src/vox.cpp")

FetchContent_Declare(
    glm 
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e
)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG e2c92645460f680fd272fd2eed591efb2be7dc31
)
FetchContent_MakeAvailable(glfw)
FetchContent_MakeAvailable(glm)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:lapwing> ${CMAKE_CURRENT_SOURCE_DIR}/../resources/
)

target_include_directories(${PROJECT_NAME}
	PUBLIC include/
  PUBLIC libraries/header_libs/include
	PUBLIC libraries/glm/include
)

target_link_libraries(${PROJECT_NAME}
	PUBLIC glfw
	PUBLIC glm::glm
)

Cmake_minimum_required(VERSION 3.9)

project("PloverRaycaster" VERSION "0.1.0" DESCRIPTION "A plover-based raycaster.")

if (WIN32)
	add_compile_options(-finstrument-functions)
endif()

if (LINUX)
	add_compile_options(-finstrument-functions-after-inlining)
	add_compile_options(-fsanitize=address)
	add_link_options(-fsanitize=address)
endif()

add_library(PloverRaycaster SHARED "src/Main.cpp")

find_package(glm REQUIRED)

add_subdirectory("plover")
message(STATUS $ENV{SHADER_COMPILE_SCRIPT})

add_custom_target(compile_shaders ALL
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/$ENV{SHADER_COMPILE_SCRIPT})

add_custom_target(copy_resources ALL
    DEPENDS compile_shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/resources
    ${PROJECT_BINARY_DIR}/resources
    COMMENT "Copy resources into binary's directory")

add_dependencies(${PROJECT_NAME} compile_shaders)
add_dependencies(${PROJECT_NAME} copy_resources)

target_include_directories(${PROJECT_NAME}
    PUBLIC plover/include
)
target_link_libraries(${PROJECT_NAME} PUBLIC glm::glm)


cmake_minimum_required(VERSION 3.14)

project(
    ImageConverter VERSION 1.0
    DESCRIPTION "Cool image converter"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

add_executable(image_converter "")

add_subdirectory(src)
cmake_minimum_required(VERSION 3.0)

project(camsrv)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_subdirectory(mdetect)
add_subdirectory(camsrvd)
add_subdirectory(misc)

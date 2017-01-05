cmake_minimum_required(VERSION 3.0)

project(mdetect)

include_directories(src)
include_directories(/usr/local/include)

link_directories(/usr/local/lib)
link_directories(/usr/lib)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/opt/camsrv/)

set(CMAKE_CXX_FLAGS "-o3 -Wall -Wextra -fomit-frame-pointer -ffast-math -s")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS}")

find_package(OpenCV REQUIRED)

add_executable(mdetect src/mdetect.cpp)
target_link_libraries(mdetect ${OpenCV_LIBS})


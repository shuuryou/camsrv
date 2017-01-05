cmake_minimum_required(VERSION 3.0)

project(camsrvd)

include_directories(src)
include_directories(/usr/local/include)

link_directories(/usr/local/lib)
link_directories(/usr/lib)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/opt/camsrv/)

set(CMAKE_CXX_FLAGS "-o3 -Wall -Wextra -fomit-frame-pointer -ffast-math -s")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS}")

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost REQUIRED COMPONENTS filesystem program_options system) 

add_executable(camsrvd src/camsrvd.cpp src/nargv/nargv.c)
target_link_libraries(camsrvd ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY})

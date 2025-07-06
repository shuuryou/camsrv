cmake_minimum_required(VERSION 3.0)

project(camsrv)

include_directories(src)
include_directories(/usr/local/include)

link_directories(/usr/local/lib)
link_directories(/usr/lib)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/)

set(CMAKE_CXX_FLAGS "-march=native -o3 -Wall -Wextra -fomit-frame-pointer -ffast-math -flto -s")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS}")

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost REQUIRED filesystem program_options system) 

find_package(OpenCV REQUIRED)

add_executable(camsrvd src/locking.cpp src/nargv/nargv.c src/camsrvd.cpp)
target_link_libraries(camsrvd ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY})

add_executable(maintenance src/maintenance.cpp src/locking.cpp )
target_link_libraries(maintenance ${OpenCV_LIBS} ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY})

add_executable(makemask src/makemask.cpp)
target_link_libraries(makemask ${OpenCV_LIBS} ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY})

cmake_minimum_required(VERSION 3.12)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "-O3")

add_library(httplib STATIC 
    httplib.cpp
)	
target_include_directories(httplib PUBLIC .)
target_link_libraries(httplib PUBLIC ws2_32.lib)
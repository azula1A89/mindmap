cmake_minimum_required(VERSION 3.12)
set (CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "-O3")
add_library(imgui STATIC 
			imgui.cpp
			imgui_draw.cpp
			imgui_tables.cpp
			imgui_widgets.cpp
			backend/imgui_impl_opengl3.cpp
			backend/imgui_impl_glfw.cpp
			thirdparty/imgui_extra_math.cpp
			thirdparty/imgui_canvas.cpp
			cpp/imgui_stdlib.cpp
			)
target_include_directories(imgui PUBLIC . cpp thirdparty backend)

add_library(fonts INTERFACE)
target_include_directories(fonts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/fonts)


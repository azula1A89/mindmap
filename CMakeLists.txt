# MIT License

# Copyright (c) [2025] [azula1A89]

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.14)
set(APP_NAME mindmap)
project(${APP_NAME} VERSION 1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O3")
add_definitions(-D_NDEBUG)
add_compile_definitions(NOMINMAX)

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/icon/app.rc")
set(THIRD_PARTY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)
set(BASE64_PATH ${THIRD_PARTY_PATH}/base64)
set(FMT_PATH ${THIRD_PARTY_PATH}/fmt)
set(GLEW_PATH ${THIRD_PARTY_PATH}/glew/build/cmake)
set(GLFW_PATH ${THIRD_PARTY_PATH}/glfw)
set(HTTPLIB_PATH ${THIRD_PARTY_PATH}/httplib)
set(IMGUI_PATH ${THIRD_PARTY_PATH}/imgui)
set(INJA_PATH ${THIRD_PARTY_PATH}/inja)
set(JSON_PATH ${THIRD_PARTY_PATH}/json)
set(LAYOUT_PATH ${THIRD_PARTY_PATH}/layout)
set(NFD_PATH ${THIRD_PARTY_PATH}/nfd-1.2.1)
set(STB_IMAGE_PATH ${THIRD_PARTY_PATH}/stb_image)

option(GLFW_BUILD_WAYLAND "Build support for Wayland" OFF)
option(GLFW_BUILD_X11 "Build support for X11" ON)
option(NFD_PORTAL "Use xdg-desktop-portal instead of GTK" ON)
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)

add_subdirectory(${BASE64_PATH})
add_subdirectory(${FMT_PATH})
add_subdirectory(${HTTPLIB_PATH})
add_subdirectory(${IMGUI_PATH})
add_subdirectory(${INJA_PATH})
add_subdirectory(${JSON_PATH})
add_subdirectory(${LAYOUT_PATH})
add_subdirectory(${NFD_PATH})
add_subdirectory(${STB_IMAGE_PATH})
add_subdirectory(${GLEW_PATH})
add_subdirectory(${GLFW_PATH})

find_package(OpenGL REQUIRED)
target_link_directories(imgui PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/glfw/src)
target_link_libraries(imgui PUBLIC OpenGL::GL glfw3)
target_include_directories(imgui PUBLIC ${GLEW_PATH}/include ${GLFW_PATH}/include ${GLFW_PATH}/deps)

add_executable(${APP_NAME}  src/mindmap.cpp src/main.cpp ${APP_ICON_RESOURCE_WINDOWS})
target_link_libraries(${APP_NAME} PRIVATE base64 fmt httplib imgui fonts inja json vpsc avoid cola nfd stb_image)
target_compile_options(${APP_NAME} PRIVATE -Wno-stringop-overflow)

#copy fonts to build
add_custom_command(TARGET ${APP_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/fonts
    $<TARGET_FILE_DIR:${APP_NAME}>/fonts
)

#copy templates to build
add_custom_command(TARGET ${APP_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/templates
    $<TARGET_FILE_DIR:${APP_NAME}>/templates
)

#copy ini to build
add_custom_command(TARGET ${APP_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/templates/imgui.ini
    $<TARGET_FILE_DIR:${APP_NAME}>/imgui.ini
)

#copy output.json to build
add_custom_command(TARGET ${APP_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/templates/output.json
    $<TARGET_FILE_DIR:${APP_NAME}>/output.json
)

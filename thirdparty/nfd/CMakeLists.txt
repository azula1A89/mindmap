set(TARGET_NAME nfd)
set(CMAKE_CXX_FLAGS "-O3")
add_library(${TARGET_NAME} STATIC nfd_win.cpp)
target_link_libraries(${TARGET_NAME} PRIVATE ole32 uuid shell32 comdlg32)
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(${TARGET_NAME} PRIVATE -nostdlib -fno-exceptions -fno-rtti)


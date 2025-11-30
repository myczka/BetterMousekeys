cmake_minimum_required(VERSION 3.8)
project(touhou-mousekeys LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
add_executable(touhoumousekeys WIN32 main.cpp)

# Link system libraries (Windows)
target_link_libraries(touhoumousekeys PRIVATE opengl32)
# user32 and gdi32 are linked automatically by Windows toolchain normally, but ensure:
target_link_libraries(touhoumousekeys PRIVATE user32 gdi32)
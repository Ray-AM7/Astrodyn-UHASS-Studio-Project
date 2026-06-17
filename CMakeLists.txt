cmake_minimum_required(VERSION 3.16)
project(astrodyn LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)
find_package(OpenGL REQUIRED)

# Dear ImGui + ImGui-SFML compiled manually.
# This avoids external/imgui-sfml/CMakeLists.txt asking for static SFML.
add_library(imgui_sfml_manual
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_tables.cpp
    external/imgui-sfml/imgui-SFML.cpp
)

target_include_directories(imgui_sfml_manual PUBLIC
    external/imgui
    external/imgui-sfml
)

target_link_libraries(imgui_sfml_manual PUBLIC
    sfml-graphics
    sfml-window
    sfml-system
    OpenGL::GL
)

add_library(astrodyn
    src/SpaceObject.cpp
    src/LargeBody.cpp
    src/SmallBody.cpp
    src/SpaceCraft.cpp
    src/TestParticle.cpp
    src/Universe.cpp
    src/Presets.cpp
)

target_include_directories(astrodyn PUBLIC include)

add_executable(astrodyn_app app/main.cpp)

target_link_libraries(astrodyn_app
    PRIVATE
        astrodyn
        imgui_sfml_manual
)
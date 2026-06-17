cmake_minimum_required(VERSION 3.16)
project(astrodyn LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

add_executable(astrodyn_app app/main.cpp)
target_link_libraries(astrodyn_app PRIVATE astrodyn sfml-graphics sfml-window sfml-system)
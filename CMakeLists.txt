cmake_minimum_required(VERSION 3.16)
project(astrodyn_starter VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(astrodyn
    src/vec2.cpp
    src/astro.cpp
    src/integrators.cpp
    src/csv.cpp
)

target_include_directories(astrodyn PUBLIC include)

add_executable(two_body_compare examples/two_body_compare.cpp)
target_link_libraries(two_body_compare PRIVATE astrodyn)

add_executable(hohmann_transfer examples/hohmann_transfer.cpp)
target_link_libraries(hohmann_transfer PRIVATE astrodyn)

add_executable(finite_burn_raise examples/finite_burn_raise.cpp)
target_link_libraries(finite_burn_raise PRIVATE astrodyn)

add_executable(sun_jupiter_asteroid examples/sun_jupiter_asteroid.cpp)
target_link_libraries(sun_jupiter_asteroid PRIVATE astrodyn)

add_executable(gravity_assist_demo examples/gravity_assist_demo.cpp)
target_link_libraries(gravity_assist_demo PRIVATE astrodyn)

# Optional live GUI sandbox. Install with: sudo apt install libsfml-dev
find_package(SFML 2.5 COMPONENTS graphics window system QUIET)
if (SFML_FOUND)
    add_executable(orbit_sandbox app/orbit_sandbox.cpp)
    target_link_libraries(orbit_sandbox PRIVATE astrodyn sfml-graphics sfml-window sfml-system)
    target_compile_definitions(orbit_sandbox PRIVATE ASTRODYN_HAS_SFML=1)
else()
    message(STATUS "SFML not found; orbit_sandbox GUI target will not be built. Install libsfml-dev to enable it.")
endif()

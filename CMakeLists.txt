cmake_minimum_required(VERSION 3.20)
project(StickminArenaWorkspace CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include(FetchContent)

# -------------------------------------------------------------
# Box2D (v3.1.1)
# -------------------------------------------------------------
message(STATUS "Fetching Box2D v3.1.1...")
FetchContent_Declare(
    box2d
    GIT_REPOSITORY https://github.com/erincatto/box2d.git
    GIT_TAG v3.1.1
)
set(BOX2D_SAMPLES OFF CACHE BOOL "" FORCE)
set(BOX2D_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(box2d)

# -------------------------------------------------------------
# SFML (3.0.2)
# -------------------------------------------------------------
message(STATUS "Fetching SFML 3.0.2...")
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO TRUE CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK TRUE CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SFML)

# -------------------------------------------------------------
# Subprojects: "The Ragdoll Engine" and "The Game"
# -------------------------------------------------------------
add_subdirectory("The Ragdoll Engine")
add_subdirectory("The Game")

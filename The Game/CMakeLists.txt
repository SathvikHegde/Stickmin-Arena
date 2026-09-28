add_executable(TheGame
    src/Main.cpp
)

target_include_directories(TheGame PRIVATE
    include
)

target_link_libraries(TheGame PRIVATE
    RagdollEngine
)

target_compile_features(TheGame PRIVATE cxx_std_17)

# Set working directory for debugging in Visual Studio
set_target_properties(TheGame PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)




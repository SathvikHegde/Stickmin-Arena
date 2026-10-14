add_executable(TheGame
    src/Main.cpp
    src/Fighter.cpp
    src/CombatManager.cpp
    src/CpuController.cpp
    src/NetworkManager.cpp
    src/MenuManager.cpp
    src/BattleHUD.cpp
    src/InputManager.cpp
    src/SceneManager.cpp
    src/MenuScene.cpp
    src/BattleScene.cpp
)

target_include_directories(TheGame PRIVATE
    src
)

target_link_libraries(TheGame PRIVATE
    RagdollEngine
)

target_compile_features(TheGame PRIVATE cxx_std_17)

# Set working directory for debugging in Visual Studio
set_target_properties(TheGame PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)




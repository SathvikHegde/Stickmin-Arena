add_executable(TheGame
    src/Main.cpp

    # Core & Architecture
    src/Core/SceneManager.cpp

    # Scenes
    src/Scenes/MenuScene.cpp
    src/Scenes/BattleScene.cpp

    # Combat
    src/Combat/Fighter.cpp
    src/Combat/CombatManager.cpp
    src/Combat/CpuController.cpp
    src/Combat/CharacterRegistry.cpp

    # Input
    src/Input/InputManager.cpp

    # Network
    src/Network/NetworkManager.cpp

    # UI
    src/UI/BattleHUD.cpp
    src/UI/MenuManager.cpp
)

target_include_directories(TheGame PRIVATE
    src
    src/Core
    src/Scenes
    src/Combat
    src/Input
    src/Network
    src/UI
)

target_link_libraries(TheGame PRIVATE
    RagdollEngine
)

target_compile_features(TheGame PRIVATE cxx_std_17)

# Set working directory for debugging in Visual Studio
set_target_properties(TheGame PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

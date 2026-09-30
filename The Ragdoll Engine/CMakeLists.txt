add_library(RagdollEngine STATIC
    # Core
    src/Core/TimeManager.cpp
    src/Core/CameraDirector.cpp
    
    # Physics
    src/Physics/PhysicsWorld.cpp
    src/Physics/RagdollSkeleton.cpp
    src/Physics/ActiveRagdollController.cpp
    
    # Render & VFX
    src/Render/RagdollRenderer.cpp
    src/Render/CharacterRegistry.cpp
    src/Render/JuiceFX.cpp
)

target_include_directories(RagdollEngine PUBLIC
    include
)

target_link_libraries(RagdollEngine PUBLIC
    box2d
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
    SFML::Network
)

target_compile_features(RagdollEngine PUBLIC cxx_std_17)

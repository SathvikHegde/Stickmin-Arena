# Stickmin Arena

Active ragdoll fighting game built with SFML 3 and Box2D.

## Prerequisites

- **C++ Compiler:** C++17 compatible compiler (MSVC / Visual Studio 2022 v17.0+, GCC 9+, or Clang 10+)
- **CMake:** Version 3.20 or newer
- **Git:** Required by CMake to fetch sub-dependencies

Dependencies (**SFML 3.0.2** and **Box2D 3.1.1**) are downloaded and built automatically via CMake `FetchContent`. No external library installations are required.

## Building

### Command Line (CMake)

1. Clone the repository:
   ```bash
   git clone https://github.com/SathvikHegde/Stickmin-Arena.git
   cd Stickmin-Arena
   ```

2. Configure the project:
   ```bash
   cmake -B build -S .
   ```

3. Build the game executable:
   ```bash
   cmake --build build --config Release --target TheGame
   ```
   *(Use `--config Debug` for debug builds).*

### Visual Studio (IDE)

1. Open the repository root folder in Visual Studio via **File > Open > Folder...** (native CMake integration).
2. Alternatively, generate a solution file:
   ```bash
   cmake -B build -S . -G "Visual Studio 17 2022"
   ```
3. Open `build/StickminArenaWorkspace.sln`, set `TheGame` as the startup project, and press **F5** to build and run.

## Running

The compiled executable is placed under `build/bin/`:

- **Release:**
  ```powershell
  .\build\bin\Release\TheGame.exe
  ```
- **Debug:**
  ```powershell
  .\build\bin\Debug\TheGame.exe
  ```

## Project Structure

- `The Ragdoll Engine/` — Reusable engine static library (active ragdoll physics, Box2D integration, skeletal hierarchy, camera director, juice FX, SFML renderer).
- `The Game/` — Game application layer (scenes, character registry, combat manager, virtual controller input, UDP netplay, and UI).

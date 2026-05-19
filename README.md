# Rendering Engine

[![Course](https://img.shields.io/badge/Course-Scalable_Rendering_for_Graphics_and_Game_Engines-blue.svg)](https://www.fib.upc.edu/en)
[![Institution](https://img.shields.io/badge/Institution-UPC_FIB-red.svg)](https://www.fib.upc.edu/en)

This repository contains the source code and build instructions for a Scalable Rendering Graphics and Game Engine.

The system consumes a shared map description, diverging into a **precomputation phase** to generate visibility data, and a **runtime phase** where the renderer consumes this data to produce an optimized graphical output.

For a detailed explanation of the project design, architecture, and implementation, see the [project report](report/report.tex).

## System Requirements

> [!IMPORTANT]
> Commands provided throughout this document assume you are executing them from the root directory of the cloned repository.

The project is configured for modern C++ development environments. It includes `gl3w` and a bundled `glm` copy, along with CMake helper modules for `SOIL`/`GLM`. Ensure your system meets the following prerequisites:

*   **Compiler:** A C++ compiler with **C++11** (or later) support (e.g., `gcc`, `clang`).
*   **Build System:** **CMake** (3.0+ recommended). `make` and a POSIX shell are optional but highly recommended for the build steps.
*   **Graphics API:** **OpenGL** development headers (provided natively by the OS on macOS/Linux).

## Navigating Lab Sessions

The project was developed incrementally across several lab sessions. You can jump back to the exact state of the code at the end of each lab by using Git tags (`lab1` through `lab7`).

### Model Assets Setup for Previous Tags
> [!WARNING]
> The final version of this repository contains the `models` directory directly. However, in all previous lab commits, `models` was tracked as a symbolic link expecting the actual folder to be exactly one directory level up (`../models`).

**Before** switching to any previous lab tag, please copy the `models` directory from this final commit into the parent directory so the older versions can locate the assets successfully.

Execute the following command from the **root of the cloned repository** on the latest commit:

```bash
cp -r models ../
```

### Switching to a Lab Commit
Once the models folder is in place, you can view and switch between the lab iterations:

```bash
# List all available tags
git tag

# Switch to a specific lab's codebase (e.g., lab1)
git checkout lab1

# To return to the latest completed project version
git checkout main
```

## Build & Execution Pipelines
> [!NOTE]
> If you run the full `SRGGE` executable without an up-to-date `visibility.txt`, results may be missing or incorrect. Always run the precomputer first when updating the map. You can skip the precomputer step if you are on tags `lab1` through `lab6`.

### 1. Visibility Precomputer
This step compiles the precomputer and generates (or overwrites) the visibility data from the map file.

For the **final version**:

```bash
g++ -O3 -o build/precomputer src/VisibilityPrecomputer.cpp
./build/precomputer map.txt visibility.txt
```

For **lab tags** (`lab7` only):
```bash
g++ -O3 -o build/precomputer VisibilityPrecomputer.cpp
./build/precomputer map.txt visibility.txt
```

### 2. Renderer
For the **final version**:
```bash
# Clean previous builds
rm -rf build

# Configure the project and generate the build system
cmake -S src -B build

# Compile the executable
cmake --build build -j

# Run the executable
./build/SRGGE map.txt visibility.txt
```

For **lab tags**:
```bash
# Clean previous builds
rm -rf build

# Configure the project and generate the build system
cmake -S . -B build

# Compile the executable
cmake --build build -j

# Run the executable
./build/SRGGE map.txt visibility.txt
```

## Input & Output Data

The engine relies on two core text files to orchestrate the rendering environment:

| File | Type | Description |
|---|---|---|
| **`map.txt`** | Input | Contains the scene/map description. Read by both the visibility precomputer and the main renderer. |
| **`visibility.txt`** | Output / Input | Produced by the precomputer phase. The renderer reads it at runtime to determine visible elements efficiently. |

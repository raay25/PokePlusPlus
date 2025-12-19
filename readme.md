# Pokémon PlusPlus

A 3D Pokémon-inspired game built with C++, OpenGL, and SDL2.

---

## Installation

### Prerequisites

#### Windows
- **Visual Studio 2022** (with C++ desktop development workload)
- **Git**

#### macOS
- **Xcode Command Line Tools**: `xcode-select --install`
- **Homebrew**: `/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"`
- **CMake and Ninja**: `brew install cmake ninja`

---

## Setup Instructions

### Step 1: Install vcpkg

#### Windows (PowerShell)
```powershell
cd C:\
git clone https://github.com/microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

#### macOS
```bash
cd ~
git clone https://github.com/microsoft/vcpkg
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
```

---

### Step 2: Install Dependencies

#### Windows
```powershell
cd C:\vcpkg
.\vcpkg install sdl2:x64-windows glm:x64-windows glad:x64-windows stb:x64-windows
```

#### macOS (Intel)
```bash
cd ~/vcpkg
./vcpkg install sdl2:x64-osx glm:x64-osx glad:x64-osx stb:x64-osx
```

#### macOS (Apple Silicon)
```bash
cd ~/vcpkg
./vcpkg install sdl2:arm64-osx glm:arm64-osx glad:arm64-osx stb:arm64-osx
```

---

### Step 3: Clone and Build

#### Clone the Repository
```bash
git clone https://github.com/raay25/PokePlusPlus.git
cd PokePlusPlus
```

#### Build - Windows
1. Open `CMakeLists.txt` in **Visual Studio 2022**
2. Select **x64-Debug** or **x64-Release** from the configuration dropdown
3. Press `Ctrl+Shift+B` to build
4. Press `F5` to run

#### Build - macOS (Intel)
```bash
cmake --preset macos-release
cmake --build build/macos-release
./build/macos-release/PokePlusPlus
```

#### Build - macOS (Apple Silicon)
```bash
cmake --preset macos-arm-release
cmake --build build/macos-arm-release
./build/macos-arm-release/PokePlusPlus
```

---

## Controls

| Action | Key |
|--------|-----|
| Move | `W` `A` `S` `D` |
| Look Around | Mouse |
| Sprint | `Shift` |
| Jump | `Space` |
| Throw Pokéball | `Left Click` (hold to charge) |
| Send Out/Recall Pokémon | `1`-`9` |
| Flashlight | `P` |
| Reset Camera | `F` |
| Exit | `Escape` |

---

## Troubleshooting

### "CMAKE_TOOLCHAIN_FILE not found"
- **Windows**: Ensure vcpkg is at `C:\vcpkg\` or set the `VCPKG_ROOT` environment variable
- **macOS**: Ensure vcpkg is at `~/vcpkg/` or run `export VCPKG_ROOT=~/vcpkg`

### "Cannot find SDL2/OpenGL headers"
Re-run the dependency installation:
```bash
vcpkg list  # Verify installed packages
```

---

## License

This project is for educational purposes.

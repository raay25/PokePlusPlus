# Poke++

Poke++ is a 3D Pokemon catching game (think of it as a battle-less Pokemon Legends: Arceus) built from the ground up using C++ and OpenGL. What initially drew me to OpenGL was the sheer amount of linear algebra involved (I'm a sucker for linear algebra). Wanting to deepen both my C++ and linear algebra skills, Poke++ was born!

In fact, I ended up so fascinated by the math behind it all that I wrote my own set of notes, giving a high-level overview of how Poke++ uses these linear algebra concepts throughout the engine. You can find them [here](https://www.rayguan.ca/PokePlusPlus_Notes.pdf) - it would mean a lot if you took a look.

Here is a video demo of the game:

[![Poke++ Demo Video](https://img.youtube.com/vi/8sKMc6CGHuI/0.jpg)](https://www.youtube.com/watch?v=8sKMc6CGHuI)

## Installation

Poke++ was developed on Windows using Visual Studio 2022. To play it on your Windows machine, download the package [here](https://drive.google.com/file/d/1PJUy61DK9rTQHPfsv-RuQo--fyH5ME7s/view?usp=sharing), extract the contents, and run the `PokePlusPlus.exe` file. If you're on macOS, or want to work on this project yourself, follow the installation guide below. 

### Prerequisites

#### Windows
- **Visual Studio 2022** (with C++ desktop development workload)

#### macOS
- **Xcode Command Line Tools**: `xcode-select --install`
- **Homebrew**: `/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"`
- **CMake and Ninja**: `brew install cmake ninja`
- **pkg-config**: `brew install pkg-config`
---

### Setup Instructions

#### Step 1: Install vcpkg

##### Windows (PowerShell)
```powershell
cd C:\
git clone https://github.com/microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

##### macOS
```bash
cd ~
git clone https://github.com/microsoft/vcpkg
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
```

---

#### Step 2: Install Dependencies

##### Windows
```powershell
cd C:\vcpkg
.\vcpkg install sdl2:x64-windows glm:x64-windows glad:x64-windows stb:x64-windows
```

##### macOS (Intel)
```bash
cd ~/vcpkg
./vcpkg install sdl2:x64-osx glm:x64-osx glad:x64-osx stb:x64-osx
```

##### macOS (Apple Silicon)
```bash
cd ~/vcpkg
./vcpkg install sdl2:arm64-osx glm:arm64-osx glad:arm64-osx stb:arm64-osx
```

---

#### Step 3: Clone and Build

##### Clone the Repository
```bash
git clone https://github.com/raay25/PokePlusPlus.git
cd PokePlusPlus
```

##### Build - Windows
1. Open `CMakeLists.txt` in **Visual Studio 2022**
2. Select **x64-Debug** or **x64-Release** from the configuration dropdown
3. Press `Ctrl+Shift+B` to build
4. Press `Ctrl + F5` to run
> If you are getting a CMake error, you likely need to go into `CMakePresets.json` and update the `generator` attribute under the `configurePresets` array to your version of Visual Studio. I had it set to `Visual Studio 17 2022` during my development. `Ctrl + F5` should then work.

##### Build - macOS (Intel)
```bash
cmake --preset macos-release
cmake --build build/macos-release
./build/macos-release/PokePlusPlus
```

##### Build - macOS (Apple Silicon)
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
| Throw Pokeball | `Left Click` (hold to charge) |
| Send Out/Recall Pokémon | `1`-`6` |
| Exit | `Escape` |

## Credits
The Pokemon models and the terrain textures were all sourced from the web. Thank you to:
- prozip @ Sketchfab for the 3D Bulbasaur model ([link](https://sketchfab.com/3d-models/bulbasaur-pokemon-animated-d2a9a7962613438387880a195cd8a1df#download))
- nguyenlouis32 @ Sketchfab for the 3D Charmander and Squirtle models ([Charmander]([https://sketchfab.com/3d-models/charmander-2637dd2cf8e44d93a05e31d1372039da#download](https://sketchfab.com/3d-models/charmander-2637dd2cf8e44d93a05e31d1372039da#download)), [Squirtle](https://sketchfab.com/3d-models/squirtle-df11cb9c684e4a5fad6eec86d590774d#download))
- gaddiellartey2010 @ Sketchfab for the 3D Pikachu model ([link](https://sketchfab.com/3d-models/pikachu-bdd57b2bf2374bb89251c083cb2d834e))
- ambientCG.com for the terrain heightmap, grass textures, and rock textures

## License

This project is for educational purposes.

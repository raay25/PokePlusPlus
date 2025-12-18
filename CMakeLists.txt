cmake_minimum_required(VERSION 3.20)
project(PokePlusPlus LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# Engine library
add_library(pokepp
  src/core/App.cpp
  src/core/FS.cpp
  src/core/Mesh.cpp
  src/core/Texture.cpp
  src/core/Shader.cpp
  src/core/Model.cpp
  src/core/tiny_obj_loader.cpp
  "include/pokeapp/Model.h" 
  "include/pokeapp/Texture.h"
  "include/pokeapp/tiny_obj_loader.h" "include/pokeapp/Constants.h" "src/core/Material.cpp" "include/pokeapp/World.h" "src/core/World.cpp" "include/pokeapp/Pokemon.h" "src/core/Pokemon.cpp" "include/pokeapp/PokemonController.h" "src/core/PokemonController.cpp" "include/pokeapp/Pokeball.h")

target_include_directories(pokepp
  PUBLIC  ${CMAKE_SOURCE_DIR}/include
  PRIVATE ${CMAKE_SOURCE_DIR}/thirdparty
)

target_link_libraries(pokepp
  PUBLIC  SDL2::SDL2 SDL2::SDL2main glad::glad OpenGL::GL glm::glm
)

# App executable - MOVED AFTER LIBRARY
add_executable(PokePlusPlus src/main.cpp "include/pokeapp/Constants.h" "src/core/Material.cpp" "include/pokeapp/World.h" "src/core/World.cpp" "include/pokeapp/Pokemon.h" "src/core/Pokemon.cpp" "include/pokeapp/PokemonController.h" "src/core/PokemonController.cpp" "include/pokeapp/Pokeball.h")
target_compile_definitions(PokePlusPlus PRIVATE SDL_MAIN_HANDLED)
target_link_libraries(PokePlusPlus PRIVATE pokepp)

# Custom commands - MOVED AFTER TARGET DEFINITIONS
add_custom_command(TARGET PokePlusPlus POST_BUILD
    COMMAND 
        ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets 
        $<TARGET_FILE_DIR:PokePlusPlus>/assets
    COMMENT "Copying assets directory..."
)

add_custom_command(TARGET PokePlusPlus POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:SDL2::SDL2> $<TARGET_FILE_DIR:PokePlusPlus>
)

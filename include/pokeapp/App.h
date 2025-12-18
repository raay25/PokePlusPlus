#pragma once

#include "pokeapp/World.h"
#include "pokeapp/Pokeball.h"
#include "pokeapp/Pokemon.h" 
#include <SDL.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

/*
	Header file for the main application class managing initialization, input handling,
	rendering, and game logic.
*/


// Forward declarations
class Shader;

namespace pokepp { 
    class Model; 
    class PokemonController; 
}

class App {
public:
    App();
    ~App();

    bool init();
    void tick();
    bool running() const { return running_; }

private:
    // Initialization methods
    bool initSDL();
    bool initOpenGL();
    bool initShaders();
    bool initGeometry();
    bool initUniforms();
    void cleanup();
    
    // Geometry setup
    void buildGrid(int half = 20, float spacing = 1.0f);
    void buildTrajectoryBuffer(int maxPoints = 64);
    void buildUIQuad(); 
    
    // Uniform management
    void getUniformLocations();
    void setDefaultUniforms();
    
    // Main update methods
    void updateTiming();
    void updatePhysics();
    void updateCameraMovement();
    void updateLighting();
    
    // Input handling methods
    void handleInput();
    void handleKeyDown(SDL_Keycode key);
    void handleMouseMotion(int xrel, int yrel);
    void handleMouseButtonDown(Uint8 button);
    void handleMouseButtonUp(Uint8 button);
    void handleWindowResize(int width, int height);
    void resetCamera();
    void updateCameraDirection();
    void handlePointLightKeys(SDL_Keycode key);
    
    // Rendering methods
    void render();
    void setupMainShader(const glm::mat4& view, const glm::mat4& proj);
    void setShaderMatrices(const glm::mat4& view, const glm::mat4& proj);
    void setShaderLighting();
    void drawGrid(const glm::mat4& view, const glm::mat4& proj);
    void drawTrajectory(const glm::mat4& view, const glm::mat4& proj);
    void drawTrajectory(const glm::mat4& view, const glm::mat4& proj, float previewSpeed);
    void drawPokeballs(const glm::mat4& view, const glm::mat4& proj);
    void drawLightGizmo(const glm::mat4& view, const glm::mat4& proj);
    void drawInventoryUI(); 
    
    // Projectile system
    void spawnPokeball();
    void spawnPokeball(float speed);
    void updatePokeballs(float dt);
    
    // Lighting helpers
    void uploadPointLightUniforms() const;
    
    // Props struct
    struct Prop {
        std::shared_ptr<pokepp::Model> model;
        glm::vec3 pos{ 0 };
        glm::vec3 scale{ 1.0f };
        glm::vec3 aabbMinLocal{ -0.5f };
        glm::vec3 aabbMaxLocal{ 0.5f };
    };
    
    void spawnRockAt(float x, float z, float scaleXZ = 1.0f, float scaleY = 1.0f);
    void scatterRocks(int count);
    void spawnTreeAt(float x, float z, float scaleXZ = 1.0f, float scaleY = 1.0f);
    void scatterTrees(int count);
    void spawnPokemonAt(float x, float z, float speed, float radius);
    void scatterPokemon(int count);
    
    // SDL/OpenGL
    SDL_Window* window_ = nullptr;
    SDL_GLContext glcontext_ = nullptr;
    
    // Window properties
    int width_ = 1280;
    int height_ = 720;
    bool running_ = false;
    
    // Shaders
    std::unique_ptr<Shader> shader_;
    std::unique_ptr<Shader> gizmoShader_;
    std::unique_ptr<Shader> unlit_;
    
    // Geometry
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
    GLuint tex_ = 0;
    GLuint gridVAO_ = 0, gridVBO_ = 0;
    int gridVertexCount_ = 0;
    GLuint trajVAO_ = 0, trajVBO_ = 0;
    int trajMaxPoints_ = 0;
    int trajCount_ = 0;
    GLuint uiQuadVAO_ = 0, uiQuadVBO_ = 0;
    
    // Pokeball
    std::unique_ptr<pokepp::Model> pokeballModel_;
    std::vector<pokepp::Pokeball> balls_;
	std::unique_ptr<Texture> pokeballTexture_;

    // World and props
    std::unique_ptr<pokepp::World> world_;
    std::vector<Prop> props_;
    std::shared_ptr<pokepp::Model> rockModel_;
    std::shared_ptr<pokepp::Model> treeModel_;
    std::shared_ptr<pokepp::Model> pokemonModel_;
    std::unique_ptr<pokepp::PokemonController> pokemonController_;
    
    // Uniform locations (main shader)
    GLint uTintLoc_ = -1;
    GLint uModelLoc_ = -1;
    GLint uViewLoc_ = -1;
    GLint uProjLoc_ = -1;
    GLint uViewPosLoc_ = -1;
    GLint uLightDirLoc_ = -1;
    GLint uLightColorLoc_ = -1;
    GLint uShininessLoc_ = -1;
    GLint uUseTextureLoc_ = -1;
    GLint uKdLoc_ = -1;
    
    // Point light uniforms
    GLint uPointPosLoc_ = -1;
    GLint uPointColorLoc_ = -1;
    GLint uPointIntensityLoc_ = -1;
    GLint uAttenConstLoc_ = -1;
    GLint uAttenLinearLoc_ = -1;
    GLint uAttenQuadLoc_ = -1;
    
    // Spotlight uniforms
    GLint uSpotPosLoc_ = -1;
    GLint uSpotDirLoc_ = -1;
    GLint uSpotCutLoc_ = -1;
    GLint uSpotOuterCutLoc_ = -1;
    
    // Terrain texture uniforms
    GLint uTexScaleLoc_ = -1;
    GLint uHasRockLoc_ = -1;
    GLint uTexLoc_ = -1;
    GLint uGrassLoc_ = -1;
    GLint uRockLoc_ = -1;
    
    // Uniform locations (gizmo shader)
    GLint gModelLoc_ = -1;
    GLint gViewLoc_ = -1;
    GLint gProjLoc_ = -1;
    GLint gColorLoc_ = -1;
    
    // Timing
    uint32_t lastTicks_ = 0;
    float dt_ = 0.0f;
    float t_ = 0.0f;
    
    // Camera
    glm::vec3 camPos_{ 0.0f, 2.0f, -5.0f };
    glm::vec3 camFront_{ 0.0f, 0.0f, -1.0f };
    glm::vec3 camUp_{ 0.0f, 1.0f, 0.0f };
    float yaw_ = -90.0f;
    float pitch_ = 0.0f;
    float mouseSens_ = 0.1f;
    float moveSpeed_ = 5.0f;
    
    // Player physics
    float verticalVelocity_ = 0.0f;
    bool isGrounded_ = true;
    float playerEyeHeight_ = 1.7f;
    float playerRadius_ = 0.4f;
    
    // Lighting
    glm::vec3 pointPos_{ 1.5f, 1.0f, 1.0f };
    glm::vec3 pointColor_{ 1.0f, 0.9f, 0.8f };
    float pointIntensity_ = 1.0f;
    float attenConst_ = 1.0f;
    float attenLinear_ = 0.09f;
    float attenQuad_ = 0.032f;
    bool flashlightOn_ = false;
    
    // Throw mechanics
    bool isCharging_ = false;
    float charge_ = 0.0f;
    float maxChargeSeconds_ = 2.0f;
    float minThrowSpeed_ = 5.0f;
    float maxThrowSpeed_ = 20.0f;
    
    // Physics
    float gravity_ = 9.8f;
    float bounceRestitution_ = 0.6f;

    // Pokemon species and models
    std::vector<pokepp::PokemonSpecies> pokemonSpecies_;
    std::vector<std::shared_ptr<pokepp::Model>> speciesModels_; 
};

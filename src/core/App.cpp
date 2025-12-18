#include "pokeapp/App.h"
#include "pokeapp/Constants.h"
#include "pokeapp/Shader.h"
#include "pokeapp/Mesh.h"
#include "pokeapp/Model.h"
#include "pokeapp/PokemonController.h"
#include "pokeapp/Pokeball.h"

#include <glad/glad.h>
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <cmath>
#include <algorithm>
#include <iostream>

/*
	The App class, which manages the main application loop, rendering, input handling,
	and game state.
*/

using namespace pokepp::constants;

namespace {
	// Visual constants (colors, directions)
	constexpr glm::vec3 DIRECTIONAL_LIGHT_DIR{ -0.2f, -1.0f, -0.3f };
	constexpr glm::vec3 DIRECTIONAL_LIGHT_COLOR{ 1.0f, 1.0f, 1.0f };
	constexpr glm::vec3 FLASHLIGHT_OFFSET{ 0.0f, -0.05f, 0.0f };
	constexpr glm::vec3 FLASHLIGHT_COLOR{ 1.0f, 0.96f, 0.9f };
	constexpr glm::vec3 CAMERA_RESET_FRONT{ 0.0f, -0.3f, -1.0f };
	constexpr glm::vec3 GRID_COLOR{ 0.25f, 0.25f, 0.25f };

	// Utility function to check for OpenGL errors
	void checkGLError(const char* operation) {
		GLenum error = glGetError();
		if (error != GL_NO_ERROR) {
			std::cerr << "OpenGL error after " << operation << ": 0x" << std::hex << error << std::endl;
		}
	}
}

App::App() = default;

App::~App() {
	cleanup();
}

// Initialize the application
bool App::init() {
	// Seed random number generator (for scattering rocks and Pokemon)
	std::srand(static_cast<unsigned int>(std::time(nullptr)));
	
	// Initialize key systems
	if (!initSDL()) return false;
	if (!initOpenGL()) return false;
	if (!initShaders()) return false;
	if (!initGeometry()) return false;
	if (!initUniforms()) return false;

	running_ = true;
	world_ = pokepp::World::FromHeightMap("assets/heightmaps/arena_heightmap.png", 0.5f, 5.0f); // Load heightmap world
	pokemonController_ = std::make_unique<pokepp::PokemonController>(); // Create Pokemon controller

	try {
		// Load our 3D models 
		rockModel_ = std::make_shared<pokepp::Model>("assets/models/rock.obj");
		treeModel_ = std::make_shared<pokepp::Model>("assets/models/tree.obj");
		auto pikachuModel = std::make_shared<pokepp::Model>("assets/models/pokemon/pikachu.obj");
		auto charmanderModel = std::make_shared<pokepp::Model>("assets/models/pokemon/charmander.obj");
		auto squirtleModel = std::make_shared<pokepp::Model>("assets/models/pokemon/squirtle.obj");
		auto bulbasaurModel = std::make_shared<pokepp::Model>("assets/models/pokemon/001.obj");
		
		// Register Pokemon species with their properties
		pokemonSpecies_.push_back({
			.name = "Pikachu",
			.model = pikachuModel.get(),
			.displayColor = glm::vec3(1.0f, 0.9f, 0.2f),
			.displayScale = 0.25f,
			.catchRate = 0.7f 
		});
		
		pokemonSpecies_.push_back({
			.name = "Charmander",
			.model = charmanderModel.get(),
			.displayColor = glm::vec3(1.0f, 0.5f, 0.1f),
			.displayScale = 0.7f,
			.catchRate = 0.3f // make Charmander harder to catch :)
		});
		
		pokemonSpecies_.push_back({
			.name = "Squirtle",
			.model = squirtleModel.get(),
			.displayColor = glm::vec3(0.3f, 0.6f, 1.0f),
			.displayScale = 0.85f,
			.catchRate = 0.5f 
		});
		
		pokemonSpecies_.push_back({
			.name = "Bulbasaur",
			.model = bulbasaurModel.get(),
			.displayColor = glm::vec3(0.3f, 0.8f, 0.4f),
			.displayScale = 100.00f,
			.catchRate = 0.5f 
		});
		
		// Store shared_ptrs to keep models alive
		speciesModels_.push_back(pikachuModel);
		speciesModels_.push_back(charmanderModel);
		speciesModels_.push_back(squirtleModel);
		speciesModels_.push_back(bulbasaurModel);
		
	} catch (const std::exception& e) {
		std::cerr << "Failed to load models: " << e.what() << std::endl;
	}

	lastTicks_ = SDL_GetTicks(); // Initialize timing, used for delta-time calculations

	// Populate the world with props and Pokemon
	scatterRocks(50);
	scatterPokemon(20);

	std::cout << "App initialized successfully!" << std::endl;
	return true;
}

// Main application tick/update, called once per frame. Calls various update methods.
void App::tick() {
	updateTiming();
	updatePhysics();
	handleInput();
	updateCameraMovement();
	updateLighting();
	render();
}

// Update timing information (delta time, physics timestep)
void App::updateTiming() {
	uint32_t now = SDL_GetTicks();
	dt_ = (now - lastTicks_) / 1000.0f;
	lastTicks_ = now;
	t_ += dt_;

	static float acc = 0.f;
	const float h = PHYSICS_TIMESTEP;
	acc += dt_;
	while (acc >= h) {
		updatePokeballs(h);
		acc -= h;
	}
}

// Update physics for player and Pokemon
void App::updatePhysics() {

	// Build obstacle list from props
	std::vector<glm::vec3> obstacles;
	obstacles.reserve(props_.size());
	for (const auto& prop : props_) {
		obstacles.push_back(prop.pos);
	}

	// Update all Pokemon with world and obstacle info
	if (pokemonController_) {
		pokemonController_->updateAll(dt_, world_.get(), obstacles);
		
		// Move captured Pokemon to inventory automatically
		pokemonController_->updateInventory();
	}
}

// Handle user input events
void App::handleInput() {
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			running_ = false;
			break;

		case SDL_KEYDOWN:
			handleKeyDown(e.key.keysym.sym);
			break;

		case SDL_MOUSEMOTION:
			handleMouseMotion(e.motion.xrel, e.motion.yrel);
			break;

		case SDL_MOUSEBUTTONDOWN:
			handleMouseButtonDown(e.button.button);
			break;

		case SDL_MOUSEBUTTONUP:
			handleMouseButtonUp(e.button.button);
			break;

		case SDL_WINDOWEVENT:
			if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
				handleWindowResize(e.window.data1, e.window.data2);
			}
			break;
		}
	}
}

// Handle key down events, including movement, camera reset, 
// flashlight toggle, and Pokemon management
void App::handleKeyDown(SDL_Keycode key) {
	switch (key) {
	case SDLK_ESCAPE:
		running_ = false;
		break;

	case SDLK_f:
		resetCamera();
		break;

	case SDLK_p:
		flashlightOn_ = !flashlightOn_;
		break;

	case SDLK_SPACE:
		if (isGrounded_) {
			verticalVelocity_ = JUMP_VELOCITY;
			isGrounded_ = false;
		}
		break;

	// Number keys 1-9 to toggle Pokemon out/in
	case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4: case SDLK_5:
	case SDLK_6: case SDLK_7: case SDLK_8: case SDLK_9:
		if (pokemonController_) {
			size_t index = static_cast<size_t>(key - SDLK_1);
			if (index < pokemonController_->getInventoryCount()) {
				// Toggle: If out, recall. If in, send out.
				if (pokemonController_->isPokemonOut(index)) {
					pokemonController_->recallPokemon(index);
				} else {
					// Send out Pokémon in front of player
					glm::vec3 sendOutPos = camPos_ + camFront_ * 3.0f;
					sendOutPos.y = world_ ? world_->heightAt(sendOutPos.x, sendOutPos.z) : 0.0f;
					pokemonController_->sendOutPokemon(index, sendOutPos);
				}
			} else {
				std::cout << "No Pokemon in slot " << (index + 1) << std::endl;
			}
		}
		break;

	default:
		handlePointLightKeys(key);
		break;
	}
}

// Handle mouse motion for camera rotation. Updates yaw and pitch based on mouse movement
// (Euler angles), then updates camera front vector.
void App::handleMouseMotion(int xrel, int yrel) {
	float xoffset = static_cast<float>(xrel) * mouseSens_;
	float yoffset = static_cast<float>(yrel) * mouseSens_;

	yaw_ += xoffset;
	pitch_ -= yoffset;

	pitch_ = glm::clamp(pitch_, -MAX_PITCH, MAX_PITCH);

	updateCameraDirection();
}

// Handle mouse button down events for charging pokeball throw
void App::handleMouseButtonDown(Uint8 button) {
	if (button == SDL_BUTTON_LEFT) {
		isCharging_ = true;
		charge_ = 0.0f;
	}
}

// Handle mouse button up events to release charged pokeball throw
void App::handleMouseButtonUp(Uint8 button) {
	if (button == SDL_BUTTON_LEFT && isCharging_) {
		isCharging_ = false;
		float t = glm::clamp(charge_, 0.0f, 1.0f);
		float speed = glm::mix(minThrowSpeed_, maxThrowSpeed_, t);
		spawnPokeball(speed);
	}
}

// Handle window resize events to adjust viewport and store new dimensions
void App::handleWindowResize(int width, int height) {
	width_ = width;
	height_ = height;
	glViewport(0, 0, width_, height_);
}

// Reset camera to default position and orientation
void App::resetCamera() {
	camPos_ = glm::vec3(0.0f, CAMERA_RESET_HEIGHT, CAMERA_RESET_DISTANCE);
	camFront_ = glm::normalize(CAMERA_RESET_FRONT);
	pitch_ = glm::degrees(asinf(camFront_.y));
	yaw_ = glm::degrees(atan2f(camFront_.z, camFront_.x));
}

// Update camera front vector based on current yaw and pitch angles
void App::updateCameraDirection() {
	float cy = cosf(glm::radians(yaw_)), sy = sinf(glm::radians(yaw_));
	float cp = cosf(glm::radians(pitch_)), sp = sinf(glm::radians(pitch_));
	camFront_ = glm::normalize(glm::vec3(cy * cp, sp, sy * cp));
}

// Update camera position based on input, collisions, and gravity
void App::updateCameraMovement() {

	// Helper lambda to push a point out of an AABB in the XZ plane. That is, it pushes the player
	// out of obstacles. Uses AABB (Axis-Aligned Bounding Box) collision detection.
	// Find the closest point on the obstacle to the player, and if the player is inside it then
	// calculate which edge is nearest and push them to that edge. Prevents things like walking through rocks.
	auto pushOutFromAABB_XZ = [&](glm::vec3& p, const glm::vec3& aabbWorldMin, const glm::vec3& aabbWorldMax) {
		glm::vec2 pos2(p.x, p.z);
		glm::vec2 bmin(aabbWorldMin.x - playerRadius_, aabbWorldMin.z - playerRadius_);
		glm::vec2 bmax(aabbWorldMax.x + playerRadius_, aabbWorldMax.z + playerRadius_);
		
		glm::vec2 clampPt;
		clampPt.x = glm::clamp(pos2.x, bmin.x, bmax.x);
		clampPt.y = glm::clamp(pos2.y, bmin.y, bmax.y);
		glm::vec2 delta = pos2 - clampPt;

		if (delta.x == 0.0f && delta.y == 0.0f) {
			float left = pos2.x - bmin.x;
			float right = bmax.x - pos2.x;
			float down = pos2.y - bmin.y;
			float up = bmax.y - pos2.y;
			float m = std::min(std::min(left, right), std::min(down, up));

			if (m == left) p.x = bmin.x;
			else if (m == right) p.x = bmax.x;
			else if (m == down) p.z = bmin.y;
			else p.z = bmax.y;
		}
	};

	// Read WASD input and calculate intended movement vector.
	const Uint8* ks = SDL_GetKeyboardState(nullptr);
	float velocity = moveSpeed_ * dt_;

	if (ks[SDL_SCANCODE_LSHIFT]) velocity *= SPRINT_MULTIPLIER;

	glm::vec3 right = glm::normalize(glm::cross(camFront_, camUp_));
	glm::vec3 fwd = glm::normalize(glm::vec3(camFront_.x, 0.0f, camFront_.z));

	// Apply WASD movement. 
	glm::vec3 next = camPos_;
	if (ks[SDL_SCANCODE_W]) next += fwd * velocity;
	if (ks[SDL_SCANCODE_S]) next -= fwd * velocity;
	if (ks[SDL_SCANCODE_A]) next -= right * velocity;
	if (ks[SDL_SCANCODE_D]) next += right * velocity;

	// Check collisions with props, and call our lambda to push the player out of obstacles.
	for (const auto& prop : props_) {
		glm::vec3 wmin = prop.pos + prop.scale * prop.aabbMinLocal;
		glm::vec3 wmax = prop.pos + prop.scale * prop.aabbMaxLocal;
		glm::vec3 realMin = glm::min(wmin, wmax);
		glm::vec3 realMax = glm::max(wmin, wmax);
		pushOutFromAABB_XZ(next, realMin, realMax);
	}

	// Handle gravity and ground collision. Physics!
	if (world_) {
		// Apply gravity to vertical velocity
		verticalVelocity_ -= GRAVITY * dt_;

		// Update Y position based on vertical velocity
		next.y = camPos_.y + verticalVelocity_ * dt_;

		// Get terrain info at new position. Use dot product to find slope angle.
		float groundY = world_->heightAt(next.x, next.z);
		glm::vec3 normal = world_->normalAt(next.x, next.z);
		float slopeAngle = glm::degrees(acosf(glm::clamp(glm::dot(normal, glm::vec3(0, 1, 0)), -1.0f, 1.0f)));

		constexpr float MAX_CLIMBABLE_SLOPE = 45.0f;
		constexpr float SLIDE_THRESHOLD = 35.0f;

		// ONLY block horizontal movement if grounded AND slope too steep.
		// This prevents things like walking up cliffs, instead making the player slide down. 
		if (isGrounded_ && slopeAngle > MAX_CLIMBABLE_SLOPE) {
			// Too steep - reject horizontal movement
			next.x = camPos_.x;
			next.z = camPos_.z;
			groundY = world_? world_->heightAt(next.x, next.z) : 0.0f;
		}

		// Check ground collision
		float targetY = groundY + playerEyeHeight_;
		if (next.y <= targetY) {
			next.y = targetY;
			verticalVelocity_ = 0.0f;
			isGrounded_ = true;

			// Apply sliding on steep slopes (only when grounded)
			if (slopeAngle > SLIDE_THRESHOLD) {
				glm::vec3 gravity(0, -1, 0);
				glm::vec3 slideDir = gravity - glm::dot(gravity, normal) * normal;
				if (glm::length(slideDir) > 0.001f) {
					slideDir = glm::normalize(slideDir);

					float slideSpeed = (slopeAngle - SLIDE_THRESHOLD) * 0.1f;
					next.x += slideDir.x * slideSpeed * dt_;
					next.z += slideDir.z * slideSpeed * dt_;

					// Recalculate ground height after sliding
					groundY = world_->heightAt(next.x, next.z);
					next.y = groundY + playerEyeHeight_;
				}
			}
		}
		else {
			isGrounded_ = false;
		}
	}
	else {
		next.y = playerEyeHeight_;
	}

	camPos_ = next;
}

// Update lighting parameters based on flashlight state
void App::updateLighting() {
	if (flashlightOn_) {
		pointPos_ = camPos_ + camFront_ * 0.5f + FLASHLIGHT_OFFSET;
		pointColor_ = FLASHLIGHT_COLOR;
		pointIntensity_ = FLASHLIGHT_INTENSITY;
		attenConst_ = 1.0f;
		attenLinear_ = 0.22f;
		attenQuad_ = 0.20f;
	}
	else {
		pointIntensity_ = 0.0f;
	}
}

// Heart of the application and graphics pipeline, handles all the drawing. 
void App::render() {
	// Clear screen to bright blue sky
	glClearColor(0.68f, 0.85f, 0.90f, 1.0f); 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Calculate matrices (view and projection)
	glm::mat4 view = glm::lookAt(camPos_, camPos_ + camFront_, camUp_);
	glm::mat4 proj = glm::perspective(glm::radians(DEFAULT_FOV),
		static_cast<float>(width_) / static_cast<float>(height_),
		NEAR_PLANE, FAR_PLANE);

	// Setup main shader
	setupMainShader(view, proj);

	// Set spotlight uniforms
	if (uSpotPosLoc_ != -1)  glUniform3f(uSpotPosLoc_, camPos_.x, camPos_.y, camPos_.z);
	if (uSpotDirLoc_ != -1)  glUniform3f(uSpotDirLoc_, camFront_.x, camFront_.y, camFront_.z);
	if (uSpotCutLoc_ != -1)  glUniform1f(uSpotCutLoc_, cosf(glm::radians(12.5f)));
	if (uSpotOuterCutLoc_ != -1) glUniform1f(uSpotOuterCutLoc_, cosf(glm::radians(17.5f)));

	// Reset ground color (select default diffuse color for terrain)
	if (uKdLoc_ != -1) {
		glUniform3f(uKdLoc_, 0.2f, 0.4f, 0.8f);
	}

	// Draw 3D world
	if (world_) {
		world_->draw(*shader_, view, proj);
	}

	// Draw pokeballs, grid, and trajectory preview
	drawPokeballs(view, proj);
	drawGrid(view, proj);
	drawTrajectory(view, proj);

	// Draw props
	if (!props_.empty()) {
		shader_->use();
		shader_->setInt("uHasRock", -1);
		shader_->setMat4("uView", glm::value_ptr(view));
		shader_->setMat4("uProj", glm::value_ptr(proj));

		for (const auto& prop : props_) {
			glm::mat4 model(1.0f); // Model matrix, transform from model to world space
			model = glm::translate(model, prop.pos);
			model = glm::scale(model, prop.scale);
			glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(model)));

			shader_->setMat4("uModel", glm::value_ptr(model));
			shader_->setMat3("uNormalMat", glm::value_ptr(normalMat));	

			if (uKdLoc_ != -1) {
				if (prop.model == treeModel_) {
					glUniform3f(uKdLoc_, 0.2f, 0.6f, 0.2f);
				} else {
					glUniform3f(uKdLoc_, 0.6f, 0.6f, 0.6f);
				}
			}

			shader_->setInt("uUseTexture", 0);
			prop.model->draw(*shader_);
		}
	}

	// Draw Pokemon
	if (pokemonController_) {
		shader_->use();
    
		//  Reset uHasRock so shader knows this is NOT terrain
		shader_->setInt("uHasRock", -1);
		pokemonController_->drawAll(*shader_);
	}

	// Draw 2D UI overlay (AFTER all 3D rendering)
	drawInventoryUI();

	SDL_GL_SwapWindow(window_);
}

// Setup main shader uniforms for view, projection, tint effect, and lighting.
// Convenience method for main Phong lighting shader. 
void App::setupMainShader(const glm::mat4& view, const glm::mat4& proj) {

	// Activate the shader
	shader_->use();

	// Set tint effect
	if (uTintLoc_ != -1) {
		float tint = TINT_AMPLITUDE * (0.5f * (std::sin(t_ * TINT_FREQUENCY) + 1.0f));
		glUniform1f(uTintLoc_, tint);
	}

	// Upload camera matrices and lighting parameters
	setShaderMatrices(view, proj);
	setShaderLighting();
}

// Upload view and projection matrices, and camera position to shader uniforms.
// Encapsulate OpenGL uniform upload calls.
void App::setShaderMatrices(const glm::mat4& view, const glm::mat4& proj) {
	if (uViewLoc_ != -1) glUniformMatrix4fv(uViewLoc_, 1, GL_FALSE, glm::value_ptr(view));
	if (uProjLoc_ != -1) glUniformMatrix4fv(uProjLoc_, 1, GL_FALSE, glm::value_ptr(proj));
	if (uViewPosLoc_ != -1) glUniform3fv(uViewPosLoc_, 1, glm::value_ptr(camPos_));
}

// Upload lighting parameters (directional light, point light, shininess) to shader uniforms.
// Encapsulate OpenGL uniform upload calls.
void App::setShaderLighting() {
	if (uLightDirLoc_ != -1) glUniform3fv(uLightDirLoc_, 1, glm::value_ptr(DIRECTIONAL_LIGHT_DIR));
	if (uLightColorLoc_ != -1) glUniform3fv(uLightColorLoc_, 1, glm::value_ptr(DIRECTIONAL_LIGHT_COLOR));
	if (uShininessLoc_ != -1) glUniform1f(uShininessLoc_, DEFAULT_SHININESS);

	uploadPointLightUniforms();
}

// Draw the ground grid for reference/debugging
void App::drawGrid(const glm::mat4& view, const glm::mat4& proj) {
	unlit_->use();
	unlit_->setMat4("uView", glm::value_ptr(view));
	unlit_->setMat4("uProj", glm::value_ptr(proj));

	glm::mat4 gridModel(1.0f);
	unlit_->setMat4("uModel", glm::value_ptr(gridModel));

	GLint colorLoc = glGetUniformLocation(unlit_->getProgram(), "uColor");
	if (colorLoc >= 0) glUniform3fv(colorLoc, 1, glm::value_ptr(GRID_COLOR));

	glBindVertexArray(gridVAO_);
	glDrawArrays(GL_LINES, 0, gridVertexCount_);
	glBindVertexArray(0);
}

// Draw the trajectory preview when charging a pokeball throw, calls overloaded method. 
// This is the one called per frame.
void App::drawTrajectory(const glm::mat4& view, const glm::mat4& proj) {
	if (!isCharging_) return;

	charge_ = glm::clamp(charge_ + dt_ / maxChargeSeconds_, 0.0f, 1.0f);
	float previewSpeed = glm::mix(minThrowSpeed_, maxThrowSpeed_, charge_);
	drawTrajectory(view, proj, previewSpeed);
}

// Draw the trajectory preview when charging a pokeball throw, overloaded with speed parameter.
// Physics!
void App::drawTrajectory(const glm::mat4& view, const glm::mat4& proj, float previewSpeed) {

	// Store the predicted trajectory points
	std::vector<glm::vec3> pts;
	pts.reserve(trajMaxPoints_);

	// Calculate the initial conditions for the trajectory simulation
	glm::vec3 p0 = camPos_ + camFront_ * PROJECTILE_SPAWN_DISTANCE;
	glm::vec3 v0 = glm::normalize(camFront_) * previewSpeed + glm::vec3(0.0f, PROJECTILE_UPWARD_VELOCITY, 0.0f);

	// Physics simulation loop. Integrate projectile motion with gravity and simple bounce.
	const float dt = 1.0f / TRAJECTORY_SIMULATION_FPS;
	glm::vec3 p = p0, v = v0;

	for (int i = 0; i < trajMaxPoints_; ++i) {
		pts.push_back(p); // save point

		// Euler integration (update physics)
		v.y -= gravity_ * dt; // apply gravity to Y velocity
		p += v * dt; // update position based on velocity

		// simple bounce preview (one reflection)
		if (p.y <= GROUND_Y) {
			p.y = GROUND_Y;
			v.y = -v.y * glm::clamp(bounceRestitution_, 0.0f, 1.0f);
			v.x *= BOUNCE_FRICTION;
			v.z *= BOUNCE_FRICTION;
		}
	}
	GLint psLoc = glGetUniformLocation(unlit_->getProgram(), "uPtSize");
	if (psLoc >= 0) glUniform1f(psLoc, TRAJECTORY_PREVIEW_POINT_SIZE);

	// Upload and draw with unlit shader (since we just want solid color points - no lighting)
	glBindBuffer(GL_ARRAY_BUFFER, trajVBO_);
	glBufferSubData(GL_ARRAY_BUFFER, 0, pts.size() * sizeof(glm::vec3), pts.data());
	trajCount_ = static_cast<int>(pts.size());

	unlit_->use();
	unlit_->setMat4("uView", glm::value_ptr(view));
	unlit_->setMat4("uProj", glm::value_ptr(proj));

	glm::mat4 M(1.0f); // identity model matrix (no transformations!)
	unlit_->setMat4("uModel", glm::value_ptr(M));

	// vivid color that scales with charge
	GLint cLoc = glGetUniformLocation(unlit_->getProgram(), "uColor");
	if (cLoc >= 0) {
		float t = (maxThrowSpeed_ > minThrowSpeed_)
			? (previewSpeed - minThrowSpeed_) / (maxThrowSpeed_ - minThrowSpeed_)
			: 0.0f;
		// green->yellow->red
		glm::vec3 col = glm::mix(glm::vec3(0.1f, 1.0f, 0.1f), glm::vec3(1.0f, 0.2f, 0.2f), t);
		glUniform3f(cLoc, col.x, col.y, col.z);
	}

	// Render trajectory points
	glBindVertexArray(trajVAO_);
	glPointSize(TRAJECTORY_POINT_SIZE);
	glDrawArrays(GL_POINTS, 0, trajCount_);
	glBindVertexArray(0);

	// restore main shader if needed
	shader_->use();
}

// Spawn a pokeball with default speed. Calls overloaded method.
void App::spawnPokeball() {
	spawnPokeball(5.0f);  // Default speed
}

// Spawn a pokeball at the camera position, moving in the camera front direction with given speed.
// Overloaded with speed parameter. 
void App::spawnPokeball(float speed) {
	glm::vec3 spawnPos = camPos_ + camFront_ * PROJECTILE_SPAWN_DISTANCE;
	glm::vec3 spawnVel = glm::normalize(camFront_) * speed + glm::vec3(0.0f, PROJECTILE_UPWARD_VELOCITY, 0.0f); 
	// --> Horizontal component + verticalcomponent! Projectile motion!

	// Create and initialize the pokeball
	pokepp::Pokeball ball;
	ball.position = spawnPos;
	ball.velocity = spawnVel;
	ball.radius = PROJECTILE_RADIUS;
	ball.life = PROJECTILE_LIFETIME;
	
	balls_.push_back(ball);
}

// Update all active pokeballs, applying physics, collisions, and capture logic. This is, in a sense, the 
// physics and game logic engine for all active pokeballs. Handles projectile motion, collisions, captures,
// and cleanup. 
void App::updatePokeballs(float dt) {
	float step = glm::min(dt, 1.0f / 60.0f); // limit time step (60FPS)

	constexpr float SHAKE_DURATION = 0.6f;
	constexpr int MAX_SHAKES = 3;

	// Helper lambda function, used for Pokeball collision detection with rocks. 
	auto closestPointOnAABB = [](const glm::vec3& p, const glm::vec3& aabbMin, const glm::vec3& aabbMax) {
		glm::vec3 cp;
		cp.x = glm::clamp(p.x, aabbMin.x, aabbMax.x);
		cp.y = glm::clamp(p.y, aabbMin.y, aabbMax.y);
		cp.z = glm::clamp(p.z, aabbMin.z, aabbMax.z);
		return cp;
	};

	// Main update loop, iterates through all the pokeballs
	for (auto& b : balls_) {

		// If locked, do horizontal shake animation!
		if (b.locked) {
			b.lockTimer += dt;
			
			// Initialize base position on first frame of lock
			if (b.lockTimer <= dt) {
				b.captureBasePos = b.position;
			}
			
			b.shakePhase += dt / SHAKE_DURATION;

			if (b.shakePhase >= 1.0f) {
				b.shakePhase = 0.0f;
				b.shakeCount++;
				
				// After 3rd shake, finalize capture result
				if (b.shakeCount >= MAX_SHAKES) {
					if (pokemonController_) {
						// Find the Pokemon being captured at this ball's position
						auto& activePokemon = pokemonController_->getPokemon();
						for (auto& p : activePokemon) {
							if (p.isCapturing() && glm::length(p.getPosition() - b.captureBasePos) < 1.0f) {
								if (b.captureSuccess) p.markCaptured();
								else p.markCaptureFailed(); 
								break;
							}
						}
					}
				}
			}

			// Only shake during first 3 shakes
			if (b.shakeCount < MAX_SHAKES) {
				float t = b.shakePhase;
				float shakeOffset = 0.12f * std::sin(t * 6.28318f);
				
				b.position.x = b.captureBasePos.x + shakeOffset;
				b.position.y = b.captureBasePos.y;
				b.position.z = b.captureBasePos.z;
			}
			continue;
		}

		if (!b.active) continue; // Skip inactive balls

		// Here, the ball is active - it is flying through the air!

		// Apply gravity (simple Euler integration)
		b.velocity.y -= gravity_ * step;
		b.position += b.velocity * step;

		// Check collision with props (AABB)
		for (const auto& prop : props_) {
			glm::vec3 aabbMin = prop.pos + prop.scale * prop.aabbMinLocal;
			glm::vec3 aabbMax = prop.pos + prop.scale * prop.aabbMaxLocal;
			glm::vec3 closestPt = closestPointOnAABB(b.position, aabbMin, aabbMax);
			glm::vec3 delta = b.position - closestPt;
			float distSq = glm::dot(delta, delta);
			float radiusSq = b.radius * b.radius;

			if (distSq < radiusSq) { // collision!

				// Find collision normal. 
				float dist = glm::sqrt(distSq);
				glm::vec3 collisionNormal = (dist > 0.0001f) ? (delta / dist) : glm::vec3(0.0f, 1.0f, 0.0f);

				float penetration = b.radius - dist; // get penetration depth
				b.position += collisionNormal * penetration; // push ball out of collision
				
				// Bounce physics
				if (glm::dot(b.velocity, collisionNormal) < 0.0f) {
					glm::vec3 reflection = glm::reflect(b.velocity, collisionNormal);
					b.velocity = reflection * bounceRestitution_;

					// Split velocity into normal and tangent components for friction
					glm::vec3 normalVel = collisionNormal * glm::dot(b.velocity, collisionNormal);
					glm::vec3 tangentVel = b.velocity - normalVel;
					tangentVel *= BOUNCE_FRICTION;
					b.velocity = normalVel + tangentVel;
				}
			}
		}

		// Terrain collision
		float terrainHeight = GROUND_Y;
		glm::vec3 terrainNormal = glm::vec3(0.0f, 1.0f, 0.0f);

		if (world_) {
			terrainHeight = world_->heightAt(b.position.x, b.position.z);
			terrainNormal = world_->normalAt(b.position.x, b.position.z);
		}

		// Terrain collision response
		glm::vec3 terrainPoint(b.position.x, terrainHeight, b.position.z);
		glm::vec3 toBall = b.position - terrainPoint;
		float distToTerrain = glm::dot(toBall, terrainNormal);

		if (distToTerrain < b.radius) { // Ball is penetrating terrain. Same logic with props.
			float penetration = b.radius - distToTerrain;
			b.position += terrainNormal * penetration;

			if (glm::dot(b.velocity, terrainNormal) < 0.0f) {
				glm::vec3 reflection = glm::reflect(b.velocity, terrainNormal);
				b.velocity = reflection * bounceRestitution_;
				glm::vec3 normalVel = terrainNormal * glm::dot(b.velocity, terrainNormal);
				glm::vec3 tangentVel = b.velocity - normalVel;
				tangentVel *= BOUNCE_FRICTION;
				b.velocity = normalVel + tangentVel;
			}
		}

		b.life -= step;
	}

	// Capture logic
	if (pokemonController_) {
		pokemonController_->handlePokeballCapture(balls_);
	}

	// Remove expired pokeballs
	balls_.erase(std::remove_if(balls_.begin(), balls_.end(),
	[](const pokepp::Pokeball& p) { 
		return p.life <= 0.0f || (p.locked && p.lockTimer > 2.8f);
	}),
	balls_.end());
}

// Draw all active pokeballs in the scene. This includes pokeballs in midair, pokeballs in the middle of a
// capture animation, etc. 
void App::drawPokeballs(const glm::mat4& view, const glm::mat4& proj) {
    if (balls_.empty() || !pokeballModel_) {
        return;
    }

    // Reset uHasRock so shader knows this is NOT terrain
    shader_->setInt("uHasRock", -1); 

	// Main render loop, going through each ball in the world. 
    for (size_t i = 0; i < balls_.size(); ++i) {
        const auto& b = balls_[i];
        
        if (!b.active && !b.locked) continue;

        glm::mat4 M(1.0f); // Model matrix
        M = glm::translate(M, b.position); // add translation

		// Shake animation
        if (b.locked && b.shakeCount < 3) {
            float t = b.shakePhase;
            float angle = 25.0f * std::sin(t * 6.28318f); // radians 
            M = glm::rotate(M, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f)); // add rotation matrix
        }

		// Scale pokeball (shrink if captured)
        float scale = b.radius;
        if (b.locked && b.shakeCount >= 3) {
            float shrinkFactor = 0.3f + 0.7f * std::exp(-2.0f * b.lockTimer);
            scale *= shrinkFactor;
        }
        
        M = glm::scale(M, glm::vec3(scale)); // add scaling matrix

		// Upload model matrix. 
        shader_->setMat4("uModel", glm::value_ptr(M)); 

		// Calculate normal matrix (address distortion from non-uniform scaling)
        glm::mat3 N = glm::transpose(glm::inverse(glm::mat3(M)));
        GLint uNormalLoc = glGetUniformLocation(shader_->getProgram(), "uNormalMat");
        if (uNormalLoc >= 0)
            glUniformMatrix3fv(uNormalLoc, 1, GL_FALSE, glm::value_ptr(N));

		// Draw model. 
        pokeballModel_->draw(*shader_);
    }
}

// Spawns a rock at the specified coordinates. It queries the world height at that 
// position to place it on the terrain.
void App::spawnRockAt(float x, float z, float scaleXZ, float scaleY) {
	if (!world_ || !rockModel_) return;

	float y = world_->heightAt(x, z);

	Prop p;
	p.model = rockModel_;
	p.pos = { x, y, z };
	p.scale = { scaleXZ, scaleY, scaleXZ };
	p.aabbMinLocal = { -0.5f, 0.0f, -0.5f }; // in LOCAL space. 
	p.aabbMaxLocal = { 0.5f, 0.6f, 0.5f };

	props_.emplace_back(std::move(p));
}

// Scatter multiple rocks randomly across the terrain, ensuring they are placed
// on relatively flat ground.
void App::scatterRocks(int count) {
	if (!world_) return;
	auto random = []() { return float(rand()) / float(RAND_MAX); };

	float maxRange = 100.0f;

	for (int i = 0; i < count; i++) {
		float x = glm::mix(-maxRange + 2.0f, maxRange - 2.0f, random());
		float z = glm::mix(-maxRange + 2.0f, maxRange - 2.0f, random());

		glm::vec3 n = world_->normalAt(x, z);
		if (glm::dot(n, glm::vec3(0, 1, 0)) < 0.90f) { // slope too steep
			i--;
			continue;
		}

		float sXZ = 0.9f + 0.8f * random();
		float sY = 0.7f + 0.6f * random();
		spawnRockAt(x, z, sXZ, sY);
	}
}

// Spawn a Pokemon at the specified coordinates. It queries the world height at that
// position to place it on the terrain. 
void App::spawnPokemonAt(float x, float z, float speed, float radius) {
	if (!world_ || pokemonSpecies_.empty() || !pokemonController_) return;

	float y = world_->heightAt(x, z);

	// Pick a random species
	auto random = []() { return float(rand()) / float(RAND_MAX); };
	int speciesIdx = static_cast<int>(random() * pokemonSpecies_.size());
	const pokepp::PokemonSpecies* species = &pokemonSpecies_[speciesIdx];

	// Use new species-based API
	pokemonController_->spawnPokemon(species, glm::vec3(x, y, z), speed, radius);
}

// Scatter multiple Pokemon randomly across the terrain, ensuring they are placed
// on relatively flat ground.
void App::scatterPokemon(int count) {
	if (!world_ || pokemonSpecies_.empty() || !pokemonController_) return;

	auto random = []() { return float(rand()) / float(RAND_MAX); };
	float maxRange = 100.0f;

	for (int i = 0; i < count; i++) {
		float x = glm::mix(-maxRange + 5.0f, maxRange - 5.0f, random());
		float z = glm::mix(-maxRange + 5.0f, maxRange - 5.0f, random());

		glm::vec3 normal = world_->normalAt(x, z);
		float slope = glm::dot(normal, glm::vec3(0, 1, 0));

		if (slope < 0.85f) { // slope too steep
			i--;
			continue;
		}

		float speed = 1.5f + 1.5f * random();
		float radius = 0.4f + 0.2f * random();

		// Pick random species
		int speciesIdx = static_cast<int>(random() * pokemonSpecies_.size());
		const pokepp::PokemonSpecies* species = &pokemonSpecies_[speciesIdx];

		float y = world_->heightAt(x, z);
		pokemonController_->spawnPokemon(species, glm::vec3(x, y, z), speed, radius);
	}
}

// Initialize SDL, create window and OpenGL context
bool App::initSDL() {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
		std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return false;
	}

	// Set OpenGL attributes
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	// Create window
	window_ = SDL_CreateWindow(
		"PokePlusPlus",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		width_, height_,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
	);

	if (!window_) {
		std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		return false;
	}

	// Create OpenGL context
	glcontext_ = SDL_GL_CreateContext(window_);
	if (!glcontext_) {
		std::cerr << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
		return false;
	}

	SDL_GL_MakeCurrent(window_, glcontext_);
	SDL_GL_SetSwapInterval(1); // Enable vsync
	SDL_SetRelativeMouseMode(SDL_TRUE); // Capture mouse

	return true;
}

// Initialize OpenGL state and load function pointers
bool App::initOpenGL() {
	// Load OpenGL functions
	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
		std::cerr << "Failed to initialize OpenGL context" << std::endl;
		return false;
	}

	// Set OpenGL state
	glViewport(0, 0, width_, height_);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_PROGRAM_POINT_SIZE);

	checkGLError("OpenGL initialization");
	return true;
}

// Initialize the shaders used in the application
bool App::initShaders() {
	// Main shader
	shader_ = std::make_unique<Shader>();
	if (!shader_->loadFromFiles("shaders/phong.vert", "shaders/phong.frag")) {
		std::cerr << "Failed to load phong shaders" << std::endl;
		return false;
	}

	// Unlit shader
	unlit_ = std::make_unique<Shader>();
	if (!unlit_->loadFromFiles("shaders/unlit.vert", "shaders/unlit.frag")) {
		std::cerr << "Failed to load unlit shaders" << std::endl;
		return false;
	}

	std::cout << "Shaders loaded successfully" << std::endl;
	return true;
}

// Initialize geometry: load models, create VAOs/VBOs
bool App::initGeometry() {
	try {
		pokeballModel_ = std::make_unique<pokepp::Model>("assets/models/pokeball.obj");
		pokeballTexture_ = std::make_unique<Texture>("assets/models/textures/pokeball.png", Texture::Kind::Diffuse);
	}
	catch (const std::exception& e) {
		std::cerr << "Failed to load pokeball model: " << e.what() << std::endl;
		return false;
	}

	// Build grid and trajectory buffers
	buildGrid(GRID_SIZE, GRID_SPACING);
	buildTrajectoryBuffer(64);
	buildUIQuad();  // NEW: Build UI quad for inventory

	std::cout << "Geometry initialized successfully" << std::endl;
	return true;
}

// Initialize shader uniform locations and set default values
bool App::initUniforms() {
	// Get uniform locations for main shader
	shader_->use();
	getUniformLocations();

	// Set default uniform values
	setDefaultUniforms();

	// Switch back to main shader
	shader_->use();

	checkGLError("uniform initialization");
	return true;
}

// Retrieve uniform locations from the main shader program. These are grabbed from the
// compiled shader and stored in member variables for later use.
void App::getUniformLocations() {
	GLuint program = shader_->getProgram();

	uTintLoc_ = glGetUniformLocation(program, "uTint");
	uModelLoc_ = glGetUniformLocation(program, "uModel");
	uViewLoc_ = glGetUniformLocation(program, "uView");
	uProjLoc_ = glGetUniformLocation(program, "uProj");
	uViewPosLoc_ = glGetUniformLocation(program, "uViewPos");
	uLightDirLoc_ = glGetUniformLocation(program, "uLightDir");
	uLightColorLoc_ = glGetUniformLocation(program, "uLightColor");
	uShininessLoc_ = glGetUniformLocation(program, "uShininess");
	uUseTextureLoc_ = glGetUniformLocation(program, "uUseTexture");
	uKdLoc_ = glGetUniformLocation(program, "uKd");

	// Point light uniforms
	uPointPosLoc_ = glGetUniformLocation(program, "uPointPos");
	uPointColorLoc_ = glGetUniformLocation(program, "uPointColor");
	uPointIntensityLoc_ = glGetUniformLocation(program, "uPointIntensity");
	uAttenConstLoc_ = glGetUniformLocation(program, "uAttenConst");
	uAttenLinearLoc_ = glGetUniformLocation(program, "uAttenLinear");
	uAttenQuadLoc_ = glGetUniformLocation(program, "uAttenQuad");

	uSpotPosLoc_ = glGetUniformLocation(program, "uSpotPos");
	uSpotDirLoc_ = glGetUniformLocation(program, "uSpotDir");
	uSpotCutLoc_ = glGetUniformLocation(program, "uSpotCut");
	uSpotOuterCutLoc_ = glGetUniformLocation(program, "uSpotOuterCut");

	// Terrain texture uniforms
	uTexScaleLoc_ = glGetUniformLocation(program, "uTexScale");
	uHasRockLoc_ = glGetUniformLocation(program, "uHasRock");
	uTexLoc_ = glGetUniformLocation(program, "uTex");
	uGrassLoc_ = glGetUniformLocation(program, "uGrass");
	uRockLoc_ = glGetUniformLocation(program, "uRock");
}

// Set default values for shader uniforms, including lighting parameters
void App::setDefaultUniforms() {
	// Set default values
	if (uTintLoc_ != -1) glUniform1f(uTintLoc_, 0.0f);
	if (uShininessLoc_ != -1) glUniform1f(uShininessLoc_, DEFAULT_SHININESS);
	if (uKdLoc_ != -1) glUniform3f(uKdLoc_, 0.2f, 0.4f, 0.8f);
	if (uUseTextureLoc_ != -1) glUniform1i(uUseTextureLoc_, 0);
	if (uHasRockLoc_ != -1) glUniform1i(uHasRockLoc_, -1);

	// Initialize point light
	pointPos_ = glm::vec3(1.5f, 1.0f, 1.0f);
	pointColor_ = glm::vec3(1.0f, 0.9f, 0.8f);
	pointIntensity_ = POINT_LIGHT_INTENSITY;
	attenConst_ = ATTENUATION_CONSTANT;
	attenLinear_ = ATTENUATION_LINEAR;
	attenQuad_ = ATTENUATION_QUADRATIC;

	uploadPointLightUniforms();
}

// Cleanup OpenGL resources and SDL, free memory
void App::cleanup() {
	// Clean up OpenGL resources
	if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
	if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
	if (ebo_) { glDeleteBuffers(1, &ebo_); ebo_ = 0; }
	if (tex_) { glDeleteTextures(1, &tex_); tex_ = 0; }
	if (gridVAO_) { glDeleteVertexArrays(1, &gridVAO_); gridVAO_ = 0; }
	if (gridVBO_) { glDeleteBuffers(1, &gridVBO_); gridVBO_ = 0; }
	if (trajVAO_) { glDeleteVertexArrays(1, &trajVAO_); trajVAO_ = 0; }
	if (trajVBO_) { glDeleteBuffers(1, &trajVBO_); trajVBO_ = 0; }
	if (uiQuadVAO_) { glDeleteVertexArrays(1, &uiQuadVAO_); uiQuadVAO_ = 0; }
	if (uiQuadVBO_) { glDeleteBuffers(1, &uiQuadVBO_); uiQuadVBO_ = 0; }

	// Clean up SDL
	if (glcontext_) { SDL_GL_DeleteContext(glcontext_); glcontext_ = nullptr; }
	if (window_) { SDL_DestroyWindow(window_); window_ = nullptr; }
	SDL_Quit();
}

// Upload point light parameters to shader uniforms.
void App::uploadPointLightUniforms() const {
	if (uPointPosLoc_ != -1) glUniform3f(uPointPosLoc_, pointPos_.x, pointPos_.y, pointPos_.z);
	if (uPointColorLoc_ != -1) glUniform3f(uPointColorLoc_, pointColor_.x, pointColor_.y, pointColor_.z);
	if (uPointIntensityLoc_ != -1) glUniform1f(uPointIntensityLoc_, pointIntensity_);
	if (uAttenConstLoc_ != -1) glUniform1f(uAttenConstLoc_, attenConst_);
	if (uAttenLinearLoc_ != -1) glUniform1f(uAttenLinearLoc_, attenLinear_);
	if (uAttenQuadLoc_ != -1) glUniform1f(uAttenQuadLoc_, attenQuad_);
}

// Handle keyboard input to move the point light and adjust its intensity.
void App::handlePointLightKeys(SDL_Keycode key) {
	float step = 0.25f;
	if (key == SDLK_j) pointPos_.x -= step;
	if (key == SDLK_l) pointPos_.x += step;
	if (key == SDLK_i) pointPos_.y += step;
	if (key == SDLK_k) pointPos_.y -= step;
	if (key == SDLK_u) pointPos_.z -= step;
	if (key == SDLK_o) pointPos_.z += step;
	if (key == SDLK_LEFTBRACKET)  pointIntensity_ = glm::max(0.0f, pointIntensity_ - 0.1f);
	if (key == SDLK_RIGHTBRACKET) pointIntensity_ += 0.1f;
}

// Build the ground grid geometry for reference/debugging (no longer rely on)
void App::buildGrid(int half, float spacing) {
	std::vector<glm::vec3> lines;
	lines.reserve((half * 2 + 1) * 4);
	float extent = half * spacing;

	for (int i = -half; i <= half; ++i) {
		float x = i * spacing;
		lines.emplace_back(-extent, GROUND_Y, x);
		lines.emplace_back(extent, GROUND_Y, x);
		lines.emplace_back(x, GROUND_Y, -extent);
		lines.emplace_back(x, GROUND_Y, extent);
	}

	gridVertexCount_ = static_cast<int>(lines.size());

	glGenVertexArrays(1, &gridVAO_);
	glGenBuffers(1, &gridVBO_);
	glBindVertexArray(gridVAO_);
	glBindBuffer(GL_ARRAY_BUFFER, gridVBO_);
	glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(glm::vec3), lines.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

// Build the trajectory preview buffer for the dynamic trajectory point updates (charge line).
// That is, preallocate GPU buffers to hold trajectory points to avoid reallocating every frame.
void App::buildTrajectoryBuffer(int maxPoints) {
	trajMaxPoints_ = maxPoints;
	glGenVertexArrays(1, &trajVAO_);
	glGenBuffers(1, &trajVBO_);

	glBindVertexArray(trajVAO_);
	glBindBuffer(GL_ARRAY_BUFFER, trajVBO_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * trajMaxPoints_, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glBindVertexArray(0);
}

// Build the quad for rendering the inventory UI.
void App::buildUIQuad() {
    float quadVertices[] = {
        -1.0f,  1.0f,    0.0f, 1.0f,
        -1.0f, -1.0f,    0.0f, 0.0f,
         1.0f, -1.0f,    1.0f, 0.0f,
        
        -1.0f,  1.0f,    0.0f, 1.0f,
         1.0f, -1.0f,    1.0f, 0.0f,
         1.0f,  1.0f,    1.0f, 1.0f
    };

    glGenVertexArrays(1, &uiQuadVAO_);
    glGenBuffers(1, &uiQuadVBO_);
    
    glBindVertexArray(uiQuadVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, uiQuadVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindVertexArray(0);
}

// Draw the inventory UI in the top-left corner of the screen. Includes the spinning 
// Pokemon model. 
void App::drawInventoryUI() {
    if (!pokemonController_ || pokemonController_->getInventoryCount() == 0) {
        return;
    }

	// Save OpenGL state so we can restore it later
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    
    glDisable(GL_CULL_FACE); // don't cull the 2D quads
	glEnable(GL_BLEND); // enabletransparency
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Use unlit shader for UI backgrounds (no lighting)
    unlit_->use();
    
    glm::mat4 orthoProj = glm::ortho(0.0f, static_cast<float>(width_), 
                                      0.0f, static_cast<float>(height_));
    
    unlit_->setMat4("uProj", glm::value_ptr(orthoProj));
    unlit_->setMat4("uView", glm::value_ptr(glm::mat4(1.0f)));

    const auto& inventory = pokemonController_->getInventory();
    size_t count = std::min(inventory.size(), size_t(6));

    const float slotSize = 70.0f;
    const float slotSpacing = 10.0f;
    const float startY = static_cast<float>(height_) - slotSpacing - slotSize;

    GLint colorLoc = glGetUniformLocation(unlit_->getProgram(), "uColor");

    // Create VAO/VBO for background and border
    float unitSquare[] = {
        0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,
        0.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f
    };

    float borderLine[] = {
        0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 0.0f
    };

    GLuint bgVAO, bgVBO;
    glGenVertexArrays(1, &bgVAO);
    glGenBuffers(1, &bgVBO);
    glBindVertexArray(bgVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bgVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(unitSquare), unitSquare, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    GLuint borderVAO, borderVBO;
    glGenVertexArrays(1, &borderVAO);
    glGenBuffers(1, &borderVBO);
    glBindVertexArray(borderVAO);
    glBindBuffer(GL_ARRAY_BUFFER, borderVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(borderLine), borderLine, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);

	// === Draw each inventory slot ===
    for (size_t i = 0; i < count; ++i) {
        float yPos = startY - i * (slotSize + slotSpacing);
        bool isOut = pokemonController_->isPokemonOut(i);
        
        glm::vec3 slotColor = isOut 
            ? glm::vec3(0.2f, 1.0f, 0.3f)
            : glm::vec3(0.8f, 0.8f, 0.8f);

        // Background
        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(slotSpacing, yPos, 0.0f));
        model = glm::scale(model, glm::vec3(slotSize, slotSize, 1.0f));
        unlit_->setMat4("uModel", glm::value_ptr(model));
        if (colorLoc >= 0) glUniform3f(colorLoc, slotColor.r, slotColor.g, slotColor.b);
        glBindVertexArray(bgVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // Border
        float borderThickness = 2.0f;
        float borderSize = slotSize + (borderThickness * 2.0f);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(slotSpacing - borderThickness, yPos - borderThickness, 0.0f));
        model = glm::scale(model, glm::vec3(borderSize, borderSize, 1.0f));
        unlit_->setMat4("uModel", glm::value_ptr(model));
        
        glm::vec3 borderColor = isOut 
            ? glm::vec3(0.05f, 0.6f, 0.1f)
            : glm::vec3(0.3f, 0.3f, 0.3f);
        if (colorLoc >= 0) glUniform3f(colorLoc, borderColor.r, borderColor.g, borderColor.b);
        
        glLineWidth(3.0f);
        glBindVertexArray(borderVAO);
        glDrawArrays(GL_LINE_STRIP, 0, 5);
        glBindVertexArray(0);
    }

    // Clean up 2D geometry
    glDeleteVertexArrays(1, &bgVAO);
    glDeleteBuffers(1, &bgVBO);
    glDeleteVertexArrays(1, &borderVAO);
    glDeleteBuffers(1, &borderVBO);

	// === Draw 3D pokemon models within each slot ===
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);
	shader_->use();
	
	// Reset uHasRock for proper texture sampling
	shader_->setInt("uHasRock", -1);

	for (size_t i = 0; i < count; ++i) {
		const auto& pokemon = inventory[i];
		pokepp::Model* model = pokemon.getModel();
		const pokepp::PokemonSpecies* species = pokemon.getSpecies();
		if (!model) continue;

		float yPos = startY - i * (slotSize + slotSpacing);

		// Create a mini perspective camera for this slot
		float aspect = 1.0f;
		glm::mat4 miniProj = glm::perspective(glm::radians(45.0f), aspect, 0.001f, 100.0f);
		glm::vec3 modelPos(0.0f, 0.18f, 0.0f);
		glm::vec3 cameraPos(0.0f, 0.5f, 0.8f);
		glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
		glm::mat4 miniView = glm::lookAt(cameraPos, modelPos, cameraUp);

		// Set up scissor test to only render inside the slot.
		// This clips rendering to a rectangle, preventing overflow between slots.
		glEnable(GL_SCISSOR_TEST);
		int scissorX = static_cast<int>(slotSpacing);
		int scissorY = static_cast<int>(yPos);
		int scissorW = static_cast<int>(slotSize);
		int scissorH = static_cast<int>(slotSize);
		glScissor(scissorX, scissorY, scissorW, scissorH);

		// Set up viewport for this slot
		glViewport(scissorX, scissorY, scissorW, scissorH);

		// Set shader uniforms
		shader_->setMat4("uView", glm::value_ptr(miniView));
		shader_->setMat4("uProj", glm::value_ptr(miniProj));

		// Model matrix - add rotations.
		glm::mat4 modelMat(1.0f);
		modelMat = glm::rotate(modelMat, t_ * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));

		// Scale for inventory display
		if (species && species->displayScale > 0.0f) {
			modelMat = glm::scale(modelMat, glm::vec3(species->displayScale * 0.3f));
		} else {
			modelMat = glm::scale(modelMat, glm::vec3(0.1f));
		}

		shader_->setMat4("uModel", glm::value_ptr(modelMat));

		// Set normal matrix
		glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(modelMat)));
		GLint uNormalLoc = glGetUniformLocation(shader_->getProgram(), "uNormalMat");
		if (uNormalLoc >= 0) {
			glUniformMatrix3fv(uNormalLoc, 1, GL_FALSE, glm::value_ptr(normalMat));
		}

		// Draw the model (materials will be applied from .mtl files)
		model->draw(*shader_);

		glDisable(GL_SCISSOR_TEST);
    }

    // Restore full viewport
    glViewport(0, 0, width_, height_);

    // Restore OpenGL state
    if (!depthTestEnabled) glDisable(GL_DEPTH_TEST);
    if (cullFaceEnabled) glEnable(GL_CULL_FACE);
    if (!blendEnabled) glDisable(GL_BLEND);

    shader_->use();
}

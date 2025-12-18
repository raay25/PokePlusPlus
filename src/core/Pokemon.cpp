#include "pokeapp/Pokemon.h"
#include "pokeapp/Model.h"
#include "pokeapp/Shader.h"
#include "pokeapp/World.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib>
#include <cmath>
#include <iostream>

/*
	Implementation of the Pokemon class. Handles movement, state management, and 
	rendering of Pokémon entities.
*/

namespace pokepp {
	static float randomFloat() {
		return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
	}

	Pokemon::Pokemon(const PokemonSpecies* species, const glm::vec3& startPos, 
	                 float moveSpeed, float collisionRadius, int id)
		: species_{ species }
		, model_{ species ? species->model : nullptr }
		, position_{ startPos }
		, speed_{ moveSpeed }
		, radius_{ collisionRadius }
		, id_{ id } {
		pickNewWanderDirection();
	}

	// Pick a new random direction for wandering
	void Pokemon::pickNewWanderDirection() {
		float angle = randomFloat() * 6.2831853f; // Angle in radians
		wanderDir_ = glm::normalize(glm::vec3{ std::cos(angle), 0.0f, std::sin(angle) });
		velocity_ = wanderDir_ * speed_;

		timeUntilDirectionChange_ = 1.0f + 2.0f * randomFloat();
		state_ = PokemonState::Walking;
	}

	// Update Pokemon state and position based on elapsed time and world state
	void Pokemon::update(float dt, const World* world, const std::vector<glm::vec3>& obstacles) {
		
		// Skip all movement if fully captured
		if (state_ == PokemonState::Captured) {
			return;
		}

		// Handle capture failure - Pokemon breaks free and resumes wandering
		if (state_ == PokemonState::CaptureFailed) {
			// Resume normal behavior after breaking free
			pickNewWanderDirection();
			visible_ = true;
			state_ = PokemonState::Idle;
			return;
		}

		// Handle capture animation timer
		if (state_ == PokemonState::Capturing) {
			captureTimer_ += dt;
			return;
		}

		// Trying to move in wander mode
		glm::vec3 oldPos = position_;
		glm::vec3 nextPos = position_ + velocity_ * dt;

		// Check for collisions with obstacles
		bool collided = false;
		for (const auto& obstacle : obstacles) {
			glm::vec2 toObstacle(obstacle.x - nextPos.x, obstacle.z - nextPos.z);
			float dist = glm::length(toObstacle);
			if (dist < radius_ + 0.7f) { // obstacle radius estimate
				collided = true;
				break;
			}
		}

		if (collided) {
			// Pick a new random direction immediately
			pickNewWanderDirection();
			nextPos = position_; // Don't move this frame
		} else {
			position_ = nextPos;
		}

		// Follow terrain height
		if (world) {
			position_.y = world->heightAt(position_.x, position_.z);
		}

		timeUntilDirectionChange_ -= dt;
		if (timeUntilDirectionChange_ <= 0.0f) {
			pickNewWanderDirection();
		}

		// Calculate rotation from movement direction
		glm::vec3 moveDir = position_ - oldPos;
		if (glm::length(moveDir) > 0.001f) {
			yRotation_ = std::atan2(moveDir.x, moveDir.z);
		}
	}

	// Render the Pokemon using the provided shader
	void Pokemon::draw(Shader& shader) const {
		if (!visible_ || !model_) return;

		// Set up model matrix
		glm::mat4 model(1.0f);
		model = glm::translate(model, position_);
		model = glm::rotate(model, yRotation_, glm::vec3(0.0f, 1.0f, 0.0f));
		
		// Apply species-specific scale if available
		// (Bulbasaur needs to scale UP, for example)
		if (species_) {
			model = glm::scale(model, glm::vec3(species_->displayScale));
		}

		// Normal matrix for correct lighting
		glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(model)));
		shader.setMat4("uModel", glm::value_ptr(model));
		
		GLint uNormalLoc = glGetUniformLocation(shader.getProgram(), "uNormalMat");
		if (uNormalLoc >= 0) {
			glUniformMatrix3fv(uNormalLoc, 1, GL_FALSE, glm::value_ptr(normalMat));
		}

		model_->draw(shader);
	}

	// Begin the capture animation process
	void Pokemon::startCapture() {
		state_ = PokemonState::Capturing;
		captureTimer_ = 0.0f;
		velocity_ = glm::vec3(0.0f);
	}

	void Pokemon::markCaptured() {
		state_ = PokemonState::Captured;
		visible_ = false;  // Hide the Pokémon after capture
	}

	void Pokemon::markCaptureFailed() {
		state_ = PokemonState::CaptureFailed;
		visible_ = true; // Make sure it's visible again
	}
} // namespace pokepp
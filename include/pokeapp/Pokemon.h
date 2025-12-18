#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>

/*
	Pokemon header file, defines a Pokemon class for simulating Pokemon behavior in the game.
*/

// Forward declarations
namespace pokepp { // namespace for pokepp library
	class Model;
	class World;
}
class Shader; // global namespace

namespace pokepp {

	// Capture states for Pokemon
	enum class PokemonState {
		Idle, Walking, Capturing, Captured, CaptureFailed  
	};

	// Pokemon species data structure
	struct PokemonSpecies {
		std::string name;
		Model* model;
		glm::vec3 displayColor; 
		float displayScale = 1.0f; 
		float catchRate = 0.5f;
	};

	class Pokemon {
	public:
		Pokemon(const PokemonSpecies* species, const glm::vec3& startPos, 
		        float moveSpeed = 2.0f, float collisionRadius = 0.5f, int id = 0);
		
		void update(float dt, const World* world = nullptr, const std::vector<glm::vec3>& obstacles = {});
		void draw(Shader& shader) const;

		const glm::vec3& getPosition() const { return position_; }
		void setPosition(const glm::vec3& p) { position_ = p; }

		PokemonState getState() const { return state_; }
		void setState(PokemonState s) { state_ = s; }

		float getRadius() const { return radius_; }

		bool isCapturing() const { return state_ == PokemonState::Capturing; }
		void startCapture();

		bool isCaptured() const { return state_ == PokemonState::Captured; }
		void markCaptured();

		bool captureFailed() const { return state_ == PokemonState::CaptureFailed; }
		void markCaptureFailed();

		bool isVisible() const { return visible_; }
		void setVisible(bool v) { visible_ = v; }

		int getId() const { return id_; }
		Model* getModel() const { return model_; }
		
		const PokemonSpecies* getSpecies() const { return species_; }
		const std::string& getSpeciesName() const { return species_ ? species_->name : "Unknown"; }

		float getCatchRate() const { return species_ ? species_->catchRate : 0.5f; }

	private:
		int id_ = 0;
		void pickNewWanderDirection();
		
		const PokemonSpecies* species_;
		Model* model_;

		glm::vec3 position_{ 0.0f };
		glm::vec3 velocity_{ 0.0f };
		glm::vec3 wanderDir_{ 0.0f };

		float speed_ = 2.0f;
		float radius_ = 0.5f;
		bool visible_ = true;
		PokemonState state_ = PokemonState::Idle;

		float timeUntilDirectionChange_ = 0.0f;
		float captureTimer_ = 0.0f;
		float captureDuration = 0.6f;

		float yRotation_ = 0.0f;  // For facing direction (when wandering)
	};
}
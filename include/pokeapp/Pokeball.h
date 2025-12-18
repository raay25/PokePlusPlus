#pragma once

#include <glm/glm.hpp>

/*
	Pokeball header file, defines a Pokeball struct for simulating Pokeball 
	behavior in the game.
*/

namespace pokepp {

	struct Pokeball {
		glm::vec3 position{ 0.0f };
		glm::vec3 velocity{ 0.0f };

		float radius = 0.3f;
		bool active = true;
		bool grounded = false;

		float life = 10.0f;

		// Capture animation state
		bool locked = false;           // For wiggle animation
		float lockTimer = 0.0f;        // Animation time
		int shakeCount = 0;            // Number of shakes completed
		float shakePhase = 0.0f;       // Current shake progress (0-1)
		glm::vec3 captureBasePos{ 0.0f };

		// NEW: Capture result
		bool captureSuccess = false;   // Determined when capture starts
		int targetPokemonId = -1;      // NEW: ID of the Pokémon this ball is attempting to capture
	};

}
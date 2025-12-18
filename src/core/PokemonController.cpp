#include "pokeapp/PokemonController.h"
#include "pokeapp/Pokemon.h"
#include "pokeapp/Pokeball.h"
#include "pokeapp/Model.h"
#include "pokeapp/Shader.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <iostream>
#include <algorithm>
#include <cstdlib>

/*
	Implementation of PokemonController class. This class manages all active Pokemon
	in the world, including spawning, updating, drawing, handling Pokeball captures,
	and managing the player's inventory of captured Pokemon.
*/

namespace pokepp {

	// Simple sphere-sphere collision detection
	static bool collide(const glm::vec3& p1, float r1, const glm::vec3& p2, float r2) {
		float rsum = r1 + r2;
		float dist2 = glm::length2(p1 - p2);
		return dist2 <= (rsum * rsum);
	}

	static float randomFloat() {
		return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
	}

	// Spawn a new wild Pokemon in the world
	void PokemonController::spawnPokemon(const PokemonSpecies* species, const glm::vec3& pos, 
                                      float speed, float radius, int id) {
		int actualId = (id == 0) ? nextPokemonId_++ : id;
		pokemon_.emplace_back(species, pos, speed, radius, actualId);
	}

	// Update all active Pokemon (wandering, capturing, etc.)
	void PokemonController::updateAll(float dt, const World* world, const std::vector<glm::vec3>& obstacles) {
		for (auto& p : pokemon_) {
			p.update(dt, world, obstacles);
		}
	}

	// Draw all active Pokemon
	void PokemonController::drawAll(Shader& shader) const {
		for (const auto& p : pokemon_) {
			p.draw(shader);
		}
	}

	// Handle collisions between Pokeballs and Pokemon for capture attempts
	void PokemonController::handlePokeballCapture(std::vector<Pokeball>& pokeballs) {
		for (auto& p : pokemon_) {
			
			// Skip if already captured or currently capturing
			if (p.isCaptured() || p.isCapturing()) continue;
			
			// Skip if capture failed (still in failed state, hasn't fully reset yet)
			// (bug fix)
			if (p.captureFailed()) continue;

			// Skip if this is one of our own Pokemon (can't recapture them!)
			if (isOwnedPokemon(p)) {
				continue;
			}

			// Check collision with each active Pokeball
			for (auto& ball : pokeballs) {

				// Skip if this ball is already locked (already attempted a capture)
				if (ball.locked) continue;
				
				// Skip if this ball already tried to capture this specific Pokemon
				// (prevents multiple collisions on same ball)
				if (ball.targetPokemonId == p.getId()) continue;
				
				// On collision, start capture process
				if (collide(ball.position, ball.radius, p.getPosition(), p.getRadius())) {					
					p.startCapture();
					p.setVisible(false);

					ball.active = false;
					ball.locked = true;
					ball.lockTimer = 0.0f;
					ball.velocity = glm::vec3(0.0f);
					ball.targetPokemonId = p.getId();  // Remember which Pokemon this ball tried to capture, to avoid re-collisions

					// SNAP animation - ball snaps to Pokemon position
					ball.position = p.getPosition() + glm::vec3(0.0f, p.getRadius(), 0.0f);

					// Calculate capture success based on catch rate
					float catchRate = p.getCatchRate();
					float roll = randomFloat();
					bool captureSuccess = (roll <= catchRate);
					
					// Store the result via the ball for later processing
					ball.captureSuccess = captureSuccess;

					break;
				}
			}
		}
	}

	// Update inventory by moving captured Pokemon from active list to inventory
	void PokemonController::updateInventory() {

		// Move SUCCESSFULLY captured Pokémon from active list to inventory
		auto it = pokemon_.begin();
		while (it != pokemon_.end()) {
			if (it->isCaptured() && !it->isVisible()) {
				// Only move to inventory if it's NOT one of our sent-out Pokémon
				if (!isOwnedPokemon(*it)) {
					inventory_.push_back(std::move(*it));
					it = pokemon_.erase(it);
				} else {
					++it;
				}
			} else {
				++it;
			}
		}
	}

	// Send out a Pokemon from inventory into the world at the specified position
	bool PokemonController::sendOutPokemon(size_t inventoryIndex, const glm::vec3& position) {
		if (inventoryIndex >= inventory_.size()) {
			return false;
		}

		// Only allow one Pokemon out at a time
		if (hasAnyPokemonOut()) {
			return false;
		}

		// Check if this specific Pokémon is already out (shouldn't happen with above check, but safe)
		if (isPokemonOut(inventoryIndex)) {
			return false;
		}

		// Create a copy of the Pokemon (keep original in inventory)
		Pokemon sentOut = inventory_[inventoryIndex];
		
		// Reset state and position for the active copy
		sentOut.setPosition(position);
		sentOut.setState(PokemonState::Idle);
		sentOut.setVisible(true);

		// Add to active Pokemon list
		pokemon_.push_back(std::move(sentOut));
		
		// Track that this inventory slot is now out
		outPokemonIndices_.push_back(inventoryIndex);

		return true;
	}

	// Recall a sent-out Pokemon back into the inventory
	bool PokemonController::recallPokemon(size_t inventoryIndex) {
		if (inventoryIndex >= inventory_.size()) {
			return false;
		}

		// Find if this Pokémon is currently out
		auto outIt = std::find(outPokemonIndices_.begin(), outPokemonIndices_.end(), inventoryIndex);
		if (outIt == outPokemonIndices_.end()) {
			return false;
		}

		// Find and remove the sent-out Pokémon by matching ID
		int targetId = inventory_[inventoryIndex].getId();
		bool found = false;

		// Search from the END backwards (sent-out Pokémon are added to the end)
		for (auto it = pokemon_.rbegin(); it != pokemon_.rend(); ++it) {
			if (it->getId() == targetId && isOwnedPokemon(*it)) {
				// Found it! Remove it using base() iterator
				pokemon_.erase(std::next(it).base());
				found = true;
				break;
			}
		}

		if (!found) {
			return false;
		}

		// Remove from tracking list
		outPokemonIndices_.erase(outIt);

		return true;
	}

	// Check if a specific inventory Pokemon is currently out in the world
	bool PokemonController::isPokemonOut(size_t inventoryIndex) const {
		return std::find(outPokemonIndices_.begin(), outPokemonIndices_.end(), inventoryIndex) 
		       != outPokemonIndices_.end();
	}

	// Check if any Pokemon are currently out in the world
	bool PokemonController::hasAnyPokemonOut() const {
		return !outPokemonIndices_.empty();
	}

	// Check if a given Pokemon is one of our owned (sent-out) Pokemon
	bool PokemonController::isOwnedPokemon(const Pokemon& p) const {
		if (outPokemonIndices_.empty()) {
			return false;
		}

		// Check if this Pokémon's ID matches any of our inventory Pokémon
		for (size_t outIdx : outPokemonIndices_) {
			if (outIdx < inventory_.size() && p.getId() == inventory_[outIdx].getId()) {
				return true;
			}
		}

		return false;
	}
}
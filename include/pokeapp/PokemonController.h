#pragma once

#include "pokeapp/Pokemon.h"
#include <vector>
#include <glm/glm.hpp>

/*
	PokemonController header file, defines a PokemonController class for controlling 
	Pokemon spawning, updating, drawing, and inventory management.
*/

// Forward declarations
namespace pokepp {
	class Model;
	struct Pokeball;
}

class Shader;

namespace pokepp {
	class PokemonController {
	public:		
		void spawnPokemon(const PokemonSpecies* species, const glm::vec3& pos, 
		                  float speed = 2.0f, float radius = 0.5f, int id = 0);
		
		void updateAll(float dt, const World* world, const std::vector<glm::vec3>& obstacles);
		void drawAll(Shader& shader) const;
		void handlePokeballCapture(std::vector<Pokeball>& pokeballs);

		// Inventory management
		void updateInventory();
		bool sendOutPokemon(size_t inventoryIndex, const glm::vec3& position);
		bool recallPokemon(size_t inventoryIndex);
		bool isPokemonOut(size_t inventoryIndex) const;
		bool hasAnyPokemonOut() const;
		const std::vector<Pokemon>& getInventory() const { return inventory_; }
		size_t getInventoryCount() const { return inventory_.size(); }

		std::vector<Pokemon>& getPokemon() { return pokemon_; }
		const std::vector<Pokemon>& getPokemon() const { return pokemon_; }

	private:
		bool isOwnedPokemon(const Pokemon& p) const;
		
		std::vector<Pokemon> pokemon_;
		std::vector<Pokemon> inventory_;
		std::vector<size_t> outPokemonIndices_;  // Tracks which inventory slots are currently out
		int nextPokemonId_ = 1;  // Auto incrementing ID for wild Pokémon
	};
}
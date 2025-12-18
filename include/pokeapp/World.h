#pragma once
#include <memory>
#include "pokeapp/Mesh.h"
#include "pokeapp/Model.h"

/*
	World header file, defines a World class for managing the 3D environment,
	including terrain generation from height maps, height queries, and rendering.
*/

namespace pokepp {

class World {
public:
	World();
	~World();

	static std::unique_ptr<Mesh> makeGround(int n, float size);

	static std::unique_ptr<Mesh> makeHeightMap(const char* path, float cellSize, float heightScale);

	float heightAt(float x, float z) const;
	glm::vec3 normalAt(float x, float z) const;

	void draw(const Shader& shader, const glm::mat4& view, const glm::mat4& proj) const;

	static std::unique_ptr<World> FromHeightMap(const char* path, float cellSize, float heightScale);

	// Textures
	std::unique_ptr<Texture> grassTex_;
	std::unique_ptr<Texture> rockTex_;
	float texScale_ = 8.0f;

private:
	std::unique_ptr<Model> ground_;

	std::vector<float> heights_;
	int imgW_ = 0, imgH_ = 0; // image dimensions
	float cellSize_ = 1.0f;
	float halfWm_ = 0.0f, halfZm_ = 0.0f; // half world size in meters

	inline int idx(int i, int j) const {
		return j * imgW_ + i;
	}
};

} // namespace pokepp
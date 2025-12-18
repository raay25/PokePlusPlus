#include "pokeapp/World.h"
#include "pokeapp/Shader.h"
#include "pokeapp/stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>

/*
	Implementation of the world. Supports height map loading and querying heights/normals
	for collision detection.
*/


namespace pokepp {

World::World() {
	auto mesh = makeGround(64, 1.0f);
	ground_ = std::make_unique<Model>(std::move(mesh));
}

World::~World() = default;

// Static method to create a World from a height map image.
// A 3D terrain mesh is generated based on the grayscale values of the image.
std::unique_ptr<World> World::FromHeightMap(const char* path, float cellSize, float heightScale) {
    auto w = std::make_unique<World>();

    // Load the heightmap. 
    int wpx, hpx, nch;
    stbi_uc* data = stbi_load(path, &wpx, &hpx, &nch, 1); // force 1 channel (grayscale)
    if (!data) return w;

    // Setup the world dimensions
    w->imgW_ = wpx;
    w->imgH_ = hpx;
    w->cellSize_ = cellSize;
    const int W = wpx - 1;        // Number of quads in x
    const int H = hpx - 1;        // Number of quads in z
    w->halfWm_ = (W * cellSize) * 0.5f;
    w->halfZm_ = (H * cellSize) * 0.5f;

    // Convert pixel brightness to height values.
	// Lighter pixels means higher elevation, and vice versa. 
    w->heights_.resize(wpx * hpx);
    for (int j = 0; j < hpx; ++j) {
        for (int i = 0; i < wpx; ++i) {
            float gray = data[j * wpx + i] / 255.0f;
            w->heights_[w->idx(i, j)] = gray * heightScale;
        }
    }
    stbi_image_free(data);

	// Build mesh from heights (create vertices with normals and uvs)
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(wpx * hpx);

    auto Hval = [&](int i, int j)->float {
        i = std::clamp(i, 0, wpx - 1);
        j = std::clamp(j, 0, hpx - 1);
        return w->heights_[w->idx(i, j)];
        };

    // Create vertices with normals via central differences
    for (int j = 0; j < hpx; ++j) {
        for (int i = 0; i < wpx; ++i) {
			// Convert grid (i,j) to world (x,z)
            float x = i * cellSize - w->halfWm_;
            float z = j * cellSize - w->halfZm_;
            float y = Hval(i, j);

			// Calculate normal using central differences (gradient)
            float hx = (Hval(i + 1, j) - Hval(i - 1, j)) / (2.0f * cellSize);
            float hz = (Hval(i, j + 1) - Hval(i, j - 1)) / (2.0f * cellSize);
            glm::vec3 n = glm::normalize(glm::vec3(-hx, 1.0f, -hz));

            // Calculate UV coordinates (for mapping the grass texture across the entire terrain)
            float u = i / float(W);   // [0,1]
            float v = j / float(H);   // [0,1]

            vertices.push_back({ glm::vec3(x,y,z), n, glm::vec2(u,v) });
        }
    }

	// Create indices for triangle list
    for (int j = 0; j < H; ++j) {
        for (int i = 0; i < W; ++i) {
            unsigned a = j * wpx + i;
            unsigned b = a + 1;
            unsigned c = (j + 1) * wpx + i;
            unsigned d = c + 1;
            indices.insert(indices.end(), { a,c,b,  b,c,d }); // CCW
        }
    }

    // Create the model and load the textuers 
    auto mesh = std::make_unique<Mesh>(vertices, indices);
    w->ground_ = std::make_unique<Model>(std::move(mesh));
	w->grassTex_ = std::make_unique<Texture>("assets/textures/grass.png", Texture::Kind::Diffuse);
	w->rockTex_ = std::make_unique<Texture>("assets/textures/rock.png", Texture::Kind::Diffuse);

    if (w->grassTex_) {
        w->grassTex_->bind(0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    if (w->rockTex_) {
        w->rockTex_->bind(1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    return w;
}

// Static method to create a flat ground mesh. Used as a fallback when 
// no height map is provided.
std::unique_ptr<Mesh> World::makeGround(int n, float size) {
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	vertices.reserve((n + 1) * (n + 1));

	float half = (n * size) / 2.0f;
    
	// Generate vertices with positions, normals, and UVs
	for (int z = 0; z <= n; ++z) {
		for (int x = 0; x <= n; ++x) {
			float wx = x * size - half;
			float wz = z * size - half;
			vertices.push_back({
				glm::vec3(wx, 0.0f, wz),    // position
				glm::vec3(0.0f, 1.0f, 0.0f), // normal
				glm::vec2(x / static_cast<float>(n), z / static_cast<float>(n)) // uv
			});
		}
	}

	// Generate indices for triangle list
	for (int z = 0; z < n; ++z) {
		for (int x = 0; x < n; ++x) {
			unsigned int topLeft = z * (n + 1) + x;
			unsigned int topRight = topLeft + 1;
			unsigned int bottomLeft = topLeft + (n + 1);
			unsigned int bottomRight = bottomLeft + 1;
			indices.push_back(topLeft);
			indices.push_back(bottomLeft);
			indices.push_back(topRight);
			indices.push_back(topRight);
			indices.push_back(bottomLeft);
			indices.push_back(bottomRight);
		}
	}

	return std::make_unique<Mesh>(vertices, indices);
}

// Query the height of the terrain at world coordinates (x,z) using bilinear interpolation
// (like a weighted average of the 4 nearest grid points).
float World::heightAt(float x, float z) const {
    if (imgW_ <= 1 || imgH_ <= 1) return 0.0f;

    // Convert world (x,z) to grid (u,v) in [0,W]x[0,H]
    float W = float(imgW_ - 1), H = float(imgH_ - 1);
    float u = (x + halfWm_) / cellSize_; // [0,W]
    float v = (z + halfZm_) / cellSize_; // [0,H]

    // Clamp to valid cell range
    u = std::clamp(u, 0.0f, W);
    v = std::clamp(v, 0.0f, H);

	// Find the grid cell and fractional position within it
    int i = int(std::floor(u));
    int j = int(std::floor(v));
    float tx = u - i;
    float tz = v - j;

    // Sample heights at the 4 corners (clamped at borders)
    auto Hc = [&](int ii, int jj) {
        ii = std::clamp(ii, 0, imgW_ - 1);
        jj = std::clamp(jj, 0, imgH_ - 1);
        return heights_[idx(ii, jj)];
        };
	float h00 = Hc(i, j); // bottom-left
	float h10 = Hc(i + 1, j); // bottom-right
	float h01 = Hc(i, j + 1); // top-left
	float h11 = Hc(i + 1, j + 1); // top-right

	// Bilinear interpolation
    float h0 = (1.0f - tx) * h00 + tx * h10;
    float h1 = (1.0f - tx) * h01 + tx * h11;
    return (1.0f - tz) * h0 + tz * h1;
}

// Compute the ssurface normal vector at world coordinates (x,z) using central differences.
// Used for collision response and lighting.
glm::vec3 World::normalAt(float x, float z) const {

    // Sample nearby heights and compute gradient numerically
    const float eps = cellSize_;
    float hL = heightAt(x - eps, z);
    float hR = heightAt(x + eps, z);
    float hD = heightAt(x, z - eps);
    float hU = heightAt(x, z + eps);

    float hx = (hR - hL) / (2.0f * eps);
    float hz = (hU - hD) / (2.0f * eps);
    glm::vec3 n = glm::normalize(glm::vec3(-hx, 1.0f, -hz));
    return n;
}

// Render the terrain using the provided shader and camera matrices.
void World::draw(const Shader& shader, const glm::mat4& view, const glm::mat4& proj) const {
	if (!ground_) return;

	shader.use();

	glm::mat4 model(1.0f);
	glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(model)));

	shader.setMat4("uModel", glm::value_ptr(model));
	shader.setMat4("uView", glm::value_ptr(view));
	shader.setMat4("uProj", glm::value_ptr(proj));
	shader.setMat3("uNormalMat", glm::value_ptr(normalMat));
	shader.setInt("uUseTexture", 1); 
	shader.setFloat("uTexScale", 32.0f);  

    if (grassTex_ && rockTex_) {
        shader.setInt("uHasRock", 1);
        
        grassTex_->bind(0);
        rockTex_->bind(1);
        
        shader.setInt("uGrass", 0);
        shader.setInt("uRock", 1);
    } else if (grassTex_) {
        shader.setInt("uHasRock", 0);
        
        grassTex_->bind(0);
        shader.setInt("uTex", 0);
    }

	ground_->draw(shader, false);
}

} // namespace pokepp
#pragma once
#include <glm/glm.hpp>

/*
	Material header file, defining Material class and MaterialProps struct.
	Materials are used to encapsulate shader programs, textures, and material properties
	for rendering 3D objects.

	More specifically, it is used to capture the logic behind how different surfaces
    should interact with light. 
*/


// Forward declarations
class Shader;
class Texture;

namespace pokepp {

	// Phong Lighting Model properties
    struct MaterialProps {
        glm::vec3 kd{ 1.0f, 1.0f, 1.0f }; // diffuse color
        glm::vec3 ks{ 0.04f, 0.04f, 0.04f }; // specular color
        float shininess{ 32.0f };
        bool useTexture{ false };
    };

    class Material {
    public:
        Material(::Shader* shader, ::Texture* diffuseTex = nullptr, const MaterialProps& props = {});

        // Bind shader + set standard uniforms
        void bind(const glm::mat4& model,
            const glm::mat4& view,
            const glm::mat4& proj,
            const glm::mat3& normalMat);
		Texture* diffuse() const { return diffuse_; }

        // Accessors
        ::Shader* shader() const { return shader_; }
        void setProps(const MaterialProps& p) { props_ = p; }
        const MaterialProps& props() const { return props_; }
        void setDiffuse(::Texture* t) { diffuse_ = t; }

    private:
        ::Shader* shader_{ nullptr };
        ::Texture* diffuse_{ nullptr };
        MaterialProps props_{};

        // Cached uniform locations (-1 if not present in the program)
        int locModel_{ -1 };
        int locView_{ -1 };
        int locProj_{ -1 };
        int locNormal_{ -1 };
        int locKd_{ -1 };
        int locKs_{ -1 };
        int locShine_{ -1 };
        int locUseTex_{ -1 };
        int locDiffuse_{ -1 };

        void cacheUniforms();
    };
} // namespace pokepp
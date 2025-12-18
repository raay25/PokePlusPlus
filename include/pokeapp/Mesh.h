#pragma once

#include <vector>
#include <glad/glad.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/*
	Mesh header file, defines a Mesh class for rendering 3D models using OpenGL.
	In OpenGL, a mesh represents a 3D shape as a collection of vertices, normals, 
	texture coordinates, and indices that define how these vertices connect to form 
    triangles. Think of it as the "skeleton" of a 3D object. 
*/

namespace pokepp {

	// Vertex structure representing a single vertex in the mesh
    struct Vertex {
        glm::vec3 position{};
        glm::vec3 normal{};
        glm::vec2 tex{};
    };

    class Mesh {
    public:
        explicit Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned>& indices);
        ~Mesh();

		// Disable copy operations (avoid OpenGL resource duplication issues)
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        
		// Enable move operations (transfer ownership of OpenGL resources)
        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(Mesh&& other) noexcept;

        void draw() const;

    private:
        void setup();

        GLuint VAO_ = 0;
        GLuint VBO_ = 0; 
        GLuint EBO_ = 0;
        
        std::vector<Vertex> vertices_;
        std::vector<unsigned> indices_;
    };

} // namespace pokepp

#pragma once
#include <string>
#include <vector>
#include <memory>
#include <pokeapp/Mesh.h>
#include <pokeapp/Shader.h>
#include <pokeapp/Texture.h>
#include <pokeapp/Material.h>
#include <glm/vec3.hpp>

/*
	Model header file, defines a Model class for loading and rendering 3D models.
	Here, a Model is the 3D visual representation of an object (i.e. the geometry and 
	appearance). A Pokemon, for example, uses a Model to define how it looks when drawn.

	A Model combines meshes and materials to represent complex 3D objects. In a way, it
	"adds skin" (materials) to the "skeleton" (meshes) of a 3D object.
*/

namespace pokepp {
    class Model {
    private:
		std::vector<Mesh> meshes_; // collection of meshes in the model
		std::vector<int> meshMatIdx_; // material index for each mesh (maps to materials_ vector)
		std::vector<std::unique_ptr<Material>> materials_; // collection of materials in the model
        std::string directory_;
        
        void loadObj(const std::string& path);

    public:
        explicit Model(const std::string& path);
        explicit Model(std::unique_ptr<Mesh> mesh); 
        void draw(const Shader& shader) const;
        void draw(const Shader& shader, bool overrideMaterial) const;

        bool loadOBJ(const char* path);

        
        const std::vector<std::unique_ptr<Material>>& materials() const { return materials_; }
    };
}
#include <pokeapp/Model.h>
#include <pokeapp/tiny_obj_loader.h>
#include <pokeapp/Shader.h> 
#include <pokeapp/Texture.h>
#include <stdexcept>
#include <cstdio>
#include <glm/vec3.hpp>
#include <iostream> 

/*
	Implementation of the Model class. Handles loading 3D models from OBJ files,
	managing their meshes and materials, and rendering them using OpenGL.
*/

using namespace pokepp;

// Helper function to join directory and filename into a full path
static std::string joinPath(const std::string& dir, const std::string& name) {
    if (dir.empty() || name.empty()) return name;
#ifdef _WIN32
    const char sep = '\\';
#else
    const char sep = '/';
#endif
    if (dir.back() == '/' || dir.back() == '\\') return dir + name;
    return dir + sep + name;
}

Model::Model(const std::string& path) { loadObj(path); }

Model::Model(std::unique_ptr<Mesh> mesh) {
    // Move the mesh into our vector
    meshes_.push_back(std::move(*mesh));
    
    // No material for procedural geometry
    meshMatIdx_.push_back(-1);
}

// Load an OBJ model from the specified file path and process its materials and meshes
void Model::loadObj(const std::string& path) {
    tinyobj::attrib_t attrib; // Raw vertex data (positions, normals, and texture coords)
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> mtls;
    std::string warn, err;

    auto pos = path.find_last_of("/\\");
    directory_ = (pos == std::string::npos) ? "" : path.substr(0, pos);

    bool ok = tinyobj::LoadObj(&attrib, &shapes, &mtls, &warn, &err,
        path.c_str(),
        directory_.empty() ? nullptr : directory_.c_str(),
        true);
    
    if (!warn.empty()) std::fprintf(stderr, "TinyObjLoader warning: %s\n", warn.c_str());
    if (!err.empty())  std::fprintf(stderr, "TinyObjLoader error: %s\n", err.c_str());
    if (!ok) throw std::runtime_error("Failed to load OBJ: " + path);
    
	// Build materials by converting tinyobj materials to our Material class
    materials_.clear();
    materials_.reserve(mtls.size());
    for (const auto& m : mtls) {        
        MaterialProps props;
        props.kd = { m.diffuse[0], m.diffuse[1], m.diffuse[2] };
        if (m.shininess > 0.0f) props.shininess = std::max(1.0f, std::min(m.shininess, 256.0f));
        
        Texture* diffuseTex = nullptr;
        if (!m.diffuse_texname.empty()) {
            auto texPath = joinPath(directory_, m.diffuse_texname);
            try {
                diffuseTex = new Texture(texPath, Texture::Kind::Diffuse);
                props.useTexture = true;
            }
            catch (const std::exception& e) {
                std::fprintf(stderr, "[MODEL]   Failed to load texture %s: %s\n", texPath.c_str(), e.what());
            }
        }
        
        materials_.push_back(std::make_unique<Material>(nullptr, diffuseTex, props));
    }

    meshes_.clear();
    meshMatIdx_.clear();
    meshes_.reserve(shapes.size());
    meshMatIdx_.reserve(shapes.size());

    // Check if this is a tree model (based on filename)
    bool isTreeModel = path.find("tree.obj") != std::string::npos;

	// Build meshes from shapes
    for (const auto& shape : shapes) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        
        bool hasNormals = !attrib.normals.empty();
        bool hasColors = !attrib.colors.empty();
        
        vertices.reserve(shape.mesh.indices.size());
        indices.reserve(shape.mesh.indices.size());

        for (const auto& idx : shape.mesh.indices) {
            Vertex v{}; 

            if (idx.vertex_index >= 0) {
                v.position = {
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2]
                };
            }
            if (hasNormals && idx.normal_index >= 0) {
                v.normal = {
                    attrib.normals[3 * idx.normal_index + 0],
                    attrib.normals[3 * idx.normal_index + 1],
                    attrib.normals[3 * idx.normal_index + 2]
                };
            } else {
                v.normal = { 0.0f, 0.0f, 0.0f };
            }
            
            if (idx.texcoord_index >= 0) {
                float texU = attrib.texcoords[2 * idx.texcoord_index + 0];
                float texV = attrib.texcoords[2 * idx.texcoord_index + 1];
                v.tex = { texU, texV }; 
            } else {
                v.tex = { 0.0f, 0.0f };
            }

            indices.push_back(static_cast<unsigned int>(vertices.size()));
            vertices.push_back(v);
        }

        // Calculate normals if missing (cross product)
        if (!hasNormals) {
            for (size_t i = 0; i < indices.size(); i += 3) {
                glm::vec3 v0 = vertices[indices[i + 0]].position;
                glm::vec3 v1 = vertices[indices[i + 1]].position;
                glm::vec3 v2 = vertices[indices[i + 2]].position;

                glm::vec3 edge1 = v1 - v0;
                glm::vec3 edge2 = v2 - v0;
                glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

                // Set same normal for all 3 vertices of this triangle (flat shading)
                vertices[indices[i + 0]].normal = normal;
                vertices[indices[i + 1]].normal = normal;
                vertices[indices[i + 2]].normal = normal;
            }
        }

        int matId = -1;
        for (int mid : shape.mesh.material_ids) { if (mid >= 0) { matId = mid; break; } }

        meshes_.emplace_back(vertices, indices);
        meshMatIdx_.push_back(matId);
    }  
}

// Draw the model using the specified shader (overload without material override)
void Model::draw(const Shader& shader) const {
    draw(shader, false);
}

// Draw the model using the specified shader, with optional material override
void Model::draw(const Shader& shader, bool overrideMaterial) const {
    
	// Draw each mesh with its associated material
    for (size_t i = 0; i < meshes_.size(); ++i) {
        if (overrideMaterial) {
			shader.setInt("uUseTexture", 0); // Let shader know no texture is used
        } else {
            // Get material for this mesh
            int mid = (i < meshMatIdx_.size()) ? meshMatIdx_[i] : -1;
            if (mid >= 0 && mid < (int)materials_.size() && materials_[mid]) {
                Material* mat = materials_[mid].get();
                
				// Configure shader uniforms based on material properties
                const auto& props = mat->props();
                if (props.useTexture) {
                    if (auto tex = mat->diffuse()) {
                        tex->bind(0); // bind to GPU unit 0
                    }
					shader.setInt("uUseTexture", 1); // inform shader to use texture
					shader.setInt("uTex", 0); // inform shader texture is in unit 0
                } else {
					shader.setInt("uUseTexture", 0); // no texture, use solid color
                    GLint kdLoc = glGetUniformLocation(shader.getProgram(), "uKd");
                    if (kdLoc != -1) {
                        glUniform3f(kdLoc, props.kd.x, props.kd.y, props.kd.z);
                    }
                }
                
				// Send shininess and specular color for Phong shading
                GLint shineLoc = glGetUniformLocation(shader.getProgram(), "uShininess");
                if (shineLoc != -1) {
                    glUniform1f(shineLoc, props.shininess);
                }
            }
        }

		// Draw mesh by sending draw call to GPU
        meshes_[i].draw();
    }
}

// Load an OBJ model from the specified file path, returning success status
bool Model::loadOBJ(const char* path) {
    try {
        loadObj(path);
        return true;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Error loading OBJ: %s\n", e.what());
        return false;
    }
}


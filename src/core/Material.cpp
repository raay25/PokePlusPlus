#include <pokeapp/Material.h>
#include <pokeapp/Shader.h>
#include <pokeapp/Texture.h>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

/*
	Implementation of the Material class for managing  material properties in rendering
    (lighting). 
*/

namespace pokepp {

Material::Material(Shader* shader, Texture* diffuseTex, const MaterialProps& props)
    : shader_(shader), diffuse_(diffuseTex), props_(props) {
    if (shader_) {
        cacheUniforms();
    }
}

// Cache uniform locations for efficiency 
void Material::cacheUniforms() {
    if (!shader_) return;
    
    GLuint prog = shader_->getProgram();
    locModel_ = glGetUniformLocation(prog, "uModel");
    locView_ = glGetUniformLocation(prog, "uView");
    locProj_ = glGetUniformLocation(prog, "uProj");
    locNormal_ = glGetUniformLocation(prog, "uNormalMat");
    locKd_ = glGetUniformLocation(prog, "uKd");
    locKs_ = glGetUniformLocation(prog, "uKs");
    locShine_ = glGetUniformLocation(prog, "uShininess");
    locUseTex_ = glGetUniformLocation(prog, "uUseTexture");
    locDiffuse_ = glGetUniformLocation(prog, "uTex");
}

// Bind shader and set standard uniforms. This is essentially just
// a convenience wrapper to batch together OpenGL state-setting calls.
void Material::bind(const glm::mat4& model,
                   const glm::mat4& view,
                   const glm::mat4& proj,
                   const glm::mat3& normalMat) {
    if (!shader_) return;
    
    shader_->use(); // Use THIS shader. 
    
    // Set matrices
    if (locModel_ != -1) glUniformMatrix4fv(locModel_, 1, GL_FALSE, glm::value_ptr(model));
    if (locView_ != -1) glUniformMatrix4fv(locView_, 1, GL_FALSE, glm::value_ptr(view));
    if (locProj_ != -1) glUniformMatrix4fv(locProj_, 1, GL_FALSE, glm::value_ptr(proj));
    if (locNormal_ != -1) glUniformMatrix3fv(locNormal_, 1, GL_FALSE, glm::value_ptr(normalMat));
    
    // Set material properties
    if (locKd_ != -1) glUniform3fv(locKd_, 1, glm::value_ptr(props_.kd));
    if (locKs_ != -1) glUniform3fv(locKs_, 1, glm::value_ptr(props_.ks));
    if (locShine_ != -1) glUniform1f(locShine_, props_.shininess);
    
    // Handle texture
    if (diffuse_ && props_.useTexture) {
        diffuse_->bind(0);
        if (locDiffuse_ != -1) glUniform1i(locDiffuse_, 0);
        if (locUseTex_ != -1) glUniform1i(locUseTex_, 1);
    } else {
        if (locUseTex_ != -1) glUniform1i(locUseTex_, 0);
    }
}

} // namespace pokepp
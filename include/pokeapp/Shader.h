#pragma once
#include <glad/glad.h>
#include <string>

/*
    Shader header file, defines a Shader class for compiling, 
    linking, and using OpenGL shaders.
*/

class Shader {
private:
    GLuint program_ = 0;
    
    GLuint compileOne(GLenum type, const char* src, const char* debugName);
    void logShaderError(GLuint shader, const char* debugName);
    void logProgramError(GLuint program);

public:
    ~Shader();
    
    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    bool compileAndLink(const std::string& vertexCode, const std::string& fragmentCode);
    
    void use() const { glUseProgram(program_); }
    GLuint getProgram() const { return program_; }
    
    void setInt(const char* name, int v) const;
    void setFloat(const char* name, float v) const;
	void setMat3(const char* name, const float* mat) const;
    void setMat4(const char* name, const float* mat) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
};
#include "pokeapp/Shader.h"
#include "pokeapp/FS.h"
#include <cstdio>
#include <vector>

/*
	Implementation of Shader class for loading and compiling OpenGL shaders.
*/

using namespace std;

Shader::~Shader() {
	if (program_) {
		glDeleteProgram(program_);
	}
}

// Load vertex and fragment shader source code from files, compile and link them into a shader program.
bool Shader::loadFromFiles(const string& vertexPath, const string& fragmentPath) {
    try {
        string v = fs::readTextFile(vertexPath);
        string f = fs::readTextFile(fragmentPath);

        return compileAndLink(v, f);
    }
    catch (const exception& e) {
        fprintf(stderr, "Shader::loadFromFiles exception: %s\n", e.what());
        return false;
    }
}

// Compile vertex and fragment shader source code and link them into a shader program.
bool Shader::compileAndLink(const string& vertexCode, const string& fragmentCode) {

	// Compile vertex shader
	GLuint vertexShader = compileOne(GL_VERTEX_SHADER, vertexCode.c_str(), "vertex");
	if (vertexShader == 0) {
		return false;
	}

	// Compile fragment shader
	GLuint fragmentShader = compileOne(GL_FRAGMENT_SHADER, fragmentCode.c_str(), "fragment");
	if (fragmentShader == 0) {
		glDeleteShader(vertexShader);
		return false;
	}

	// Create shader program
	program_ = glCreateProgram();

	// Attach shaders and link program
	glAttachShader(program_, vertexShader);
	glAttachShader(program_, fragmentShader);
	glLinkProgram(program_);

	// Check link status (success/failure)
	GLint linkStatus = GL_FALSE;
	glGetProgramiv(program_, GL_LINK_STATUS, &linkStatus);
	if (linkStatus != GL_TRUE) {
		logProgramError(program_);
		glDeleteProgram(program_);
		program_ = 0;
	}

	// Clean up shaders (no longer needed after linking)
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return program_ != 0; // Return true if program linked successfully
}

// Compile a single shader of given type (vertex/fragment) from source code.
GLuint Shader::compileOne(GLenum type, const char* src, const char* debugName) {

	// Create a shader of the specified type
	GLuint shader = glCreateShader(type);

	// Set shader source and compile
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	// Check compile status (success/failure)
	GLint compileStatus = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

	if (compileStatus != GL_TRUE) {
		logShaderError(shader, debugName);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

// Log shader compilation errors to stderr.
void Shader::logShaderError(GLuint shader, const char* debugName) {
	GLint logLen = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
	if (logLen > 1) {
		vector<GLchar> log(logLen);
		glGetShaderInfoLog(shader, logLen, nullptr, log.data());
		fprintf(stderr, "Shader compile error (%s): %s\n", debugName, log.data());
	}
	else {
		fprintf(stderr, "Shader compile error (%s): <no log>\n", debugName);
	}
}

// Log program linking errors to stderr.
void Shader::logProgramError(GLuint program) {
	GLint logLen = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
	if (logLen > 1) {
		vector<GLchar> log(logLen);
		glGetProgramInfoLog(program, logLen, nullptr, log.data());
		fprintf(stderr, "Program link error: %s\n", log.data());
	}
	else {
		fprintf(stderr, "Program link error: <no log>\n");
	}
}

// ====== Convenience methods to set uniform variables (wraps OpenGL methods) ======

void Shader::setInt(const char* name, int v) const {
	GLint loc = glGetUniformLocation(program_, name);
	glUniform1i(loc, v);
}

void Shader::setFloat(const char* name, float v) const {
	GLint loc = glGetUniformLocation(program_, name);
	glUniform1f(loc, v);
}

void Shader::setMat3(const char* name, const float* mat) const {
	GLint loc = glGetUniformLocation(program_, name);
	if (loc != -1) {
		glUniformMatrix3fv(loc, 1, GL_FALSE, mat);
	}
}

void Shader::setMat4(const char* name, const float* mat) const {
	GLint loc = glGetUniformLocation(program_, name);
	glUniformMatrix4fv(loc, 1, GL_FALSE, mat);
}

void Shader::setInt(const std::string& name, int value) const {
	glUniform1i(glGetUniformLocation(program_, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const {
	glUniform1f(glGetUniformLocation(program_, name.c_str()), value);
}
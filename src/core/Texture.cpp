#include <pokeapp/Texture.h>
#include <glad/glad.h>

// Define the implementation exactly once here
#define STB_IMAGE_IMPLEMENTATION
#include "../../thirdparty/stb_image.h"

#include <stdexcept>
#include <cstdio>
#include <algorithm>

/*
	Implementation of Texture class for loading and managing OpenGL textures.
    It acts as the bridge between image files and the rendered Pokemon textures.
*/

Texture::Texture(const std::string& path, Kind kind) : path_(path) {
    int width, height, channels;
    
    // Load image from the disk
	stbi_set_flip_vertically_on_load(1); // Flip vertically to match OpenGL coords
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (!data) {
        throw std::runtime_error("Failed to load texture: " + path);
    }
    
	// Create OpenGL texture object
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);
    
	// Handle different image formats based on channel count
    GLenum format, internalFormat;
    switch (channels) {
        case 1: // Greyscale
            format = GL_RED;
            internalFormat = GL_RED;
            // Set swizzle mask to replicate red channel to RGB
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
            break;
		case 2: // Greyscale + Alpha
            format = GL_RG;
            internalFormat = GL_RG;
            break;
        case 3: // RGB
            format = GL_RGB;
            internalFormat = GL_RGB;
            break;
		case 4: // RGBA
            format = GL_RGBA;
            internalFormat = GL_RGBA;
            break;
        default:
            stbi_image_free(data);
            throw std::runtime_error("Unsupported texture channel count: " + std::to_string(channels));
    }
    
	// Upload texture data to GPU
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);

	// Generate mipmaps for better scaling
    glGenerateMipmap(GL_TEXTURE_2D);
    
	// Configure texture wrapping to repeat
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    // Use LINEAR filtering for smooth, sharp textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Maximum anisotropic filtering for sharpness at angles
    float maxAniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
    if (maxAniso > 0.0f) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAniso); // Use max available
        std::printf("  Anisotropic filtering: %.1fx\n", maxAniso);
    }

    // Cleanup
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture() {
    if (id_) glDeleteTextures(1, &id_);
}

// Set as active texture unit and bind this texture
void Texture::bind(int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id_);
}

unsigned int Texture::getId() const {
    return id_;
}

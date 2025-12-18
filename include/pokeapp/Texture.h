#pragma once
#include <string>
#include <memory>

/*
	Texture header file, defines a Texture class for loading and managing textures in OpenGL.

	Textures are essentially images applied (think wrapping paper) to 3D models to 
	give them color and detail. This is used primarily for the grassy terrain in the world.

*/

class Texture {
public:
    enum class Kind { Diffuse };
    
    Texture(const std::string& path, Kind kind);
    ~Texture();
    
    void bind(int unit = 0) const;
    unsigned int getId() const; 
    
private:
    unsigned int id_ = 0;
    std::string path_;
};

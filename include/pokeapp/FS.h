#pragma once
#include <string>

/*
	File system utility functions for reading files.
*/

namespace fs {
    std::string readTextFile(const std::string& filepath);
}
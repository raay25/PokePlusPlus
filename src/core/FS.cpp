#include "../include/pokeapp/FS.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

/*
	File system utility implementation file, provides functions for reading files.
	Used to load important files like shaders. 
*/

using namespace std;

namespace fs {
	string readTextFile(const string& path) {
		ifstream file(path, ios::in);
		if (!file.is_open()) {
			throw runtime_error("Failed to open file: " + path);
		}
		ostringstream ss;
		ss << file.rdbuf();
		return ss.str();
	}
};
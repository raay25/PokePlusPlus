#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform float uPtSize;      // <--- add this

void main() {
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
    gl_PointSize = uPtSize; // <--- critical for core profile
}

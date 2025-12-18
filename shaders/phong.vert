#version 330 core

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=3) in vec2 aTex;   

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vTex;

uniform mat4 uModel, uView, uProj;
uniform mat3 uNormalMat;

void main() {
  vec4 wp = uModel * vec4(aPos, 1.0);
  vWorldPos = wp.xyz;
  vNormal   = normalize(uNormalMat * aNormal);
  vTex      = aTex;
  gl_Position = uProj * uView * wp;
}
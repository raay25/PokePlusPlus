#version 330 core

// Inputs from vertex shader
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTex;

// Material properties
uniform vec3 uKd;
uniform float uShininess;
uniform int uUseTexture;
uniform sampler2D uTex;

// Lighting
uniform vec3 uViewPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;

// Point light
uniform vec3 uPointPos;
uniform vec3 uPointColor;
uniform float uPointIntensity;
uniform float uAttenConst;
uniform float uAttenLinear;
uniform float uAttenQuad;

// Spotlight
uniform vec3 uSpotPos;
uniform vec3 uSpotDir;
uniform float uSpotCut;
uniform float uSpotOuterCut;

// Tint effect
uniform float uTint;

// Terrain texturing (optional)
uniform float uTexScale;
uniform int uHasRock;
uniform sampler2D uGrass, uRock;

out vec4 FragColor;

vec3 sampleTerrainAlbedo() {
  vec2 tiled = vTex * uTexScale;

  if (uHasRock == 0) {
    return texture(uGrass, tiled).rgb; 
  } else {
    vec3 n = normalize(vNormal);
    float slope = clamp(1.0 - dot(n, vec3(0,1,0)), 0.0, 1.0);
    float t = smoothstep(0.3, 0.7, slope);
    vec3 g = texture(uGrass, tiled).rgb;
    vec3 r = texture(uRock, tiled * 0.5).rgb;
    return mix(g, r, t);
  }
}

void main() {
  vec3 N = normalize(vNormal);
  vec3 V = normalize(uViewPos - vWorldPos);
  
  // First, Phong shading (ambient + diffuse + specular))

  // Get base color (texture or material color)
  vec3 baseColor;
  if (uUseTexture == 1) {
    if (uHasRock >= 0) {
      // Terrain texturing
      baseColor = sampleTerrainAlbedo();
    } else {
      // Regular texture sampling (for pokeball, etc.)
      baseColor = texture(uTex, vTex).rgb;
    }
  } else {
    baseColor = uKd;
  }
  
  vec3 ambient = 0.4 * baseColor;
  
  // Directional light
  vec3 L = normalize(-uLightDir);
  float NdotL = max(dot(N, L), 0.0);
  vec3 R = reflect(-L, N);
  float spec = pow(max(dot(R, V), 0.0), uShininess);
  
  vec3 diffuse = NdotL * baseColor * uLightColor;
  vec3 specular = spec * vec3(0.3) * uLightColor;
  
  vec3 result = ambient + diffuse + specular;
  
  // Point light
  if (uPointIntensity > 0.0) {
    vec3 pointDir = uPointPos - vWorldPos;
    float distance = length(pointDir);
    pointDir = normalize(pointDir);
    
    float attenuation = 1.0 / (uAttenConst + uAttenLinear * distance + uAttenQuad * distance * distance);
    
    float pointNdotL = max(dot(N, pointDir), 0.0);
    vec3 pointR = reflect(-pointDir, N);
    float pointSpec = pow(max(dot(pointR, V), 0.0), uShininess);
    
    vec3 pointDiffuse = pointNdotL * baseColor * uPointColor * uPointIntensity;
    vec3 pointSpecular = pointSpec * vec3(0.3) * uPointColor * uPointIntensity;
    
    result += (pointDiffuse + pointSpecular) * attenuation;
  }
  
  // Spotlight (flashlight)
  vec3 spotDir = uSpotPos - vWorldPos;
  float spotDistance = length(spotDir);
  spotDir = normalize(spotDir);
  
  float theta = dot(spotDir, normalize(-uSpotDir));
  float epsilon = uSpotCut - uSpotOuterCut;
  float intensity = clamp((theta - uSpotOuterCut) / epsilon, 0.0, 1.0);
  
  if (intensity > 0.0) {
    float spotNdotL = max(dot(N, spotDir), 0.0);
    vec3 spotR = reflect(-spotDir, N);
    float spotSpec = pow(max(dot(spotR, V), 0.0), uShininess);
    
    float spotAtten = 1.0 / (1.0 + 0.09 * spotDistance + 0.032 * spotDistance * spotDistance);
    
    vec3 spotDiffuse = spotNdotL * baseColor * vec3(1.0, 0.96, 0.9);
    vec3 spotSpecular = spotSpec * vec3(0.5);
    
    result += (spotDiffuse + spotSpecular) * intensity * spotAtten;
  }
  
  // Apply tint
  result = mix(result, result * vec3(1.0, 0.8, 0.6), uTint);
  
  FragColor = vec4(result, 1.0);
}
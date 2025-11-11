#version 300 es
precision highp float;

// Inputs from vertex shader
in vec2 v_bumpMapTexCoord;
in vec3 v_refractionTexCoord;
in vec3 v_reflectionTexCoord;
in vec3 v_position3D;

// Output
out vec4 FragColor;

// Uniforms - Textures
uniform sampler2D u_waterBump;       // Normal map para ondas
uniform sampler2D u_refractionMap;   // O que vês ATRAVÉS da água
uniform sampler2D u_reflectionMap;   // O que vês REFLETIDO na água

// Uniforms - Water properties
uniform vec3 u_cameraPosition;
uniform float u_waveHeight;
uniform vec4 u_waterColor;
uniform float u_colorBlendFactor;

 

const float LOG2 = 1.442695;

int u_debugMode=1;


vec3 sunlightColor = vec3(1.0, 1.0, 1.0);
vec3 sunlightDir = normalize(vec3(-1.0, -1.0, 0.5));

in vec3 fromFragmentToCamera;

// Changes over time, making the water look like it's moving
//uniform float dudvOffset;
in vec4 clipSpace;
in vec2 textureCoords;

const float waterDistortionStrength = 0.03;
const float shineDamper = 20.0;

const float waterReflectivity=0.5;
const float fresnelStrength=1.5;
const float wave_speed= 0.06;
const vec4 shallowWaterColor =  vec4(0.0, 0.1, 0.3, 1.0);
const vec4 deepWaterColor = vec4(0.0, 0.1, 0.2, 1.0);

vec3 getNormal(vec2 textureCoords);
 
void main()
{

 vec2 ndc = (clipSpace.xy / clipSpace.w) / 2.0 + 0.5;
vec2 refractTexCoords = vec2(ndc.x, ndc.y);
vec2 reflectTexCoords = vec2(ndc.x, -ndc.y);

// ============================================
// CALCULAR ÂNGULO DE VISÃO
// ============================================
vec3 eyeVector = normalize(u_cameraPosition - v_position3D);
vec3 upVector = vec3(0.0, 1.0, 0.0);
float viewDot = abs(dot(eyeVector, upVector));

// ============================================
// THRESHOLD: Desabilitar distorção em ângulos muito baixos
// ============================================
const float MIN_ANGLE = 0.15; // Ajustável (0.1 - 0.3)

vec2 perturbation = vec2(0.0);

if (viewDot > MIN_ANGLE) {
    // Aplicar bump map normalmente
    vec4 bumpColor = texture(u_waterBump, v_bumpMapTexCoord);
    
    // Fade suave baseado no ângulo
    float distortionStrength = smoothstep(MIN_ANGLE, 0.5, viewDot);
    perturbation = u_waveHeight * distortionStrength * (bumpColor.rg - 0.5);
}
// else: perturbation fica (0,0) - sem distorção

refractTexCoords += perturbation;
reflectTexCoords += perturbation;

// Clamp
refractTexCoords = clamp(refractTexCoords, 0.001, 0.999);
reflectTexCoords.x = clamp(reflectTexCoords.x, 0.001, 0.999);
reflectTexCoords.y = clamp(reflectTexCoords.y, -0.999, -0.001);

vec4 reflectiveColor = texture(u_reflectionMap, reflectTexCoords);
vec4 refractiveColor = texture(u_refractionMap, refractTexCoords);

// Fresnel
float fresnelTerm = pow(1.0 - max(dot(eyeVector, upVector), 0.0), 3.0);
vec4 combinedColor = refractiveColor * fresnelTerm + reflectiveColor * (1.0 - fresnelTerm);
vec4 finalColor = u_colorBlendFactor * u_waterColor + (1.0 - u_colorBlendFactor) * combinedColor;
     FragColor = finalColor;
	
 
}


vec3 getNormal(vec2 textureCoords) {
    vec4 normalMapColor = texture(u_waterBump, textureCoords);
    float makeNormalPointUpwardsMore = 2.6;
    vec3 normal = vec3(
      normalMapColor.r * 2.0 - 1.0,
      normalMapColor.b * makeNormalPointUpwardsMore,
      normalMapColor.g * 2.0 - 1.0
    );
    normal = normalize(normal);

    return normal;
}
#version 300 es
precision highp float;

// Inputs from vertex shader
in vec2 v_bumpMapTexCoord;
in vec2 v_mapTexCoord;
in vec3 v_refractionTexCoord;
in vec3 v_reflectionTexCoord;
in vec3 v_position3D;
in vec3 v_normal;
 


// Output
out vec4 FragColor;

// Uniforms - Textures
uniform sampler2D u_waterBump;        //0
uniform sampler2D u_foamTexture;        //1
uniform sampler2D u_refractionMap;    //2
uniform sampler2D u_reflectionMap;    //3
uniform sampler2D u_depthMap;         //4

// Uniforms - Water properties
uniform vec3 u_cameraPosition;
uniform float u_waveHeight;
uniform vec4 u_waterColor;
uniform float u_colorBlendFactor;

uniform float u_time;

const float near = 0.1;
const float far = 1000.0;
 
const float LOG2 = 1.442695;
const float FOAM_INTENSITY       = 0.8;   // força da espuma
const float FOAM_INTERSECTION_BAND = 0.01; // quão “grossa” é a zona de interseção (em depth)
const float FOAM_TILING          = 4.0;
const float FOAM_SPEED           = 0.05;


float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main()
{
    vec4 bumpColor = texture(u_waterBump, v_bumpMapTexCoord);
     vec2 perturbation = u_waveHeight * (bumpColor.rg - 0.5);
    
    vec2 ProjectedRefractionTexCoords = clamp(
        v_refractionTexCoord.xy / v_refractionTexCoord.z + perturbation, 
        0.0, 
        1.0
    );
    vec4 refractiveColor = texture(u_refractionMap, ProjectedRefractionTexCoords);
    
    vec2 ProjectedReflectionTexCoords = clamp(
        v_reflectionTexCoord.xy / v_reflectionTexCoord.z + perturbation, 
        0.0, 
        1.0
    );
    vec4 reflectiveColor = texture(u_reflectionMap, ProjectedReflectionTexCoords);

    vec3 eyeVector = normalize(u_cameraPosition - v_position3D);
    vec3 upVector = vec3(0.0, 1.0, 0.0);
    float fresnelTerm = max(dot(eyeVector, upVector), 0.0);
 
    
    float depth  = texture(u_depthMap, ProjectedRefractionTexCoords).r;
    float floorDistance = 2.0 * near * far /(far + near - (2.0 * depth - 1.0) * (far - near));

    depth = gl_FragCoord.y;
    float waterDistance = 2.0 * near * far /(far + near - (2.0 * depth - 1.0) * (far - near));
    float waterDepth = floorDistance - waterDistance;

  

    vec4 combinedColor = refractiveColor * fresnelTerm + reflectiveColor * (1.0 - fresnelTerm);
    vec4 finalColor = u_colorBlendFactor * u_waterColor + (1.0 - u_colorBlendFactor) * combinedColor;
   
 
     
    FragColor = finalColor;
 
}
#version 300 es
precision highp float;

// Inputs from vertex shader
in vec2 v_bumpMapTexCoord;
in vec2 v_mapTexCoord;
in vec3 v_refractionTexCoord;
in vec3 v_reflectionTexCoord;
in vec3 v_position3D;
in vec3 v_normal;
 

in vec4 clipSpace;
in vec3 toCameraPosition;
 
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
    
    vec2 ndc =(clipSpace.xz/clipSpace.w)/2.0 + 0.5;
    vec2 refractTextCoord = vec2(ndc.x,ndc.y);
    vec2 reflectTextCoord = vec2(ndc.x,ndc.y);

    vec4 refractColor = texture(u_refractionMap,refractTextCoord);
    vec4 reflectColor = texture(u_reflectionMap,reflectTextCoord);
    
   
    float depth  = texture(u_depthMap, reflectTextCoord).r;
    float floorDistance = 2.0 * near * far /(far + near - (2.0 * depth - 1.0) * (far - near));
    depth = gl_FragCoord.z;
    float waterDistance = 2.0 * near * far /(far + near - (2.0 * depth - 1.0) * (far - near));

    float waterDepth = floorDistance - waterDistance;





   // FragColor = vec4(waterDepth/50.0);
    FragColor = mix(reflectColor,refractColor,0.5);
}
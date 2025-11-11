#version 300 es
precision highp float;

in vec2 v_bumpMapTexCoord;
in vec3 v_refractionTexCoord;
in vec3 v_reflectionTexCoord;
in vec3 v_position3D;

out vec4 FragColor;

uniform sampler2D u_waterBump;
uniform sampler2D u_refractionMap;
uniform sampler2D u_reflectionMap;

uniform vec3 u_cameraPosition;
uniform float u_waveHeight;
uniform vec4 u_waterColor;
uniform float u_colorBlendFactor;

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
    
    vec4 combinedColor = refractiveColor * fresnelTerm + reflectiveColor * (1.0 - fresnelTerm);
    
    vec4 finalColor = u_colorBlendFactor * u_waterColor + (1.0 - u_colorBlendFactor) * combinedColor;
    
    FragColor = reflectiveColor;
}
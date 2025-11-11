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

    float near = 0.1;
    float far = 50.0;

    // ============================================
    // 1. BUMP MAP (Ondas - Distorção)
    // ============================================
    vec4 bumpColor = texture(u_waterBump, v_bumpMapTexCoord);
    
    // Converte [0,1] para [-0.5, 0.5] e escala pela altura das ondas
    vec2 perturbation = u_waveHeight * (bumpColor.rg - 0.5);
   // vec2 perturbation = u_waveHeight * 0.015 * (bumpColor.rg - 0.5);  
    //float depthFactor = clamp(distance / 50.0, 0.0, 1.0);  // Mais longe = menos distorção
    //vec2 perturbation = u_waveHeight * 0.015 * (bumpColor.rg - 0.5) * (1.0 - depthFactor);

 
    // ============================================
    // 2. REFRACTION (O que vês ATRAVÉS da água)
    // ============================================
    
	vec2 ProjectedRefractionTexCoords = clamp(v_refractionTexCoord.xy / v_refractionTexCoord.z + perturbation, 0.0, 1.0);
	vec4 refractiveColor = texture2D(u_refractionMap, ProjectedRefractionTexCoords );

 

    // ============================================
    // 3. REFLECTION (O que vês REFLETIDO)
    // ============================================
	vec2 ProjectedReflectionTexCoords = clamp(v_reflectionTexCoord.xy / v_reflectionTexCoord.z + perturbation, 0.0, 1.0);
    vec4 reflectiveColor = texture2D(u_reflectionMap, ProjectedReflectionTexCoords );

 
	    if (u_debugMode == 1) {
        // Ver só refraction
        vec2 coords = v_refractionTexCoord.xy;
        //FragColor = texture(u_refractionMap, coords);
        FragColor = reflectiveColor;
        return;
    }
    if (u_debugMode == 2) {
        // Ver só reflection
        vec2 coords = v_reflectionTexCoord.xy;
        FragColor = texture(u_reflectionMap, coords);
        return;
    }
    if (u_debugMode == 3) {
        // Ver coordenadas (debug)
        FragColor = vec4(v_refractionTexCoord.xy, 0.0, 1.0);
        return;
    }
    if (u_debugMode == 4) {
        // Ver só reflection
        
        FragColor = texture(u_waterBump, v_bumpMapTexCoord);
        return;
    }
        


    refractTexCoords = clamp(refractTexCoords, 0.001, 0.999);
    reflectTexCoords.x = clamp(reflectTexCoords.x, 0.001, 0.999);
    reflectTexCoords.y = clamp(reflectTexCoords.y, -0.999, -0.001);

    refractTexCoords += perturbation;
    reflectTexCoords += perturbation;

    reflectiveColor = texture(u_reflectionMap, reflectTexCoords);
    refractiveColor = texture(u_refractionMap, refractTexCoords);

    // ============================================
    // 4. FRESNEL EFFECT
    // ============================================
	vec3 eyeVector = normalize(u_cameraPosition - v_position3D);
	vec3 upVector = vec3(0.0, 1.0, 0.0);
	float viewDot = abs(dot(eyeVector, upVector));

    
	//fresnel can not be lower than 0
	//float fresnelTerm = max( dot(eyeVector, upVector), 0.0 );
  //  float fresnelTerm = pow(max(dot(eyeVector, upVector), 0.0), 2.0);
    float fresnelTerm = pow(1.0 - max(dot(eyeVector, upVector), 0.0), 3.0);


    vec4 combinedColor = refractiveColor * fresnelTerm + reflectiveColor * (1.0 - fresnelTerm);
	vec4 finalColor = u_colorBlendFactor * u_waterColor + (1.0 - u_colorBlendFactor) * combinedColor;

/*
        // Calcular especular (brilho da luz)
        vec3 lightDir = normalize(vec3(1.0, 1.0, 0.5)); // Direção da luz (sol)
        vec3 halfVector = normalize(lightDir + eyeVector);
        float specular = pow(max(dot(upVector, halfVector), 0.0), 128.0);
      // finalColor.rgb += vec3(specular * 0.5); //  intensidade


        float distance = length(v_position3D - u_cameraPosition);
        float fogAmount = clamp((distance - 200.0) / 50.0, 0.0, 1.0);
        vec3 fogColor = vec3(0.5, 0.6, 0.7); // Cor do fog (azul claro)
         // finalColor.rgb = mix(finalColor.rgb, fogColor, fogAmount);



    // Calcular profundidade baseado na distorção
    float depth = 1.0 - fresnelTerm; // 0 = raso, 1 = profundo

    // Misturar com cor da água baseado na profundidade
    vec4 deepWaterColor = vec4(0.0, 0.1, 0.3, 1.0); // Azul escuro
    vec4 shallowWaterColor = vec4(0.3, 0.5, 0.7, 1.0); // Azul claro
    vec4 waterTintColor = mix(shallowWaterColor, deepWaterColor, depth);
    //finalColor = mix(finalColor, waterTintColor, 0.3); // 30% de tint
*/

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
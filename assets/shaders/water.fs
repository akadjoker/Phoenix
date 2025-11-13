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
 
    
    // float sceneDepth  = texture(u_depthMap, ProjectedRefractionTexCoords).r;
    // float floorDistance = 2.0 * near * far /(far + near - (2.0 * sceneDepth - 1.0) * (far - near));

    // float waterDepth = gl_FragCoord.y;
    // float waterDistance = 2.0 * near * far /(far + near - (2.0 * waterDepth - 1.0) * (far - near));
    // float depthDiff = floorDistance - waterDistance;

  

    vec4 combinedColor = refractiveColor * fresnelTerm + reflectiveColor * (1.0 - fresnelTerm);
    vec4 finalColor = u_colorBlendFactor * u_waterColor + (1.0 - u_colorBlendFactor) * combinedColor;

 


   vec2 depthCoords = clamp(v_refractionTexCoord.xy / v_refractionTexCoord.z, 0.0, 1.0);
    
    float sceneDepth = LinearizeDepth(texture(u_depthMap, ProjectedRefractionTexCoords).r);
    float waterDepth = LinearizeDepth(gl_FragCoord.y);

float depthDiff = sceneDepth - waterDepth;
float edgeFoam = 1.0 - smoothstep(0.0, 0.5, depthDiff);

// 3. Calcula wave foam (espuma nas cristas)
float waveFoam = pow(1.0 - dot(normalize(v_normal), vec3(0, 1, 0)), 20.0);

// 4. Calcula noise foam (textura animada)
vec2 foamUV = v_position3D.xz * 2.0 + u_time * 0.1;
float foamPattern = texture(u_foamTexture, foamUV).r;

// 5. Combina todos os tipos de foam
float finalFoam = (edgeFoam + waveFoam * 0.5) * foamPattern;
vec3 foamColor = vec3(1.0) * finalFoam;

// 6. BLEND foam com a cor da água
// Usa o foam como alpha para misturar
FragColor = vec4(mix(finalColor.rgb, foamColor, finalFoam), 1.0);
 

/*
    FragColor =finalColor;
     vec2 depthCoords = clamp(v_refractionTexCoord.xy / v_refractionTexCoord.z, 0.0, 1.0);
    
    float sceneDepth = LinearizeDepth(texture(u_depthMap, v_bumpMapTexCoord).r);
    float waterDepth = LinearizeDepth(v_refractionTexCoord.y);//← NÃO gl_FragCoord.z!
    float depthDiff = max(sceneDepth - waterDepth, 0.0);
    
    // Blend só onde há interseção próxima
    float edgeBlend = (1.0 - smoothstep(0.0, 1.0, depthDiff)) * 0.3;
    finalColor.rgb += vec3(edgeBlend);
    
   // FragColor = finalColor;

     if (depthDiff < 1.0) 
     {
     //   FragColor = vec4(1.0, 0.0, 0.0, 1.0); // VERMELHO onde blend ativo
      //  return;
    }
    
    // Clareia onde toca (0.5m range)
    float edgeBrightness = 1.0 - smoothstep(0.0, 0.5, depthDiff);
    
    // Adiciona brilho
    //finalColor.rgb += vec3(edgeBrightness * 0.3); // Clareia 30%

    
   // FragColor.a = clamp(depthDiff/1.0,0.0,1.0);

     
    FragColor = finalColor;
    */
 
}
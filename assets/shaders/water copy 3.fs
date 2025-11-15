#version 300 es
precision highp float;

 

out vec4 FragColor;

in vec4 ClipSpace;
in vec2 TexCoord;
in vec3 ToCameraVector;
in vec3 WorldPos;
in vec3 v_normal;

in vec2 v_bumpMapTexCoord;
in vec2 v_mapTexCoord;

uniform sampler2D reflectionTexture;
uniform sampler2D refractionTexture;
uniform sampler2D refractionDepth;
uniform sampler2D waterBump;
uniform sampler2D foamTexture;


uniform vec3 u_cameraPosition;
uniform float u_waveHeight;
uniform vec4 u_waterColor;
uniform float u_colorBlendFactor;
uniform float u_time;

uniform float mult;
        
//const float foamEdgeDistance = 2.0;  // Distância da borda onde aparece foam
const float foamCutoff = 0.5;        // Threshold para foam
const float foamScale = 1.0;         // Scale da textura de foam
        
 void main() 
 {
     float foamEdgeDistance =mult;
     vec4 bumpColor = texture(waterBump, v_bumpMapTexCoord);
     vec2 perturbation = u_waveHeight * (bumpColor.rg - 0.5);

    vec2 ndc = (ClipSpace.xy / ClipSpace.w) * 0.5 + 0.5;
    vec2 reflectTexCoords = vec2(ndc.x, 1.0 - ndc.y);
    vec2 refractTexCoords = vec2(ndc.x, ndc.y);
    
    float near = 0.1;
    float far = 1000.0;
    
    // Depth calculation
    float depth = texture(refractionDepth, refractTexCoords).r;
    float floorDistance = 2.0 * near * far / (far + near - (2.0 * depth - 1.0) * (far - near));
    
    depth = gl_FragCoord.z;
    float waterDistance = 2.0 * near * far / (far + near - (2.0 * depth - 1.0) * (far - near));
    float waterDepth = floorDistance - waterDistance;
    float normalizedDepth = clamp(waterDepth / mult, 0.0, 1.0);
 
    
    reflectTexCoords = clamp(reflectTexCoords + perturbation, 0.001, 0.999);
    refractTexCoords = clamp(refractTexCoords + perturbation, 0.001, 0.999);
    
    vec4 reflectColor = texture(reflectionTexture, reflectTexCoords);
    vec4 refractColor = texture(refractionTexture, refractTexCoords);
    
    vec3 eyeVector = normalize(u_cameraPosition - WorldPos);
    vec3 upVector = vec3(0.0, 1.0, 0.0);
    float fresnelTerm = max(dot(eyeVector, upVector), 0.0);

    vec4 combinedColor = refractColor * fresnelTerm + reflectColor * (1.0 - fresnelTerm);
    vec4 finalColor = u_colorBlendFactor * u_waterColor + (1.0 - u_colorBlendFactor) * combinedColor;
    
   
   

   // 1. Edge foam (nas bordas/shore)
    float edgeFoam = smoothstep(foamEdgeDistance, 0.0, waterDepth);
    edgeFoam = pow(edgeFoam, 2.0); // Curve para transição mais suave
    
    // 2. Foam texture com animação
    vec2 foamUV1 = WorldPos.xz * foamScale + vec2(u_time * 0.05, u_time * 0.03);
    vec2 foamUV2 = WorldPos.xz * foamScale * 0.7 - vec2(u_time * 0.04, u_time * 0.06);
    
    float foamPattern1 = texture(foamTexture, foamUV1).r;
    float foamPattern2 = texture(foamTexture, foamUV2).r;
    float foamPattern = (foamPattern1 + foamPattern2) * 0.5;
    
    // 3. Wave crest foam (baseado nas normais)
    // Ondas com normais mais verticais têm mais foam
    float waveHeight = 1.0 - v_normal.y; // 0 = flat, 1 = steep
    float crestFoam = smoothstep(0.4, 0.8, waveHeight) * 0.5;
    
    // 4. Combinar foams
    float foamFactor = max(edgeFoam, crestFoam);
    foamFactor = foamFactor * smoothstep(foamCutoff - 0.1, foamCutoff + 0.1, foamPattern);
    
    // Cor do foam (branco com leve tint azulado)
    vec3 foamColor = vec3(0.95, 0.98, 1.0);


    finalColor.rgb = mix(finalColor.rgb, foamColor, foamFactor);


    
    FragColor =finalColor;
    
    //FragColor = vec4(waterDepth/mult);

 }
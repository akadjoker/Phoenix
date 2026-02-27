#version 300 es
precision highp float;
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_sceneTexture;
uniform sampler2D u_godRaysTexture;

void main()
{
    vec3 scene = texture(u_sceneTexture, TexCoords).rgb;
    vec3 godRays = texture(u_godRaysTexture, TexCoords).rgb;
    
    // Opção 1: Additive blending com clamp (mais intenso)
    //vec3 finalColor = scene + godRays;
    
    // Opção 2: Screen blending (mais suave, evita saturação)
     //vec3 finalColor = vec3(1.0) - (vec3(1.0) - scene) * (vec3(1.0) - godRays);
    
    // Opção 3: Additive com soft clamp (boa para HDR)
     vec3 finalColor = scene + godRays * 0.8;

    
    vec3 rayTint = vec3(1.0, 0.95, 0.85);  // Tom quente
 //   vec3 finalColor = vec4(scene + godRays * rayTint, 1.0);

    
    FragColor = vec4((finalColor* rayTint), 1.0);
}
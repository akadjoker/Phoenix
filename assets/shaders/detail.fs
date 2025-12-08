#version 300 es
precision mediump float;   

// Inputs
in vec2 vTexCoord;
in vec2 vLightmapCoord;

// Uniforms
uniform sampler2D uTexture;
uniform sampler2D uLightmap;
 

// Output
out vec4 FragColor;

void main()
{
    vec4 baseColor   = texture(uTexture, vTexCoord);
    if (baseColor.a < 0.5)
        discard;
   vec3 lightmap = texture(uLightmap, vLightmapCoord).rgb;
    FragColor = vec4(baseColor.rgb * lightmap * 4.0, baseColor.a);

    //FragColor = mix(baseColor, texture(uLightmap, vLightmapCoord),  0.8);


    

  
}

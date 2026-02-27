#version 300 es
precision mediump float;   

// Inputs
in vec2 vTexCoord;
in vec2 vLightmapCoord;

// Uniforms
uniform sampler2D uTexture;
uniform sampler2D uLightmap;
const float lightmapBlend= 0.5; 

// Output
out vec4 FragColor;

void main()
{
   // vec4 baseColor   = texture(uTexture, vTexCoord);
   // if (baseColor.a < 0.5)
   //     discard;
  // vec3 lightmap = texture(uLightmap, vLightmapCoord).rgb;
  //  FragColor = vec4(baseColor.rgb * lightmap * 5.0, baseColor.a);

   //FragColor = mix(baseColor, texture(uLightmap, vec2( vLightmapCoord.x,   vLightmapCoord.y)),  0.9);

    vec4 texColor = texture(uTexture, vTexCoord) ; 
    //if (texColor.a < 0.1) discard;


    vec4 lightmapColor = texture(uLightmap, vLightmapCoord);
    vec3 fragColor = vec3(0.5);

    vec3 result = texColor.rgb + lightmapColor.rgb ;
    result *= mix(vec3(0.5), fragColor.rgb, lightmapBlend);
    FragColor = vec4(result, texColor.a);
    

  
}

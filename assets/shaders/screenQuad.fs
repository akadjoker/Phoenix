#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D tex;


const float NEAR = 0.1;
const float FAR = 1000.0;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // [0,1] -> [-1,1] NDC
    return (2.0 * NEAR * FAR) / (FAR + NEAR - z * (FAR - NEAR));
}


void main() 
{

     float depth = texture(tex, TexCoord).r;
    
    // Lineariza e normaliza para visualização
    float linear = LinearizeDepth(depth);
    float normalized = linear / FAR; // [0, FAR] -> [0, 1]
    
    FragColor = vec4(vec3(normalized), 1.0);


  //  FragColor = texture(tex, TexCoord);
}
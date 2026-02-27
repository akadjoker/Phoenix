#version 300 es

precision highp float;

// Inputs
in vec2 v_texCoord;
in vec4 v_color;

// Uniforms
uniform sampler2D u_texture;
uniform int u_hasTexture;  // 0 = sem textura, 1 = com textura

// Output
out vec4 FragColor;

void main()
{
    vec4 texColor = vec4(1.0);
    
    if (u_hasTexture == 1)
    {
        texColor = texture(u_texture, v_texCoord);
    }
    
    // Multiplicar textura por vertex color
    FragColor = texColor * v_color;
    
    // Descartar se muito transparente (evita z-fighting)
    if (FragColor.a < 0.01)
        discard;
}

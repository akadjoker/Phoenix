#version 300 es
precision highp float;
in vec2 vTexCoord;
in vec4 vColor;

uniform sampler2D uTexture;

out vec4 FragColor;

void main()
{
    vec4 tex = texture(uTexture, vTexCoord);
   vec4 color = vColor * tex;

    // ?? reforçar o brilho do centro
    // color.rgb *= 1.2;

    // Se a alpha estiver muito baixa, vamos descartar:
    if (color.a <= 0.01)
        discard;

    FragColor = color;
}

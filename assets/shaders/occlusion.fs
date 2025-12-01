#version 300 es
precision highp float;
out vec4 FragColor;

uniform vec3 u_occlusionColor;

void main()
{
    FragColor = vec4(u_occlusionColor, 1.0);
}

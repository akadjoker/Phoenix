#version 300 es
precision highp float;

in vec2 vTexCoord;
in vec3 vColor;

uniform sampler2D uTexture;

out vec4 FragColor;

void main() {
    vec4 texColor = texture(uTexture, vTexCoord);
    FragColor = vec4(vColor, 1.0) * texColor;
}
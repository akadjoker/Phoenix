#version 300 es
precision highp float;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

uniform mat4 view;
uniform mat4 projection;
 
out vec2 vTexCoord;
out vec4 vColor;

void main()
{
    vTexCoord = aTexCoord;
    vColor = aColor;
 
    vec4 worldPos = vec4(aPosition, 1.0);
    gl_Position = projection * view * worldPos;
 
}

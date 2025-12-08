#version 300 es
precision highp float;



// Vertex attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;      // UV textura
layout(location = 2) in vec2 aLightmapCoord; // UV lightmap

// Uniforms

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

// Outputs
out vec2 vTexCoord;
out vec2 vLightmapCoord;

void main()
{
    vTexCoord = aTexCoord;
    vLightmapCoord = aLightmapCoord;
    
    gl_Position = projection * view * model * vec4(aPosition, 1.0);
}

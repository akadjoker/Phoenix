#version 300 es

precision highp float;

// Inputs
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texCoord;
layout(location = 2) in vec4 a_color;

// Uniforms
uniform mat4 u_view;
uniform mat4 u_projection;

// Outputs
out vec2 v_texCoord;
out vec4 v_color;

void main()
{
    // Billboard já calculado no CPU, só transformar por view/projection
    vec4 viewPos = u_view * vec4(a_position, 1.0);
    gl_Position = u_projection * viewPos;
    
    v_texCoord = a_texCoord;
    v_color = a_color;
}

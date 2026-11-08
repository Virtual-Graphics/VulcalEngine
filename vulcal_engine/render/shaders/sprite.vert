#version 460 core

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_uv;

layout (std140, binding = 0) uniform CameraBlock {
    mat4 u_view_proj;
    vec4 u_view_pos;
};

uniform mat4 u_model;

out vec2 v_uv;

void main() {
    v_uv = a_uv;
    gl_Position = u_view_proj * u_model * vec4(a_position, 0.0, 1.0);
}

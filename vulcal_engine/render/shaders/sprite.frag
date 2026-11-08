#version 460 core

in vec2 v_uv;
out vec4 f_color;

uniform sampler2D u_texture;
uniform vec4 u_color;
uniform vec4 u_uv_rect;
uniform vec2 u_flip;

void main() {
    vec2 uv = mix(v_uv, vec2(1.0) - v_uv, u_flip);
    uv = mix(u_uv_rect.xy, u_uv_rect.zw, uv);

    vec4 texel = texture(u_texture, uv);
    f_color = texel * u_color;
}

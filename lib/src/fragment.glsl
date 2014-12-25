#version 430
//#extension GL_ARB_texture_rectangle : enable

layout(location = 0) uniform int vp_width;
layout(location = 1) uniform int vp_height;
layout(location = 2) uniform vec4 color;
layout(location = 3) uniform sampler2D sampler;
layout(location = 5) uniform int render_mode;

in  vec2 texel_position;
out vec4 fragment_color;

void main() {

    if (render_mode == 1) {
        fragment_color = color;
    }
    else if (render_mode == 2) {
        ivec2 tex_size = textureSize(sampler, 0);
        ivec2 tex_pos = ivec2(texel_position);
        tex_pos %= tex_size;
        //vec2 uv = vec2((tex_pos.x + 0.5) / float(tex_size.x), (tex_pos.y + 0.5) / float(tex_size.y));
        //fragment_color = textureLod(sampler, uv, 0);
        fragment_color = texelFetch(sampler, tex_pos, 0);
    }
}
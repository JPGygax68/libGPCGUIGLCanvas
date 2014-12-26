#version 430

layout(location = 0) uniform int vp_width;
layout(location = 1) uniform int vp_height;
layout(location = 2) uniform vec4 color;
layout(location = 3) uniform sampler2DRect sampler;
layout(location = 5) uniform int render_mode;
layout(location = 6) uniform ivec2 offset;

in  vec2 texel_position;
out vec4 fragment_color;

void main() {

    if (render_mode == 1) {
        fragment_color = color;
    }
    else if (render_mode == 2) {
        ivec2 tex_size = textureSize(sampler, 0);
        fragment_color = texelFetch(sampler, (ivec2(texel_position) + offset) % tex_size);
    }
}
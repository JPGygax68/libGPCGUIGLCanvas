#version 430

layout(location = 0) uniform int            vp_width;
layout(location = 1) uniform int            vp_height;
layout(location = 2) uniform vec4           color;
layout(location = 3) uniform sampler2DRect  sampler;
layout(location = 5) uniform int            render_mode;
layout(location = 6) uniform ivec2          offset;             // when rendering images: top-left corner inside image
layout(location = 7) uniform samplerBuffer  font_pixels; 
layout(location = 8) uniform int            glyph_base;
layout(location = 9) uniform ivec4          glyph_cbox;

in  vec2 texel_position;
out vec4 fragment_color;

void main() {

    // Apply single color
    if (render_mode == 1) {

        fragment_color = color;
    }
    // Image pasting
    else if (render_mode == 2) {

        ivec2 tex_size = textureSize(sampler, 0);
        fragment_color = texelFetch(sampler, (ivec2(texel_position) + offset) % tex_size);
    }
    // Glyph rendering
    else if (render_mode == 3) {

        int x_min = glyph_cbox[0], x_max = glyph_cbox[1], y_min = glyph_cbox[2], y_max = glyph_cbox[3];
        int w = x_max - x_min, h = y_max - y_min;

        int col = int(texel_position.x) - x_min;
        #ifdef Y_AXIS_DOWN
        int row = int(texel_position.y) + y_max - 1;
        #else
        int row = y_max - int(texel_position.y) - 1;
        #endif

        float value = texelFetch(font_pixels, glyph_base + row * w + col);

        fragment_color = vec4(color.rgb, value);
    }
}
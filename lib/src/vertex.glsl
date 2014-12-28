#version 430

// Viewport width and height
layout(location =  0) uniform int           vp_width;
layout(location =  1) uniform int           vp_height;
layout(location =  4) uniform ivec2         origin; // todo: rename to "offset" ?
layout(location =  5) uniform int           render_mode;
layout(location =  9) uniform ivec4         glyph_cbox;
layout(location = 10) uniform ivec2         position;

in  vec2 vertex_position;
out vec2 texel_position;

void main() {

    if (render_mode == 1 || render_mode == 2) {

        #ifdef Y_AXIS_DOWN
        gl_Position = vec4(2 * vertex_position.x / float(vp_width) - 1, - (2 * vertex_position.y / float(vp_height) - 1), 0.0, 1.0);
        #else
        gl_Position = vec4(2 * vertex_position.x / float(vp_width) - 1, 2 * vertex_position.y / float(vp_height) - 1, 0.0, 1.0);
        #endif
        texel_position = vertex_position.xy - origin;
    }
    else if (render_mode == 3) {

        ivec2 pixel_position = position + ivec2(vertex_position);

        #ifdef Y_AXIS_DOWN
        gl_Position = vec4(2 * pixel_position.x / float(vp_width) - 1, - (2 * pixel_position.y / float(vp_height) - 1), 0.0, 1.0);
        #else
        gl_Position = vec4(2 * pixel_position.x / float(vp_width) - 1, 2 * pixel_position.y / float(vp_height) - 1, 0.0, 1.0);
        #endif

        texel_position = vertex_position;
    }
}

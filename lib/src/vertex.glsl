#version 430

// Viewport width and height
layout(location = 0) uniform int   vp_width;
layout(location = 1) uniform int   vp_height;
layout(location = 4) uniform ivec2 origin;

in vec3  vertex_position;
out vec2 texel_position;

void main() {

    texel_position = vertex_position.xy - origin;

#ifdef Y_AXIS_DOWN
    gl_Position = vec4(2 * vertex_position.x / float(vp_width) - 1, - (2 * vertex_position.y / float(vp_height) - 1), 0.0, 1.0);
#else
    gl_Position = vec4(2 * vertex_position.x / float(vp_width) - 1, 2 * vertex_position.y / float(vp_height) - 1, 0.0, 1.0);
#endif
}

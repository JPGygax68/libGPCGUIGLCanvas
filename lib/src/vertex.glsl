#version 430

// Viewport width and height
layout(location = 0) uniform int vp_width;
layout(location = 1) uniform int vp_height;

in vec3 vertex_position;

void main() {

    // TODO: option to flip vertical coordinates (and possibly horizontal ones too)
    gl_Position = vec4(2 * vertex_position.x / float(vp_width) - 1, 2 * vertex_position.y / float(vp_height) - 1, 0.0, 1.0);
}

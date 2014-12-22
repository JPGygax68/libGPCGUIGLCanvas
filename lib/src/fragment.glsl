#version 430

layout(location = 2) uniform vec4 color;

out vec4 fragment_color;

void main() {
    fragment_color = color;
}
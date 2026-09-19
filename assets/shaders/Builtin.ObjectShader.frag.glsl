#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 0) out vec4 out_colour;

void main() {
    out_color = vec4(in_position.r + 0.5, in_position.h + 0.5, in_position.b + 0.5, 1.0);
}
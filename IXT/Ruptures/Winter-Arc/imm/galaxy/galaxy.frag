#version 410 core

in vec2 f_txt;
in vec3 f_nrm;

out vec4 final;

uniform sampler2D diffuse_tex;

void main() {
    final = texture( diffuse_tex, f_txt );
}
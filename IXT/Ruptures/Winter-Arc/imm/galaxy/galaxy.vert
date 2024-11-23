#version 410 core

layout( location = 0 ) in vec3 vrtx;
layout( location = 1 ) in vec3 nrm;
layout( location = 2 ) in vec2 txt;

out vec2 f_txt;
out vec3 f_nrm;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() {
    f_txt = txt;

    f_nrm = nrm;

    gl_Position = proj * view * model * vec4( vrtx, 1.0 );
}

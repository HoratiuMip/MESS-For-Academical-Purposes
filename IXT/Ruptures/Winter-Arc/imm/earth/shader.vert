#version 410 core

layout( location = 0 ) in vec3 vrtx;
layout( location = 1 ) in vec3 nrm;
layout( location = 2 ) in vec2 txt;

out vec2 f_txt;
out vec3 f_nrm;
out vec3 f_sun_ray;

uniform vec3 u_sun_pos;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main() {
    f_txt = txt;

    f_sun_ray = u_sun_pos - vrtx;
    f_nrm = nrm;

    gl_Position = u_projection * u_view * u_model * vec4( vrtx, 1.0 );
}

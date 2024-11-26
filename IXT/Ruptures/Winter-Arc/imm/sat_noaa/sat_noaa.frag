#version 410 core

in VS_OUT {
    vec2 tex_crd;
    vec3 nrm;
    vec3 sun_ray;
    vec3 lens_ray;
} vs_out;

out vec4 final;

uniform sampler2D diffuse_tex;

void main() {
    final = texture( diffuse_tex, vs_out.tex_crd );
    final.r *= 0.86;
    final.w = 1.0;
}
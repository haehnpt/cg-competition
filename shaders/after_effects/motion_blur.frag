#version 330 core
in vec2 interp_uv;

out vec4 frag_color;

uniform sampler2D tex;
uniform float alpha;

void main() {
    frag_color = alpha * texture(tex, interp_uv);
}

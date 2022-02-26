#version 330

in vec3 vPosition;
in vec2 vUv;
in vec3 vNormal;

uniform sampler2D hdri;
uniform vec3 cameraPosition;

out vec4 FragColor;

const float PI2 = 1.570796;
const float PI = 3.141592;

float angle (vec3 a, vec3 b) {
    return acos(dot(normalize(a), normalize(b)));
}

void main () {
    FragColor = vec4(vNormal * 0.5 + 0.5, 1.0);
}
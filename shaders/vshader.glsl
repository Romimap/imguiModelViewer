#version 330

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iUv;
layout(location = 2) in vec3 iNormal;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform sampler2D hdri;


out vec3 vPosition;
out vec2 vUv;
out vec3 vNormal;

void main () {
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(iPosition * 0.5, 1.0);

    vPosition = iPosition;
    vUv = iUv;
    vNormal = iNormal;
}
#version 330

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec3 iColor;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec3 vPosition;
out vec3 vColor;

void main () {
    gl_Position = vec4(iPosition * 0.5, 1.0);

    vPosition = iPosition;
    vColor = iColor;
}
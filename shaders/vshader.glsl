#version 330

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iUv;
layout(location = 2) in vec3 iNormal;
layout(location = 3) in vec3 iTangent;
layout(location = 4) in vec3 iBitangent;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform float TIME;


out vec3 vPosition;
out vec2 vUv;
out vec3 vNormal;
out vec3 vTangent;
out vec3 vBitangent;

void main () {
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(iPosition, 1.0);

    vPosition = iPosition;
    vUv = iUv*10 + (vec2(0.1, 0) * sin(TIME));
    vNormal = iNormal;
    vTangent = iTangent;
    vBitangent = iBitangent;
}






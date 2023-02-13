#version 330

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iUv;
layout(location = 2) in vec3 iNormal;
layout(location = 3) in vec3 iTangent;
layout(location = 4) in vec3 iBitangent;

out vec3 vPosition;
out vec2 vUv;
out vec3 vNormal;
out vec3 vTangent;
out vec3 vBitangent;

const float pi = 3.1415;


mat4 rotationX(float theta) {
    return mat4(1.0,       0.0,           0.0,           0.0,
                0.0,       cos(theta),    -sin(theta),   0.0,
                0.0,       sin(theta),    cos(theta),    0.0,
                0.0,       0.0,           0.0,           1.0);
}


void main () {
    gl_Position = rotationX(-pi / 2.0) * vec4(iPosition, 1.0);

    vPosition = iPosition;
    vUv = iUv;
    vNormal = iNormal;
    vTangent = iTangent;
    vBitangent = iBitangent;
}

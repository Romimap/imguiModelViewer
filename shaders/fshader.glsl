#version 400

in vec3 vPosition;
in vec2 vUv;
in vec3 vNormal;
in vec3 vTangent;
in vec3 vBitangent;

in vec4 gl_FragCoord;
out vec4 FragColor;

uniform mat4 viewMatrix;
uniform sampler2D albedo;
uniform sampler2D normal;
uniform sampler2D bmap;
uniform sampler2D mmap;
uniform sampler2D roughness;
uniform sampler2D mipchart;
uniform sampler2D constantSigma;
uniform sampler2D var;

uniform vec3 cameraPosition;

uniform float TIME;
uniform float DTIME;

void main () {
	FragColor = vec4(gl_FragCoord.xy * 0.005, 1.0, 1.0);
}



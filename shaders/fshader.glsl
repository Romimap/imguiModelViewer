#version 330

in vec3 vPosition;
in vec2 vUv;
in vec3 vNormal;
in vec3 vTangent;
in vec3 vBitangent;

uniform mat4 viewMatrix;
uniform sampler2D albedo;
uniform sampler2D normal;
uniform sampler2D roughness;
uniform sampler2D hdri;

uniform vec3 cameraPosition;

out vec4 FragColor;

const vec3 lightDir = vec3(0.2182, 0.8728, 0.4364);
const vec3 lightColor = vec3(1.3, 1.1, 0.9);
const vec3 ambientColor = vec3(0.1, 0.15, 0.25);

const float pi = 3.141592;

vec3 getSpecular (vec3 normal, float m) {
	vec3 viewDir = -normalize(vPosition - cameraPosition);
	vec3 h = (viewDir + lightDir) / 2.0;
	//if (dot(h, vNormal) < 0.1) return vec3(0);
	
	vec3 hBar = h / dot(h, normal); //Project h on a plane 1 unit away from the normal
	float x = distance(hBar, normal); //Get the distance between the projected point and the center of the plane
	if (dot(normal, lightDir) < .1) x = pi;
	if (length(h) < 0.1) x = pi;
	if (x > pi) x = pi;
	float s = pow((cos(x) + 1) * 0.5, m);
	
	return s * lightColor;
}

vec3 getDiffuse (vec3 normal) {
	return min(1.0, max(0.0, dot(lightDir, normal))) * lightColor + ambientColor;
}

vec3 getAlbedo () {
	return texture(albedo, vUv).rgb;
}

float angle (vec3 a, vec3 b) {
	return acos(dot(normalize(a), normalize(b)));
}

vec3 computeNormal(float p) {
	vec3 tNormal = texture(normal, vUv).xyz;
	tNormal = tNormal * 2.0 - vec3(1.0);
	return normalize(vTangent * p * tNormal.x + vBitangent * p * -tNormal.y + vNormal * tNormal.z);
}

void main () {
	vec3 nrm = computeNormal(0.2);
	FragColor = vec4(getDiffuse(nrm) * getAlbedo() + (getSpecular(nrm, 20) * 0.2), 1.0);
	//FragColor = vec4(getSpecular(normal, 100), 1);
}


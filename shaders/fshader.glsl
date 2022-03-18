#version 460

in vec3 vPosition;
in vec2 vUv;
in vec3 vNormal;
in vec3 vTangent;
in vec3 vBitangent;

uniform mat4 viewMatrix;
uniform sampler2D albedo;
uniform sampler2D normal;
uniform sampler2D bmap;
uniform sampler2D mmap;
uniform sampler2D roughness;
uniform sampler2D hdri;

uniform vec3 cameraPosition;

uniform float TIME;
uniform float DTIME;

out vec4 FragColor;

const vec3 lightColor = vec3(1.3, 1.1, 0.9);
const vec3 ambientColor = vec3(0.1, 0.15, 0.25);

const float pi = 3.141592;

vec3 viewDirection () {
	return normalize(cameraPosition - vPosition);
}

vec3 lightDirection () {
	return normalize(vec3(2, 5, 0));
}

vec3 h() {
	return (viewDirection() + lightDirection()) / 2;
}

vec3 GlobalToNormalSpace(vec3 v) {
	return vec3(dot(v, vBitangent), dot(v, vNormal), dot(v, vTangent));
}

vec3 NormalToGlobalSpace(vec3 v) {
	return v.x * -vBitangent + v.y * vNormal + v.z * vTangent;
}


vec3 getSigma (vec3 b, vec3 m) {
	return vec3(m.x - b.x * b.x, m.y - b.y * b.y, m.z - b.x * b.y);
}

vec3 getDiffuse () {
	return texture(albedo, vUv).rgb * dot(vNormal, lightDirection());
}


void main () {
	vec3 b = texture(bmap, vUv).rgb * 2 - 1;
	vec3 m = texture(mmap, vUv).rgb * 2 - 1;
	vec3 n = normalize(vec3(b.x * b.z, b.y * b.z, b.z));
	
	vec3 hn = GlobalToNormalSpace(h());
	hn /= hn.z;
	vec2 hb = hn.xy + b.xy;
	hn = NormalToGlobalSpace(hn);

	vec3 sigma = getSigma(b, m);
	float det = sigma.x * sigma.y - sigma.z * sigma.z;
	
	float e = (hb.x*hb.x*sigma.y + hb.y*hb.y*sigma.x - 2.0*hb.x*hb.y*sigma.z);
	float spec = (det <= 0.0) ? 0.0 : exp(-0.5 * e / det) / sqrt(det);
	
	vec3 color = getDiffuse() + spec;
	
	color = vec3(1 - length(hb)) + vNormal;
	FragColor = vec4(color, 1.0);
}


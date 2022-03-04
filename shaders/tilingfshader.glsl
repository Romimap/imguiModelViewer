#version 460

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

uniform float TIME;
uniform float DTIME;

out vec4 FragColor;

const vec3 lightColor = vec3(1.3, 1.1, 0.9);
const vec3 ambientColor = vec3(0.1, 0.15, 0.25);

const float pi = 3.141592;

float getLod() {
	return textureQueryLod(albedo, vUv).x * 0.9;
}

vec3 printIDS(vec2 id, vec2 tcoord) {
	bool t2 = (tcoord.x + tcoord.y) > 1;
	vec3 color = vec3(id.x -  100.0, id.y, 1.0);
	if (t2) color.b = 0.8;
	
	return color;
}

void main () {

	vec2 uv = vUv;
	uv.x += 100;
	
	mat2 shear = mat2(
		vec2(1, 0),
		vec2(-0.5, 1)
	);
	
	mat2 scale = mat2(
		vec2(1, 0),
		vec2(0, 1 / sqrt(0.75))
	);
	
	uv = shear * scale * uv;
	
	vec2 id = vec2(int(uv.x * 10), int(uv.y * 10)) / 10;
	vec2 tcoord = (uv - id) * 10;
	
	vec3 color = vec3(0.8);
	if (tcoord.x + tcoord.y > 1) color = vec3(0.5);
	//color = vec3(uv.x - 100, uv.y, 1);
	color = vec3(tcoord, 1);
	
	FragColor = vec4(color, 1.0);
}

















































































































































































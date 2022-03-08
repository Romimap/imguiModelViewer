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

const mat2 shear = mat2(
		vec2(1, 0),
		vec2(-0.5, 1)
	);
	
const mat2 scale = mat2(
		vec2(1, 0),
		vec2(0, 1 / sqrt(0.75))
	);

float getLod() {
	return textureQueryLod(albedo, vUv).x * 0.9;
}

vec3 printIDS(vec2 id, vec2 tcoord) {
	bool t2 = (tcoord.x + tcoord.y) > 1;
	vec3 color = vec3(id.x -  100.0, id.y, 1.0);
	if (t2) color.b = 0.8;
	
	return color;
}

vec3 barycentricCoord(vec2 tcoord) {
	vec2 A = shear * scale * vec2(0, 0);
	vec2 B = shear * scale * vec2(0, 1);
	vec2 C = shear * scale * vec2(1, 0);
	
	vec2 P = tcoord;
	if (tcoord.x + tcoord.y > 1) {
		float d = 1 - P.x;
		float o = P.y - d;
		P.x -= o;
		P.y -= o;
	}
	P = shear * scale * P; //Define P into an orthonormed base

	float a = distance(A, P);
	float b = distance(B, P);
	float c = distance(C, P);

	float ab = distance(A, B);
	float ac = distance(A, C);
	float bc = distance(B, C);

	float x_apc = (c*c - a*a + ac*ac) / (2.0 * ac);
	float h_apc = sqrt(c*c - x_apc*x_apc);
	float a_apc = (ac * h_apc) / 2.0;

	float x_abp = (a*a - b*b + ab*ab) / (2.0 * ab);
	float h_abp = sqrt(a*a - x_abp*x_abp);
	float a_abp = (ab * h_abp) / 2.0;

	float x_pbc = (b*b - c*c + bc*bc) / (2.0 * bc);
	float h_pbc = sqrt(b*b - x_pbc*x_pbc);
	float a_pbc = (bc * h_pbc) / 2.0;

	float a_abc = 0.4330127;

	float alpha = a_apc / a_abc;
	float beta = a_abp / a_abc;
	float gamma = a_pbc / a_abc;

	

	return vec3(alpha, beta, gamma);
}

void main () {

	vec2 uv = vUv;
	uv.x += 100;
	
	
	
	uv = shear * scale * uv;
	
	vec2 id = vec2(int(uv.x * 10), int(uv.y * 10)) / 10;
	vec2 tcoord = (uv - id) * 10;
	
	vec3 color = vec3(0);
	//color = vec3(uv.x - 100, uv.y, 1);
	//color = texture(albedo,tcoord).rgb;
	//if (tcoord.x + tcoord.y < 1)
		color = barycentricCoord(tcoord);

	//color = vec3(tcoord, 0);
	
	FragColor = vec4(color.rgb, 1.0);
}



















































































































































































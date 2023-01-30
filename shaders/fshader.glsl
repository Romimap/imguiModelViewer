#version 400

in vec3 vPosition;
in vec2 vUv;
in vec3 vNormal;
in vec3 vTangent;
in vec3 vBitangent;

in vec4 gl_FragCoord;
out vec4 FragColor;

uniform sampler2D albedo;
uniform sampler2D priority;

uniform float TIME;
uniform float DTIME;

float rand(vec2 co){
	return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec2 hash (ivec2 seed) {
	return vec2(rand(seed.xy + ivec2(124, 325)), rand(seed.yx + ivec2(965, 254)));
}


void getSquareGrid (vec2 uv, out ivec2 vertex, out float w, bool offset = false) {
	uv *= 3.21;
	if (offset) uv += vec2(0.5) + vec2(12, 34); //Offsets the grid by 0.5, add a translation so we dont get the same synthesis
	uv += vec2(0.5);
	vertex = ivec2(uv);
	vec2 c = vec2(vertex) + vec2(0.5);
	vec2 d = uv - c;
	w = min(abs(d.x), abs(d.y));
	vertex = ivec2(uv - 0.5);
}

vec3 dualTilingAndBlending (vec2 uv, sampler2D tex, vec2 duvx, vec2 duvy) {
	ivec2 v1, v2;
	float w1, w2;

	vec3 E = textureLod(tex, vec2(0.5), 100).rgb;

	getSquareGrid(uv, v1, w1, false);
	vec3 G1 = textureGrad(tex, uv + hash(v1), duvx, duvy).rgb - E;

	getSquareGrid(uv, v2, w2, true);
	vec3 G2 = textureGrad(tex, uv + hash(v2), duvx, duvy).rgb - E;

	float w3 = (1.0 - w1) * (1.0 - w2);
	w3 = pow(w3, 25.0);
	float W1 = w1 / sqrt(w1*w1 + w2*w2 + w3*w3);
	float W2 = w2 / sqrt(w1*w1 + w2*w2 + w3*w3);
	
	return G1 * W1 + G2 * W2 + E;
}

vec3 dualMaxPriorityBlending (vec2 uv, sampler2D tex, sampler2D p, vec2 duvx, vec2 duvy) {
	ivec2 v1, v2;
	float w1, w2;

	getSquareGrid(uv, v1, w1, false);
	vec3 G1 = textureGrad(tex, uv + hash(v1), duvx, duvy).rgb;
	vec3 P1 = textureGrad(p  , uv + hash(v1), duvx, duvy).rgb;

	getSquareGrid(uv, v2, w2, true);
	vec3 G2 = textureGrad(tex, uv + hash(v2), duvx, duvy).rgb;
	vec3 P2 = textureGrad(p  , uv + hash(v2), duvx, duvy).rgb;

	if (P1.x + w1 > P2.x + w2) return G1;
	return G2;
}

void main () {
	vec2 uv = vec2(ivec2(gl_FragCoord.xy)) / vec2(textureSize(albedo, 0));
	uv = uv * 0.25;

	vec3 color = dualTilingAndBlending(uv, albedo, dFdx(uv), dFdy(uv));

	FragColor = vec4(color, 1.0);
}






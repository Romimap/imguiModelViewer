#version 400

in vec3 vPosition;
in vec2 vUv;
in vec3 vNormal;
in vec3 vTangent;
in vec3 vBitangent;

in vec4 gl_FragCoord;
out vec4 FragColor;

uniform sampler2D albedo;

uniform float TIME;
uniform float DTIME;

void main () {
	vec2 uv = vec2(ivec2(gl_FragCoord.xy)) / vec2(textureSize(albedo, 0));
	uv *= 1;
	vec3 color = textureLod(albedo, uv, 0).rgb;
	FragColor = vec4(color.rgb, 1.0);
}





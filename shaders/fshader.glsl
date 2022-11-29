#version 400

in vec3 vPosition;
in vec2 vUv;
in vec3 vNormal;
in vec3 vTangent;
in vec3 vBitangent;

uniform mat4 viewMatrix;

uniform sampler2D b;
uniform sampler2D m;
uniform sampler2D c;

uniform vec3 cameraPosition;

uniform float TIME;
uniform float DTIME;

out vec4 FragColor;

const float pi = 3.141592;


/////////// UTILS

float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

vec3 colorRamp (float t, float vmin=0, float vmax=1) {
	t = (t / (vmax - vmin)) - vmin;
	vec3 C[5] = vec3[5](
				vec3(0.0, 0.0, 0.2),
				vec3(0.2, 1.0, 0.1),
				vec3(1.0, 1.0, 0.2),
				vec3(1.0, 0.2, 0.1),
				vec3(1.0, 1.0, 1.0));
	
	float q[5] = float[5](
				0,
				0.125,
				0.25,
				0.5,
				1);
	
	int i;
	
	for (i = 1; i < 4; i++) {
		if (t < q[i]) break;
	}
	
	vec3 c1 = C[i - 1];
	vec3 c2 = C[i];
	float m = (t - q[i - 1])/(q[i] - q[i - 1]);
	
	return mix(c1, c2, m);
}

/////////// MAIN

void main () {
	vec2 uv = vUv;
	vec2 duvdx = dFdx(vUv);
	vec2 duvdy = dFdy(vUv);
	
	if (gl_FragCoord.x > 600) {



		//NAIVE FILTERING
		float mu = textureGrad(b, uv, duvdx, duvdy).r;
		vec2 rampfetch = vec2(mu + 0.5, 0.0);
		rampfetch = clamp(rampfetch, vec2(0.01), vec2(0.99));
		vec3 color = textureLod(c, rampfetch, 0).rgb;
		FragColor = vec4(color, 1.0);



	} else if (gl_FragCoord.x > 599) {
		FragColor = vec4(1.0);
	} else if (gl_FragCoord.x > 300) {



		//HNPN 13
		float mu = textureGrad(b, uv, duvdx, duvdy).r;
		float var =  textureGrad(m, uv, duvdx, duvdy).r - mu*mu;
		float y = var * 8;
		
		vec2 rampfetch = vec2(mu + 0.5, y);
		rampfetch = clamp(rampfetch, vec2(0.01), vec2(0.99));
		vec3 color = textureLod(c, rampfetch, 0).rgb;
		FragColor = vec4(color, 1.0);



	} else if (gl_FragCoord.x > 299) {
		FragColor = vec4(1.0);
	} else {



		//GROUND TRUTH
		int s = 16;
		vec3 color = vec3(0);
		for (int x = 0; x < s; x++) {
			for (int y = 0; y < s; y++) {
				vec2 pos = vec2(float(x) / float(s - 1) - 0.5, float(y) / float(s - 1) - 0.5);
				vec2 nuv = uv + pos.x * duvdx + pos.y * duvdy;
				float v = textureLod(b, nuv, 0).r;
				color += textureLod(c, vec2(v + 0.5, 0.01), 0).rgb;
			}
		}
		FragColor = vec4(color / float(s*s), 1.0);



	}
}






















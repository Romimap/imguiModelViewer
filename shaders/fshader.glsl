#version 330

in vec3 vPosition;
in vec2 vUv;
in vec3 vNormal;
in vec3 vTangent;
in vec3 vBitangent;

in vec4 gl_FragCoord;

uniform mat4 viewMatrix;
uniform sampler2D albedo;
uniform sampler2D normal;
uniform sampler2D bmap;
uniform sampler2D mmap;
uniform sampler2D roughness;
uniform sampler2D mipchart;
uniform sampler2D constantSigma;

uniform vec3 cameraPosition;

uniform float TIME;
uniform float DTIME;

out vec4 FragColor;

const vec3 lightColor = vec3(1.0, .92, .85) * 2;
const vec3 ambientColor = vec3(0.3, 0.25, 0.35) * 1;

const float pi = 3.141592;


const mat2 shear = mat2(
		vec2(1, 0),
		vec2(-0.5, 1)
	);
	
const mat2 scale = mat2(
		vec2(1, 0),
		vec2(0, 1 / sqrt(0.75))
	);


/////////// UTILS


//Returns a random vec2 in the range [0, 1]
vec2 rand2(vec2 seed) {
	return fract(vec2(sin((seed.x + 12.25546)* 12.98498), sin((seed.y - 15.54546) * 71.20153)) * 43.513453);
}

//Returns the view direction (surface to camera)
vec3 viewDirection () {
	return normalize(cameraPosition - vPosition);
}


//Returns the light direction (surface to light)
vec3 lightDirection () {
	return normalize(vec3(0, 5, 3));
}


//Returns h, the half vector between view/light direction
vec3 h() {
	return (viewDirection() + lightDirection()) / 2;
}


//Transforms a vector from World to Normal Space
// /!\ IMPORTANT NOTE : 
//     based on the implementation, the return value might need to be tweaked, especially the tangent & bitangent directions.
vec3 GlobalToNormalSpace(vec3 v) {
	mat3 tangentTransform = mat3(-cross(vTangent, vNormal), vNormal, -vTangent);
	return v * tangentTransform;
}


//Transforms a vector from Normal to World Space
// /!\ IMPORTANT NOTE : 
//     based on the implementation, the return value might need to be tweaked, especially the tangent & bitangent directions.
vec3 NormalToGlobalSpace(vec3 v) {
	mat3 tangentTransform = mat3(-cross(vTangent, vNormal), vNormal, -vTangent);
	return v * inverse(tangentTransform);
}


vec3 getMicroNormal (sampler2D bm, int lod, float intensity = 1) {
	vec3 b;
	if (lod == -1) {
		b = texture(bm, vUv).rgb;
	} else {
		b = textureLod(bm, vUv, lod).rgb;
	}
	return normalize(vec3(b.xy / (b.z / intensity), (b.z / intensity)).xzy);
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


void triangleGrid(vec2 uv,
	out float w1, out float w2, out float w3,
	out vec2 vertex1, out vec2 vertex2, out vec2 vertex3)
{

	// Skew input space into simplex triangle grid
    mat2 T = scale * shear;
	vec2 skewedCoord = T * uv;

	// Compute local triangle vertex IDs and local barycentric coordinates
	vec2 baseId = floor(skewedCoord);
	vec3 temp = vec3(fract(skewedCoord), 0);
	temp.z = 1.0 - temp.x - temp.y;
	if (temp.z > 0.)
	{
		w1 = temp.z;
		w2 = temp.y;
		w3 = temp.x;
		vertex1 = baseId;
		vertex2 = baseId + vec2(0, 1.);
		vertex3 = baseId + vec2(1., 0);
	}
	else
	{
		w1 = -temp.z;
		w2 = 1.0 - temp.y;
		w3 = 1.0 - temp.x;
		vertex1 = baseId + vec2(1., 1.);
		vertex2 = baseId + vec2(1., 0);
		vertex3 = baseId + vec2(0, 1.);
	}
    
    mat2 scale_inv = inverse(scale);
    mat2 shear_inv = inverse(shear);
    mat2 T_inv = shear_inv * scale_inv;
           
    vertex1 = T_inv*vec2(vertex1);
    vertex2 = T_inv*vec2(vertex2);
    vertex3 = T_inv*vec2(vertex3);
    
}


/////////// LEAN MAPPING



//Returns a specular intensity based on the B map, the M map and a lod.
// /!\ IMPORTANT NOTE : 
//     we assume that <bm> and <mm> are filled with floats in the [-1; 1] range.
float getSpecularIntensity (sampler2D bm, sampler2D mm, int lod) {
	if (dot(h(), vNormal) < 0) return 0.0; //Prevents specular if h is facing inside

	vec3 b;
	vec3 m;
	if (lod == -1) {
		b = texture(bm, vUv).rgb;
		m = texture(mm, vUv).rgb;
	} else {
		b = textureLod(bm, vUv, lod).rgb;
		m = textureLod(mm, vUv, lod).rgb;
	}
	

	vec3 hn = normalize(GlobalToNormalSpace(h())); //h in a space where hn.y is aligned with the mesh normal
	hn /= hn.y;
	vec2 hb = hn.xz - b.xy;
	
	vec3 sigma = m - vec3(b.x * b.x, b.y * b.y, b.x * b.y);
	float det = sigma.x * sigma.y - sigma.z * sigma.z;
	
	float e = (hb.x*hb.x*sigma.y + hb.y*hb.y*sigma.x - 2.0*hb.x*hb.y*sigma.z);
	float spec = (det <= 0.0) ? 0.0 : exp(-0.5 * e / det) / sqrt(det);
	
	return spec;
}




//Returns a specular intensity based on a covariance matrix
float getSpecularIntensity (float meanx, float meany, float varx, float vary, float covxy) {
	if (dot(h(), vNormal) < 0) return 0.0; //Prevents specular if h is facing inside

	vec3 hn = normalize(GlobalToNormalSpace(h())); //h in a space where hn.y is aligned with the mesh normal
	hn /= hn.y;
	vec2 hb = hn.xz - vec2(meanx, meany);
	
	vec3 sigma = vec3(varx, vary, covxy);
	float det = sigma.x * sigma.y - sigma.z * sigma.z;
	
	float e = (hb.x*hb.x*sigma.y + hb.y*hb.y*sigma.x - 2.0*hb.x*hb.y*sigma.z);
	float spec = (det <= 0.0) ? 0.0 : exp(-0.5 * e / det) / sqrt(det);
	
	return spec;
}


/////////// EYE CANDY



//Returns a specular color.
vec3 getSpecular (int lod, float intensity, bool constSigm) {
	vec3 b;
	vec3 m;
	vec3 s;
	
	if (lod == -1) {
		b = texture(bmap, vUv).rgb;
		m = texture(mmap, vUv).rgb;
		s = texture(constantSigma, vUv).rgb;
	} else {
		b = textureLod(bmap, vUv, lod).rgb;
		m = textureLod(mmap, vUv, lod).rgb;
		s = textureLod(constantSigma, vUv, lod).rgb;
	}
	float meanx = b.x;
	float meany = b.y;
	float varx = m.x - pow(b.x, 2);
	float vary = m.y - pow(b.y, 2);
	float covxy = m.z - b.x * b.y;
	
	if (constSigm) {
		varx = s.x;
		vary = s.y;
		covxy = s.z;
	}
	
	//covxy = s.z;
	
	//covxy = 0; //If covariance is a pain to compute, you might want to set it to 0. Thats a strong simplification but can work.
	float float_Specular = getSpecularIntensity(meanx, meany, varx, vary, covxy);
	

	return max(float_Specular * intensity, 0.0) * lightColor;
}


//Returns the diffuse color.
vec3 getDiffuse (float bias, int lod) {
	vec3 color;
	if (lod == -1) {
		color = texture(albedo, vUv).rgb;
	} else {
		color = textureLod(albedo, vUv, lod).rgb;
	}
	
	//Reduce normal map force
	vec3 micronormal = getMicroNormal(bmap, lod);
	vec3 n = NormalToGlobalSpace(micronormal);
	return (max(lightColor * (dot(n, lightDirection()) * (1.0 - bias) + bias), 0.0) + ambientColor) * color;
}


vec3 colorManagement (vec3 color, float exposure) {
	return tanh(color * exposure);
}



/////////// MAIN

float Max(float a, float b) {
	if (a > b) return a;
	return b;
}

void main () {
	int lod = int(mod(TIME, 6));
	lod = -1; //Auto lod
	
	////////// SPECULAR USING A B & M MAP
	float float_SpecularBM = getSpecularIntensity(bmap, mmap, lod);
	
	
	////////// SPECULAR USING THE MEAN, VARIANCE & COVARIANCE
	vec3 b;
	vec3 m;
	vec3 s;
	if (lod == -1) {
		b = texture(bmap, vUv).rgb;
		m = texture(mmap, vUv).rgb;
		s = texture(constantSigma, vUv).rgb;
	} else {
		b = textureLod(bmap, vUv, lod).rgb;
		m = textureLod(mmap, vUv, lod).rgb;
		s = textureLod(constantSigma, vUv, lod).rgb;
	}
	float meanx = b.x;
	float meany = b.y;
	float varx = m.x - pow(b.x, 2);
	float vary = m.y - pow(b.y, 2);
	float covxy = m.z - b.x * b.y;
	//covxy = 0; //If covariance is a pain to compute, you might want to set it to 0. Thats a strong simplification but can work.
	float float_SpecularCovariance = getSpecularIntensity(meanx, meany, varx, vary, covxy);
	
	
	////////// EYE CANDY
	vec3 color_EyeCandy = getSpecular(lod, 0.1, false); //+ getDiffuse(0.05, lod);
	
	////////// COLOR MANAGEMENT
	vec3 color = color_EyeCandy;//Set that variable to color_EyeCandy, color_SpecularCovariance or color_SpecularBM.
	float exposure = 1;
	color = colorManagement(color, exposure);
	
	
	///////// COLOR RAMP
	{
		if (gl_FragCoord.x < 500) {
			varx = s.x;
			vary = s.y;
			covxy = s.z;
		}
		covxy = s.z;
		float t;
		t = getSpecularIntensity(meanx, meany, varx, vary, covxy);
		color = colorRamp(t, 0, 100);
	}
	

	////////// MIP CHART
	//if (gl_FragCoord.x > 498 && gl_FragCoord.x < 502)
	float mult = 250;
	float dUdx = dFdx(vUv.x) * mult;
	float dUdy = dFdy(vUv.x) * mult;
	float dVdx = dFdx(vUv.y) * mult;
	float dVdy = dFdy(vUv.y) * mult;
	float flod = (0.5 * log2(max(pow(dUdx, 2) + pow(dVdx, 2) , pow(dUdy, 2) + pow(dVdy,2))));

	flod = 
	(
		0.5 * log2
		(
			max
			(
				max(pow(dUdx, 2),pow(dUdy, 2)),
				max(pow(dVdx, 2),pow(dVdy, 2))
			)
		)
	);
	flod = 
	(
		0.5 * log2
		(
			max
			(
				pow(dUdx, 2) + pow(dVdy, 2),
				pow(dVdx, 2) + pow(dUdy, 2)
			)
		)
	);
	color = textureLod(mipchart, vUv, flod).rgb;
	if (mod(TIME, 1) < 0.5)
	color = texture(mipchart, vUv).rgb;
	//color = vec3(texture(mmap, vUv).r - pow(texture(bmap, vUv).r, 2)) * 0.5;
	//color = texture(constantSigma, vUv).rgb;

	////////// FRAGMENT COLOR
	FragColor = vec4(color, 1.0);
}






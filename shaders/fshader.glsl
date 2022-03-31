#version 400

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
uniform sampler2D var;

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
	return normalize(vec3(0, 1, 5));
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



/////////// TILING & BLENDING



vec2 hash(vec2 p)
{
	return fract(sin((p) * mat2(127.1, 311.7, 269.5, 183.3) )*43758.5453);
}


// Compute local triangle barycentric coordinates and vertex IDs
void TriangleGrid(vec2 uv, out float w1, out float w2, out float w3, out ivec2 vertex1, out ivec2 vertex2, out ivec2 vertex3) {
	// Scaling of the input
	uv *= 3.464; // 2 * sqrt(3)

	// Skew input space into simplex triangle grid
	const mat2 gridToSkewedGrid = mat2(1.0, 0.0, -0.57735027, 1.15470054);
	vec2 skewedCoord = gridToSkewedGrid * uv;

	// Compute local triangle vertex IDs and local barycentric coordinates
	ivec2 baseId = ivec2(floor(skewedCoord));
	vec3 temp = vec3(fract(skewedCoord), 0);
	temp.z = 1.0 - temp.x - temp.y;
	if (temp.z > 0.0)
	{
		w1 = temp.z;
		w2 = temp.y;
		w3 = temp.x;
		vertex1 = baseId;
		vertex2 = baseId + ivec2(0, 1);
		vertex3 = baseId + ivec2(1, 0);
	}
	else
	{
		w1 = -temp.z;
		w2 = 1.0 - temp.y;
		w3 = 1.0 - temp.x;
		vertex1 = baseId + ivec2(1, 1);
		vertex2 = baseId + ivec2(1, 0);
		vertex3 = baseId + ivec2(0, 1);
	}
}


// By-Example procedural noise at uv
vec3 TilingAndBlendingMean(vec2 uv)
{
	// Get triangle info
	float w1, w2, w3;
	ivec2 vertex1, vertex2, vertex3;
	TriangleGrid(uv, w1, w2, w3, vertex1, vertex2, vertex3);

	float l = 5;

	float wp1 = pow(w1,l) / (pow(w1,l) + pow(w2,l) + pow(w3,l));
	float wp2 = pow(w2,l) / (pow(w1,l) + pow(w2,l) + pow(w3,l));
	float wp3 = pow(w3,l) / (pow(w1,l) + pow(w2,l) + pow(w3,l));
	
	// Assign random offset to each triangle vertex
	vec2 uv1 = uv + hash(vertex1);
	vec2 uv2 = uv + hash(vertex2);
	vec2 uv3 = uv + hash(vertex3);

	// Precompute UV derivatives 
	vec2 duvdx = dFdx(uv);
	vec2 duvdy = dFdy(uv);

	// Fetch Gaussian input
	vec3 G1 = textureGrad(bmap, uv1, duvdx, duvdy).rgb;
	vec3 G2 = textureGrad(bmap, uv2, duvdx, duvdy).rgb;
	vec3 G3 = textureGrad(bmap, uv3, duvdx, duvdy).rgb;

	// Variance-preserving blending
	vec3 G = wp1*G1 + wp2*G2 + wp3*G3;
	return G;
}



// By-Example procedural noise at uv
vec3 TilingAndBlendingVariance(vec2 uv)
{
	// Get triangle info
	float w1, w2, w3;
	ivec2 vertex1, vertex2, vertex3;
	TriangleGrid(uv, w1, w2, w3, vertex1, vertex2, vertex3);

	float l = 1;

	float wp1 = pow(w1,l) / (pow(w1,l) + pow(w2,l) + pow(w3,l));
	float wp2 = pow(w2,l) / (pow(w1,l) + pow(w2,l) + pow(w3,l));
	float wp3 = pow(w3,l) / (pow(w1,l) + pow(w2,l) + pow(w3,l));
	
	// Assign random offset to each triangle vertex
	vec2 uv1 = uv + hash(vertex1);
	vec2 uv2 = uv + hash(vertex2);
	vec2 uv3 = uv + hash(vertex3);

	// Precompute UV derivatives 
	vec2 duvdx = dFdx(uv);
	vec2 duvdy = dFdy(uv);

	// Fetch Gaussian input
	vec3 G1 = textureGrad(var, uv1, duvdx, duvdy).rgb;
	vec3 G2 = textureGrad(var, uv2, duvdx, duvdy).rgb;
	vec3 G3 = textureGrad(var, uv3, duvdx, duvdy).rgb;

	// non Variance-preserving blending
	vec3 G = wp1*G1 + wp2*G2 + wp3*G3;
	return G;
}

// By-Example procedural noise at uv
vec3 TilingAndBlendingVarianceSq(vec2 uv)
{
	// Get triangle info
	float w1, w2, w3;
	ivec2 vertex1, vertex2, vertex3;
	TriangleGrid(uv, w1, w2, w3, vertex1, vertex2, vertex3);

	float l = 1;

	float wp1 = pow(w1,l) / (pow(w1,l) + pow(w2,l) + pow(w3,l));
	float wp2 = pow(w2,l) / (pow(w1,l) + pow(w2,l) + pow(w3,l));
	float wp3 = pow(w3,l) / (pow(w1,l) + pow(w2,l) + pow(w3,l));
	
	// Assign random offset to each triangle vertex
	vec2 uv1 = uv + hash(vertex1);
	vec2 uv2 = uv + hash(vertex2);
	vec2 uv3 = uv + hash(vertex3);

	// Precompute UV derivatives 
	vec2 duvdx = dFdx(uv);
	vec2 duvdy = dFdy(uv);

	// Fetch Gaussian input
	vec3 G1 = textureGrad(var, uv1, duvdx, duvdy).rgb;
	vec3 G2 = textureGrad(var, uv2, duvdx, duvdy).rgb;
	vec3 G3 = textureGrad(var, uv3, duvdx, duvdy).rgb;

	// non Variance-preserving blending
	vec3 G = pow(wp1, 2)*G1 + pow(wp2, 2)*G2 + pow(wp3, 2)*G3;
	return G;
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
	vec3 color_EyeCandy = getSpecular(lod, 0.01, false) + getDiffuse(0.05, lod);
	
	////////// COLOR MANAGEMENT
	vec3 color = color_EyeCandy;//Set that variable to color_EyeCandy, color_SpecularCovariance or color_SpecularBM.
	float exposure = 0.5;
	color = colorManagement(color, exposure);
	
	
	///////// COLOR RAMP
	//{
	//	float t;
	//	t = getSpecularIntensity(meanx, meany, varx, vary, covxy);
	//	color = colorRamp(t, 0, 100);
	//}
	

	////////// MIP CHART
	//if (gl_FragCoord.x > 498 && gl_FragCoord.x < 502)
	//color = texture(mipchart, vUv).rgb;
	
	
	////////// FRAGMENT COLOR
	if (gl_FragCoord.x < 500) {
		meanx = TilingAndBlendingMean(vUv).x;
		meany = TilingAndBlendingMean(vUv).y;
		varx = TilingAndBlendingVarianceSq(vUv).x;
		vary = TilingAndBlendingVarianceSq(vUv).y;
	}
	covxy = 0;
	
	color = colorRamp(getSpecularIntensity(meanx, meany, varx, vary, covxy), 0, 50);
	
	FragColor = vec4(color, 1.0);
}

#version 400

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
uniform sampler2D mipchart;
uniform sampler2D constantSigma;
uniform sampler2D var;
uniform samplerCube skybox;

uniform vec3 cameraPosition;

uniform float TIME;
uniform float DTIME;

const float s = 10; //25

out vec4 FragColor;

const float pi = 3.141592;


const mat2 shear = mat2(
		vec2(1, 0),
		vec2(-0.5, 1)
	);
	
const mat2 scale = mat2(
		vec2(1, 0),
		vec2(0, 1 / sqrt(0.75))
	);
	
#define MEAN 0
#define VAR 1
#define COV 2


/////////// UTILS

float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}


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
	return normalize(vec3(1,.5, 0)); //0 0.1 1
}


//Returns h, the half vector between view/light direction
vec3 h() {
	return (viewDirection() + lightDirection()) / 2;
}


mat3 tangentTransformMatrix () {
 return mat3(vTangent, vNormal, cross(vTangent, vNormal));
}

//Transforms a vector from World to Normal Space
// /!\ IMPORTANT NOTE : 
//     based on the implementation, the return value might need to be tweaked, especially the tangent & bitangent directions.
vec3 globalToNormalSpace(vec3 v) {
	return tangentTransformMatrix() * v;
}


//Transforms a vector from Normal to World Space
// /!\ IMPORTANT NOTE : 
//     based on the implementation, the return value might need to be tweaked, especially the tangent & bitangent directions.
vec3 normalToGlobalSpace(vec3 v) {
	return inverse(tangentTransformMatrix()) * v;
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


/////////// SHADING



//Returns a specular intensity based on a covariance matrix
float getSpecular (float meanx, float meany, float varx, float vary, float covxy, float meanroughness) {
	if (dot(h(), vNormal) < 0) return 0.0; //Prevents specular if h is facing inside

	vec3 hn = normalize(globalToNormalSpace(h())); //h in a space where hn.y is aligned with the mesh normal
	hn /= hn.y;
	vec2 hb = hn.xz - vec2(meanx, meany);
	
	vec3 sigma = vec3(varx + (1.0 / meanroughness), vary + (1.0 / meanroughness), covxy);
	float det = sigma.x * sigma.y - sigma.z * sigma.z;
	
	float e = (hb.x*hb.x*sigma.y + hb.y*hb.y*sigma.x - 2.0*hb.x*hb.y*sigma.z);
	float spec = (det <= 0.0) ? 0.0 : exp(-0.5 * e / det) / sqrt(det);
	
	return spec;
}

float getDiffuse(float meanx, float meany) {
	vec3 nrm = normalize(vec3(meanx, 1, meany));
	vec3 snrm = normalize(normalToGlobalSpace(nrm)); //h in a space where hn.y is aligned with the mesh normal
	
	return max(0, dot(lightDirection(), snrm));
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
vec4 TilingAndBlending(sampler2D tex, vec2 uv, float lod, int fetch)
{
	// Get triangle info
	float w1, w2, w3;
	ivec2 vertex1, vertex2, vertex3;
	TriangleGrid(uv, w1, w2, w3, vertex1, vertex2, vertex3);

	float l = 1;

	// Assign random offset to each triangle vertex
	vec2 uv1 = uv + hash(vertex1);// + vec2(float(vertex1.y * 0.05) * TIME * .3, 0);
	vec2 uv2 = uv + hash(vertex2);// + vec2(float(vertex2.y * 0.05) * TIME * .3, 0);
	vec2 uv3 = uv + hash(vertex3);// + vec2(float(vertex3.y * 0.05) * TIME * .3, 0);

	// Fetch centered gaussian input
	vec4 G1 = textureLod(tex, uv1, lod) - textureLod(tex, vec2(0), 100);
	vec4 G2 = textureLod(tex, uv2, lod) - textureLod(tex, vec2(0), 100);
	vec4 G3 = textureLod(tex, uv3, lod) - textureLod(tex, vec2(0), 100);
	
	
	float wp1 = w1 / length(vec3(w1, w2, w3));
	float wp2 = w2 / length(vec3(w1, w2, w3));
	float wp3 = w3 / length(vec3(w1, w2, w3));
	
	// Variance-preserving / non preserving blending
	vec4 G = vec4(0);
	if (fetch == MEAN) {
		G += wp1*G1 + wp2*G2 + wp3*G3;
	} else {
		G += w1*G1 + w2*G2 + w3*G3;
	}
	
	G += textureLod(tex, vec2(0), 100);
	return G;
}

vec4 TilingAndBlendingAniso(sampler2D tex, sampler2D var, vec2 uv, int maxAniso, int fetch) {
	vec2 texSize = textureSize(tex, 0);
	float bias = 1; //Should be set to 1 ! (Used so we consider bigger footprints to filter a bit more)
	vec2 duvdx = dFdx(vUv) * bias;
	vec2 duvdy = dFdy(vUv) * bias;
	
	float px = length(duvdx) * texSize.x;
	float py = length(duvdy) * texSize.y;
	
	float pmax = max(px, py);
	float pmin = min(px, py);
	
	float P = min(ceil(pmax/pmin), maxAniso);
	
	float sbias = 1; //Should be set to 1 ! (Used so we consider bigger samples to filter a bit more)
	float lod = log2(pmax/P * sbias);
	
	P = min(P,ceil(pmax));
	
	vec2 duvdt = (px > py) ? duvdx : duvdy;
	
	vec4  N1 = vec4(0);
	vec2  N2 = vec2(0);
	float NN = 0;
	
	for (int i = 1; i <= P; i += 1) {
		float u = uv.x;
		float v = uv.y;
		float dudt = duvdt.x;
		float dvdt = duvdt.y;
		
		vec2 curr_uv = vec2(
			u + dudt * (float(i)/(P + 1) - 0.5), 
			v + dvdt * (float(i)/(P + 1) - 0.5)
		);
		
		vec4 n1 = TilingAndBlending(tex, curr_uv, lod, MEAN);
		vec3 sigma2 = TilingAndBlending(var, curr_uv, lod, VAR).xyz;
		N1 += n1;
		N2 += pow(n1.xy, vec2(2)) + sigma2.xy;
		NN += n1.x * n1.y + sigma2.z;


	}
	
	N1 /= P;
	N2 /= P;
	NN /= P;
	
	if (fetch == MEAN) return N1;
	if (fetch == VAR) return vec4(N2 - pow(N1.xy, vec2(2)), 0.0, 0.0);
	if (fetch == COV) return vec4(NN - N1.x * N1.y, 0.0, 0.0, 0.0);
}


vec4 textureAniso(sampler2D tex, vec2 uv, int maxAniso) {
	//Unfiltered
	if (maxAniso == -1) {
		return textureLod(tex, uv, 0);
	}

	vec2 texSize = textureSize(tex, 0);
	float bias = 1; //Should be set to 1 ! (Used so we consider bigger footprints to filter a bit more)
	vec2 duvdx = dFdx(vUv) * bias;
	vec2 duvdy = dFdy(vUv) * bias;
	
	float px = length(duvdx) * texSize.x;
	float py = length(duvdy) * texSize.y;
	
	float pmax = max(px, py);
	float pmin = min(px, py);
	
	float N = min(ceil(pmax/pmin), maxAniso);
	
	float sbias = 1; //Should be set to 1 ! (Used so we consider bigger samples to filter a bit more)
	float lod = log2(pmax/N * sbias);
	
	N = min(N,ceil(pmax));
	
	vec2 duvdt = (px > py) ? duvdx : duvdy;
	
	vec4 taniso = vec4(0);
	
	for (int i = 1; i <= N; i += 1) {
		float u = uv.x;
		float v = uv.y;
		float dudt = duvdt.x;
		float dvdt = duvdt.y;
		
		vec2 curr_uv = vec2(
			u + dudt * (float(i)/(N + 1) - 0.5), 
			v + dvdt * (float(i)/(N + 1) - 0.5)
		);
		
		taniso += textureLod(tex, curr_uv, lod);
	}
	
	taniso /= N;
	
	return taniso;
}




/////////// EYE CANDY


vec3 colorManagement (vec3 color, float exposure, float contrast) {
	return pow(tanh(color * exposure) + 0.5, vec3(contrast)) - 0.5;
}


/////////// MAIN





vec3 render_tilingblending (int maxAniso) {
	vec2 uv = vUv;
	vec3 lightColor = vec3(1.0, .8, .7) * 3;
	vec3 ambientColor = vec3(0.15, 0.25, 0.35) * 2;
	float r = 100;

	//FETCH
	vec2 b = TilingAndBlendingAniso(bmap, bmap, uv, maxAniso, MEAN).xy;
	vec2 v = TilingAndBlendingAniso(bmap, var, uv, maxAniso, VAR).xy;
	float c = TilingAndBlendingAniso(bmap, var, uv, maxAniso, COV).x;
	vec3 a = TilingAndBlendingAniso(albedo, albedo, uv, maxAniso, MEAN).rgb;

	//DATA
	float meanx = b.x;
	float meany = b.y;
	float varx = v.x;
	float vary = v.y;
	float covxy = c;
	float meanr = r;
	vec3 meana = a;


	vec3 diffuse = getDiffuse(meanx, meany) * lightColor + ambientColor;
	vec3 specular = getSpecular(meanx, meany, varx, vary, covxy, meanr) * lightColor;

	vec3 color = (meana * diffuse * 0.9);// + (specular * 0.05);

	color = colorManagement(color, 0.5, 1);
	return color;
}





vec3 render_texture (int maxAniso) {
	vec2 uv = vUv;
	vec3 lightColor = vec3(1.0, .8, .7) * 3;
	vec3 ambientColor = vec3(0.15, 0.25, 0.35) * 2;
	float r = 100;
	
	//FETCH
	vec2 b = textureAniso(bmap, uv, maxAniso).xy;
	vec3 m = textureAniso(mmap, uv, maxAniso).xyz;
	vec3 a = textureAniso(albedo, uv, maxAniso).rgb;

	//DATA
	float meanx = b.x;
	float meany = b.y;
	float varx = m.x - b.x * b.x;
	float vary = m.y - b.y * b.y;
	float covxy = m.z - b.x * b.y;
	float meanr = r;
	vec3 meana = a;


	vec3 diffuse = getDiffuse(meanx, meany) * lightColor + ambientColor;
	vec3 specular = getSpecular(meanx, meany, varx, vary, covxy, meanr) * lightColor;

	vec3 color = (meana * diffuse * 0.9);// + (specular * 0.05);

	color = colorManagement(color, 0.5, 1);
	return color;
}


vec3 groundTruth () {

return vec3(0);

}



void main () {
	FragColor = vec4(render_tilingblending(1), 1.0);
	//FragColor = vec4(textureAniso(albedo, vUv, 16).rgb, 1.0);
}



















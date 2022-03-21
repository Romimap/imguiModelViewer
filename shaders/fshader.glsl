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

const vec3 lightColor = vec3(2, 1.2, 0.8);
const vec3 ambientColor = vec3(0.1, 0.15, 0.25);

const float pi = 3.141592;



/////////// UTILS



//Returns the view direction (surface to camera)
vec3 viewDirection () {
	return normalize(cameraPosition - vPosition);
}


//Returns the light direction (surface to light)
vec3 lightDirection () {
	return normalize(vec3(2, 5, 0));
}


//Returns h, the half vector between view/light direction
vec3 h() {
	return (viewDirection() + lightDirection()) / 2;
}


//Transforms a vector from World to Normal Space
// /!\ IMPORTANT NOTE : 
//     based on the implementation, the return value might need to be tweaked, especially the tangent & bitangent directions.
vec3 GlobalToNormalSpace(vec3 v) {
	return vec3(-dot(v, vTangent), dot(v, vNormal), dot(v, vBitangent));
}


//Transforms a vector from Normal to World Space
// /!\ IMPORTANT NOTE : 
//     based on the implementation, the return value might need to be tweaked, especially the tangent & bitangent directions.
vec3 NormalToGlobalSpace(vec3 v) {
	return v.x * -vTangent + v.y * vNormal + v.z * -vBitangent;
}


//Returns the Sigma matrix from the paper. a = d so we can store it in a vec3.
vec3 getSigma (vec3 b, vec3 m) {
	return vec3(m.x - b.x * b.x, m.y - b.y * b.y, m.z - b.x * b.y);
}

vec3 getMicroNormal (sampler2D bm, int lod) {
	vec3 b;
	if (lod == -1) {
		b = texture(bm, vUv).rgb;
	} else {
		b = textureLod(bm, vUv, lod).rgb;
	}
	return normalize(vec3(b.xy / b.z, b.z).xzy);
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
	

	vec3 hn = GlobalToNormalSpace(h()); //Put H in normal space
	hn /= hn.y;
	vec2 hb = hn.xz + b.xy; //Get H bar, h projected into a plane defined by the mesh normal

	vec3 sigma = getSigma(b, m);
	float det = sigma.x * sigma.y - sigma.z * sigma.z;
	
	float e = (hb.x*hb.x*sigma.y + hb.y*hb.y*sigma.x - 2.0*hb.x*hb.y*sigma.z);
	float spec = (det <= 0.0) ? 0.0 : exp(-0.5 * e / det) / sqrt(det);
	
	return spec;
}



/////////// EYE CANDY



//Returns a specular color.
vec3 getSpecular (sampler2D bm, sampler2D mm, int lod, float intensity) {
	return max(getSpecularIntensity(bm, mm, lod) * intensity, 0.0) * lightColor;
}


//Returns the diffuse color.
vec3 getDiffuse (sampler2D bm, sampler2D albedo, int lod) {
	vec3 color;
	if (lod == -1) {
		color = texture(albedo, vUv).rgb;
	} else {
		color = textureLod(albedo, vUv, lod).rgb;
	}
	//return dot(getMicroNormal(bm, lod), lightDirection()) * lightColor;
	return (max(lightColor * dot(getMicroNormal(bm, lod), lightDirection()), 0.0) + ambientColor) * color;
}


vec3 colorManagement (vec3 color, float exposure) {
	return tanh(color * exposure);
}



/////////// MAIN



void main () {
	int lod = int(mod(TIME, 6));
	lod = -1; //AUTO LOD

	vec3 color;
	color = getDiffuse(bmap, albedo, lod) + getSpecular(bmap, mmap, lod, 0.5);
	//color = getSpecular(bmap, mmap, lod, 1);
	float exposure = 0.5;
	color = colorManagement(color, exposure);
	
	FragColor = vec4(color, 1.0);
}











}











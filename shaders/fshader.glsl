#version 330

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

const vec3 lightColor = vec3(.95, .92, .85) * 1;
const vec3 ambientColor = vec3(0.3, 0.25, 0.35) * 1;

const float pi = 3.141592;



/////////// UTILS



//Returns the view direction (surface to camera)
vec3 viewDirection () {
	return normalize(cameraPosition - vPosition);
}


//Returns the light direction (surface to light)
vec3 lightDirection () {
	return normalize(vec3(2, 2, 2));
}


//Returns h, the half vector between view/light direction
vec3 h() {
	return (viewDirection() + lightDirection()) / 2;
}


//Transforms a vector from World to Normal Space
// /!\ IMPORTANT NOTE : 
//     based on the implementation, the return value might need to be tweaked, especially the tangent & bitangent directions.
vec3 GlobalToNormalSpace(vec3 v) {
	mat3 tangentTransform = mat3(vTangent, vNormal, cross(vTangent, vNormal));
	return v * tangentTransform;
	return vec3(dot(v, vTangent), dot(v, vNormal), dot(v, -vBitangent));
}


//Transforms a vector from Normal to World Space
// /!\ IMPORTANT NOTE : 
//     based on the implementation, the return value might need to be tweaked, especially the tangent & bitangent directions.
vec3 NormalToGlobalSpace(vec3 v) {
	return v.x * -vTangent + v.y * vNormal + v.z * -vBitangent;
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
	
	//Reduce normal map force
	vec3 micronormal = getMicroNormal(bm, lod);
	return (max(lightColor * (dot(micronormal, lightDirection()) * 0.5 + 0.5), 0.0) + ambientColor) * color;
}


vec3 colorManagement (vec3 color, float exposure) {
	return tanh(color * exposure);
}



/////////// MAIN



void main () {
	int lod = int(mod(TIME, 6));
	lod = -1; //Auto lod
	
	
	
	////////// SPECULAR USING A B & M MAP
	vec3 color_SpecularBM = vec3(getSpecularIntensity(bmap, mmap, lod));
	
	
	
	////////// SPECULAR USING THE MEAN, VARIANCE & COVARIANCE
	vec3 b;
	vec3 m;
	if (lod == -1) {
		b = texture(bmap, vUv).rgb;
		m = texture(mmap, vUv).rgb;
	} else {
		b = textureLod(bmap, vUv, lod).rgb;
		m = textureLod(mmap, vUv, lod).rgb;
	}
	float meanx = b.x;
	float meany = b.y;
	float varx = m.x - pow(b.x, 2);
	float vary = m.y - pow(b.y, 2);
	float covxy = m.z - b.x * b.y;
	//covxy = 0; //If covariance is a pain to compute, you might want to set it to 0. Thats a strong simplification but can work.
	vec3 color_SpecularCovariance = vec3(getSpecularIntensity(meanx, meany, varx, vary, covxy));
	
	
	
	////////// EYE CANDY
	vec3 color_EyeCandy = getSpecular(bmap, mmap, lod, 0.2) + getDiffuse(bmap, albedo, lod);
	
	
	
	////////// COLOR MANAGEMENT
	vec3 color = color_SpecularCovariance;//Set that variable to color_EyeCandy, color_SpecularCovariance or color_SpecularBM.
	float exposure = 1;
	color = colorManagement(color, exposure);
	
	
	
	////////// FRAGMENT COLOR
	FragColor = vec4(color, 1.0);
}



ure(normal, vUv).xyz;
	////////// FRAGMENT COLOR
	FragColor = vec4(color, 1.0);
}




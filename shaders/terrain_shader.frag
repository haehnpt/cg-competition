#version 330 core

in vec4 interp_color;
in vec3 interp_normal;
in vec3 interp_light_dir;
in vec2 uv;
in vec2 tex_height;


out vec4 frag_color;

uniform sampler2D stone_tex;
uniform sampler2D grass_tex;
uniform sampler2D snow_tex;

uniform float albedo; // albedo
uniform float roughness; // sigma
uniform float refractionIndex;

const float pi = 3.14159265359;

// Syntatic sugar. Make sure dot products only map to hemisphere
float cdot(vec3 a, vec3 b) {
    return clamp(dot(a,b), 0.0, 1.0);
}

// D
float beckmannDistribution(float dotNH) {
    float sigma2 = roughness * roughness;
    // TASK: Compute d-term
    float alpha = acos(dotNH);
	return exp(-pow(tan(alpha),2) / sigma2) / (pi * sigma2 * pow(cos(alpha), 4));
}

// F
float schlickApprox(float dotVH, float n1, float n2) {
    // TASK: Compute f-term
    float R0 = pow((n1-n2)/(n1+n2),2);
	return R0 + (1 - R0) * pow(1 - dotVH, 5);
}

// G
float geometricAttenuation(float dotNH, float dotVN, float dotVH, float dotNL) {
    // TASK: Compute g-term
    float Gm = 2 * dotNH * dotVN / dotVH;
	float Gs = 2 * dotNH * dotNL / dotVH;
	return min(Gm,min(Gs,1));
}

float cooktorranceTerm(vec3 n, vec3 l) {
    vec3 v = vec3(0.0, 0.0, 1.0); // in eye space direction towards viewer simply is the Z axis
    vec3 h = normalize(l + v); // half-vector between V and L

    // precompute to avoid redundant computation
    float dotVN = cdot(v, n);
    float dotNL = cdot(n, l);
    float dotNH = cdot(n, h);
    float dotVH = cdot(v, h);

    float D = beckmannDistribution(dotNH);
    float F = schlickApprox(dotVH, 1.0, refractionIndex);
    float G = geometricAttenuation(dotNH, dotVN, dotVH, dotNL);

    return max(D * F * G / (4.0 * dotVN * dotNL), 0.0);
}

float orennayarTerm(float lambert, vec3 n, vec3 l) {
    vec3 v = vec3(0.0, 0.0, 1.0); // Im eye space ist die Richtung zum Betrachter schlicht die Z-Achse
    float sigma2 = roughness * roughness; // sigma^2

    // TASK: implement Oren-Nayar Term and use instead of 1.0 below:
	float rho = albedo; //1.0f;

	float A = 1 - 0.5 * sigma2 / (sigma2 + 0.57);
	float B = 0.45 * sigma2 / (sigma2 + 0.09);

	float theta_L = acos(cdot(n,l) / length(n) / length(l));
	float theta_V = acos(cdot(n,v) / length(n) / length(v));

	float alpha = max(theta_L, theta_V);
	float beta = min(theta_L, theta_V);

	vec3 ortho_V = normalize(v - cdot(v,n) * n);
	vec3 ortho_L = normalize(l - cdot(l,n) * n);
	float cos_azimuth = cdot(ortho_L, ortho_V);

	// rho * cos(theta_L)
	float LV = cos(theta_L) * albedo * (A + (B * max(0, cos_azimuth) * sin(alpha) * tan(beta)));

    return lambert * LV;
}

void main() {
    float diffuseTerm = cdot(interp_normal, interp_light_dir);
    diffuseTerm = orennayarTerm(diffuseTerm, interp_normal, interp_light_dir);
    diffuseTerm = max(diffuseTerm, 0.1);
    float specularTerm = cooktorranceTerm(interp_normal, interp_light_dir);

	vec4 diff;
	vec4 spec;

	if (tex_height.x > 0.8){
		diff = texture2D(snow_tex, uv);
	}
	else if (tex_height.x > 0.7){
		float weight = (tex_height.x - 0.7) * 10.0;
		diff =  weight * texture2D(snow_tex, uv) + (1.0 - weight) * texture2D(stone_tex, uv);
	}
	else if (tex_height.x > 0.5){
		diff = texture2D(stone_tex, uv);
	}
	else if (tex_height.x > 0.4){
		float weight = (tex_height.x - 0.4) * 10.0;
		diff =  weight * texture2D(stone_tex, uv) + (1.0 - weight) * texture2D(grass_tex, uv);
	}
	else{
		diff = texture2D(grass_tex, uv);
	}

	spec = diff + vec4(0.1,0.1,0.09,0.0);
	// spec = vec4(10.0,0.0,0.0,1.0);
	frag_color = vec4(vec3(clamp(diff * diffuseTerm + spec * specularTerm, 0.0, 1.0)), 1);
}

#include "material.h"
#include "scene.h"
#include "util.h"

using namespace glm;

vec3 layered_brdf::f(const diff_geom &geom, const vec3 &w_o, const vec3 &w_i) {
	// todo proper fresnel reflection for layered material
	const float F = fresnel_dielectric(absdot(geom.ns, w_o), 1.0f, geom.mat->ior);
	vec3 diff = base->f(geom, w_o, w_i);
	vec3 spec = coat->f(geom, w_o, w_i);
	return (1.0f-F)*diff + F*spec;
}


// lambertian_reflection

vec3 lambertian_reflection::f(const diff_geom &geom, const vec3 &w_o, const vec3 &w_i) {
	if (!same_hemisphere(w_i, geom.ns)) return vec3(0);
	// todo
	// return vec3(0);
	return one_over_pi * geom.albedo();
}


// specular_reflection

vec3 phong_specular_reflection::f(const diff_geom &geom, const vec3 &w_o, const vec3 &w_i) {
	if (!same_hemisphere(w_i, geom.ng)) return vec3(0);
	// todo
	// return vec3(0);
	float exponent = exponent_from_roughness(geom.mat->roughness);
	vec3 r = 2.0f * geom.ns * dot(geom.ns, w_i) - w_i;
	float cos_theta = cdot(w_o, r);
	const float norm_f = (exponent + 2.0f) * one_over_2pi;
	return (coat ? vec3(1) : geom.albedo()) * powf(cos_theta, exponent) * norm_f * cdot(w_i,geom.ns);
}


// Microfacet distribution helper functions
#define sqr(x) ((x)*(x))

// In the following, make sure that corner cases are taken care of (positive
// dot products, >0 denominators, correctly aligned directions, etc)

inline float ggx_d(const float NdotH, float roughness) {
	// todo TR NDF
	// return 0.0f;
	if (NdotH <= 0) return 0.f;
    const float tan2 = tan2_theta(NdotH);
    if (!std::isfinite(tan2)) return 0.f;
    const float a2 = sqr(roughness);
    return a2 / (pi * sqr(sqr(NdotH)) * sqr(a2 + tan2));
}

inline float ggx_g1(const float NdotV, float roughness) {
	// todo TR Smith G_1
	// return 0.0f;
	if (NdotV <= 0) return 0.f;
    const float tan2 = tan2_theta(NdotV);
    if (!std::isfinite(tan2)) return 0.f;
    return 2.f / (1.f + sqrtf(1.f + sqr(roughness) * tan2));
}



vec3 gtr2_reflection::f(const diff_geom &geom, const vec3 &w_o, const vec3 &w_i) {
	// todo full TR Microfacet BRDF, use ggx_d and ggx_g1
	// return vec3(0);
	if (!same_hemisphere(geom.ng, w_i)) return vec3(0);
    const float NdotV = cdot(geom.ns, w_o);
    const float NdotL = cdot(geom.ns, w_i);
    if (NdotV == 0.f || NdotV == 0.f) return vec3(0);
    const vec3 H = normalize(w_o + w_i);
    const float NdotH = cdot(geom.ns, H);
    const float HdotL = cdot(H, w_i);
    const float roughness = geom.mat->roughness;
    const float F = fresnel_dielectric(HdotL, 1.f, geom.mat->ior);
    const float D = ggx_d(NdotH, roughness);
    const float G = ggx_g1(NdotV, roughness) * ggx_g1(NdotL, roughness);
    const float microfacet = (F * D * G) / (4 * abs(NdotV) * abs(NdotL));
    return coat ? vec3(microfacet) : geom.albedo() * microfacet;
}



brdf *new_brdf(const std::string name, scene &scene) {
	if (scene.brdfs.count(name) == 0) {
		brdf *f = nullptr;
		if (name == "lambert")
			f = new lambertian_reflection;
		else if (name == "phong")
			f = new phong_specular_reflection;
		else if (name == "layered-phong") {
			brdf *base = new_brdf("lambert", scene);
			specular_brdf *coat = dynamic_cast<specular_brdf*>(new_brdf("phong", scene));
			assert(coat);
			f = new layered_brdf(coat, base);
		}
		else if (name == "gtr2")
			f = new gtr2_reflection;
		else if (name == "layered-gtr2") {
			brdf *base = new_brdf("lambert", scene);
			specular_brdf *coat = dynamic_cast<specular_brdf*>(new_brdf("gtr2", scene));
			assert(coat);
			f = new layered_brdf(coat, base);
		}
		else
			throw std::runtime_error(std::string("No such brdf defined: ") + name);
		return f;
	}
	return scene.brdfs[name];
}


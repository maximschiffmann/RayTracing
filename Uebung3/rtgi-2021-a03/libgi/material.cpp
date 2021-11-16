#include "material.h"
#include "scene.h"
#include "util.h"

using namespace glm;

vec3 layered_brdf::f(const diff_geom &geom, const vec3 &w_o, const vec3 &w_i) {
	// todo proper fresnel reflection for layered material
	vec3 diff = base->f(geom, w_o, w_i);
	vec3 spec = coat->f(geom, w_o, w_i);
	return diff+spec;
}


// lambertian_reflection

vec3 lambertian_reflection::f(const diff_geom &geom, const vec3 &w_o, const vec3 &w_i) {
	if (!same_hemisphere(w_i, geom.ns)) return vec3(0);
	// todo
	return vec3(0);
}


// specular_reflection

vec3 phong_specular_reflection::f(const diff_geom &geom, const vec3 &w_o, const vec3 &w_i) {
	if (!same_hemisphere(w_i, geom.ng)) return vec3(0);
	// todo
	return vec3(0);
}




// In the following, make sure that corner cases are taken care of (positive
// dot products, >0 denominators, correctly aligned directions, etc)

inline float ggx_d(const float NdotH, float roughness) {
	// todo TR NDF
	return 0.0f;
}

inline float ggx_g1(const float NdotV, float roughness) {
	// todo TR Smith G_1
	return 0.0f;
}



vec3 gtr2_reflection::f(const diff_geom &geom, const vec3 &w_o, const vec3 &w_i) {
	// todo full TR Microfacet BRDF, use ggx_d and ggx_g1
	return vec3(0);
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
			throw std::runtime_error(std::string("Not implemented yet: ") + name);
		}
		else
			throw std::runtime_error(std::string("No such brdf defined: ") + name);
		return f;
	}
	return scene.brdfs[name];
}


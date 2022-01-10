#include "direct.h"

#include "libgi/rt.h"
#include "libgi/context.h"
#include "libgi/intersect.h"
#include "libgi/util.h"
#include "libgi/color.h"
#include "libgi/sampling.h"

using namespace glm;
using namespace std;

gi_algorithm::sample_result direct_light::sample_pixel(uint32_t x, uint32_t y, uint32_t samples, const render_context &rc) {
	sample_result result;
	this->rc = &rc;
	for (int sample = 0; sample < samples; ++sample) {
		vec3 radiance(0);
		ray view_ray = cam_ray(rc.scene.camera, x, y, glm::vec2(rc.rng.uniform_float()-0.5f, rc.rng.uniform_float()-0.5f));
		triangle_intersection closest = rc.scene.rt->closest_hit(view_ray);
		if (closest.valid()) {
			diff_geom dg(closest, rc.scene);
			if (dg.mat->emissive != vec3(0)) {
				radiance = dg.mat->emissive;
			}
			else {
				brdf *brdf = dg.mat->brdf;
				//auto col = dg.mat->albedo_tex ? dg.mat->albedo_tex->sample(dg.tc) : dg.mat->albedo;
				if      (sampling_mode == sample_uniform)   radiance = sample_uniformly(dg, view_ray);
				else if (sampling_mode == sample_light)     radiance = sample_lights(dg, view_ray);
				else if (sampling_mode == sample_cosine)    radiance = sample_cosine_weighted(dg, view_ray);
				else if (sampling_mode == sample_brdf)      radiance = sample_brdfs(dg, view_ray);
			}
		}
		result.push_back({radiance,vec2(0)});
	}
	return result;
}

vec3 direct_light::sample_uniformly(const diff_geom &hit, const ray &view_ray) {
	// set up a ray in the hemisphere that is uniformly distributed
	vec2 xi = rc->rng.uniform_float2();
	float z = xi.x;
	float phi = 2*pi*xi.y;
	// z is cos(theta), sin(theta) = sqrt(1-cos(theta)^2)
	float sin_theta = sqrtf(1.0f - z*z);
	vec3 sampled_dir = vec3(sin_theta * cosf(phi),
							sin_theta * sinf(phi),
							z);
	
	vec3 w_i = align(sampled_dir, hit.ng);
	ray sample_ray(hit.x, w_i);

	// find intersection and store brightness if it is a light
	vec3 brightness(0);
	triangle_intersection closest = rc->scene.rt->closest_hit(sample_ray);
	if (closest.valid()) {
		diff_geom dg(closest, rc->scene);
		brightness = dg.mat->emissive;
	}
	// evaluate reflectance
	return 2*pi * brightness * hit.mat->brdf->f(hit, -view_ray.d, sample_ray.d) * cdot(sample_ray.d, hit.ns);
}

vec3 direct_light::sample_cosine_weighted(const diff_geom &hit, const ray &view_ray) {
	vec2 xi = rc->rng.uniform_float2();
	// todo: implement importance sampling on the cosine-term
	vec3 sampled_dir = cosine_sample_hemisphere(xi);
	vec3 w_i = align(sampled_dir, hit.ng);
	ray sample_ray(hit.x, w_i);

	// find intersection and store brightness if it is a light
	vec3 brightness(0);
	triangle_intersection closest = rc->scene.rt->closest_hit(sample_ray);
	if (closest.valid()) {
		diff_geom dg(closest, rc->scene);
		brightness = dg.mat->emissive;
	}

	// evaluate reflectance
	return brightness * hit.mat->brdf->f(hit, -view_ray.d, sample_ray.d) * pi;
}

vec3 direct_light::sample_lights(const diff_geom &hit, const ray &view_ray) {
	auto [l_id, l_pdf] = rc->scene.light_distribution->sample_index(rc->rng.uniform_float());
	light *l = rc->scene.lights[l_id];
	auto [shadow_ray,l_col,pdf] = l->sample_Li(hit, rc->rng.uniform_float2());
	if (l_col != vec3(0))
		if (!rc->scene.rt->any_hit(shadow_ray))
			return l_col * hit.mat->brdf->f(hit, -view_ray.d, shadow_ray.d) * cdot(shadow_ray.d, hit.ns) / (pdf * l_pdf);
	return vec3(0);
}

vec3 direct_light::sample_brdfs(const diff_geom &hit, const ray &view_ray) {
	auto [w_i, f, pdf] = hit.mat->brdf->sample(hit, -view_ray.d, rc->rng.uniform_float2());
	ray light_ray(hit.x, w_i);
	if (auto is = rc->scene.rt->closest_hit(light_ray); is.valid())
		if (diff_geom hit_geom(is, rc->scene); hit_geom.mat->emissive != vec3(0))
			return f * hit_geom.mat->emissive * cdot(hit.ns, w_i) / pdf;
	return vec3(0);
}

bool direct_light::interprete(const std::string &command, std::istringstream &in) {
	string value;
	if (command == "is") {
		in >> value;
		if (value == "uniform") sampling_mode = sample_uniform;
		else if (value == "cosine") sampling_mode = sample_cosine;
		else if (value == "light") sampling_mode = sample_light;
		else if (value == "brdf") sampling_mode = sample_brdf;
		else cerr << "unknown sampling mode in " << __func__ << ": " << value << endl;
		return true;
	}
	return false;
}



// todo: implement member functions of direct_light_mis
// Note: feel free to make use of the helper functions provided in the base class

// separate version to not include the rejection part in all methods
// this should be improved upon
gi_algorithm::sample_result direct_light_mis::sample_pixel(uint32_t x, uint32_t y, uint32_t samples, const render_context &rc) {
	sample_result result;
	for (int sample = 0; sample < samples; ++sample) {
		vec3 radiance(0);
		ray view_ray = cam_ray(rc.scene.camera, x, y, glm::vec2(rc.rng.uniform_float()-0.5f, rc.rng.uniform_float()-0.5f));
		triangle_intersection closest = rc.scene.rt->closest_hit(view_ray);
		if (closest.valid()) {
			diff_geom dg(closest, rc.scene);
			if (dg.mat->emissive != vec3(0)) {
				radiance = dg.mat->emissive;
			}
			else {
				brdf *brdf = dg.mat->brdf;
					
				float pdf_light = 0,
					  pdf_brdf = 0;
				if (sample < samples/2-1) {
					auto [l_id, l_pdf] = rc.scene.light_distribution->sample_index(rc.rng.uniform_float());
					light *l = rc.scene.lights[l_id];
					auto [shadow_ray,l_col,pdf] = l->sample_Li(dg, rc.rng.uniform_float2());
					pdf_light = l_pdf*pdf;
					pdf_brdf  = brdf->pdf(dg, -view_ray.d, shadow_ray.d);
					if (l_col != vec3(0))
						if (auto is = rc.scene.rt->closest_hit(shadow_ray); !is.valid() || is.t > shadow_ray.t_max)
							radiance = l_col * brdf->f(dg, -view_ray.d, shadow_ray.d) * cdot(shadow_ray.d, dg.ns);
				}
				else {
					auto [w_i, f, pdf] = brdf->sample(dg, -view_ray.d, rc.rng.uniform_float2());
					ray light_ray(dg.x, w_i);
					pdf_brdf  = pdf;
					if (f != vec3(0))
						if (auto is = rc.scene.rt->closest_hit(light_ray); is.valid())
							if (diff_geom hit_geom(is, rc.scene); hit_geom.mat->emissive != vec3(0)) {
								trianglelight tl(rc.scene, is.ref);
								pdf_light = luma(tl.power()) / rc.scene.light_distribution->integral();
								pdf_light *= tl.pdf(light_ray, hit_geom);
								radiance = f * hit_geom.mat->emissive * cdot(dg.ns, w_i);
							}
				}
				// make sure to really be on the safest side possible ;)
				assert(pdf_light >= 0);
				assert(pdf_brdf >= 0);
				assert(radiance.x >= 0);assert(std::isfinite(radiance.x));
				assert(radiance.y >= 0);assert(std::isfinite(radiance.y));
				assert(radiance.z >= 0);assert(std::isfinite(radiance.z));
				assert(std::isfinite(pdf_light));
				assert(std::isfinite(pdf_brdf));
				float balance = pdf_light + pdf_brdf; // 1920/229
				if (balance != 0.0f)
					radiance /= balance*0.5;
				else {
					// do another round as this was useless
					sample--;
					continue;
				}
			}
		}
		result.push_back({radiance,vec2(0)});
	}
	return result;
}

bool direct_light_mis::interprete(const std::string &command, std::istringstream &in) {
	// nothing to do but prevent call to base
	return false;
}



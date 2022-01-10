#include "direct.h"

#include "libgi/rt.h"
#include "libgi/context.h"
#include "libgi/intersect.h"
#include "libgi/util.h"
#include "libgi/color.h"
#include "libgi/sampling.h"

#include "libgi/global-context.h"

using namespace glm;
using namespace std;

gi_algorithm::sample_result direct_light::sample_pixel(uint32_t x, uint32_t y, uint32_t samples)
{
	sample_result result;
	for (int sample = 0; sample < samples; ++sample)
	{
		vec3 radiance(0, 0, 0);
		ray view_ray = cam_ray(rc->scene.camera, x, y, glm::vec2(rc->rng.uniform_float() - 0.5f, rc->rng.uniform_float() - 0.5f));
		triangle_intersection closest = rc->scene.single_rt->closest_hit(view_ray);
		if (closest.valid())
		{
			diff_geom dg(closest, rc->scene);
			flip_normals_to_ray(dg, view_ray);

			// todo: compute direct lighting contribution
			if (dg.mat->emissive != vec3(0))
			{
				radiance = dg.mat->emissive;
			}
			else
			{
				brdf *brdf = dg.mat->brdf;
				// auto col = dg.mat->albedo_tex ? dg.mat->albedo_tex->sample(dg.tc) : dg.mat->albedo;
				if (sampling_mode == sample_uniform)
					radiance = sample_uniformly(dg, view_ray);
				else if (sampling_mode == sample_light)
					radiance = sample_lights(dg, view_ray);
			}
		}
		result.push_back({radiance, vec2(0)});
	}
	return result;
}

vec3 direct_light::sample_uniformly(const diff_geom &hit, const ray &view_ray)
{
	// set up a ray in the hemisphere that is uniformly distributed
	vec2 xi = rc->rng.uniform_float2();
	float z = xi.x;
	float phi = 2*pi*xi.y;
	// z is cos(theta), sin(theta) = sqrt(1-cos(theta)^2)
	float sin_theta = sqrtf(1.0f - z*z);
	vec3 sampled_dir = vec3(sin_theta * cosf(phi),
							sin_theta * sinf(phi),
							z);
	// dreht die ausrichtung
	vec3 w_i = align(sampled_dir, hit.ng);
	ray sample_ray(hit.x, w_i);

	// find intersection and store brightness if it is a light
	vec3 brightness(0);
	triangle_intersection closest = rc->scene.single_rt->closest_hit(sample_ray);
	if (closest.valid()) {
		diff_geom dg(closest, rc->scene);
		brightness = dg.mat->emissive;
	}
	// evaluate reflectance
	return 2*pi * brightness * hit.mat->brdf->f(hit, -view_ray.d, sample_ray.d) * cdot(sample_ray.d, hit.ns);						
	// todo: implement uniform hemisphere sampling
}

vec3 direct_light::sample_lights(const diff_geom &hit, const ray &view_ray)
{
	// todo: implement uniform sampling on the first few of the scene's lights' surfaces
	const size_t N_max = 2;
	int lighs_processed = 0;
	vec3 accum(0);
	for (int i = 0; i < rc->scene.lights.size(); ++i) {
		const trianglelight *tl = dynamic_cast<trianglelight*>(rc->scene.lights[i]);
		vec2 xi = rc->rng.uniform_float2();
		float sqrt_xi1 = sqrt(xi.x);
		float beta = 1.0f - sqrt_xi1;
		float gamma = xi.y * sqrt_xi1;
		float alpha = 1.0f - beta - gamma;
		const vertex &a = rc->scene.vertices[tl->a];
		const vertex &b = rc->scene.vertices[tl->b];
		const vertex &c = rc->scene.vertices[tl->c];
		vec3 target = alpha*a.pos  + beta*b.pos  + gamma*c.pos;
		vec3 normal = alpha*a.norm + beta*b.norm + gamma*c.norm;
	
		vec3 w_i = target - hit.x;
		float tmax = length(w_i);
		w_i /= tmax;
		ray r(hit.x, w_i);
		r.length_exclusive(tmax);

		if (!rc->scene.single_rt->any_hit(r)) {
			auto mat = rc->scene.materials[tl->material_id];
			accum += mat.emissive * hit.mat->brdf->f(hit, -view_ray.d, r.d) * cdot(hit.ns, r.d) * cdot(normal, -r.d) / (tmax*tmax);
		}

		if (++lighs_processed == N_max)
			break;
	}
	return accum / (float)lighs_processed;
}

bool direct_light::interprete(const std::string &command, std::istringstream &in)
{
	string value;
	if (command == "is")
	{
		in >> value;
		if (value == "uniform")
			sampling_mode = sample_uniform;
		else if (value == "cosine")
			sampling_mode = sample_cosine;
		else if (value == "light")
			sampling_mode = sample_light;
		else if (value == "brdf")
			sampling_mode = sample_brdf;
		else
			cerr << "unknown sampling mode in " << __func__ << ": " << value << endl;
		return true;
	}
	return false;
}

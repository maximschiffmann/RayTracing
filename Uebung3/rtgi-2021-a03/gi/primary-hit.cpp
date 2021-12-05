#include "primary-hit.h"

#include "libgi/rt.h"
#include "libgi/context.h"
#include "libgi/intersect.h"
#include "libgi/util.h"
#include "libgi/color.h"

#include "libgi/global-context.h"

#include "libgi/wavefront-rt.h"

using namespace glm;
using namespace std;

gi_algorithm::sample_result primary_hit_display::sample_pixel(uint32_t x, uint32_t y, uint32_t samples) {
	sample_result result;
	for (int sample = 0; sample < samples; ++sample) {
		vec3 radiance(0);
		ray view_ray = cam_ray(rc->scene.camera, x, y, glm::vec2(rc->rng.uniform_float()-0.5f, rc->rng.uniform_float()-0.5f));
		triangle_intersection closest = rc->scene.single_rt->closest_hit(view_ray);
		if (closest.valid()) {
			diff_geom dg(closest, rc->scene);
			radiance = dg.albedo();
		}
		result.push_back({radiance,vec2(0)});
	}
	return result;
}

gi_algorithm::sample_result local_illumination::sample_pixel(uint32_t x, uint32_t y, uint32_t samples) {
	sample_result result;
	for (int sample = 0; sample < samples; ++sample) {
		vec3 radiance(0);
		ray view_ray = cam_ray(rc->scene.camera, x, y, glm::vec2(rc->rng.uniform_float()-0.5f, rc->rng.uniform_float()-0.5f));
		triangle_intersection closest = rc->scene.single_rt->closest_hit(view_ray);
		if (closest.valid()) {
			diff_geom dg(closest, rc->scene);
			brdf *brdf = dg.mat->brdf;
			assert(!rc->scene.lights.empty());
			pointlight *pl = dynamic_cast<pointlight*>(rc->scene.lights[0]);
			assert(pl);
			// todo: implement local illumination via the BRDF
			vec3 to_light = pl->pos - dg.x;
			vec3 w_i = normalize(to_light);
			vec3 w_o = -view_ray.d;
			float d = sqrt(dot(to_light, to_light));

			ray shadow_ray(dg.x, w_i);
			shadow_ray.length_exclusive(d);
			if (!rc->scene.single_rt->any_hit(shadow_ray)) {
				radiance = pl->power() * brdf->f(dg, w_o, w_i) / (d*d);
			}
		}
		result.push_back({radiance,vec2(0)});
	}
	return result;
}


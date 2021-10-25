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

gi_algorithm::sample_result primary_hit_display::sample_pixel(uint32_t x, uint32_t y, uint32_t samples)
{
	// sample_result result;

	// for (int i = 0; i < rc->scene.triangles.size(); ++i)
	// {
	// 	ray ray = cam_ray(rc->scene.camera, x, y);
	// 	// triangle tri = rc->scene.triangles[0];
	// 	triangle_intersection info;
	// 	bool intersection = intersect(rc->scene.triangles[i], rc->scene.vertices.data(), ray, info);
	// 	// std::cout << "Primary intersection!" << std::endl;
	// 	vec3 col;
	// 	col.x = info.beta;
	// 	col.y = info.gamma;
	// 	col.z = info.t;
	// 	result.push_back({col, vec2(0)});
	// }
	// return result;
	
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

namespace wf
{
	namespace cpu
	{
		struct store_hitpoint_albedo : public batch_ray_and_intersection_processing_cpu
		{
			void run() override
			{
				auto res = rc->resolution();
				float one_over_samples = 1.0f / rc->sppx;
				auto *rt = dynamic_cast<batch_rt *>(rc->scene.batch_rt);
				assert(rt != nullptr);
				cout << res << endl;
				#pragma omp parallel for
				for (int y = 0; y < res.y; ++y)
					for (int x = 0; x < res.x; ++x)
					{
						vec3 radiance(0);
						for (int sample = 0; sample < rc->sppx; ++sample)
						{
							triangle_intersection closest = rt->rd.intersections[y * res.x + x];
							if (closest.valid())
							{
								diff_geom dg(closest, rc->scene);
								radiance += dg.albedo();
							}
						}
						radiance *= one_over_samples;
						rc->framebuffer.color(x, y) = vec4(radiance, 1);
					}
			}
		};
	}
}

void primary_hit_display_wf::compute_samples()
{

	// 	#pragma omp parallel for
	// 	for (int y = 0; y < res.y; ++y)
	// 		for (int x = 0; x < res.x; ++x)
	// 			rays[y*w+x] = cam_ray(rc->scene.camera, x, y, glm::vec2(rc->rng.uniform_float()-0.5f, rc->rng.uniform_float()-0.5f));
	//
	wf::cpu::batch_cam_ray_setup_cpu().run();
	auto *batch_rt = rc->scene.batch_rt;
	assert(batch_rt != nullptr);
	batch_rt->compute_closest_hit();

	wf::cpu::store_hitpoint_albedo().run();
}

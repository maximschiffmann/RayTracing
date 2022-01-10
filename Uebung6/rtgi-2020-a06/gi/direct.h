#pragma once

#include "libgi/algorithm.h"
#include "libgi/material.h"

class direct_light : public gi_algorithm {
	enum sampling_mode { sample_uniform, sample_cosine, sample_light, sample_brdf, both };
	enum sampling_mode sampling_mode = sample_light;

	const render_context *rc;
	
	vec3 sample_uniformly(const diff_geom &hit, const ray &view_ray);
	vec3 sample_lights(const diff_geom &hit, const ray &view_ray);
	vec3 sample_cosine_weighted(const diff_geom &hit, const ray &view_ray);
	vec3 sample_brdfs(const diff_geom &hit, const ray &view_ray);
	

public:
	gi_algorithm::sample_result sample_pixel(uint32_t x, uint32_t y, uint32_t samples, const render_context &r) override;
	bool interprete(const std::string &command, std::istringstream &in) override;
};

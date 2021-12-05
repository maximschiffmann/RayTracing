#pragma once

#include "libgi/algorithm.h"
#include "libgi/material.h"

class direct_light : public recursive_algorithm {
	enum sampling_mode { sample_uniform, sample_cosine, sample_light, sample_brdf };
	enum sampling_mode sampling_mode = sample_light;

protected:
	vec3 sample_uniformly(const diff_geom &hit, const ray &view_ray);
	vec3 sample_lights(const diff_geom &hit, const ray &view_ray);
	
#ifndef RTGI_AXX
	sample_result full_mis(uint32_t x, uint32_t y, uint32_t samples);
#endif

public:
	gi_algorithm::sample_result sample_pixel(uint32_t x, uint32_t y, uint32_t samples) override;
	bool interprete(const std::string &command, std::istringstream &in) override;
};


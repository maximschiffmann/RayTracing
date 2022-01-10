#pragma once

#include "rt.h"

#include <vector>
#include <string>
#include <utility>
#include <cstdint>

class distribution_1d {
	std::vector<float> f, cdf;
	float integral_1spaced;
	void build_cdf();

public:
	distribution_1d(const std::vector<float> &f);
	distribution_1d(std::vector<float> &&f);
	pair<uint32_t,float> sample_index(float xi) const;
	float pdf(uint32_t index) const;
	void debug_out(const std::string &p) const;
	float integral() const { return integral_1spaced; }

	float value_at(int index) const { return f[index]; }
	int size() const { return f.size(); }
	

};


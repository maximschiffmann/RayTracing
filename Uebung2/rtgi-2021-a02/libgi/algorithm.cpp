#include "algorithm.h"

#include "libgi/rt.h"
#include "libgi/context.h"
#include "libgi/util.h"

#include "libgi/global-context.h"

#include <chrono>

float gi_algorithm::uniform_float() const {
	return rc->rng.uniform_float();
}

glm::vec2 gi_algorithm::uniform_float2() const {
	return rc->rng.uniform_float2();
}



//! A hacky way to convert ms to a human readable indication of how long this is going to be.
static std::string timediff(unsigned ms) {
	if (ms > 2000) {
		ms /= 1000;
		if (ms > 60) {
			ms /= 60;
			if (ms > 60) {
				return "hours";
			}
			else return std::to_string((int)floor(ms)) + " min";
		}
		else {
			return std::to_string((int)ms) + " sec";
		}
	}
	else return std::to_string(ms) + " ms";
}



/*  Implementaiton for "one path at a time" traversal on the CPU over the complete image.
 *
 *  Note: We first compute a single sample to get a rough estimate of how long rendering is going to take.
 *
 *  Note: Times reported via \ref stats_timer might not be perfectly reliable as of now.  This is because the
 *        timer-overhead is in the one (at times even two) digit percentages of the individual fragments measured.
 *
 */

void recursive_algorithm::compute_samples() {
	using namespace std::chrono;
	auto start = system_clock::now();
	rc->framebuffer.color.for_each([&](unsigned x, unsigned y) {
										rc->framebuffer.add(x, y, sample_pixel(x, y, 1));
    							   });
	auto delta_ms = duration_cast<milliseconds>(system_clock::now() - start).count();
	std::cout << "Will take around " << timediff(delta_ms*(rc->sppx-1)) << " to complete" << std::endl;
	
	rc->framebuffer.color.for_each([&](unsigned x, unsigned y) {
										rc->framebuffer.add(x, y, sample_pixel(x, y, rc->sppx-1));
    							   });
	delta_ms = duration_cast<milliseconds>(system_clock::now() - start).count();
	std::cout << "Took " << timediff(delta_ms) << " (" << delta_ms << " ms) " << " to complete" << std::endl;
}

void recursive_algorithm::prepare_frame() {
	assert(rc->scene.single_rt != nullptr);
}



/*  Implementaiton for "one bounce at a time" traversal
 *
 *
 */

void wavefront_algorithm::prepare_frame() {
	assert(rc->scene.batch_rt != nullptr);
}


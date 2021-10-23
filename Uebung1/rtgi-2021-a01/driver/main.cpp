#include "libgi/rt.h"
#include "libgi/camera.h"
#include "libgi/scene.h"
#include "libgi/intersect.h"
#include "libgi/framebuffer.h"
#include "libgi/context.h"
#include "libgi/timer.h"

#include "libgi/global-context.h"

#include "interaction.h"

#include "cmdline.h"

#include <png++/png.hpp>
#include <iostream>
#include <chrono>
#include <cstdio>
#include <omp.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/random.hpp>

#include <iostream>

using namespace std;
using namespace glm;
using namespace png;

rgb_pixel to_png(vec3 col01) {
	col01 = clamp(col01, vec3(0), vec3(1));
	col01 = pow(col01, vec3(1.0f/2.2f));
	return rgb_pixel(col01.x*255, col01.y*255, col01.z*255);
}

/*! \brief This is called from the \ref repl to compute a single image
 *  
 */
void run(gi_algorithm *algo) {
	using namespace std::chrono;
	algo->prepare_frame();
	test_camrays(rc->scene.camera);
	rc->framebuffer.clear();

	algo->compute_samples();
	algo->finalize_frame();
	
	rc->framebuffer.png().write(cmdline.outfile);
}

void rt_bench() {
	//create Buffer for rays and intersections with the size of the camera resolution
	buffer<triangle_intersection> triangle_intersections(rc->scene.camera.w, rc->scene.camera.h);
	buffer<ray> rays(rc->scene.camera.w, rc->scene.camera.h);
	
	//init Buffer with Camera rays
	rays.for_each([&](unsigned x, unsigned y) {
		rays(x, y) = cam_ray(rc->scene.camera, x, y);
	});
	
	//calculate closest triangle intersection for each ray
	raii_timer bench_timer("rt_bench");
	rays.for_each([&](unsigned x, unsigned y) {
		triangle_intersections(x, y) = rc->scene.single_rt->closest_hit(rays(x, y));
	});
}

int main(int argc, char **argv)
{

	// vec3 a = vec3(-10, 0, 0);
	// vec3 b = vec3(10, -10, 0);
	// vec3 c = vec3(0, 10, 0);

	// vertex ver[3];
	// ver[0].pos = a;
	// ver[1].pos = b;
	// ver[2].pos = c;

	// triangle tri;
	// tri.a = 0;
	// tri.b = 1;
	// tri.c = 2;

	// vec3 o = vec3(0, 0, -2);
	// vec3 d = vec3(0, 0, -1);
	// ray r = ray(o, d);

	// triangle_intersection info;

//	intersect(tri, ver, r, info);

	parse_cmdline(argc, argv);

	repl_update_checks uc;
	if (cmdline.script != "") {
		ifstream script(cmdline.script);
		repl(script, uc);
	}
	if (cmdline.interact)
		repl(cin, uc);
	
	stats_timer.print();

	delete rc->algo;
	return 0;
}

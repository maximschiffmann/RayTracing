#include "camera.h"

#include "random.h"

#include <fstream>
#include <iostream>
using namespace glm;
using namespace std;

//! Set up a camera ray using cam.up, cam.dir, cam.w, cam.h (see \ref camera::update_frustum)
ray cam_ray(const camera &cam, int x, int y, vec2 offset) {
	// todo: compute the camera rays for each pixel.
	// note: you can use the following function to export them as an obj model and look at them in blender.
	vec3 U = cross(cam.dir, cam.up);
	vec3 V = cross(U, cam.dir);
	float px = ((float)x + 0.5f + offset.x) / (float)cam.w * 2.0f - 1.0f;
	float py = ((float)y + 0.5f + offset.y) / (float)cam.h * 2.0f - 1.0f;
	px = px * cam.near_w;
	py = py * cam.near_h;
	vec3 d = normalize(cam.dir + U * px + V * py);
	return ray(cam.pos, d);
}

void test_camrays(const camera &camera, int stride) {
	ofstream out("test.obj");
	int i = 1;
	for (int y = 0; y < camera.h; y += stride)
		for (int x = 0; x < camera.w; x += stride) {
			ray ray = cam_ray(camera, x, y);
			out << "v " << ray.o.x << " " << ray.o.y << " " << ray.o.z << endl;
			out << "v " << ray.d.x << " " << ray.d.y << " " << ray.d.z << endl;
			out << "l " << i++ << " " << i++ << endl;
		}
}



#pragma once

#include "rt.h"
#include<iostream>

struct aabb
{
	vec3 min, max;
	aabb() : min(FLT_MAX), max(-FLT_MAX) {}
	void grow(vec3 v)
	{
		min = glm::min(v, min);
		max = glm::max(v, max);
	}
	void grow(const aabb &other)
	{
		min = glm::min(other.min, min);
		max = glm::max(other.max, max);
	}
};

inline float calculateDeterminante(vec3 a, vec3 b, vec3 c)
{
	float result = (a.x * b.y * c.z) + (b.x * c.y * a.z) + (c.x * a.y * b.z) - (a.z * b.y * c.x) - (b.z * c.y * a.x) - (c.z * a.y * b.x);

	return result;
}

// See Shirley (2nd Ed.), pp. 206. (library or excerpt online)
inline bool intersect(const triangle &t, const vertex *vertices, const ray &ray, triangle_intersection &info)
{
	
	float oX = ray.o.x;
	float oY = ray.o.y;
	float oZ = ray.o.z;
	// std::cout << "oX: " << oX << std::endl;
	// std::cout << "oY: " << oY << std::endl;
	// std::cout << "oZ: " << oZ << std::endl;


	float dX = ray.d.x;
	float dY = ray.d.y;
	float dZ = ray.d.z;
	// std::cout << "dX: " << dX << std::endl;
	// std::cout << "dY: " << dY << std::endl;
	// std::cout << "dZ: " << dZ << std::endl;

	vec3 aPos = vertices[t.a].pos;
	float aX = aPos.x;
	float aY = aPos.y;
	float aZ = aPos.z;
	// std::cout << "aPos: " << aPos << std::endl;
	// std::cout << "aX: " << aX << std::endl;
	// std::cout << "aY: " << aY << std::endl;
	// std::cout << "aZ: " << aZ << std::endl;

	vec3 bPos = vertices[t.b].pos;
	float bX = bPos.x;
	float bY = bPos.y;
	float bZ = bPos.z;
	// std::cout << "bPos: " << bPos << std::endl;
	// std::cout << "bX: " << bX << std::endl;
	// std::cout << "bY: " << bY << std::endl;
	// std::cout << "bZ: " << bZ << std::endl;

	vec3 cPos = vertices[t.c].pos;
	float cX = cPos.x;
	float cY = cPos.y;
	float cZ = cPos.z;
	// std::cout << "cPos: " << cPos << std::endl;
	// std::cout << "cX: " << cX << std::endl;
	// std::cout << "cY: " << cY << std::endl;
	// std::cout << "cZ: " << cZ << std::endl;

	float vecAOx = aX - oX;
	float vecAOy = aY - oY;
	float vecAOz = aZ - oZ;

	float vecABx = aX - bX;
	float vecABy = aY - bY;
	float vecABz = aZ - bZ;

	float vecACx = aX - cX;
	float vecACy = aY - cY;
	float vecACz = aZ - aZ;

	vec3 AO = vec3(vecAOx, vecAOy, vecAOz);
//	std::cout << "AO: " << AO << std::endl;
	vec3 AB = vec3(vecABx, vecABy, vecABz);
//	std::cout << "AB: " << AB << std::endl;
	vec3 AC = vec3(vecACx, vecACy, vecACz);
//	std::cout << "AC: " << AC << std::endl;
	vec3 D = vec3(dX, dY, dZ);
//	std::cout << "D: " << D << std::endl;

	float detA = calculateDeterminante(AB, AC, D);
//	std::cout << "detA: " << detA << std::endl;

	float beta = calculateDeterminante(AO, AC, D) / detA;
//	std::cout << "beta: " << beta << std::endl;
	float gamma = calculateDeterminante(AB, AO, D) / detA;
//	std::cout << "gamma: " << gamma << std::endl;
	float tRay = (calculateDeterminante(AB, AC, AO) / detA);
//	std::cout << "tRay: " << tRay << std::endl;

	if (tRay >= 0 && beta >= 0 && gamma >= 0 && beta + gamma <= 1)
	{
	//	std::cout << "Intersection with triangle." << std::endl;
		info.t = tRay;
		info.beta = beta;
		info.gamma = gamma;
		return true;
	}
	// std::cout << "No intersection with triangle." << std::endl;
	return false;
	

	/////////////////////////////////////////////////////////////////
	// vec3 pos = vertices[t.a].pos;
	// const float a_x = pos.x;
	// const float a_y = pos.y;
	// const float a_z = pos.z;

	// pos = vertices[t.b].pos;
	// const float &a = a_x - pos.x;
	// const float &b = a_y - pos.y;
	// const float &c = a_z - pos.z;
	
	// pos = vertices[t.c].pos;
	// const float &d = a_x - pos.x;
	// const float &e = a_y - pos.y;
	// const float &f = a_z - pos.z;
	
	// const float &g = ray.d.x;
	// const float &h = ray.d.y;
	// const float &i = ray.d.z;
	
	// const float &j = a_x - ray.o.x;
	// const float &k = a_y - ray.o.y;
	// const float &l = a_z - ray.o.z;

	// float common1 = e*i - h*f;
	// float common2 = g*f - d*i;
	// float common3 = d*h - e*g;
	// float M 	  = a * common1  +  b * common2  +  c * common3;
	// float beta 	  = j * common1  +  k * common2  +  l * common3;

	// common1       = a*k - j*b;
	// common2       = j*c - a*l;
	// common3       = b*l - k*c;
	// float gamma   = i * common1  +  h * common2  +  g * common3;
	// float tt    = -(f * common1  +  e * common2  +  d * common3);

	// beta /= M;
	// gamma /= M;
	// tt /= M;

	// if (tt > ray.t_min && tt < ray.t_max)
	// 	if (beta > 0 && gamma > 0 && beta + gamma <= 1)
	// 	{
	// 		info.t = tt;
	// 		info.beta = beta;
	// 		info.gamma = gamma;
	// 		return true;
	// 	}

	// return false;
}

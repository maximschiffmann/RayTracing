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
	// todo
	float oX = ray.o.x;
	float oY = ray.o.y;
	float oZ = ray.o.z;

	float dX = ray.d.x;
	float dY = ray.d.y;
	float dZ = ray.d.z;

	vec3 aPos = vertices[t.a].pos;
	float aX = aPos.x;
	float aY = aPos.y;
	float aZ = aPos.z;

	vec3 bPos = vertices[t.b].pos;
	float bX = bPos.x;
	float bY = bPos.y;
	float bZ = bPos.z;

	vec3 cPos = vertices[t.c].pos;
	float cX = cPos.x;
	float cY = cPos.y;
	float cZ = cPos.z;

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
	vec3 AB = vec3(vecABx, vecABy, vecABz);
	vec3 AC = vec3(vecACx, vecACy, vecACz);
	vec3 D = vec3(dX, dY, dZ);

	float detA = calculateDeterminante(AB, AC, D);

	float beta = calculateDeterminante(AO, AC, D) / detA;
	float gamma = calculateDeterminante(AB, AO, D) / detA;
	float tRay = calculateDeterminante(AB, AC, AO) / detA;

	if (tRay >= 0 && beta >= 0 && gamma >= 0 && beta + gamma <= 1)
	{
		return true;
	}
	return false;
}

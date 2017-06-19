#pragma once
#include "hmath.h"
struct BoundingShape {
	vec3* vertices;
	int num_vertices;
};

__declspec(dllexport) bool collides(BoundingShape* b1, BoundingShape* b2);
//__declspec(dllexport) vec3* minkowski_difference(BoundingBox* b1, BoundingBox* b2);

__declspec(dllexport) void DEBUG_gjk();
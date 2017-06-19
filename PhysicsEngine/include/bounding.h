#pragma once
#include "hmath.h"
struct BoundingShape {
	vec3* vertices;
	int num_vertices;
};

__declspec(dllexport) bool collides(BoundingShape* b1, BoundingShape* b2);
__declspec(dllexport) bool gjk_collides(BoundingShape* b1, BoundingShape* b2);
__declspec(dllexport) void transform_shape(BoundingShape* base, BoundingShape* b, mat4& m);
__declspec(dllexport) void DEBUG_gjk();
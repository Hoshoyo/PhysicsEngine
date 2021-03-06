#pragma once
#include <hmath.h>
#include <quaternion.h>

struct BoundingShape {
	vec3* vertices;
	int num_vertices;
};

struct CollisionInfo {
	vec3 face_shape1[3];
	vec3 face_shape2[3];
};

__declspec(dllexport) bool collides(BoundingShape* b1, BoundingShape* b2);
__declspec(dllexport) bool gjk_collides(BoundingShape* b1, BoundingShape* b2, CollisionInfo* info);
__declspec(dllexport) void transform_shape(BoundingShape* base, BoundingShape* b, hm::mat4& m);
__declspec(dllexport) void uncollide(hm::vec3 direction, hm::vec3* position, Quaternion* rotation, float _scale, BoundingShape* b1, BoundingShape* b2, BoundingShape* moving_obj_src);
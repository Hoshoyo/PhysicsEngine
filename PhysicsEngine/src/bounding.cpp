#include "..\include\bounding.h"
#include <malloc.h>
#include <memory.h>
#include <float.h>

__declspec(dllexport) float global_distance = 0.0f;
__declspec(dllexport) void(*render_vec)(vec3 v, vec3 pos) = 0;

bool collides(BoundingShape* b1, BoundingShape* b2)
{
	return false;
}

bool gjk_collides(BoundingShape* b1, BoundingShape* b2);
vec3 gjk_support(BoundingShape* b1, BoundingShape* b2, vec3 direction);

void DEBUG_gjk()
{
}

void transform_shape(BoundingShape* base, BoundingShape* b, mat4& m)
{
	for (int i = 0; i < b->num_vertices; ++i) {
		b->vertices[i] = m * base->vertices[i];
	}
}

vec3 gjk_support(BoundingShape* b1, BoundingShape* b2, vec3 direction) 
{
	float max = -FLT_MAX;
	int index = 0;
	for (int i = 0; i < b1->num_vertices; ++i)
	{
		float dot = vec3::dot(b1->vertices[i], direction);
		if (dot > max) 	{
			max = dot;
			index = i;
		}
	}
	int b1_index = index;

	max = -FLT_MAX;
	index = 0;
	for (int i = 0; i < b2->num_vertices; ++i)
	{
		float dot = vec3::dot(b2->vertices[i], -direction);
		if (dot > max) {
			max = dot;
			index = i;
		}
	}
	int b2_index = index;

	vec3 result = b1->vertices[b1_index] - b2->vertices[b2_index];
	return result;
}

struct SupportList {
	vec3 list[4];
	int current_index;
	
	SupportList() {
		current_index = 0;
	}

	void add(vec3 v) {
		list[current_index] = v;
		current_index++;
	}

	void clear() {
		current_index = 0;
	}
};

SupportList support_list;
#define GJK_3D
bool gjk_simplex(vec3& direction)
{
	int num_entries = support_list.current_index;

	if (num_entries == 2) {
		vec3 A = support_list.list[1];
		vec3 B = support_list.list[0];
		vec3 AO = -A;	//(0,0,0) - A
		vec3 AB = B - A;
		if (vec3::dot(AB, AO) > 0) {
			direction = vec3::cross(vec3::cross(AB, AO), AB);
		}
		else {
			support_list.list[0] = support_list.list[1];
			support_list.current_index--;
			direction = AO;
		}
	}
	else if (num_entries == 3) {
		vec3 B = support_list.list[0];
		vec3 C = support_list.list[1];
		vec3 A = support_list.list[2];

		vec3 AB = B - A;
		vec3 AC = C - A;
		vec3 ABC = vec3::cross(AB, AC);	// normal to the triangle
		vec3 AO = -A;

		vec3 edge4 = vec3::cross(AB, ABC);
		vec3 edge1 = vec3::cross(ABC, AC);

		if (vec3::dot(edge1, AO) > 0) {
			if (vec3::dot(AC, AO) > 0) {
				// simplex is A, C
				support_list.list[0] = A;
				support_list.current_index--;
				direction = vec3::cross(vec3::cross(AC, AO), AC);
			}
			else {
				if (vec3::dot(AB, AO) > 0) {
					// simplex is A, B
					support_list.list[0] = A;
					support_list.list[1] = B;
					support_list.current_index--;
					direction = vec3::cross(vec3::cross(AB, AO), AB);
				}
				else {
					// simplex is A
					support_list.list[0] = A;
					support_list.current_index -= 2;
					direction = AO;
				}
			}
		}
		else {
			if (vec3::dot(edge4, AO) > 0) {
				if (vec3::dot(AB, AO) > 0) {
					// simplex is A, B
					support_list.list[0] = A;
					support_list.list[1] = B;
					support_list.current_index--;
					direction = vec3::cross(vec3::cross(AB, AO), AB);
				}
				else {
					// simplex is A
					support_list.list[0] = A;
					support_list.current_index -= 2;
					direction = AO;
				}
			}
			else {
				// for 2D this is enough
#ifndef GJK_3D
				return true;
#else
				if (vec3::dot(ABC, AO) > 0) {
					// simplex is A, B, C
					support_list.list[0] = A;
					support_list.list[1] = B;
					support_list.list[2] = C;
					direction = ABC;
				}
				else {
					// simplex is A, C, B
					support_list.list[0] = A;
					support_list.list[1] = C;
					support_list.list[2] = B;
					direction = -ABC;
				}
#endif
			}
		}
	}
#ifdef GJK_3D
	else if (num_entries == 4) {
		vec3 A = support_list.list[3];
		vec3 B = support_list.list[2];
		vec3 C = support_list.list[1];
		vec3 D = support_list.list[0];

		vec3 AO = -A;
		vec3 AB = B - A;
		vec3 AC = C - A;
		vec3 AD = D - A;

		vec3 ABC = vec3::cross(AB, AC);

		if (vec3::dot(ABC, AO) > 0) {
			// in front of ABC
			support_list.list[0] = C;
			support_list.list[1] = B;
			support_list.list[2] = A;
			support_list.current_index--;
			direction = ABC;
			return false;
		}

		vec3 ADB = vec3::cross(AD, AB);
		if (vec3::dot(ADB, AO) > 0) {
			// in front of ADB
			support_list.list[0] = B;
			support_list.list[1] = D;
			support_list.list[2] = A;
			support_list.current_index--;
			direction = ADB;
			return false;
		}

		vec3 ACD = vec3::cross(AC, AD);
		if (vec3::dot(ACD, AO) > 0) {
			// in front of ACD
			support_list.list[0] = D;
			support_list.list[1] = C;
			support_list.list[2] = A;
			support_list.current_index--;
			direction = ACD;
			return false;
		}

		return true;	// inside the tetrahedron
	}
#endif
	return false;
}

bool gjk_collides(BoundingShape* b1, BoundingShape* b2)
{
	support_list.clear();
	vec3 search_direction(1.0f, 0.0f, 0.0f);
	vec3 support = gjk_support(b1, b2, search_direction);
	support_list.add(support);

	vec3 opposite_direction = -search_direction;

	while (true) {
		vec3 a = gjk_support(b1, b2, opposite_direction);
		float dotval = vec3::dot(a, opposite_direction);
		if (dotval < 0) {
			return false;	// there is no intersection
		}
		support_list.add(a);
		if (gjk_simplex(opposite_direction)) {
			return true;
		}
	}
}
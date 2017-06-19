#include "..\include\bounding.h"
#include <malloc.h>
#include <memory.h>
#include <limits>

bool collides(BoundingShape* b1, BoundingShape* b2)
{
	return false;
}

bool gjk_collides(BoundingShape* b1, BoundingShape* b2);
vec3 gjk_support(BoundingShape* b1, BoundingShape* b2, vec3 direction);

void DEBUG_gjk()
{
	BoundingShape b1;
	BoundingShape b2;
	b1.vertices = (vec3*)malloc(3 * sizeof(vec3));
	b2.vertices = (vec3*)malloc(4 * sizeof(vec3));
	vec3 b1_verts[3] = {
		{4.0f, 11.0f, 0.0f},
		{9.0f, 9.0f, 0.0f},
		{4.0f, 5.0f, 0.0f}
	};
	vec3 b2_verts[4] = {
		{5.0f, 5.0f, 0.0f},
		{7.0f, 3.0f, 0.0f},
		{12.0f, 7.0f, 0.0f},
		{10.0f, 2.0f, 0.0f}
	};
	memcpy(b1.vertices, b1_verts, sizeof(b1_verts));
	memcpy(b2.vertices, b2_verts, sizeof(b2_verts));
	b1.num_vertices = 3;
	b2.num_vertices = 4;
	
	//vec3 max = gjk_support(&b1, &b2, vec3(1.0f, 1.0f, 0.0f));
	bool collides = gjk_collides(&b1, &b2);
	int x = 1;
}

vec3 gjk_support(BoundingShape* b1, BoundingShape* b2, vec3 direction) 
{
	float min = -std::numeric_limits<float>::max();
	unsigned int mm = *(unsigned int*)&min;
	float max = reinterpret_cast<float>(0xff7fffff);
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

	max = -10000.0f;
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
	vec3 list[1024];
	int current_index;
	
	void add(vec3 v) {
		list[current_index] = v;
		current_index++;
	}

	void clear() {
		current_index = 0;
	}
};

SupportList support_list;

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
	return false;
}

bool gjk_collides(BoundingShape* b1, BoundingShape* b2)
{
	vec3 search_direction(1.0f, 0.0f, 0.0f);
	vec3 support = gjk_support(b1, b2, search_direction);
	support_list.add(support);

	vec3 opposite_direction = -search_direction;

	while (true) {
		vec3 a = gjk_support(b1, b2, opposite_direction);
		if (vec3::dot(a, opposite_direction) < 0) {
			return false;	// there is no intersection
		}
		support_list.add(a);
		if (gjk_simplex(opposite_direction)) {
			return true;
		}
	}
}
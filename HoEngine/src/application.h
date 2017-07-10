#pragma once
#include "ho_gl.h"
#include "hmath.h"
#include "shader.h"
#include "quaternion.h"
#include <hphysics.h>

struct Vertex3D {
	float pos[3];
	float normal[3];
	float tex[2];
};

struct IndexedModel3D {
	GLuint vao;
	GLuint vbo;
	GLuint ebo;

	Vertex3D* vertices;
	u16* indices;

	int num_vertices;
	int num_indices;

	bool is_colliding;
	int colliding_with_index = -1;

	vec3 position;
	vec3 last_pos;
	vec3 orientation;
	Quaternion rotation = Quaternion(0,0,0,1);

	float scale;
	mat4 model_matrix;

	BoundingShape bshape;
	BoundingShape bshape_temp;

	bool simulating = false;
	bool static_object = false;
	vec3 velocity = vec3(0.0f, 0.0f, 0.0f);
	float time = 0.0f;
	float mass;
};

void init_application();

void update_and_render();
#pragma once
#include "ho_gl.h"
#include "hmath.h"
#include "shader.h"
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

	vec3 position;
	mat4 model_matrix;

	BoundingShape bshape;
	BoundingShape bshape_temp;
};

void init_application();

void update_and_render();

void DEBUG_app(IndexedModel3D* m1, IndexedModel3D* m2);
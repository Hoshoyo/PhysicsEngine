#include "ho_gl.h"
#include "hmath.h"
#include "input.h"
#include "shader.h"
#include <hphysics.h>

extern Window_State win_state;

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

	vec3 position;
	mat4 model_matrix;
};

struct Model3D {
	GLuint vao;
	GLuint vbo;

	Vertex3D* vertices;
	int num_vertices;

	vec3 position;
	mat4 model_matrix;

	u32 winding_order;
};

#include "camera.cpp"
#include "load_model.cpp"

struct GameState {
	Camera camera;
	GLuint shader;

	Model3D object;
	IndexedModel3D indexed_object;
};

static GameState global_game_state = {};

void DEBUG_app()
{
	DEBUG_gjk();
}

void init_object(IndexedModel3D* m) {
	m->vertices = (Vertex3D*)malloc(4 * sizeof(Vertex3D));
	m->indices = (u16*)malloc(6 * sizeof(u16));
	m->num_indices = 6;
	m->num_vertices = 4;
	u16 inds[] = { 0, 1, 2, 2, 1, 3 };

	Vertex3D verts[] = {
		{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ {  1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
		{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
		{ {  1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }
	};

	memcpy(m->vertices, verts, 4 * sizeof(Vertex3D));
	memcpy(m->indices, inds, 6 * sizeof(u16));

	glGenVertexArrays(1, &m->vao);
	glBindVertexArray(m->vao);

	glGenBuffers(1, &m->ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m->num_indices * sizeof(u16), m->indices, GL_STATIC_DRAW);

	glGenBuffers(1, &m->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
	glBufferData(GL_ARRAY_BUFFER, m->num_vertices * sizeof(Vertex3D), m->vertices, GL_STATIC_DRAW);

	GLuint pos_attrib_loc = glGetAttribLocation(global_game_state.shader, "pos_coord");
	GLuint tex_coord_attrib_loc = glGetAttribLocation(global_game_state.shader, "tex_coord");
	GLuint normal_attrib_loc = glGetAttribLocation(global_game_state.shader, "normal_coord");
	glEnableVertexAttribArray(pos_attrib_loc);
	glEnableVertexAttribArray(normal_attrib_loc);
	glEnableVertexAttribArray(tex_coord_attrib_loc);

	glVertexAttribPointer(pos_attrib_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)0);
	glVertexAttribPointer(normal_attrib_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)&((Vertex3D*)0)->normal);
	glVertexAttribPointer(tex_coord_attrib_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)&((Vertex3D*)0)->tex);
}

void init_object(Model3D* m) {
	load_model(&global_game_state.object, (u8*)"res/cube.in");
	m->winding_order = GL_CCW;

	glGenVertexArrays(1, &m->vao);
	glBindVertexArray(m->vao);

	glGenBuffers(1, &m->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
	glBufferData(GL_ARRAY_BUFFER, m->num_vertices * sizeof(Vertex3D), m->vertices, GL_STATIC_DRAW);

	GLuint pos_attrib_loc = glGetAttribLocation(global_game_state.shader, "pos_coord");
	GLuint tex_coord_attrib_loc = glGetAttribLocation(global_game_state.shader, "tex_coord");
	GLuint normal_attrib_loc = glGetAttribLocation(global_game_state.shader, "normal_coord");
	glEnableVertexAttribArray(pos_attrib_loc);
	glEnableVertexAttribArray(normal_attrib_loc);
	glEnableVertexAttribArray(tex_coord_attrib_loc);

	glVertexAttribPointer(pos_attrib_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)0);
	glVertexAttribPointer(normal_attrib_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)&((Vertex3D*)0)->normal);
	glVertexAttribPointer(tex_coord_attrib_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)&((Vertex3D*)0)->tex);
}

void init_application()
{
	init_camera(&global_game_state.camera, (float)win_state.win_width / win_state.win_height, 45.0f, 0.5f, 100.0f);
	global_game_state.shader = load_shader(vert_shader, frag_shader, sizeof(vert_shader) - 1, sizeof(frag_shader) - 1);

	init_object(&global_game_state.object);
	init_object(&global_game_state.indexed_object);

	// opengl
	glClearColor(0.5f, 0.5f, 0.6f, 1.0f);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);
}

void render_object(IndexedModel3D* model) {
	glBindVertexArray(model->vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ebo);
	glBindBuffer(GL_ARRAY_BUFFER, model->vbo);

	glUniformMatrix4fv(glGetUniformLocation(global_game_state.shader, "persp_matrix"), 1, GL_FALSE, (GLfloat*)global_game_state.camera.projection_matrix.data);
	glUniformMatrix4fv(glGetUniformLocation(global_game_state.shader, "view_matrix"), 1, GL_FALSE, (GLfloat*)global_game_state.camera.view_matrix.data);
	mat4 ident;
	mat4::identity(ident);
	glUniformMatrix4fv(glGetUniformLocation(global_game_state.shader, "model_matrix"), 1, GL_TRUE, (GLfloat*)ident.data);

	glDrawElements(GL_TRIANGLES, model->num_indices, GL_UNSIGNED_SHORT, 0);
}

void render_object(Model3D* model)
{
	glFrontFace(model->winding_order);

	glBindVertexArray(model->vao);
	glBindBuffer(GL_ARRAY_BUFFER, model->vbo);

	glUniformMatrix4fv(glGetUniformLocation(global_game_state.shader, "persp_matrix"), 1, GL_FALSE, (GLfloat*)global_game_state.camera.projection_matrix.data);
	glUniformMatrix4fv(glGetUniformLocation(global_game_state.shader, "view_matrix"), 1, GL_FALSE, (GLfloat*)global_game_state.camera.view_matrix.data);
	mat4 ident;
	mat4::identity(ident);
	glUniformMatrix4fv(glGetUniformLocation(global_game_state.shader, "model_matrix"), 1, GL_TRUE, (GLfloat*)ident.data);

	glDrawArrays(GL_TRIANGLES, 0, global_game_state.object.num_vertices);
}

void update_and_render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(global_game_state.shader);
	
	input_camera(&global_game_state.camera);

	//render_object(&global_game_state.object);
	render_object(&global_game_state.indexed_object);

	glUseProgram(0);
}
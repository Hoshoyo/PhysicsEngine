#include "application.h"
#include "input.h"

extern Window_State win_state;

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
	IndexedModel3D indexed_object1;
	IndexedModel3D indexed_object2;
};

static GameState global_game_state = {};
__declspec(dllimport) float global_distance;
__declspec(dllimport) void(*render_vec)(vec3 v, vec3 pos);

#define NUMV1 5
#define NUMV2 4
void DEBUG_app(IndexedModel3D* model1, IndexedModel3D* model2)
{

	vec3 b1_verts[NUMV1] = {
		{ 5.0f, 7.0f, 0.0f },
		{ 7.0f, 3.0f, 0.0f },
		{ 10.0f, 2.0f, 0.0f },
		{ 12.0f, 7.0f, 0.0f },
		{ 5.0f, 5.0f, -10.0f}
	};
	vec3 b2_verts[NUMV2] = {
		{ 9.0f, 9.0f, 0.0f },
		{ 4.0f, 11.0f, 0.0f },
		{ 4.0f, 5.0f, 0.0f },
		{ 5.0f, 5.0f, -10.0f}
	};

	model1->bshape.vertices = (vec3*)malloc(NUMV1 * sizeof(vec3));
	model2->bshape.vertices = (vec3*)malloc(NUMV2 * sizeof(vec3));
	memcpy(model1->bshape.vertices, b1_verts, sizeof(b1_verts));
	memcpy(model2->bshape.vertices, b2_verts, sizeof(b2_verts));
	model1->bshape.num_vertices = NUMV1;
	model2->bshape.num_vertices = NUMV2;

	model1->bshape_temp.vertices = (vec3*)malloc(NUMV1 * sizeof(vec3));
	model2->bshape_temp.vertices = (vec3*)malloc(NUMV2 * sizeof(vec3));
	memcpy(model1->bshape_temp.vertices, b1_verts, sizeof(b1_verts));
	memcpy(model2->bshape_temp.vertices, b2_verts, sizeof(b2_verts));
	model1->bshape_temp.num_vertices = NUMV1;
	model2->bshape_temp.num_vertices = NUMV2;
}

void load_model2(IndexedModel3D* m) {
	mat4::identity(m->model_matrix);
	m->position = vec3(0.0f, 0.0f, 0.0f);
	m->rotation = vec3(0.0f, 0.0f, 0.0f);
	m->is_colliding = false;
	m->vertices = (Vertex3D*)malloc(4 * sizeof(Vertex3D));
	m->indices = (u16*)malloc(12 * sizeof(u16));
	m->num_indices = 12;
	m->num_vertices = 4;
	u16 inds[] = { 0, 1, 2, 0, 2, 3, 0, 2, 3, 2, 1, 3 };

	vec3 n1 = vec3::cross(vec3(9, 9, 0) - vec3(4, 5, 0), vec3(4, 11, 0) - vec3(4, 5, 0));

	Vertex3D verts[] = {
		{ { 4.0f, 11.0f, 0.0f },{ n1.x, n1.y, n1.z},{ 0.0f, 0.0f } },
		{ { 4.0f, 5.0f, 0.0f },{ n1.x, n1.y, n1.z },{ 1.0f, 0.0f } },
		{ { 9.0f, 9.0f, 0.0f },{ n1.x, n1.y, n1.z },{ 0.0f, 1.0f } },
		{ { 5.0f, 5.0f, -10.0f },{ 0.0f, -1.0f, 1.0f },{ 0.0f, 1.0f } },
	};

	memcpy(m->vertices, verts, 4 * sizeof(Vertex3D));
	memcpy(m->indices, inds, 12 * sizeof(u16));
}

void load_model1(IndexedModel3D* m) {
	mat4::identity(m->model_matrix);
	m->position = vec3(0.0f, 0.0f, 0.0f);
	m->rotation = vec3(0.0f, 0.0f, 0.0f);
	m->is_colliding = false;
	m->vertices = (Vertex3D*)malloc(5 * sizeof(Vertex3D));
	m->indices = (u16*)malloc(18 * sizeof(u16));
	m->num_indices = 18;
	m->num_vertices = 5;
	u16 inds[] = { 0, 1, 2, 2, 3, 0, 
				0, 4, 1, 0, 3, 4, 3, 2, 4, 2, 1, 4};

	Vertex3D verts[] = {
		{ { 5.0f, 7.0f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 0.0f, 0.0f } },
		{ { 7.0f, 3.0f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f } },
		{ { 10.0f, 2.0f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 0.0f, 1.0f } },
		{ { 12.0f, 7.0f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f } },
		{ { 5.0f, 5.0f, -10.0f },{ 0.0f, 0.0f, 1.0f },{ 0.0f, 1.0f } },
	};

	memcpy(m->vertices, verts, 5 * sizeof(Vertex3D));
	memcpy(m->indices, inds, 18 * sizeof(u16));
}

void init_object(IndexedModel3D* m) {
	
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

void render_vector(vec3 vec, vec3 position)
{
	vec4 black(0, 0, 0, 1);
	glUniform4fv(glGetUniformLocation(global_game_state.shader, "vertex_color"), 1, (float*)&black);
	glBegin(GL_LINES);
	glVertex3f(position.x, position.y, position.z);
	glVertex3f(vec.x + position.x, vec.y + position.y, vec.z + position.z);
	glEnd();
}

void init_application()
{
	render_vec = render_vector;
	init_camera(&global_game_state.camera, (float)win_state.win_width / win_state.win_height, 45.0f, 0.5f, 100.0f);
	global_game_state.camera.set_cam_position(vec3(5.0f, 5.0f, 15.0f));
	global_game_state.shader = load_shader(vert_shader, frag_shader, sizeof(vert_shader) - 1, sizeof(frag_shader) - 1);

	load_model1(&global_game_state.indexed_object1);
	init_object(&global_game_state.indexed_object1);

	load_model2(&global_game_state.indexed_object2);
	init_object(&global_game_state.indexed_object2);

	DEBUG_app(&global_game_state.indexed_object1, &global_game_state.indexed_object2);

	// opengl
	glClearColor(0.5f, 0.5f, 0.6f, 1.0f);
	//glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CCW);
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
	glUniformMatrix4fv(glGetUniformLocation(global_game_state.shader, "model_matrix"), 1, GL_TRUE, (GLfloat*)model->model_matrix.m);

	vec4 red(1.0f, 0.0f, 0.0f, 1.0f);
	vec4 green(0.0f, 1.0f, 0.0f, 1.0f);
	if (model->is_colliding) {
		glUniform4fv(glGetUniformLocation(global_game_state.shader, "vertex_color"), 1, (float*)&red);
	}
	else {
		glUniform4fv(glGetUniformLocation(global_game_state.shader, "vertex_color"), 1, (float*)&green);
	}

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

void input()
{
	float velocity = 0.1f;
	if (keyboard_state.key[VK_SHIFT]) {
		velocity = 0.005f;
	}
	if (keyboard_state.key[VK_LEFT]) {
		global_game_state.indexed_object1.position.x -= velocity;
	}
	if (keyboard_state.key[VK_RIGHT]) {
		global_game_state.indexed_object1.position.x += velocity;
	}
	if (keyboard_state.key[VK_UP]) {
		global_game_state.indexed_object1.position.y += velocity;
	}
	if (keyboard_state.key[VK_DOWN]) {
		global_game_state.indexed_object1.position.y -= velocity;
	}

	if (keyboard_state.key['X']) {
		global_game_state.indexed_object1.rotation.x += velocity * 5.0f;
	}
	if (keyboard_state.key['Y']) {
		global_game_state.indexed_object1.rotation.y += velocity * 5.0f;
	}
	if (keyboard_state.key['Z']) {
		global_game_state.indexed_object1.rotation.z += velocity * 5.0f;
	}

	vec3 rot = global_game_state.indexed_object1.rotation;
	vec3 pos = global_game_state.indexed_object1.position;
	mat4 rotx = mat4::rotate(vec3(1, 0, 0), rot.x);
	mat4 roty = mat4::rotate(vec3(0, 1, 0), rot.y);
	mat4 rotz = mat4::rotate(vec3(0, 0, 1), rot.z);
	mat4 rotation_matrix = rotx * roty * rotz;

	global_game_state.indexed_object1.model_matrix = mat4::translate(pos) * rotation_matrix;
}

void update_and_render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(global_game_state.shader);

	vec3 A(1, 0, 0);
	vec3 C(-1, 0, 0);
	vec3 B(0, 1, 0);

	vec3 res = vec3::cross(C - A, B - A);
	
	input_camera(&global_game_state.camera);
	input();

	transform_shape(&global_game_state.indexed_object1.bshape, &global_game_state.indexed_object1.bshape_temp, global_game_state.indexed_object1.model_matrix);
	global_game_state.indexed_object1.is_colliding = gjk_collides(&global_game_state.indexed_object2.bshape_temp, &global_game_state.indexed_object1.bshape_temp);
	global_game_state.indexed_object2.is_colliding = global_game_state.indexed_object1.is_colliding;

	render_object(&global_game_state.indexed_object1);
	render_object(&global_game_state.indexed_object2);

	glUseProgram(0);
}
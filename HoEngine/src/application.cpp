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
	IndexedModel3D indexed_object3;
};

static GameState global_game_state = {};
__declspec(dllimport) float global_distance;
__declspec(dllimport) void(*render_vec)(vec3 v, vec3 pos);

void load_model(char* filename, IndexedModel3D* model) {
	load_objfile(filename, model);
	mat4::identity(model->model_matrix);
	model->position = vec3(0.0f, 0.0f, 0.0f);
	model->rotation = vec3(0.0f, 0.0f, 0.0f);
	model->is_colliding = false;
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

	load_model("res/cube.obj", &global_game_state.indexed_object3);
	init_object(&global_game_state.indexed_object3);

	load_model("res/cube.obj", &global_game_state.indexed_object2);
	init_object(&global_game_state.indexed_object2);

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
	vec3 last_pos = global_game_state.indexed_object3.position;
	vec3 last_rot = global_game_state.indexed_object3.rotation;
	float velocity = 0.1f;
	bool collision = true;
	if (keyboard_state.key[VK_CONTROL]) {
		collision = false;
	}
	if (keyboard_state.key[VK_SHIFT]) {
		velocity = 0.005f;
	}
	if (keyboard_state.key[VK_LEFT]) {
		global_game_state.indexed_object3.position.x -= velocity;
	}
	if (keyboard_state.key[VK_RIGHT]) {
		global_game_state.indexed_object3.position.x += velocity;
	}
	if (keyboard_state.key[VK_UP]) {
		global_game_state.indexed_object3.position.y += velocity;
	}
	if (keyboard_state.key[VK_DOWN]) {
		global_game_state.indexed_object3.position.y -= velocity;
	}

	if (keyboard_state.key['X']) {
		global_game_state.indexed_object3.rotation.x += velocity * 5.0f;
	}
	if (keyboard_state.key['Y']) {
		global_game_state.indexed_object3.rotation.y += velocity * 5.0f;
	}
	if (keyboard_state.key['Z']) {
		global_game_state.indexed_object3.rotation.z += velocity * 5.0f;
	}

	vec3 rot = global_game_state.indexed_object3.rotation;
	vec3 pos = global_game_state.indexed_object3.position;
	mat4 rotx = mat4::rotate(vec3(1, 0, 0), rot.x);
	mat4 roty = mat4::rotate(vec3(0, 1, 0), rot.y);
	mat4 rotz = mat4::rotate(vec3(0, 0, 1), rot.z);
	mat4 rotation_matrix = rotx * roty * rotz;
	mat4 final_matrix = mat4::translate(pos) * rotation_matrix;

	transform_shape(&global_game_state.indexed_object3.bshape, &global_game_state.indexed_object3.bshape_temp, final_matrix);
	global_game_state.indexed_object2.is_colliding = gjk_collides(&global_game_state.indexed_object3.bshape_temp, &global_game_state.indexed_object2.bshape_temp);
	global_game_state.indexed_object3.is_colliding = global_game_state.indexed_object2.is_colliding;

	if (!global_game_state.indexed_object3.is_colliding || !collision) {
		global_game_state.indexed_object3.model_matrix = final_matrix;
	} else {
		global_game_state.indexed_object3.position = last_pos;
		global_game_state.indexed_object3.rotation = last_rot;
	}
}

void update_and_render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(global_game_state.shader);

	input_camera(&global_game_state.camera);
	input();

	render_object(&global_game_state.indexed_object2);
	render_object(&global_game_state.indexed_object3);

	glUseProgram(0);
}
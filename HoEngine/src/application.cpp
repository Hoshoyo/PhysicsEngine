#include "application.h"
#include "input.h"
#include "quaternion.h"

extern Window_State win_state;

#include "camera.cpp"
#include "load_model.cpp"

struct GameState {
	Camera camera;
	GLuint shader;

	//IndexedModel3D indexed_object1;
	//IndexedModel3D indexed_object2;
	//IndexedModel3D indexed_object3;

	IndexedModel3D models[3];

	bool start_simulation = false;
};

static GameState ggs = {};
__declspec(dllimport) float global_distance;
__declspec(dllimport) void(*render_vec)(vec3 v, vec3 pos);
__declspec(dllimport) void(*render_fac)(vec3 v1, vec3 v2, vec3 v3, vec3 color);
__declspec(dllimport) void(*render_lin)(vec3 p1, vec3 p2);

void load_model(char* filename, IndexedModel3D* model) {
	load_objfile(filename, model);
	mat4::identity(model->model_matrix);
	model->position = vec3(0.0f, 0.0f, 0.0f);
	model->rotation = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
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

	GLuint pos_attrib_loc = glGetAttribLocation(ggs.shader, "pos_coord");
	GLuint tex_coord_attrib_loc = glGetAttribLocation(ggs.shader, "tex_coord");
	GLuint normal_attrib_loc = glGetAttribLocation(ggs.shader, "normal_coord");
	glEnableVertexAttribArray(pos_attrib_loc);
	glEnableVertexAttribArray(normal_attrib_loc);
	glEnableVertexAttribArray(tex_coord_attrib_loc);

	glVertexAttribPointer(pos_attrib_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)0);
	glVertexAttribPointer(normal_attrib_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)&((Vertex3D*)0)->normal);
	glVertexAttribPointer(tex_coord_attrib_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)&((Vertex3D*)0)->tex);
}

void render_line(vec3 start, vec3 end) 
{
	mat4 ident;
	mat4::identity(ident);
	vec4 magenta(1, 0, 1, 1);
	glUniformMatrix4fv(glGetUniformLocation(ggs.shader, "model_matrix"), 1, GL_TRUE, (float*)ident.data);
	glUniform4fv(glGetUniformLocation(ggs.shader, "vertex_color"), 1, (float*)&magenta);
	glLineWidth(4.0f);
	glBegin(GL_LINES);
	glVertex3f(start.x, start.y, start.z);
	glVertex3f(end.x, end.y, end.z);
	glEnd();
	glLineWidth(1.0f);
}

void render_vector(vec3 vec, vec3 position)
{
	mat4 ident;
	mat4::identity(ident);
	vec4 black(0, 0, 0, 1);
	glUniformMatrix4fv(glGetUniformLocation(ggs.shader, "model_matrix"), 1, GL_TRUE, (float*)ident.data);
	glUniform4fv(glGetUniformLocation(ggs.shader, "vertex_color"), 1, (float*)&black);
	glBegin(GL_LINES);
	glVertex3f(position.x, position.y, position.z);
	glVertex3f(vec.x + position.x, vec.y + position.y, vec.z + position.z);
	glEnd();
}

void render_face(vec3 p1, vec3 p2, vec3 p3, vec3 c) {
	mat4 ident;
	mat4::identity(ident);
	vec4 color(c.r, c.g, c.b, 1);
	glUniformMatrix4fv(glGetUniformLocation(ggs.shader, "model_matrix"), 1, GL_TRUE, (float*)ident.data);
	glUniform4fv(glGetUniformLocation(ggs.shader, "vertex_color"), 1, (float*)&color);
	glDisable(GL_CULL_FACE);
	glBegin(GL_TRIANGLES);
	glVertex3f(p1.x, p1.y, p1.z);
	glVertex3f(p3.x, p3.y, p3.z);
	glVertex3f(p2.x, p2.y, p2.z);
	glEnd();

	glLineWidth(4.0f);
	glBegin(GL_LINES);
	glVertex3f(p1.x, p1.y, p1.z);
	glVertex3f(p3.x, p3.y, p3.z);
	glVertex3f(p2.x, p2.y, p2.z);
	glVertex3f(p1.x, p1.y, p1.z);
	glEnd();
	glLineWidth(1.0f);
	glEnable(GL_CULL_FACE);

	if (p1 == p2 && p2 == p3) {
		glPointSize(5.0f);
		glBegin(GL_POINTS);
		glVertex3f(p1.x, p1.y, p1.z);
		glEnd();
	}
}

void init_application()
{
	render_vec = render_vector;
	render_fac = render_face;
	render_lin = render_line;
	init_camera(&ggs.camera, (float)win_state.win_width / win_state.win_height, 45.0f, 0.2f, 1000.0f);
	ggs.camera.set_cam_position(vec3(5.0f, 30.0f, 60.0f));
	ggs.shader = load_shader(vert_shader, frag_shader, sizeof(vert_shader) - 1, sizeof(frag_shader) - 1);


	// Models
	load_model("res/esfera_10.obj", &ggs.models[0]);
	init_object(&ggs.models[0]);
	ggs.models[0].position = vec3(0.0f, 45.0f, 0.0f);
	ggs.models[0].scale = 0.3f;

	load_model("res/cube.obj", &ggs.models[1]);
	init_object(&ggs.models[1]);
	ggs.models[1].scale = 0.3f;

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

	glUniformMatrix4fv(glGetUniformLocation(ggs.shader, "persp_matrix"), 1, GL_FALSE, (GLfloat*)ggs.camera.projection_matrix.data);
	glUniformMatrix4fv(glGetUniformLocation(ggs.shader, "view_matrix"), 1, GL_FALSE, (GLfloat*)ggs.camera.view_matrix.data);
	mat4 ident;
	mat4::identity(ident);
	glUniformMatrix4fv(glGetUniformLocation(ggs.shader, "model_matrix"), 1, GL_TRUE, (GLfloat*)model->model_matrix.m);

	vec4 red(1.0f, 0.0f, 0.0f, 1.0f);
	vec4 green(0.0f, 1.0f, 0.0f, 1.0f);
	if (model->is_colliding) {
		glUniform4fv(glGetUniformLocation(ggs.shader, "vertex_color"), 1, (float*)&red);
	}
	else {
		glUniform4fv(glGetUniformLocation(ggs.shader, "vertex_color"), 1, (float*)&green);
	}

	glDrawElements(GL_TRIANGLES, model->num_indices, GL_UNSIGNED_SHORT, 0);
}

//static Quaternion totalq(0, 0, 0, 1);
void input()
{
	vec3 last_pos = ggs.models[0].position;
	float velocity = 0.1f;
	bool collision = true;
	if (keyboard_state.key[VK_CONTROL]) {
		collision = false;
	}
	if (keyboard_state.key[VK_SHIFT]) {
		velocity = 0.005f;
	}
	if (keyboard_state.key[VK_LEFT]) {
		ggs.models[0].position.x -= velocity;
	}
	if (keyboard_state.key[VK_RIGHT]) {
		ggs.models[0].position.x += velocity;
	}
	if (keyboard_state.key[VK_UP]) {
		ggs.models[0].position.y += velocity;
	}
	if (keyboard_state.key[VK_DOWN]) {
		ggs.models[0].position.y -= velocity;
	}

	if (keyboard_state.key['X']) {
		Quaternion local2 = QuatFromAxisAngle(vec3(1, 0, 0), 0.5f);
		ggs.models[0].rotation = local2 * ggs.models[0].rotation;
	}

	if (keyboard_state.key['Y']) {
		Quaternion local2 = QuatFromAxisAngle(vec3(0, 1, 0), 0.5f);
		ggs.models[0].rotation = local2 * ggs.models[0].rotation;
	}

	if (keyboard_state.key['Z']) {
		Quaternion local2 = QuatFromAxisAngle(vec3(0, 0, 1), 0.5f);
		ggs.models[0].rotation = local2 * ggs.models[0].rotation;
	}
	mat4 rot = RotFromQuat(ggs.models[0].rotation);

	vec3 pos = ggs.models[0].position;
	mat4 rotation_matrix = rot;
	mat4 scale_matrix = mat4::scale(ggs.models[0].scale);
	mat4 final_matrix = mat4::translate(pos) * rotation_matrix * scale_matrix;

	transform_shape(&ggs.models[0].bshape, &ggs.models[0].bshape_temp, final_matrix);
	ggs.models[1].is_colliding = gjk_collides(&ggs.models[0].bshape_temp, &ggs.models[1].bshape_temp, 0);
	ggs.models[0].is_colliding = ggs.models[1].is_colliding;

	if (!ggs.models[0].is_colliding || !collision) {
		ggs.models[0].model_matrix = final_matrix;
	} else {
		ggs.models[0].position = last_pos;
		ggs.models[0].rotation = ggs.models[0].rotation;
	}
}



void update(IndexedModel3D* im) {
	const float mass = 10.0f;
	static vec3 acceleration = vec3(0.0f, -10.0f, 0.0f);

	const float time_step = 1.0f / 250.0f;
	
	im->time += time_step;

	im->velocity = acceleration * im->time + im->velocity;

	im->position = im->velocity * im->time + im->position;
}


void update_and_render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(ggs.shader);

	if (ggs.start_simulation && !ggs.models[0].is_colliding) {
		ggs.models[0].last_pos = ggs.models[0].position;
		update(&ggs.models[0]);
	} else {
		vec3 pos = ggs.models[0].position;
		vec3 direction = ggs.models[0].last_pos - pos;
		if (ggs.models[0].is_colliding) {
			uncollide(direction,
				&ggs.models[0].position,
				&ggs.models[0].rotation,
				ggs.models[0].scale,
				&ggs.models[0].bshape_temp,
				&ggs.models[1].bshape_temp,
				&ggs.models[0].bshape);
			ggs.models[0].is_colliding = false;
			ggs.models[1].is_colliding = false;
		}

		ggs.models[0].velocity = vec3(0, 0, 0);
		ggs.start_simulation = false;
		ggs.models[0].time = 0.0f;
	}
	if (keyboard_state.key_event['H']) {
		ggs.start_simulation = true;
		keyboard_state.key_event['H'] = false;
	}
	
	input_camera(&ggs.camera);
	input();

	render_object(&ggs.models[0]);
	render_object(&ggs.models[1]);

	glUseProgram(0);
}
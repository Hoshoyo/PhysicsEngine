#include "application.h"
#include "input.h"
#include "quaternion.h"

extern Window_State win_state;

#include "camera.cpp"
#include "load_model.cpp"

struct GameState {
	Camera camera;
	GLuint shader;

	IndexedModel3D* models;
	int num_models;

	bool start_simulation = false;
	bool move_objects = false;
	bool wireframe = false;
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
	ggs.models = (IndexedModel3D*)malloc(sizeof(IndexedModel3D) * 3);

	load_model("res/cube.obj", &ggs.models[0]);
	init_object(&ggs.models[0]);
	ggs.models[0].position = vec3(0.0f, 45.0f, 0.0f);
	ggs.models[0].scale = 0.3f;
	ggs.models[0].simulating = true;
	ggs.models[0].static_object = false;

	load_model("res/cube.obj", &ggs.models[1]);
	init_object(&ggs.models[1]);
	ggs.models[1].scale = 1.0f;
	ggs.models[1].simulating = false;
	ggs.models[1].static_object = true;

	load_model("res/esfera_10.obj", &ggs.models[2]);
	init_object(&ggs.models[2]);
	ggs.models[2].position = vec3(-15.0f, 45.0f, 0.0f);
	ggs.models[2].scale = 0.3f;
	ggs.models[2].simulating = true;
	ggs.models[2].static_object = false;

	ggs.num_models = 3;

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
	u32 render_form = GL_TRIANGLES;
	if (ggs.wireframe) render_form = GL_LINES;
	glDrawElements(render_form, model->num_indices, GL_UNSIGNED_SHORT, 0);
}

void input()
{
	float velocity = 0.5f;
	float rot_velocity = 3.0f;
	
	if (keyboard_state.key[VK_SHIFT]) {
		velocity = 0.005f;
		rot_velocity = -3.0f;
	}
	if (keyboard_state.key[VK_LEFT]) {
		ggs.models[0].last_pos.x = ggs.models[0].position.x;
		ggs.models[0].position.x -= velocity;
	}
	if (keyboard_state.key[VK_RIGHT]) {
		ggs.models[0].last_pos.x = ggs.models[0].position.x;
		ggs.models[0].position.x += velocity;
	}
	if (keyboard_state.key[VK_UP]) {
		ggs.models[0].last_pos.y = ggs.models[0].position.y;
		ggs.models[0].position.y += velocity;
	}
	if (keyboard_state.key[VK_DOWN]) {
		ggs.models[0].last_pos.y = ggs.models[0].position.y;
		ggs.models[0].position.y -= velocity;
	}

	if (keyboard_state.key['X']) {
		Quaternion local2 = QuatFromAxisAngle(vec3(1, 0, 0), rot_velocity);
		ggs.models[0].rotation = local2 * ggs.models[0].rotation;
	}

	if (keyboard_state.key['Y']) {
		Quaternion local2 = QuatFromAxisAngle(vec3(0, 1, 0), rot_velocity);
		ggs.models[0].rotation = local2 * ggs.models[0].rotation;
	}

	if (keyboard_state.key['Z']) {
		Quaternion local2 = QuatFromAxisAngle(vec3(0, 0, 1), rot_velocity);
		ggs.models[0].rotation = local2 * ggs.models[0].rotation;
	}
	vec3 rot_vec = vec3(ggs.models[0].rotation.x, ggs.models[0].rotation.y, ggs.models[0].rotation.z);
	rot_vec = vec3::normalize(rot_vec) * 20.0f;
	render_vec(rot_vec, ggs.models[0].position);
	//render_vec(ggs.models[0].position, rot_vec);


	if (keyboard_state.key_event['H']) {
		ggs.start_simulation = !ggs.start_simulation;
		keyboard_state.key_event['H'] = false;
	}
	if (keyboard_state.key_event['M']) {
		ggs.move_objects = !ggs.move_objects;
		keyboard_state.key_event['M'] = false;
	}
	if (keyboard_state.key_event['I']) {
		ggs.wireframe = !ggs.wireframe;
		keyboard_state.key_event['I'] = false;
	}
}

void update(IndexedModel3D* im) {
	if (ggs.move_objects) return;
	static vec3 acceleration = vec3(0.0f, -10.0f, 0.0f);

	const float time_step = 1.0f / 250.0f;
	im->time += time_step;
	im->velocity = acceleration * im->time + im->velocity;
	im->position = im->velocity * im->time + im->position;
}

void update_model(IndexedModel3D* im) 
{
	bool colliding = false;

	vec3 last_pos = im->position;
	mat4 rot = RotFromQuat(im->rotation);

	vec3 pos = im->position;
	mat4 rotation_matrix = rot;
	mat4 scale_matrix = mat4::scale(im->scale);
	mat4 final_matrix = mat4::translate(pos) * rotation_matrix * scale_matrix;

	transform_shape(&im->bshape, &im->bshape_temp, final_matrix);

	if (im->simulating) {
		for (int i = 0; i < ggs.num_models; ++i) {
			if (ggs.models + i == im) continue;

			colliding = gjk_collides(&im->bshape_temp, &ggs.models[i].bshape_temp, 0);
			ggs.models[i].is_colliding = colliding;
			if (colliding) {
				im->colliding_with_index = i;
				im->is_colliding = true;
				ggs.models[i].is_colliding = true;
				break;
			} else {
				im->colliding_with_index = -1;
			}
		}
	}
	im->is_colliding = colliding;
	im->model_matrix = final_matrix;
}

void update_and_render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(ggs.shader);
	
	input_camera(&ggs.camera);
	input();

	bool resolve_collision = false;
	int collision_responsible = 0;
	for (int i = 0; i < ggs.num_models; ++i) {
		update_model(ggs.models + i);
		render_object(ggs.models + i);
		if (ggs.models[i].is_colliding) {
			resolve_collision = true;
			collision_responsible = i;
		}
	}
	if (ggs.start_simulation && !resolve_collision) {
		int num_simulating = 0;
		for (int i = 0; i < ggs.num_models; ++i) {
			if (!ggs.models[i].simulating) continue;
			ggs.models[i].last_pos = ggs.models[i].position;
			update(&ggs.models[i]);
			num_simulating++;
		}
		if (num_simulating == 0) {
			for (int i = 0; i < ggs.num_models; ++i) {
				if (!ggs.models[i].static_object) ggs.models[i].simulating = true;
			}
			ggs.start_simulation = false;
		}
	} else {
		if (!ggs.move_objects) {
			if (resolve_collision) {
				int x = 0;
			}
			for (int i = 0; i < ggs.num_models; ++i) {
				if (!ggs.models[i].simulating) continue;
				if (collision_responsible != i && ggs.start_simulation) continue;

				int coll_index = ggs.models[i].colliding_with_index;
				coll_index = (coll_index < 0) ? 0 : coll_index;
				vec3 pos = ggs.models[i].position;
				vec3 direction = ggs.models[i].last_pos - pos;
				if (ggs.models[i].is_colliding) {
					uncollide(direction,
						&ggs.models[i].position,
						&ggs.models[i].rotation,
						ggs.models[i].scale,
						&ggs.models[i].bshape_temp,
						&ggs.models[coll_index].bshape_temp,
						&ggs.models[i].bshape);
					ggs.models[i].is_colliding = false;
					ggs.models[coll_index].is_colliding = false;
					ggs.models[i].simulating = false;
				}
				ggs.models[i].velocity = vec3(0, 0, 0);
				ggs.models[i].time = 0.0f;
			}
		}
	}

	glUseProgram(0);
}
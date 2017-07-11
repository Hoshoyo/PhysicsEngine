#include "..\include\particle.h"

using namespace hm;

std::vector<Particle*> particles;

Particle::Particle()
{
	position = vec3(0, 0, 0);
	velocity = vec3(0, 0, 0);
	gravity = vec3(0, -50.0f, 0);
	lifetime = 5.0f;
	rotation = 0.0f;
	scale = 0.3f;
	elapsed = 0.0f;
}

Particle::Particle(vec3 position, vec3 velocity, float life, float scale)
{
	this->position = position;
	this->velocity = velocity;
	gravity = vec3(0, -50.0f, 0);
	lifetime = life;
	rotation = 0.0f;
	this->scale = scale;
	elapsed = 0.0f;
}

Particle::~Particle() {
}

bool Particle::update(float delta) {
	velocity = velocity + (gravity * delta);
	vec3 change = velocity * delta;
	position = position + change;
	elapsed += delta;
	return elapsed < lifetime;
}

void ParticleEmitter::update() {
	const float delta = 1.0f / 60.0f;
	for (int i = 0; i < particles.size(); ++i) {
		bool is_alive = particles[i]->update(delta);
		if (!is_alive) {
			Particle* p = particles[i];
			delete p;
			particles.erase(particles.begin() + i);
		}
	}
}

ParticleEmitter::ParticleEmitter(hm::vec3 emitter_pos, float delta_time, float particles_ps, float average_life)
{
	this->emitter_pos = emitter_pos;
	this->delta_time = delta_time;
	this->particles_ps = particles_ps;
	this->average_life = average_life;
	this->gravity = vec3(0, -50.0f, 0);

	velocity = vec3(0.0f, 10.0f, 0.0f);
	velocity_variation = vec3(40.0f, 40.0f, 40.0f);
}

void ParticleEmitter::create_particle(vec3 pos, vec3 velocity, float life, float scale) {
	Particle* p = new Particle(pos, velocity, life, scale);
	p->gravity = gravity;
	particles.push_back(p);
}

void ParticleEmitter::render(render_particle_func func) {
	for (int i = 0; i < particles.size(); ++i) {
		func(particles[i]->position, particles[i]->scale);
	}
}

void ParticleEmitter::emitt()
{
	float max = delta_time * particles_ps;
	int max_int = (int)(max * 100.0f);
	int val = rand() % 100;
	if (val > max_int) return;

	for (int i = 0; i < max; ++i) {

		float rx = (float)(rand() % 1000) / 1000.0f;
		float ry = (float)(rand() % 1000) / 1000.0f;
		float rz = (float)(rand() % 1000) / 1000.0f;
		float half_var_x = velocity_variation.x / 2.0f;
		float half_var_y = velocity_variation.y / 2.0f;
		float half_var_z = velocity_variation.z / 2.0f;

		rx = rx * velocity_variation.x - half_var_x;
		ry = ry * velocity_variation.y - half_var_y;
		rz = rz * velocity_variation.z - half_var_z;

		rx += velocity.x;
		ry += velocity.y;
		rz += velocity.z;

		float life = (float)(rand() % 1000) / 1000.0f;
		life *= average_life;
		float scale = (float)(rand() % 100) / 1000.0f;

		create_particle(emitter_pos, vec3(rx, ry, rz), life, scale);
	}
}
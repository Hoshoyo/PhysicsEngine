#pragma once
#include <hmath.h>
#include <vector>

struct ParticleEmitter;

struct Particle {
	hm::vec3 position;
	hm::vec3 velocity;
	hm::vec3 gravity;
	float lifetime;
	float rotation;
	float scale;

	float elapsed;

	__declspec(dllexport) Particle();
	__declspec(dllexport) Particle(hm::vec3 position, hm::vec3 velocity, float life, float scale);
	~Particle();

	bool update(float delta);
};

typedef void render_particle_func(hm::vec3, float);

struct ParticleEmitter {
	hm::vec3 emitter_pos;
	float delta_time;
	bool emitting;
	float particles_ps;
	float average_life;
	hm::vec3 gravity;
	hm::vec3 velocity;
	hm::vec3 velocity_variation;

	__declspec(dllexport) ParticleEmitter(hm::vec3 emitter_pos, float delta_time, float particles_ps, float average_life);

	__declspec(dllexport) void update();
	__declspec(dllexport) void create_particle(hm::vec3 pos, hm::vec3 velocity, float life, float scale);
	__declspec(dllexport) void render(render_particle_func);
	__declspec(dllexport) void emitt();
};
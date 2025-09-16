/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#if !defined( PARTICLEDAN_H )
#define PARTICLEDAN_H
#ifdef _WIN32
#pragma once
#endif

#include <iostream>
#include <array>
#include <vector>

#define PDAN_ABSOLUTE_MIN_PARTICLES 512
#define PDAN_MAX_PARTICLES 32768

#define	PDAN_ANIMATE_DIE					0x00000001 // Particle dies after animation finishes.
#define	PDAN_COLLIDE_WORLD					0x00000002 // Particle can collide w/ the world.
#define	PDAN_COLLIDE_KILL					0x00000004 // Particle dies upon touching the world.
#define PDAN_WATER_KILL						0x00000008 // Particle is allergic to water.
#define PDAN_ANIMATED_ALPHA					0x00000010 // Particle can have user-defined keyframes for alpha.
#define PDAN_ANIMATED_SCALE					0x00000020 // Particle can have user-defined keyframes for scale.
#define PDAN_GROWTH							0x00000040 // Particles can grow over time using pre-defined step and growth values.

typedef struct particledan_s
{
	vec3_t		org;			// Origin of particles.
	vec3_t		vel;			// Velocity of particles.
	vec3_t		color;			// Color of particles. Must be divided by 255!
	float		alpha;			// Alpha/Transparency of particles. Must be divided by 255!
	float		brightness;		// Color multiplier for particles.

	float		die;			// Lifetime of particles.
	float		gravity;		// Gravity of particles.
	std::array<float, 2> scale;		// Scale.

	int			flags;			// Flags to help adjust particles.
	int			frame;			// The particle's current frame.
	int			max_frames;		// Number of frames for looped animation s.
	float		framerate;		// Framerate of sprite animations

	float		scale_step;					// Multiplier for frametime scaling.
	float		alpha_step;					// Multiplier for frametime alpha values.
	std::array<float, 5> scale_keyframe[2];	// "Keyframe-able" scaling.
	std::array<float, 5> alpha_keyframe;	// "Keyframe-able" transparency.
	std::array<float, 2> growth_max;		// Growth rate for particles.

	struct		model_s* model;	// The sprite for particles to use.
	int			rendermode;		// Rendermode used by particles.

	// I wouldn't touch these if I were you...
	struct		particledan_s* next;	// r_part.c thing..
	float		nextanimate;			// Timer for next time something animates.
	float		scale_time;				// frametime scaling.
	float		alpha_time;				// frametime alpha values.
	float		part_freq;				// freq of particles...
	vec3_t		oldorg;					// OLD ORIGIN
} particledan_t;

class CParticleDan
{
public:
	int Init(void);
	int VidInit(void);
	void Shutdown(void);
	void InitParticles(void);
	void ClearParticles(void);
	particledan_t* AllocParticle(void);
	particledan_t* AllocParticleDelay(float delay);
	void ParticleThink(float frametime, float realtime);
	void DrawParticle(particledan_t* p);
};

#endif

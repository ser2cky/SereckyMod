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

#define PDAN_ABSOLUTE_MIN_PARTICLES 512
#define PDAN_MAX_PARTICLES 8096

#define	PDAN_ANIMATE_DIE					0x00000000 // Particle dies after animation finishes.
#define	PDAN_COLLIDE_WORLD					0x00000001 // Particle can collide w/ the world.
#define	PDAN_COLLIDE_KILL					0x00000002 // Particle dies upon touching the world.
#define PDAN_WATER_KILL						0x00000003 // Particle is allergic to water.

typedef struct particledan_s
{
	vec3_t		org;			// Origin of particles.
	vec3_t		vel;			// Velocity of particles.
	vec3_t		color;			// Color of particles. Must be divided by 255!
	float		alpha;			// Alpha/Transparency of particles. Must be divided by 255!

	float		die;			// Lifetime of particles.
	float		gravity;		// Gravity of particles.
	float		scale[2];		// Scale.

	int			flags;			// Flags to help adjust particles.
	int			frame;			// The particle's current frame.
	int			max_frames;		// Number of frames for looped animations.
	float		framerate;		// Framerate of sprite animations

	struct		model_s* model;	// The sprite for particles to use.
	int			rendermode;		// Rendermode used by particles.

	// I wouldn't touch these if I were you...
	struct		particledan_s* next;	
	float		nextthink;
} particledan_t;

class CParticleDan
{
public:
	int Init(void);
	int VidInit(void);
	void InitParticles(void);
	void ClearParticles(void);
	particledan_t* AllocParticle(void);
	void ParticleThink(float frametime, float realtime);
	void DrawParticle(particledan_t* p);
private:
};

#endif

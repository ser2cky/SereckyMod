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

/*
=============================================
	particledan.cpp
	Roided up Quake particles on crack.

	History:

	9/3/25:
	Created.

	9/3/25-9/12/25:
	Particle system is pretty decent
	now. The only issues it has now, is
	the inability for it to rotate, and
	create trails.
=============================================
*/

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "particledan.h"
#include "r_efx.h"

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <vector>

#include "triangleapi.h"

static particledan_t *active_particles, *free_particles;
static particledan_t *particles = NULL;

static float flNextAnimateTime = 0.0f;
static float flServerGravity = 800.0f;

int r_numparticles = PDAN_MAX_PARTICLES;
extern vec3_t v_angles;

float Lerp(float a2, float a1, float frac)
{
	return a2 + frac * (a1 - a2);
}

/*
======================================
InitParticles
======================================
*/

int CParticleDan::Init(void)
{
	InitParticles();

	return 1;
}

/*
======================================
InitParticles
======================================
*/

int CParticleDan::VidInit(void)
{
	ClearParticles();

	return 1;
}

/*
======================================
InitParticles
======================================
*/

void CParticleDan::InitParticles(void)
{
	particles = (particledan_t*)malloc(r_numparticles * sizeof(particledan_t));
	ClearParticles();
}

/*
======================================
ClearParticles
======================================
*/

void CParticleDan::ClearParticles(void)
{
	int		i;

	free_particles = &particles[0];
	active_particles = NULL;

	for (i = 0; i < r_numparticles; i++)
		particles[i].next = &particles[i + 1];
	particles[r_numparticles - 1].next = NULL;
}

/*
======================================
Shutdown
======================================
*/

void CParticleDan::Shutdown(void)
{
	ClearParticles();
	if (particles)
		free(particles);
	particles = NULL;
}

/*
======================================
AllocParticle
======================================
*/

particledan_t* CParticleDan::AllocParticle(void)
{
	particledan_t* p;

	if (!free_particles)
	{
		gEngfuncs.Con_Printf("particleDan: Sorry! No free particles! Limit is: 32768\nPlease optimize particle usage! Thanks!\n");
		return NULL;
	}

	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;

	//gEngfuncs.Con_Printf("particleDan: particle allocated!\n");

	// clear old particle
	VectorClear(p->vel);
	VectorClear(p->org);
	p->die = gEngfuncs.GetClientTime();
	p->model = NULL;
	p->rendermode = 0;
	p->color = {0, 0, 0};
	p->brightness = 1.0f;

	p->frame = 0;
	p->max_frames = 0;
	p->framerate = 1.0f;
	p->nextanimate = 0.0f;
	p->gravity = 0.0f;

	p->alpha = 0.0f;
	p->alpha_time = 0.0f;
	p->alpha_step = 0.0f;
	p->alpha_keyframe = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

	p->scale = {0.0f, 0.0f};
	p->scale_time = 0.0f;
	p->scale_step = 0.0f;
	p->scale_keyframe[0] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	p->scale_keyframe[1] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	p->growth_max = {0.0f, 0.0f};

	p->part_freq = 0.0f;
	p->flags = 0;
	return p;
}

/*
======================================
AllocParticleDelay
	I tried getting this to work..
	but it doesn't and I'm
	too lazy to fix it!
	- serecky 9.14.25
======================================
*/

particledan_t* CParticleDan::AllocParticleDelay(float delay)
{
	particledan_t* p = AllocParticle();

	if (p)
	{
		if (p->part_freq < gEngfuncs.GetClientTime() || p->part_freq - gEngfuncs.GetClientTime() > delay)
		{
			p->part_freq = gEngfuncs.GetClientTime() + delay;
			return p;
		}
	}

	return NULL;
}

/*
======================================
Draw
======================================
*/

void CParticleDan::ParticleThink(float frametime, float realtime)
{
	particledan_t *p, *kill;
	vec3_t delta;

	if (frametime <= 0.0f)
		return;
	//gEngfuncs.pfnConsolePrint(UTIL_VarArgs("Frametime: %f\n", frametime));

	for (;; )
	{
		kill = active_particles;
		if (kill && kill->die < realtime)
		{
			active_particles = kill->next;
			kill->next = free_particles;
			free_particles = kill;
			//gEngfuncs.Con_Printf("particleDan: killed a particle!\n");
			continue;
		}   
		break;
	}

	for (p = active_particles; p; p = p->next)
	{
		for (;; )
		{
			kill = p->next;
			if (kill && kill->die < realtime)
			{
				p->next = kill->next;
				kill->next = free_particles;
				free_particles = kill;
				//gEngfuncs.Con_Printf("particleDan: killed a particle!\n");
				continue;
			}
			break;
		}

		DrawParticle(p);

		VectorCopy(p->org, p->oldorg);

		if ((p->flags & PDAN_GROWTH) != 0)
		{
			p->scale[0] = Lerp(p->scale[0], p->growth_max[0], frametime * p->scale_step);
			p->scale[1] = Lerp(p->scale[1], p->growth_max[1], frametime * p->scale_step);
		}

		if ((p->flags & PDAN_ANIMATED_ALPHA) != 0)
		{
			if (!p->alpha_time)
				p->alpha_time = 0.0f;
			p->alpha = Lerp(p->alpha, p->alpha_keyframe[(int)p->alpha_time], frametime * p->alpha_step);
			//p->alpha_time = fmod(p->alpha_time + (frametime * p->alpha_step), 4);
			p->alpha_time = min(p->alpha_time + (frametime * p->alpha_step), 4);
		}

		if ((p->flags & PDAN_ANIMATED_SCALE) != 0)
		{
			if (!p->scale_time)
				p->scale_time = 0.0f;

			p->scale[0] = Lerp(p->scale[0], p->scale_keyframe[0][(int)p->scale_time], frametime * p->scale_step);
			p->scale[1] = Lerp(p->scale[1], p->scale_keyframe[1][(int)p->scale_time], frametime * p->scale_step);
			//p->scale_time = fmod(p->scale_time + (frametime * p->scale_step), 4);
			p->scale_time = min(p->scale_time + (frametime * p->scale_step), 4);
		}

		// Added to prevent division by zero.
		if (p->framerate < 1)
			p->framerate = 1;

		if (p->gravity > 0.0f)
			p->vel[2] -= frametime * p->gravity; // Apply gravity.
		else
			p->vel[2] += frametime * p->gravity; // Apply gravity.

		// Loop frames unless PDAN_ANIMATE_DIE is set.
		if (p->max_frames > 0)
		{
			if (p->nextanimate < realtime)
			{
				if (((p->frame + 1) == p->max_frames) && ((p->flags & PDAN_ANIMATE_DIE) != 0))
				{
					//gEngfuncs.Con_Printf("particleDan: finished animating!\n");
					p->die = realtime;
				}
				p->frame = (p->frame + 1) % p->max_frames;
				p->nextanimate = realtime + (1 / p->framerate);
			}
		}

		// Check to see if we're hitting a solid, and apply the appropriate flag effects!
		if ( (gEngfuncs.PM_PointContents(p->org, NULL) != CONTENTS_EMPTY))
		{
			if ((p->flags & PDAN_COLLIDE_WORLD) != 0)
			{
				p->vel = Vector(0, 0, 0);
				p->gravity = 0.0f;
			}

			if ((p->flags & (PDAN_COLLIDE_KILL)) != 0)
			{
				gEngfuncs.Con_Printf("particleDan: A particle hit the world and died!\n");
				p->die = realtime;
				return;
			}
		}

		p->org[0] += p->vel[0] * frametime;
		p->org[1] += p->vel[1] * frametime;
		p->org[2] += p->vel[2] * frametime;

		//gEngfuncs.pfnConsolePrint(UTIL_VarArgs("OLD: %.2f %.2f %.2f\n", p->oldorg[0], p->oldorg[1], p->oldorg[2]));
		//gEngfuncs.pfnConsolePrint(UTIL_VarArgs("new: %.2f %.2f %.2f\n", p->org[0], p->org[1], p->org[2]));
	}
}

/*
======================================
DrawParticle
======================================
*/

void CParticleDan::DrawParticle(particledan_t* p)
{
	vec3_t angles, forward, right, up;

	angles = v_angles;
	AngleVectors(angles, forward, right, up);

	// Scale funnies.
	VectorScale(right, p->scale[0], right);
	VectorScale(up, p->scale[1], up);

	// START RENDERING!!!
	gEngfuncs.pTriAPI->RenderMode(p->rendermode);
	gEngfuncs.pTriAPI->Color4f(p->color[0], p->color[1], p->color[2], p->alpha);
	gEngfuncs.pTriAPI->Brightness(p->brightness);

	gEngfuncs.pTriAPI->SpriteTexture(p->model, p->frame);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad

	//top left
	gEngfuncs.pTriAPI->TexCoord2f(0, 0);
	gEngfuncs.pTriAPI->Vertex3f( p->org[0] - right[0] + up[0] ,  p->org[1] - right[1] + up[1] ,  p->org[2] - right[2] + up[2] );

	//bottom left
	gEngfuncs.pTriAPI->TexCoord2f(0, 1);
	gEngfuncs.pTriAPI->Vertex3f( p->org[0] - right[0] - up[0] ,  p->org[1] - right[1] - up[1] ,  p->org[2] - right[2] - up[2] );

	//bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1, 1);
	gEngfuncs.pTriAPI->Vertex3f( p->org[0] + right[0] - up[0] , p->org[1] + right[1] - up[1] , p->org[2] + right[2] - up[2] );

	//top right
	gEngfuncs.pTriAPI->TexCoord2f(1, 0);
	gEngfuncs.pTriAPI->Vertex3f( p->org[0] + right[0] + up[0] , p->org[1] + right[1] + up[1] , p->org[2] + right[2] + up[2] );

	gEngfuncs.pTriAPI->End(); //end our list of vertexes
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
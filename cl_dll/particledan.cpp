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
=============================================
*/

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "particledan.h"

#include <string.h>
#include <stdio.h>

#include "triangleapi.h"

static particledan_t *active_particles, *free_particles;
static particledan_t *particles = NULL;

static float flNextAnimateTime = 0.0f;
static float flServerGravity = 800.0f;

int r_numparticles;
extern vec3_t v_angles;

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
	int		i;

	//i = COM_CheckParm("-particles");

	//if (i)
	//{
		r_numparticles += 1;
		if (r_numparticles < PDAN_ABSOLUTE_MIN_PARTICLES)
			r_numparticles = PDAN_ABSOLUTE_MIN_PARTICLES;
	//}
//	else
	//{
	//	r_numparticles = MAX_PARTICLES;
	//}
	particles = (particledan_t*)malloc(r_numparticles * sizeof(particledan_t));
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
AllocParticle
======================================
*/

particledan_t* CParticleDan::AllocParticle(void)
{
	particledan_t* p;

	if (!free_particles)
	{
		gEngfuncs.Con_Printf("particleDan: Sorry! No free particles! Limit is: 8192\n");
		return NULL;
	}

	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;

	gEngfuncs.Con_Printf("particleDan: particle allocated!\n");

	// clear old particle
	VectorClear(p->vel);
	VectorClear(p->org);
	p->die = gEngfuncs.GetClientTime();
	p->model = NULL;
	p->rendermode = 0;
	p->color[0] = 0.0f;
	p->color[1] = 0.0f;
	p->color[2] = 0.0f;
	p->alpha = 0.0f;
	p->gravity = 0.0f;
	p->scale[0] = 0.0f;
	p->scale[1] = 0.0f;
	p->frame = 0;
	p->max_frames = 0;
	p->framerate = 1.0f;
	p->flags = 0;
	p->nextthink = 0;

	return p;
}

/*
======================================
Draw
======================================
*/

void CParticleDan::ParticleThink(float frametime, float realtime)
{
	particledan_t *p, *kill;

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

		// Added to prevent division by zero.
		if (p->framerate < 1)
			p->framerate = 1;

		p->vel[2] -= frametime * p->gravity; // Apply gravity.
		p->scale[0] += frametime * 50.0f; // Apply x-scale.
		p->scale[1] += frametime * 50.0f; // Apply y-scale.

		// Loop frames unless PDAN_ANIMATE_DIE is set.
		if (p->max_frames > 0)
		{
			if (p->nextthink < realtime)
			{
				if (p->frame == p->max_frames && (p->flags & PDAN_ANIMATE_DIE))
				{
					//gEngfuncs.Con_Printf("particleDan: finished animating!\n");
					p->die = realtime;
				}

				p->frame = (p->frame + 1) % p->max_frames;
				p->nextthink = realtime + (1 / p->framerate);
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
	gEngfuncs.pTriAPI->Brightness(1.0f);
	gEngfuncs.pTriAPI->Color4f(p->color[0], p->color[1], p->color[2], p->alpha);

	gEngfuncs.pTriAPI->SpriteTexture(p->model, p->frame);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad

	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);
	gEngfuncs.pTriAPI->Vertex3f(p->org[0] - right[0] + up[0], p->org[1] - right[1] + up[1], p->org[2] - right[2] + up[2]);

	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);
	gEngfuncs.pTriAPI->Vertex3f(p->org[0] + right[0] + up[0], p->org[1] + right[1] + up[1], p->org[2] + right[2] + up[2]);

	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);
	gEngfuncs.pTriAPI->Vertex3f(p->org[0] + right[0] - up[0], p->org[1] + right[1] - up[1], p->org[2] + right[2] - up[2]);

	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);
	gEngfuncs.pTriAPI->Vertex3f(p->org[0] - right[0] - up[0], p->org[1] - right[1] - up[1], p->org[2] - right[2] - up[2]);

	gEngfuncs.pTriAPI->End(); //end our list of vertexes
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
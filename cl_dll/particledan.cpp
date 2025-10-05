/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	Copyright (c) 2025 Serecky
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

	10/4/25: Added PDAN_QUAKE flag to the
	ParticleDan system... It's for Q2
	particles since they're a little different
	compared to say, Q1 particles, which
	are tied to the Quake1 pallete on the
	engine's side for some ungodly reason,
	and can't have transparencies :(
=============================================
*/

#include "hud.h"
#include "cl_util.h"
#include "triangleapi.h"
#include "com_model.h"

#include "particledan.h"
#include "gl/glew.h"

#include "triangleapi.h"
#include "r_studioint.h"

extern engine_studio_api_t IEngineStudio;

static particledan_t *active_particles, *free_particles;
static particledan_t *particles = NULL;

static float flNextAnimateTime = 0.0f;
static float flServerGravity = 800.0f;

int r_numparticles = PDAN_MAX_PARTICLES;

extern vec3_t v_angles;
extern vec3_t v_origin;

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
	float gravity  = gHUD.m_pSV_Gravity->value ? gHUD.m_pSV_Gravity->value : 800.0f;

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

		if ((p->flags & PDAN_QUAKE) != 0)
		{
			if (p->alpha_step != INSTANT_PARTICLE)
				p->alpha += frametime * p->alpha_step;
			if (p->alpha <= 0.0f)
				p->die = 0.0f;
		}

		if ((p->flags & PDAN_GROWTH) != 0)
		{
			p->scale[0] = Interpolate(p->scale[0], p->growth_max[0], frametime * p->scale_step);
			p->scale[1] = Interpolate(p->scale[1], p->growth_max[1], frametime * p->scale_step);
		}

		if ((p->flags & PDAN_ANIMATED_ALPHA) != 0)
		{
			if (!p->alpha_time)
				p->alpha_time = 0.0f;
			p->alpha = Interpolate(p->alpha, p->alpha_keyframe[(int)p->alpha_time], frametime * p->alpha_step);
			//p->alpha_time = fmod(p->alpha_time + (frametime * p->alpha_step), 4);
			p->alpha_time = min(p->alpha_time + (frametime * p->alpha_step), 4);
		}

		if ((p->flags & PDAN_ANIMATED_SCALE) != 0)
		{
			if (!p->scale_time)
				p->scale_time = 0.0f;

			p->scale[0] = Interpolate(p->scale[0], p->scale_keyframe[0][(int)p->scale_time], frametime * p->scale_step);
			p->scale[1] = Interpolate(p->scale[1], p->scale_keyframe[1][(int)p->scale_time], frametime * p->scale_step);
			//p->scale_time = fmod(p->scale_time + (frametime * p->scale_step), 4);
			p->scale_time = min(p->scale_time + (frametime * p->scale_step), 4);
		}

		// Added to prevent division by zero.
		if (p->framerate < 1)
			p->framerate = 1;

		p->vel[2] -= frametime * gravity * p->gravity; // Apply gravity.

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

		//gEngfuncs.pfnConsolePrint(UTIL_VarArgs("new: %.2f %.2f %.2f\n", p->org[0], p->org[1], p->org[2]));
	}
}

/*
======================================
DrawParticle
======================================
*/

void CParticleDan::DrawQParticle(particledan_t* p)
{
	vec3_t angles, forward, right, up;

	angles = v_angles;
	AngleVectors(angles, forward, right, up);

	float scale;

	VectorScale(right, 1.5f, right);
	VectorScale(up, 1.5f, up);

	// hack a scale up to keep particles from disapearing
	scale = (p->org[0] - v_origin[0]) * forward[0] + (p->org[1] - v_origin[1]) * forward[1]
		+ (p->org[2] - v_origin[2]) * forward[2];

	if (scale < 20)
		scale = 1;
	else
		scale = 1 + scale * 0.004;

	// For some reason I couldn't get transparencies to work in OpenGL mode,
	// so I'm just gonna call OpenGL32 directly :p - serecky 10.4.25
	if (IEngineStudio.IsHardware())
	{
		if (!gEngfuncs.GetSpritePointer(SPR_Load("sprites/particle.spr")))
			return;

		gEngfuncs.pTriAPI->RenderMode(kRenderTransTexture);
		gEngfuncs.pTriAPI->SpriteTexture((struct model_s*)
			gEngfuncs.GetSpritePointer(SPR_Load("sprites/particle.spr")), 0);
		glDepthMask(GL_FALSE);		// no z buffering
		glEnable(GL_BLEND);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glBegin(GL_TRIANGLES);

		glColor4f(p->color[0], p->color[1], p->color[2], p->alpha);
		glTexCoord2f(0, 0);
		glVertex3fv(p->org);
		glTexCoord2f(1, 0);
		glVertex3f(p->org[0] + up[0] * scale, p->org[1] + up[1] * scale, p->org[2] + up[2] * scale);
		glTexCoord2f(0, 1);
		glVertex3f(p->org[0] + right[0] * scale, p->org[1] + right[1] * scale, p->org[2] + right[2] * scale);

		glEnd();
		glDisable(GL_BLEND);
		glDepthMask(1);		// back to normal Z buffering
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
	}
	else
	{
		if (!gEngfuncs.GetSpritePointer(SPR_Load("sprites/white.spr")))
			return;

		gEngfuncs.pTriAPI->RenderMode(kRenderTransTexture);
		gEngfuncs.pTriAPI->SpriteTexture((struct model_s*)
			gEngfuncs.GetSpritePointer(SPR_Load("sprites/white.spr")), 0);
		gEngfuncs.pTriAPI->Color4f(p->color[0], p->color[1], p->color[2], p->alpha);
		gEngfuncs.pTriAPI->Brightness(p->brightness);
		gEngfuncs.pTriAPI->CullFace(TRI_NONE);
		gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
		//top left
		gEngfuncs.pTriAPI->TexCoord2f(0, 0);
		gEngfuncs.pTriAPI->Vertex3f(p->org[0] - right[0] + up[0], p->org[1] - right[1] + up[1], p->org[2] - right[2] + up[2]);
		//bottom left
		gEngfuncs.pTriAPI->TexCoord2f(0, 1);
		gEngfuncs.pTriAPI->Vertex3f(p->org[0] - right[0] - up[0], p->org[1] - right[1] - up[1], p->org[2] - right[2] - up[2]);
		//bottom right
		gEngfuncs.pTriAPI->TexCoord2f(1, 1);
		gEngfuncs.pTriAPI->Vertex3f(p->org[0] + right[0] - up[0], p->org[1] + right[1] - up[1], p->org[2] + right[2] - up[2]);
		//top right
		gEngfuncs.pTriAPI->TexCoord2f(1, 0);
		gEngfuncs.pTriAPI->Vertex3f(p->org[0] + right[0] + up[0], p->org[1] + right[1] + up[1], p->org[2] + right[2] + up[2]);

		gEngfuncs.pTriAPI->End();
		gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
	}
}

void CParticleDan::DrawParticle(particledan_t* p)
{
	if ((p->flags & PDAN_QUAKE) != 0)
	{
		DrawQParticle(p);
		return;
	}

	vec3_t angles, forward, right, up;

	angles = v_angles;
	AngleVectors(angles, forward, right, up);

	// Scale funnies.
	VectorScale(right, p->scale[0], right);
	VectorScale(up, p->scale[1], up);

	// START RENDERING!!!
	if (!gEngfuncs.GetSpritePointer(SPR_Load(p->model)))
	{
		gEngfuncs.Con_Printf("particleDan: Can't get sprite! Check to see if it's in \"sprites\" folder!\n");
		return;
	}

	gEngfuncs.pTriAPI->RenderMode(p->rendermode);
	gEngfuncs.pTriAPI->SpriteTexture((struct model_s*)
		gEngfuncs.GetSpritePointer(SPR_Load(p->model)), p->frame);
	gEngfuncs.pTriAPI->Color4f(p->color[0], p->color[1], p->color[2], p->alpha);
	gEngfuncs.pTriAPI->Brightness(p->brightness);
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
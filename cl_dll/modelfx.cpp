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

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "particledan.h"
#include "r_efx.h"
#include "modelfx.h"

/*
=============================================
	modelfx.cpp

	Effects that I can reuse pretty much
	everywhere.

	History:
	10/4/25: Created.
=============================================
*/

vec3_t q2pal[256] = { {0, 0, 0}, { 15, 15, 15 }, { 31, 31, 31 }, { 47, 47, 47 }, { 63, 63, 63 },
 { 75, 75, 75 }, { 91, 91, 91 }, { 107, 107, 107 }, { 123, 123, 123 }, { 139, 139, 139 },
 { 155, 155, 155 }, { 171, 171, 171 }, { 187, 187, 187 }, { 203, 203, 203 }, { 219, 219, 219 },
 { 235, 235, 235 }, { 99, 75, 35 }, { 91, 67, 31 }, { 83, 63, 31 }, { 79, 59, 27 }, { 71, 55, 27 },
 { 63, 47, 23 }, { 59, 43, 23 }, { 51, 39, 19 }, { 47, 35, 19 }, { 43, 31, 19 }, { 39, 27, 15 },
 { 35, 23, 15 }, { 27, 19, 11 }, { 23, 15, 11 }, { 19, 15, 7 }, { 15, 11, 7 }, { 95, 95, 111 },
 { 91, 91, 103 }, { 91, 83, 95 }, { 87, 79, 91 }, { 83, 75, 83 }, { 79, 71, 75 }, { 71, 63, 67 },
 { 63, 59, 59 }, { 59, 55, 55 }, { 51, 47, 47 }, { 47, 43, 43 }, { 39, 39, 39 }, { 35, 35, 35 },
 { 27, 27, 27 }, { 23, 23, 23 }, { 19, 19, 19 }, { 143, 119, 83 }, { 123, 99, 67 }, { 115, 91, 59 },
 { 103, 79, 47 }, { 207, 151, 75 }, { 167, 123, 59 }, { 139, 103, 47 }, { 111, 83, 39 },
 { 235, 159, 39 }, { 203, 139, 35 }, { 175, 119, 31 }, { 147, 99, 27 }, { 119, 79, 23 },
 { 91, 59, 15 }, { 63, 39, 11 }, { 35, 23, 7 }, { 167, 59, 43 }, { 159, 47, 35 }, { 151, 43, 27 },
 { 139, 39, 19 }, { 127, 31, 15 }, { 115, 23, 11 }, { 103, 23, 7 }, { 87, 19, 0 }, { 75, 15, 0 },
 { 67, 15, 0 }, { 59, 15, 0 }, { 51, 11, 0 }, { 43, 11, 0 }, { 35, 11, 0 }, { 27, 7, 0 }, { 19, 7, 0 },
 { 123, 95, 75 }, { 115, 87, 67 }, { 107, 83, 63 }, { 103, 79, 59 }, { 95, 71, 55 }, { 87, 67, 51 },
 { 83, 63, 47 }, { 75, 55, 43 }, { 67, 51, 39 }, { 63, 47, 35 }, { 55, 39, 27 }, { 47, 35, 23 },
 { 39, 27, 19 }, { 31, 23, 15 }, { 23, 15, 11 }, { 15, 11, 7 }, { 111, 59, 23 }, { 95, 55, 23 },
 { 83, 47, 23 }, { 67, 43, 23 }, { 55, 35, 19 }, { 39, 27, 15 }, { 27, 19, 11 }, { 15, 11, 7 },
 { 179, 91, 79 }, { 191, 123, 111 }, { 203, 155, 147 }, { 215, 187, 183 }, { 203, 215, 223 },
 { 179, 199, 211 }, { 159, 183, 195 }, { 135, 167, 183 }, { 115, 151, 167 }, { 91, 135, 155 },
 { 71, 119, 139 }, { 47, 103, 127 }, { 23, 83, 111 }, { 19, 75, 103 }, { 15, 67, 91 },
 { 11, 63, 83 }, { 7, 55, 75 }, { 7, 47, 63 }, { 7, 39, 51 }, { 0, 31, 43 }, { 0, 23, 31 },
 { 0, 15, 19 }, { 0, 7, 11 }, { 0, 0, 0 }, { 139, 87, 87 }, { 131, 79, 79 }, { 123, 71, 71 },
 { 115, 67, 67 }, { 107, 59, 59 }, { 99, 51, 51 }, { 91, 47, 47 }, { 87, 43, 43 }, { 75, 35, 35 },
 { 63, 31, 31 }, { 51, 27, 27 }, { 43, 19, 19 }, { 31, 15, 15 }, { 19, 11, 11 }, { 11, 7, 7 }, { 0, 0, 0 },
 { 151, 159, 123 }, { 143, 151, 115 }, { 135, 139, 107 }, { 127, 131, 99 }, { 119, 123, 95 }, { 115, 115, 87 },
 { 107, 107, 79 }, { 99, 99, 71 }, { 91, 91, 67 }, { 79, 79, 59 }, { 67, 67, 51 }, { 55, 55, 43 }, { 47, 47, 35 },
 { 35, 35, 27 }, { 23, 23, 19 }, { 15, 15, 11 }, { 159, 75, 63 }, { 147, 67, 55 }, { 139, 59, 47 }, { 127, 55, 39 },
 { 119, 47, 35 }, { 107, 43, 27 }, { 99, 35, 23 }, { 87, 31, 19 }, { 79, 27, 15 }, { 67, 23, 11 }, { 55, 19, 11 },
 { 43, 15, 7 }, { 31, 11, 7 }, { 23, 7, 0 }, { 11, 0, 0 }, { 0, 0, 0 }, { 119, 123, 207 }, { 111, 115, 195 },
 { 103, 107, 183 }, { 99, 99, 167 }, { 91, 91, 155 }, { 83, 87, 143 }, { 75, 79, 127 }, { 71, 71, 115 },
 { 63, 63, 103 }, { 55, 55, 87 }, { 47, 47, 75 }, { 39, 39, 63 }, { 35, 31, 47 }, { 27, 23, 35 }, { 19, 15, 23 },
 { 11, 7, 7 }, { 155, 171, 123 }, { 143, 159, 111 }, { 135, 151, 99 }, { 123, 139, 87 }, { 115, 131, 75 },
 { 103, 119, 67 }, { 95, 111, 59 }, { 87, 103, 51 }, { 75, 91, 39 }, { 63, 79, 27 }, { 55, 67, 19 }, { 47, 59, 11 },
 { 35, 47, 7 }, { 27, 35, 0 }, { 19, 23, 0 }, { 11, 15, 0 }, { 0, 255, 0 }, { 35, 231, 15 }, { 63, 211, 27 },
 { 83, 187, 39 }, { 95, 167, 47 }, { 95, 143, 51 }, { 95, 123, 51 }, { 255, 255, 255 }, { 255, 255, 211 },
 { 255, 255, 167 }, { 255, 255, 127 }, { 255, 255, 83 }, { 255, 255, 39 }, { 255, 235, 31 }, { 255, 215, 23 },
 { 255, 191, 15 }, { 255, 171, 7 }, { 255, 147, 0 }, { 239, 127, 0 }, { 227, 107, 0 }, { 211, 87, 0 }, { 199, 71, 0 },
 { 183, 59, 0 }, { 171, 43, 0 }, { 155, 31, 0 }, { 143, 23, 0 }, { 127, 15, 0 }, { 115, 7, 0 }, { 95, 0, 0 }, { 71, 0, 0 },
 { 47, 0, 0 }, { 27, 0, 0 }, { 239, 0, 0 }, { 55, 55, 255 }, { 255, 0, 0 }, { 0, 0, 255 }, { 43, 43, 35 }, { 27, 27, 23 },
 { 19, 19, 15 }, { 235, 151, 127 }, { 195, 115, 83 }, { 159, 87, 51 }, { 123, 63, 27 }, { 235, 211, 199 }, { 199, 171, 155 },
 { 167, 139, 119 }, { 135, 107, 87 }, { 159, 91, 83 } };


static int	ramp1[8] = { 0x6f, 0x6d, 0x6b, 0x69, 0x67, 0x65, 0x63, 0x61 };
static int	ramp2[8] = { 0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x68, 0x66 };
static int	ramp3[8] = { 0x6d, 0x6b, 6, 5, 4, 3 };

//============================================
//	ParticleTypes
//	From Quake1's R_DrawParticles
//============================================

void ParticleTypes(particle_t* p, float frametime, ptype_t type)
{
	float time2, time3, time1, dvel, grav, sv_gravity;
	int	 i;

	time3 = frametime * 15;
	time2 = frametime * 10; // 15;
	time1 = frametime * 5;

	sv_gravity = gHUD.m_pSV_Gravity->value ? gHUD.m_pSV_Gravity->value : 800.0f;
	grav = frametime * sv_gravity * 0.05;
	dvel = 4 * frametime;

	p->org[0] += p->vel[0] * frametime;
	p->org[1] += p->vel[1] * frametime;
	p->org[2] += p->vel[2] * frametime;

	switch (type)
	{
	case pt_static:
		break;
	case pt_fire:
		p->ramp += time1;
		if (p->ramp >= 6)
			p->die = -1;
		else
			p->color = ramp3[(int)p->ramp];
		p->vel[2] += grav;
		break;

	case pt_explode:
		p->ramp += time2;
		if (p->ramp >= 8)
			p->die = -1;
		else
			p->color = ramp1[(int)p->ramp];
		for (i = 0; i < 3; i++)
			p->vel[i] += p->vel[i] * dvel;
		p->vel[2] -= grav;
		break;

	case pt_explode2:
		p->ramp += time3;
		if (p->ramp >= 8)
			p->die = -1;
		else
			p->color = ramp2[(int)p->ramp];
		for (i = 0; i < 3; i++)
			p->vel[i] -= p->vel[i] * frametime;
		p->vel[2] -= grav;
		break;

	case pt_blob:
		for (i = 0; i < 3; i++)
			p->vel[i] += p->vel[i] * dvel;
		p->vel[2] -= grav;
		break;

	case pt_blob2:
		for (i = 0; i < 2; i++)
			p->vel[i] -= p->vel[i] * dvel;
		p->vel[2] -= grav;
		break;

	case pt_grav:
	case pt_slowgrav:
		p->vel[2] -= grav;
		break;
	}
}

void PT_FireCallback(struct particle_s* particle, float frametime) { ParticleTypes(particle, frametime, pt_fire); }
void PT_ExplodeCallback(struct particle_s* particle, float frametime) { ParticleTypes(particle, frametime, pt_explode); }
void PT_Explode2Callback(struct particle_s* particle, float frametime) { ParticleTypes(particle, frametime, pt_explode2); }
void PT_BlobCallback(struct particle_s* particle, float frametime) { ParticleTypes(particle, frametime, pt_blob); }
void PT_Blob2Callback(struct particle_s* particle, float frametime) { ParticleTypes(particle, frametime, pt_blob2); }
void PT_GravCallback(struct particle_s* particle, float frametime) { ParticleTypes(particle, frametime, pt_grav); }
void PT_SlowGravCallback(struct particle_s* particle, float frametime) { ParticleTypes(particle, frametime, pt_slowgrav); }

/*
===============
R_ParticleExplosion

===============
*/
void R_ParticleExplosion(vec3_t org)
{
	int			i, j;

	for (i = 0; i < 1024; i++)
	{
		particle_t* p = gEngfuncs.pEfxAPI->R_AllocParticle(i & 1 ? PT_ExplodeCallback : PT_Explode2Callback);

		if (!p)
			return;

		p->type = pt_clientcustom;
		p->die = gEngfuncs.GetClientTime() + 5;
		p->color = ramp1[0];
		p->ramp = rand() & 3;

		if (i & 1)
		{
			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + ((rand() % 32) - 16);
				p->vel[j] = (rand() % 512) - 256;
			}
		}
		else
		{
			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + ((rand() % 32) - 16);
				p->vel[j] = (rand() % 512) - 256;
			}
		}
	}
}

/*
===============
R_BlobExplosion

===============
*/
void R_BlobExplosion(vec3_t org)
{
	int			i, j;

	for (i = 0; i < 1024; i++)
	{
		particle_t* p = gEngfuncs.pEfxAPI->R_AllocParticle(i & 1 ? PT_BlobCallback : PT_Blob2Callback);

		if (!p)
			return;

		p->die = gEngfuncs.GetClientTime() + 1 + (rand() & 8) * 0.05;
		p->type = pt_clientcustom;

		if (i & 1)
		{
			p->color = 66 + rand() % 6;
			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + ((rand() % 32) - 16);
				p->vel[j] = (rand() % 512) - 256;
			}
		}
		else
		{
			p->color = 150 + rand() % 6;
			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + ((rand() % 32) - 16);
				p->vel[j] = (rand() % 512) - 256;
			}
		}
	}
}

/*
===============
R_RunParticleEffect

===============
*/
void R_RunParticleEffect(vec3_t org, vec3_t dir, int color, int count)
{
	int			i, j;
	int			scale;

	if (count > 130)
		scale = 3;
	else if (count > 20)
		scale = 2;
	else
		scale = 1;

	for (i = 0; i < count; i++)
	{
		particle_t* p = gEngfuncs.pEfxAPI->R_AllocParticle(PT_GravCallback);
		if (!p)
			return;

		p->die = gEngfuncs.GetClientTime() + 0.1 * (rand() % 5);
		p->color = (color & ~7) + (rand() & 7);
		p->type = pt_clientcustom;
		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + scale * ((rand() & 15) - 8);
			p->vel[j] = dir[j] * 15;// + (rand()%300)-150;
		}
	}
}


/*
===============
R_LavaSplash

===============
*/
void R_LavaSplash(vec3_t org)
{
	int			i, j, k;
	float		vel;
	vec3_t		dir;

	for (i = -16; i < 16; i++)
		for (j = -16; j < 16; j++)
			for (k = 0; k < 1; k++)
			{
				particle_t* p = gEngfuncs.pEfxAPI->R_AllocParticle(PT_GravCallback);

				if (!p)
					return;

				p->type = pt_clientcustom;
				p->die = gEngfuncs.GetClientTime() + 2 + (rand() & 31) * 0.02;
				p->color = 224 + (rand() & 7);

				dir[0] = j * 8 + (rand() & 7);
				dir[1] = i * 8 + (rand() & 7);
				dir[2] = 256;

				p->org[0] = org[0] + dir[0];
				p->org[1] = org[1] + dir[1];
				p->org[2] = org[2] + (rand() & 63);

				VectorNormalize(dir);
				vel = 50 + (rand() & 63);
				VectorScale(dir, vel, p->vel);
			}
}

/*
===============
R_TeleportSplash

===============
*/
void R_TeleportSplash(vec3_t org)
{
	int			i, j, k;
	float		vel;
	vec3_t		dir;

	for (i = -16; i < 16; i += 4)
		for (j = -16; j < 16; j += 4)
			for (k = -24; k < 32; k += 4)
			{
				particle_t* p = gEngfuncs.pEfxAPI->R_AllocParticle(PT_GravCallback);

				if (!p)
					return;

				p->type = pt_clientcustom;
				p->die = gEngfuncs.GetClientTime() + 0.2 + (rand() & 7) * 0.02;
				p->color = 7 + (rand() & 7);

				dir[0] = j * 8;
				dir[1] = i * 8;
				dir[2] = k * 8;

				p->org[0] = org[0] + i + (rand() & 3);
				p->org[1] = org[1] + j + (rand() & 3);
				p->org[2] = org[2] + k + (rand() & 3);

				VectorNormalize(dir);
				vel = 50 + (rand() & 63);
				VectorScale(dir, vel, p->vel);
			}
}

/*
===============
R_RocketTrail

===============
*/

void R_RocketTrail(vec3_t start, vec3_t end, int type)
{
	vec3_t	vec;
	float	len;
	int			j;
	particle_t* p;

	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);
	while (len > 0)
	{
		len -= 3;

		switch (type)
		{
		case 4:
		case 2:
			p = gEngfuncs.pEfxAPI->R_AllocParticle(PT_SlowGravCallback);
			break;
		case 6:
		case 3:
		case 5:
			p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL);
			break;
		case 1:
		case 0:
			p = gEngfuncs.pEfxAPI->R_AllocParticle(PT_FireCallback);
			break;
		defaut:
			break;
		}

		if (!p)
			return;

		VectorCopy(vec3_origin, p->vel);
		p->type = pt_clientcustom;
		p->die = gEngfuncs.GetClientTime() + 2;

		if (type == 4)
		{	// slight blood
			p->type = pt_clientcustom;
			p->color = 67 + (rand() & 3);
			for (j = 0; j < 3; j++)
				p->org[j] = start[j] + ((rand() % 6) - 3);
			len -= 3;
		}
		else if (type == 2)
		{	// blood
			p->type = pt_clientcustom;
			p->color = 67 + (rand() & 3);
			for (j = 0; j < 3; j++)
				p->org[j] = start[j] + ((rand() % 6) - 3);
		}
		else if (type == 6)
		{	// voor trail
			p->color = 9 * 16 + 8 + (rand() & 3);
			p->type = pt_static;
			p->die = gEngfuncs.GetClientTime() + 0.3;
			for (j = 0; j < 3; j++)
				p->org[j] = start[j] + ((rand() & 15) - 8);
		}
		else if (type == 1)
		{	// smoke smoke
			p->ramp = (rand() & 3) + 2;
			p->color = ramp3[(int)p->ramp];
			p->type = pt_clientcustom;
			for (j = 0; j < 3; j++)
				p->org[j] = start[j] + ((rand() % 6) - 3);
		}
		else if (type == 0)
		{	// rocket trail
			p->ramp = (rand() & 3);
			p->color = ramp3[(int)p->ramp];
			p->type = pt_clientcustom;
			for (j = 0; j < 3; j++)
				p->org[j] = start[j] + ((rand() % 6) - 3);
		}
		else if (type == 3 || type == 5)
		{	// tracer
			static int tracercount;

			p->die = gEngfuncs.GetClientTime() + 0.5;
			p->type = pt_static;
			if (type == 3)
				p->color = 52 + ((tracercount & 4) << 1);
			else
				p->color = 230 + ((tracercount & 4) << 1);

			tracercount++;

			VectorCopy(start, p->org);
			if (tracercount & 1)
			{
				p->vel[0] = 30 * vec[1];
				p->vel[1] = 30 * -vec[0];
			}
			else
			{
				p->vel[0] = 30 * -vec[1];
				p->vel[1] = 30 * vec[0];
			}
		}
		VectorAdd(start, vec, start);
	}
}


//============================================
//	Q2CrossProduct
//	From Quake2.
//============================================

void Q2CrossProduct(vec3_t v1, vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
	cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
	cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

//============================================
//	MakeNormalVectors
//	From Quake2. Used by RailTrail.
//============================================

void MakeNormalVectors(vec3_t forward, vec3_t right, vec3_t up)
{
	float		d;

	// this rotate and negat guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct(right, forward);
	VectorMA(right, -d, forward, right);
	VectorNormalize(right);
	Q2CrossProduct(right, forward, up);
}

//============================================
//	R_RailTrail
//	From Quake2.
//============================================

void R_RailTrail(vec3_t start, vec3_t end, vec3_t angles)
{
	vec3_t vecSrc, vecEnd, origin, forward, right, up;
	vec3_t move, vec, dir;
	int i, j, len;
	float d, c, s, dec;
	particledan_t* p;
	int clr = 116;

	AngleVectors(angles, forward, right, up);
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	MakeNormalVectors(vec, right, up);

	for (i = 0; i < len; i++)
	{
		p = gHUD.m_ParticleDan.AllocParticle();

		if (!p)
			return;

		p->die = gEngfuncs.GetClientTime() + 1.0f;
		p->flags = PDAN_QUAKE;

		d = i * 0.1f;
		c = (float)cos(d);
		s = (float)sin(d);

		VectorScale(right, c, dir);
		VectorMA(dir, s, up, dir);

		p->alpha = 1.0;
		p->alpha_step = -1.0f / (1 + gEngfuncs.pfnRandomFloat(0.0f, 1.0f) * 0.2f);
		p->color = q2pal[clr + (rand() & 7)];
		p->color = p->color / 255.0f;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + dir[j] * 3;
			p->vel[j] = dir[j] * 6;
		}
		VectorAdd(move, vec, move);
	}

	dec = 0.75;
	VectorScale(vec, dec, vec);
	VectorCopy(start, move);

	while (len > 0)
	{
		len -= dec;

		p = gHUD.m_ParticleDan.AllocParticle();

		if (!p)
			return;

		p->die = gEngfuncs.GetClientTime() + 1.0f;
		p->flags = PDAN_QUAKE;

		VectorClear(p->vel);

		p->alpha = 1.0;
		p->alpha_step = -1.0f / (1 + gEngfuncs.pfnRandomFloat(0.0f, 1.0f) * 0.2f);
		p->color = q2pal[0 + (rand() & 15)];
		p->color = p->color / 255.0f;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + gEngfuncs.pfnRandomLong(-3, 3);
			p->vel[j] = gEngfuncs.pfnRandomLong(-3, 3);
		}
		VectorAdd(move, vec, move);
	}
}
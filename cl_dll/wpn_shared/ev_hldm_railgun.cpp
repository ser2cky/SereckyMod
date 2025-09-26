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
#include "../hud.h"
#include "../cl_util.h"
#include "cl_entity.h"
#include "pm_defs.h"

#include "../eventscripts.h"
#include "../ev_hldm.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "../in_defs.h"

#include "com_model.h"
#include "../particledan.h"

TEMPENTITY* pFlameSpawner = nullptr;

/*
======================================
EV_FlameParticles
======================================
*/

void EV_FlameParticles(vec3_t org, vec3_t vel)
{
	int i;
	particledan_t* p = gHUD.m_ParticleDan.AllocParticle();

	if (p)
	{
		for (i = 0; i < 3; i++)
			p->org[i] = org[i] + gEngfuncs.pfnRandomFloat(-0.5f, 0.5f);
		p->vel = vel;
		p->gravity = 0.0f;
		p->flags = PDAN_ANIMATED_ALPHA | PDAN_GROWTH | PDAN_ANIMATE_DIE;

		// Visual.
		p->model = (struct model_s*)gEngfuncs.GetSpritePointer(SPR_Load("sprites/kp_explosion.spr"));
		p->brightness = 1.0f;
		p->rendermode = kRenderTransAdd;
		p->color = Vector(1.0f, 1.0f, 1.0f);
		p->alpha = gEngfuncs.pfnRandomFloat(0.5f, 1.0f);
		p->scale = {2.0f, 2.0f};

		// Animation.
		p->scale_step = gEngfuncs.pfnRandomFloat(1.0f, 2.5f);
		p->growth_max = { 128.0f, 128.0f };

		p->alpha_step = 20.0f;
		p->alpha_keyframe = { p->alpha, p->alpha, p->alpha, p->alpha, 0.0f };

		p->frame = gEngfuncs.pfnRandomLong(0, 13);
		p->max_frames = 13;
		p->framerate = 60.0f;
		p->die = gEngfuncs.GetClientTime() + 1.0f;
	}

	
}

void EV_FlameParticles1(vec3_t org, vec3_t vel)
{
	int i;

	particledan_t* p = gHUD.m_ParticleDan.AllocParticle();

	if (p)
	{
		for (i = 0; i < 3; i++)
			p->org[i] = org[i] + gEngfuncs.pfnRandomFloat(-1.5f, 1.5f);
		p->gravity = 0.0f;
		p->vel = vel;
		p->vel[2] += gEngfuncs.pfnRandomFloat(20.0f, 40.0f);
		p->flags = PDAN_ANIMATED_ALPHA | PDAN_GROWTH;

		// Visual.
		p->model = (struct model_s*)gEngfuncs.GetSpritePointer(SPR_Load("sprites/kp_fthrow1f.spr"));
		p->brightness = 1.0f;
		p->rendermode = kRenderTransAdd;
		p->color = Vector(1.0f, 1.0f, 1.0f);
		p->alpha = gEngfuncs.pfnRandomFloat(0.5f, 1.0f);
		p->scale = { 2.0f, 2.0f };

		// Animation.
		p->scale_step = 2.0;
		p->growth_max = { 144.0f, 144.0f };

		p->alpha_step = 25.0f;
		p->alpha_keyframe = { 0.0f, 0.0f, 0.0f, gEngfuncs.pfnRandomFloat(0.3f, 0.6f), 0.0f };

		p->frame = gEngfuncs.pfnRandomLong(0, 1);
		p->max_frames = 1;
		p->framerate = 60.0f;
		p->die = gEngfuncs.GetClientTime() + 0.15f;
	}
}

/*
======================================
EV_FlameCallback
======================================
*/

void EV_FlameCallback(struct tempent_s* ent, float frametime, float currenttime)
{
	vec3_t angles, org, up, right, forward, vecVelocity;
	vec3_t offset = Vector(0, 0, 0);

	//If the Player is not on our PVS, then go back
	if (!CheckPVS(ent->clientIndex))
		return;

	cl_entity_t* player = gEngfuncs.GetEntityByIndex(ent->clientIndex);

	if (!player)
		return;

	gEngfuncs.pEventAPI->EV_LocalPlayerViewheight(org);
	VectorAdd(org, player->origin, org);
	VectorCopy(gHUD.m_vecAngles, angles);

	AngleVectors(angles, forward, right, up);
	VectorMA(offset, -8.0f, up, offset);
	VectorMA(offset, 8.0f, right, offset);
	VectorMA(offset, 36.0f, forward, offset);
	VectorAdd(org, offset, org);

	VectorScale(forward, 1200.0f, vecVelocity);
	EV_FlameParticles(org, vecVelocity);
	EV_FlameParticles1(org, vecVelocity);
}

/*
======================================
EV_FireRailgun
======================================
*/

void EV_FireRailgun(event_args_t* args)
{
	int modelIndex, idx = args->entindex;
	vec3_t vecFlameOrg, vecVelocity;
	vec3_t up, right, forward;
	vec3_t origin, vecSrc;

	char* model = "sprites/smoke.spr";
	modelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex(model);

	VectorCopy(args->origin, origin);
	EV_GetGunPosition(args, vecSrc, origin);

	if (EV_IsLocal(idx))
	{
		pFlameSpawner = gEngfuncs.pEfxAPI->R_TempModel(vecSrc, args->velocity, args->angles, 9999, modelIndex, TE_BOUNCE_NULL);

		if (pFlameSpawner != NULL)
		{
			pFlameSpawner->entity.origin = vecSrc;
			pFlameSpawner->flags |= (FTENT_PLYRATTACHMENT | FTENT_PERSIST | FTENT_NOMODEL | FTENT_CLIENTCUSTOM);
			pFlameSpawner->clientIndex = idx;
			pFlameSpawner->callback = EV_FlameCallback;
		}
	}
}

/*
======================================
EV_StopFlames
======================================
*/

void EV_StopFlames(event_args_t* args)
{
	int idx;
	vec3_t origin;

	idx = args->entindex;
	VectorCopy(args->origin, origin);

	if (pFlameSpawner != nullptr)
	{
		gEngfuncs.pEfxAPI->R_KillAttachedTents(idx);
		pFlameSpawner->die = 0.0f; //you will DIE.
		pFlameSpawner = nullptr;
	}
}
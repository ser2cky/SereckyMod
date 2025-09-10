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
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "entity_types.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_materials.h"

#include "../eventscripts.h"
#include "../ev_hldm.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "../in_defs.h"

#include <string.h>

#include "r_studioint.h"
#include "com_model.h"
#include "../particledan.h"

extern "C"
{
	// HLDM
	void EV_FireRailgun(struct event_args_s* args);
}

void EV_FlameParticles(vec3_t org, vec3_t vel)
{
	int i;
	particledan_t* p = gHUD.m_ParticleDan.AllocParticle();

	for (i = 0; i < 3; i++)
	{
		if (p)
		{
			for (i = 0; i < 3; i++)
				p->org[i] = org[i] + gEngfuncs.pfnRandomFloat(-0.25f, 0.25f);
			p->vel = vel;
			p->gravity = 0.0f;
			p->flags = PDAN_ANIMATED_ALPHA | PDAN_ANIMATED_SCALE | PDAN_ANIMATE_DIE;
			p->frame = gEngfuncs.pfnRandomLong(0, 3);

			// Visual.
			p->model = (struct model_s*)gEngfuncs.GetSpritePointer(SPR_Load("sprites/kp_explosion.spr"));
			p->brightness = gEngfuncs.pfnRandomFloat(0.0f, 1.0f);
			p->rendermode = kRenderTransAdd;
			p->color = Vector(1.0f, 1.0f, 1.0f);
			p->alpha = 1.0f;
			p->scale[0] = 8.0f;
			p->scale[1] = 8.0f;

			// Animation.
			p->scale_step = 10.0f;
			p->alpha_step = 5.0f;
			p->alpha_keyframe = { 1.0f, 1.0f, 1.0f, 0.5f, 0.0f };
			p->scale_keyframe[0] = { 48.0f, 72.0f, 72.0f, 72.0f, 72.0f };
			p->scale_keyframe[1] = { 48.0f, 72.0f, 72.0f, 72.0f, 72.0f };
			p->max_frames = 13;
			p->framerate = 30.0f;
			p->die = gEngfuncs.GetClientTime() + 1.0f;
		}
	}
}

void EV_FireRailgun(event_args_t* args)
{
	int idx = args->entindex;
	vec3_t vecFlameOrg, vecVelocity;
	vec3_t up, right, forward;
	vec3_t origin, angles;

	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	AngleVectors(angles, forward, right, up);

	EV_GetGunPosition(args, vecFlameOrg, origin);
	VectorMA(vecFlameOrg, -8.0f, up, vecFlameOrg);
	VectorMA(vecFlameOrg, 8.0f, right, vecFlameOrg);
	VectorMA(vecFlameOrg, 56.0f, forward, vecFlameOrg);

	VectorScale(forward, 800.0f, vecVelocity);

	EV_FlameParticles(vecFlameOrg, vecVelocity);
}
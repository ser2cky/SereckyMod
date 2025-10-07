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
#include "../modelfx.h"

#include "r_studioint.h"

void V_Q2Punch(float* kick_org, float* kick_ang);

void EV_FireMachinegun(event_args_t* args)
{
	int machinegun_shots = args->iparam1;
	int idx = args->entindex;
	static float nxth = 0.0f;
	float viewheight;
	int i;

	vec3_t angles, up, right, forward;
	vec3_t origin, start, end, offset;
	vec3_t kick_origin, kick_angles;

	VectorCopy(args->origin, origin);
	viewheight = args->ducking == 1 ? VEC_DUCK_VIEW : DEFAULT_VIEWHEIGHT;

	if (EV_IsLocal(idx))
	{
		if (nxth <= gEngfuncs.GetClientTime() || (nxth - gEngfuncs.GetClientTime()) > 0.2f)
		{
			EV_WeaponAnimation(Q2_FIRE, 0);
			nxth = gEngfuncs.GetClientTime() + 0.2f;
		}
	}

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, 
		UTIL_VarArgs("weapons/q2/Machgf%db.wav", gEngfuncs.pfnRandomLong( 1, 5 ))
		, 0.95, ATTN_NORM, 0, PITCH_NORM);

	for (i = 1; i < 3; i++)
	{
		kick_origin[i] = gEngfuncs.pfnRandomFloat(-1.0f, 1.0f) * 0.35;
		kick_angles[i] = gEngfuncs.pfnRandomFloat(-1.0f, 1.0f) * 0.7;
	}
	kick_origin[0] = gEngfuncs.pfnRandomFloat(-1.0f, 1.0f) * 0.35;
	kick_angles[0] = machinegun_shots * -1.5;

	V_Q2Punch((float*)&kick_origin, (float*)&kick_angles);

	dlight_t* dl = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
	VectorCopy(origin, dl->origin);
	AngleVectors(args->angles, forward, right, NULL);
	VectorMA(dl->origin, 18, forward, dl->origin);
	VectorMA(dl->origin, 16, right, dl->origin);
	dl->radius = 200 + (rand() & 31);
	dl->minlight = 32;
	dl->die = gEngfuncs.GetClientTime();
	dl->color.r = 255;
	dl->color.g = 255;
	dl->color.b = 0;
}
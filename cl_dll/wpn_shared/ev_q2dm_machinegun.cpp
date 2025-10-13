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
	vec3_t kick_origin;

	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);

	viewheight = args->ducking == 1 ? VEC_DUCK_VIEW : DEFAULT_VIEWHEIGHT;

	if (EV_IsLocal(idx))
	{
		if (nxth <= gEngfuncs.GetClientTime() || (nxth - gEngfuncs.GetClientTime()) > 0.2f)
		{
			EV_WeaponAnimation(Q2_FIRE, 0);
			nxth = gEngfuncs.GetClientTime() + 0.2f;
		}
	}

	AngleVectors(angles, forward, right, up);
	VectorSet(offset, 0, 8, viewheight - 8);

	EV_ProjectSource(origin, offset, forward, right, start);
	EV_Q2_FireBullets(idx, start, forward, forward, right, up, 0, args->fparam1, args->fparam2);

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, 
		UTIL_VarArgs("weapons/q2/Machgf%db.wav", gEngfuncs.pfnRandomLong( 1, 5 ))
		, 0.95, ATTN_NORM, 0, PITCH_NORM);

	for (i = 1; i < 3; i++)
		kick_origin[i] = gEngfuncs.pfnRandomFloat(-1.0f, 1.0f) * 0.35;
	kick_origin[0] = gEngfuncs.pfnRandomFloat(-1.0f, 1.0f) * 0.35;

	V_Q2Punch((float*)&kick_origin, NULL);

	R_ParseMuzzleFlash(MZ_MACHINEGUN, angles, origin);
}
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

#include "hud.h"
#include "cl_util.h"
#include "cl_entity.h"
#include "pm_defs.h"

#include "eventscripts.h"
#include "ev_hldm.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"

#include "com_model.h"
#include "modelfx.h"

void V_PunchAxis(int axis, float punch);

void EV_Quake_FireSuperShot(event_args_t* args)
{
	int idx = args->entindex;
	vec3_t angles, origin;

	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);

	if (EV_IsLocal(idx))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(Q2_FIRE, 0);
		R_Muzzleflash(origin, angles);
		V_PunchAxis(0, -4);
	}
	gEngfuncs.pfnConsolePrint("Yes\n");
	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/quake/shotgn2.wav", 1, ATTN_NORM, 0, PITCH_NORM);
}

void EV_Quake_FireRocket(event_args_t* args)
{
	int idx = args->entindex;
	vec3_t angles, origin;

	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);

	if (EV_IsLocal(idx))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(Q2_FIRE, 0);
		R_Muzzleflash(origin, angles);
		V_PunchAxis(0, -2);
	}

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/quake/sgun1.wav", 1, ATTN_NORM, 0, PITCH_NORM);
}
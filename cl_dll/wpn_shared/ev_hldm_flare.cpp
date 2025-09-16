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

#include "pm_defs.h"
#include "../eventscripts.h"
#include "../ev_hldm.h"

#include "event_api.h"
#include "event_args.h"
#include "../in_defs.h"

enum flaregun_e {
	FLAREGUN_IDLE1 = 0,
	FLAREGUN_IDLE2,
	FLAREGUN_IDLE3,
	FLAREGUN_IDLE4,
	FLAREGUN_FIRE1,
	FLAREGUN_FIRE2,
	FLAREGUN_FIRE3,
	FLAREGUN_RELOAD,
	FLAREGUN_DRAW1,
	FLAREGUN_DRAW2
};

extern "C"
{
	// HLDM
	void EV_FireFlare(struct event_args_s* args);
}

//======================
//	    FLARE START
//======================

void EV_FireFlare(event_args_t* args)
{
	int idx = args->entindex;
	vec3_t origin;

	VectorCopy(args->origin, origin);

	if (EV_IsLocal(idx))
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(FLAREGUN_FIRE2 + gEngfuncs.pfnRandomLong(0, 1), 0);
	}

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/flaregun/fire.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_NORM, 0, PITCH_NORM);
}

//======================
//	   FLARE END
//======================
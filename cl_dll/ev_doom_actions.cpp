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
#include "my_sprites.h"
#include "ev_doom_actions.h"

/*
=============================================
	ev_doom_actions.cpp

	Clientsided versions of the DooM actions.
	Most functions based on those found in
	"p_pspr.c" !!!

	History:
	10/4/25 (serecky): Created. 
	Moved actions from "ev_hldm_railgun.cpp" 
	to this file.
=============================================
*/

//===========================================
// A_WeaponReady
// The player can fire the weapon
// or change to another weapon at this time.
// Follows after getting weapon up,
// or after previous attack/fire sequence.
//===========================================

void A_WeaponReady(spr_object_t* spr, float frametime, float time)
{
	return;
}

//====================================
// A_ReFire
// The player can re-fire the weapon
// without lowering it entirely.
//====================================

void A_ReFire(spr_object_t* spr, float frametime, float time)
{
	if (gHUD.m_iKeyBits & IN_ATTACK)
		spr->force_mode = SPR_GUN_FIRING;
}

//=================================
// A_Lower
// Lowers current weapon,
//  and changes weapon at bottom.
//=================================

void A_Lower(spr_object_t* spr, float frametime, float time)
{
	spr->old_ofs[1] = max(spr->ofs[1], -0.85f);
	if (spr->ofs[1] > -0.85f)
	{
		spr->ofs[1] -= frametime * 6.0f;
	}
	else
	{
		spr->mode = SPR_GUN_IDLE;
		spr->frame = spr->idle_frame[0];
	}
}

//=========
// A_Raise
//=========

void A_Raise(spr_object_t* spr, float frametime, float time)
{
	spr->old_ofs[1] = min(spr->ofs[1], 0.0f);
	if (spr->ofs[1] < 0)
	{
		spr->ofs[1] += frametime * 6.0f;
	}
	else
	{
		if (spr->ofs[1] > 0.0f)
			spr->ofs[1] = 0.0f;
		spr->mode = SPR_GUN_IDLE;
		spr->frame = spr->idle_frame[0];
	}
}

//===
// ?
//===

void A_Light1(spr_object_t* spr, float frametime, float time)
{
	spr->flags2 = SPR_OVERRIDE_LIGHT;
	spr->brightness2 = 0.5f;
	spr->color2 = { 1, 1, 1 };
}

void A_Light2(spr_object_t* spr, float frametime, float time)
{
	spr->flags2 = SPR_OVERRIDE_LIGHT;
	spr->brightness2 = 1.0f;
	spr->color2 = { 1, 1, 1 };
}
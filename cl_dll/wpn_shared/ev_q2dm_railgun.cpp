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

void EV_ProjectSource(float *point, float *distance, float *forward, float *right, float *result)
{
	result[0] = point[0] + forward[0] * distance[0] + right[0] * distance[1];
	result[1] = point[1] + forward[1] * distance[0] + right[1] * distance[1];
	result[2] = point[2] + forward[2] * distance[0] + right[2] * distance[1] + distance[2];
}

void EV_FireRailgun(event_args_t* args)
{
	int idx = args->entindex;
	vec3_t angles, up, right, forward;
	vec3_t origin, start, end, offset;
	vec3_t kick_origin, kick_angles;
	float viewheight;

	VectorCopy(args->origin, origin);
	viewheight = args->ducking == 1 ? VEC_DUCK_VIEW : DEFAULT_VIEWHEIGHT;

	if (EV_IsLocal(idx))
	{
		cl_entity_t* pl = gEngfuncs.GetEntityByIndex(idx);
		pmtrace_t tr;

		gEngfuncs.pEventAPI->EV_WeaponAnimation(Q2_FIRE, 0);
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/q2/RAILGF1A.WAV", 0.95, ATTN_NORM, 0, PITCH_NORM);

		if (pl)
		{
			VectorCopy(gHUD.m_vecAngles, angles);

			AngleVectors(angles, forward, right, up);
			VectorScale(forward, -3, kick_origin);
			kick_angles[0] = -3;

			V_Q2Punch((float*)&kick_origin, (float*)&kick_angles);

			VectorSet(offset, 0, 7, viewheight - 8);
			EV_ProjectSource(origin, offset, forward, right, start);

			VectorMA(start, 2048, forward, end);

			gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);

			// Store off the old count
			gEngfuncs.pEventAPI->EV_PushPMStates();

			// Now add in all of the players.
			gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);

			gEngfuncs.pEventAPI->EV_SetTraceHull(2);
			gEngfuncs.pEventAPI->EV_PlayerTrace(start, end, PM_WORLD_ONLY, -1, &tr);

			gEngfuncs.pEventAPI->EV_PopPMStates();

			R_RailTrail(start, tr.endpos, angles);
		}
	}

	dlight_t* dl = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
	VectorCopy(origin, dl->origin);
	AngleVectors(args->angles, forward, right, NULL);
	VectorMA(dl->origin, 18, forward, dl->origin);
	VectorMA(dl->origin, 16, right, dl->origin);
	dl->radius = 200 + (rand() & 31);
	dl->minlight = 32;
	dl->die = gEngfuncs.GetClientTime() + 0.1f;
	dl->color.r = 128; 
	dl->color.g = 128;
	dl->color.b = 255;
}
/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	2025 serecky :D
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
==================================================================
	bot_start.cpp

	Bot code based off MrElusives writeup of
	the Omnicron bot inner workings.

	https://mrelusive.com/oldprojects/obots/ObotDoc/index.htm

	Goals:
	Create Base class for a bot which can escort players
	and/or fight players effectively without the use of waypoints.

	History:

	8/26/25:
	Started.

==================================================================
*/

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"soundent.h"
#include	"weapons.h"

#define		BOT_FIGHT			0
#define		BOT_SEEK			1
#define		BOT_STAND			2
#define		BOT_INTERMISSION	4

/*
==============================================
Class def.
==============================================
*/

class CBaseBot : public CBaseMonster
{
public:
	void Spawn(void);

	// All bot thinkers will be put here.
	void EXPORT	MonsterThink(void);

//	void Move(float flInterval);
//	void Bot_WalkMove(float yaw, float dist);
//	BOOL Bot_MoveStep(const Vector& vecMove);

	int Classify(void) { return CLASS_NONE; };
	int ISoundMask(void) { return bits_SOUND_NONE; };
private:
	int m_iBotState;
};

LINK_ENTITY_TO_CLASS(bot, CBaseBot);

/*
==============================================
Spawn
==============================================
*/

void CBaseBot::Spawn(void)
{
	pev->classname = MAKE_STRING("player");
	pev->health = 100;
	pev->armorvalue = 0;
	pev->takedamage = DAMAGE_AIM;
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	pev->max_health = pev->health;
	pev->flags |= FL_FAKECLIENT;
	pev->air_finished = gpGlobals->time + 12;
	pev->dmg = 2;				// initial water damage
	pev->effects = 0;
	pev->deadflag = DEAD_NO;
	pev->dmg_take = 0;
	pev->dmg_save = 0;

	m_flFieldOfView = 0.5;// some monsters use this to determine whether or not the player is looking at them.

	m_bloodColor = BLOOD_COLOR_RED;
	m_flNextAttack = UTIL_WeaponTimeBase();

	SET_MODEL(ENT(pev), "models/player.mdl");
	pev->sequence = LookupActivity(ACT_IDLE);

	if (FBitSet(pev->flags, FL_DUCKING))
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);

	pev->view_ofs = VEC_VIEW;
}

/*
==============================================
MonsterThink
==============================================
*/

void CBaseBot::MonsterThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1;// keep monster thinking.

	float flInterval = StudioFrameAdvance(); // animate

	if (m_flGroundSpeed != 0)
	{
		Move(flInterval);
	}
}
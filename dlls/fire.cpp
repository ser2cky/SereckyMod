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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "soundent.h"

/*
=========================================
	fire.cpp
	flame that deals damage overtime for 
	both players & enemies

	History:

	9/15/25:
	Code lifted & improved from ReAlpha.
	- serecky
==========================================
*/

LINK_ENTITY_TO_CLASS(flame, CFlame);

void CFlame::Spawn(void)
{
	pev->movetype = MOVETYPE_NONE;
	pev->classname = MAKE_STRING("flame");
	pev->solid = SOLID_NOT;

	pev->health = 20;
	pev->takedamage = DAMAGE_NO;

	pev->effects = EF_NODRAW;
	SET_MODEL(ENT(pev), "models/grenade.mdl");
	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	m_flNextDamageTime = gpGlobals->time;
	pev->ltime = gpGlobals->time + 5.0;
	pev->nextthink = gpGlobals->time + 0.1;

	SetThink(&CFlame::FlameThink);
	Precache();
}

void CFlame::Precache(void)
{
	PRECACHE_MODEL("models/grenade.mdl");
}

CFlame* CFlame::FlameSpawn(CBaseEntity* pOwner, CBaseEntity* pTarget)
{
	if (!pTarget->m_flNumFlames)
		pTarget->m_flNumFlames = 0.0f;

	// Just making sure...
	if (pTarget->m_flNumFlames > 1.0f || !pTarget->pev->takedamage)
		return 0;

	CFlame* pFlame = GetClassPtr((CFlame*)NULL);
	pFlame->Spawn();

	UTIL_SetOrigin(pFlame->pev, pTarget->Center());
	pFlame->pev->enemy = pTarget->edict();

	if (pOwner != NULL)
		pFlame->pev->owner = pOwner->edict();

	pTarget->m_flNumFlames++;
	return pFlame;
}

void CFlame::FlameThink(void)
{
	CBaseEntity* pEnemy = CBaseEntity::Instance(pev->enemy);
	entvars_t* pOwner = VARS(pev->owner);

	if (pev->ltime < gpGlobals->time || pev->waterlevel >= 2)
	{
		FlameDestroy();
		return;
	}

	if (pEnemy != nullptr)
	{
		pev->origin = pEnemy->Center();

		if (pEnemy->pev->deadflag > DEAD_DYING)
		{
			FlameDestroy();
			return;
		}

		if (m_flNextDamageTime <= gpGlobals->time)
		{
			ClearMultiDamage();
			if (pOwner != NULL)
			{
				if (pEnemy->pev->takedamage)
					pEnemy->TakeDamage(pev, pOwner, 5, DMG_BURN | DMG_NEVERGIB); // changed from 2 to 5 -serecky 7.13.25
				ApplyMultiDamage(pev, pOwner);
			}
			else
			{
				if (pEnemy->pev->takedamage)
					pEnemy->TakeDamage(pev, pev, 5, DMG_BURN | DMG_NEVERGIB); // changed from 2 to 5 -serecky 7.13.25
				ApplyMultiDamage(pev, pev);
			}
			m_flNextDamageTime = gpGlobals->time + 0.5f;
		}
		// Setting rendermode and color to this value will create a hard-coded flame effect for monsters.
		// trust me, if there was a better way to do this, I would 1000000000%
		// go with it instead - serecky 9.15.25
		pEnemy->pev->renderfx = kRenderFxGlowShell;
		pEnemy->pev->rendercolor = Vector(255, 128, 0);
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

void CFlame::FlameDestroy(void)
{
	CBaseEntity* pEnemy = CBaseEntity::Instance(pev->enemy);
	if (pEnemy != nullptr)
	{
		pEnemy->m_flNumFlames = 0.0f;
		pEnemy->pev->renderamt = 255;
		pEnemy->pev->rendermode = kRenderNormal;
		pEnemy->pev->renderfx = kRenderFxNone;
	}

	pev->enemy = NULL;
	pev->owner = NULL;

	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}
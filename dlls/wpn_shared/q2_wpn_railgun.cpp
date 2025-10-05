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
#include "nodes.h"
#include "player.h"

LINK_WEAPON_TO_CLASS(weapon_railgun, CQuake2Railgun);

/*
======================================
Spawn
======================================
*/

void CQuake2Railgun::Spawn()
{
	pev->classname = MAKE_STRING("weapon_railgun"); // hack to allow for old names
	Precache();
	m_iId = WEAPON_RAILGUN;
	SET_MODEL(ENT(pev), "models/w_9mmhandgun.mdl");

	m_iDefaultAmmo = RAILGUN_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

/*
======================================
Precache
======================================
*/

void CQuake2Railgun::Precache(void)
{
	PRECACHE_MODEL("models/v_rail.mdl");
	PRECACHE_MODEL("models/w_9mmhandgun.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");

	m_usFireRailgun = PRECACHE_EVENT(1, "events/railgun.sc");
}

/*
======================================
GetItemInfo
======================================
*/

int CQuake2Railgun::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "uranium";
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = RAILGUN_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_RAILGUN;
	p->iWeight = RAILGUN_WEIGHT;

	return 1;
}

/*
======================================
Deploy
======================================
*/

BOOL CQuake2Railgun::Deploy()
{
	return DefaultDeploy("models/v_rail.mdl", "models/p_9mmhandgun.mdl", Q2_DRAW, "onehanded", 0);
}

/*
======================================
PrimaryAttack
======================================
*/

#define VectorSet(v, x, y, z)	(v[0]=(x), v[1]=(y), v[2]=(z))
void P_ProjectSource(float* point, float* distance, float* forward, float* right, float* result)
{
	result[0] = point[0] + forward[0] * distance[0] + right[0] * distance[1];
	result[1] = point[1] + forward[1] * distance[0] + right[1] * distance[1];
	result[2] = point[2] + forward[2] * distance[0] + right[2] * distance[1] + distance[2];
}

void CQuake2Railgun::PrimaryAttack(void)
{
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;
	int			damage;
	int			kick;

	if (gpGlobals->deathmatch)
	{	// normal damage is too extreme in dm
		damage = 100;
		kick = 200;
	}
	else
	{
		damage = 150;
		kick = 250;
	}

	//if (is_quad)
	//{
	//	damage *= 4;
	//	kick *= 4;
	//}

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);

	VectorSet(offset, 0, 7, m_pPlayer->pev->view_ofs[2] - 8);
	P_ProjectSource(m_pPlayer->pev->origin, offset, gpGlobals->v_forward, gpGlobals->v_right, start);
	Fire_Rail(start, gpGlobals->v_forward, damage);

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireRailgun, 0.0, (float*)&g_vecZero, (float*)&g_vecZero, g_vecZero.x, g_vecZero.y, 0, 0, 0, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.5f;
}

/*
=================
Fire_Rail
=================
*/
void CQuake2Railgun::Fire_Rail(vec3_t start, vec3_t aimdir, int damage)
{
	vec3_t		from;
	vec3_t		end;
	TraceResult tr;

	edict_t *ignore = ENT(m_pPlayer->pev);
	end = start + aimdir * 8192;

	while (ignore)
	{
		UTIL_TraceLine(start, end, dont_ignore_monsters, ignore, &tr);
		start = tr.vecEndPos;
		end = start + aimdir * 8192;

		if ((tr.pHit->v.flags & FL_MONSTER) || (tr.pHit->v.flags & FL_CLIENT))
			ignore = tr.pHit;
		else
			ignore = NULL;

		if ((tr.pHit != m_pPlayer->edict()) && (tr.pHit->v.takedamage))
			CBaseEntity::Instance(tr.pHit)->TakeDamage(m_pPlayer->pev, m_pPlayer->pev, damage, DMG_BULLET);
	}

	//if (self->client)
	//	PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
}

/*
======================================
WeaponIdle
======================================
*/

void CQuake2Railgun::WeaponIdle(void)
{
	
}
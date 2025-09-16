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
	m_usStopRailgun = PRECACHE_EVENT(1, "events/railgun1.sc");
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

void CQuake2Railgun::PrimaryAttack(void)
{
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireRailgun, 0.0, (float*)&g_vecZero, (float*)&g_vecZero, g_vecZero.x, g_vecZero.y, 0, 0, 0, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1f;
}

/*
======================================
ItemPostFrame
======================================
*/

void CQuake2Railgun::ItemPostFrame(void)
{
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	if (!(m_pPlayer->pev->button & IN_ATTACK))
		PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usStopRailgun, 0.0, (float*)&g_vecZero, (float*)&g_vecZero, g_vecZero.x, g_vecZero.y, 0, 0, 0, 0);

	CBasePlayerWeapon::ItemPostFrame();
}

/*
======================================
WeaponIdle
======================================
*/

void CQuake2Railgun::WeaponIdle(void)
{

}
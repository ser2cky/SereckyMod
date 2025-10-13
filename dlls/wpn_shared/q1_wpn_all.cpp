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
#include "player.h"

/*
=============================================
	q1_wpn_all.cpp

	Stores Quake weapon linking & stuff 
	for both CLIENT and SERVER DLLS

	-serecky 10.12.25

	History:
	10/12/25: Created.
=============================================
*/

/*
======================================

super-shotgun

======================================
*/

LINK_WEAPON_TO_CLASS(weapon_supershotgun, C_Q1SuperShot);

/*
======================================
Spawn
======================================
*/

void C_Q1SuperShot::Spawn()
{
	pev->classname = MAKE_STRING("weapon_supershotgun");
	Precache();
	m_iId = WEAPON_Q1SSHOT;
	SET_MODEL(ENT(pev), "models/null.mdl");

	m_iDefaultAmmo = Q1SSHOT_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

/*
======================================
Precache
======================================
*/

void C_Q1SuperShot::Precache(void)
{
	PRECACHE_MODEL("models/quake/v_shot2.mdl");
	m_usQ1SuperShotgun = PRECACHE_EVENT(1, "events/quake1_shot2.sc");
}

/*
======================================
GetItemInfo
======================================
*/

int C_Q1SuperShot::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);	// Gun's name...
	p->pszAmmo1 = "buckshot";				// Ammo type...
	p->iMaxAmmo1 = BUCKSHOT_MAX_CARRY;		// Max primary ammo...
	p->pszAmmo2 = NULL;						// Secondary Ammo type...
	p->iMaxAmmo2 = -1;						// Max secondary ammo...
	p->iMaxClip = WEAPON_NOCLIP;			// Max clip for your gun...
	p->iSlot = 1;							// Gun's slot position
	p->iPosition = 4;						// Gun's position in slot bucket...
	p->iFlags = 0;							// Special gun flags...
	p->iId = m_iId = WEAPON_Q1SSHOT;		// Gun's I.D
	p->iWeight = Q1SSHOT_WEIGHT;			// Gun's weight for auto-switching

	return 1;
}

/*
======================================
Deploy
======================================
*/

BOOL C_Q1SuperShot::Deploy()
{
	return DefaultDeploy(
		"models/quake/v_shot2.mdl",
		"models/null.mdl",
		Q1_IDLE,
		"onehanded",
		0, 0, 0.0f);
}

/*
======================================
PrimaryAttack
======================================
*/

void C_Q1SuperShot::PrimaryAttack(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return;

	W_FireSuperShotgun();

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT(flags, m_pPlayer->edict(), m_usQ1SuperShotgun);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.7f;
}

/*
======================================

rocket-launcher

======================================
*/

LINK_WEAPON_TO_CLASS(weapon_rocketlauncher, C_Q1RocketLauncher);

/*
======================================
Spawn
======================================
*/

void C_Q1RocketLauncher::Spawn()
{
	pev->classname = MAKE_STRING("weapon_rocketlauncher");
	Precache();
	m_iId = WEAPON_Q1ROCKET;
	SET_MODEL(ENT(pev), "models/null.mdl");

	m_iDefaultAmmo = RPG_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

/*
======================================
Precache
======================================
*/

void C_Q1RocketLauncher::Precache(void)
{
	PRECACHE_MODEL("models/quake/v_rock2.mdl");
	m_usQ1RocketLauncher = PRECACHE_EVENT(1, "events/quake1_rock2.sc");
}

/*
======================================
GetItemInfo
======================================
*/

int C_Q1RocketLauncher::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);	// Gun's name...
	p->pszAmmo1 = "buckshot";				// Ammo type...
	p->iMaxAmmo1 = BUCKSHOT_MAX_CARRY;		// Max primary ammo...
	p->pszAmmo2 = NULL;						// Secondary Ammo type...
	p->iMaxAmmo2 = -1;						// Max secondary ammo...
	p->iMaxClip = WEAPON_NOCLIP;			// Max clip for your gun...
	p->iSlot = 0;							// Gun's slot position
	p->iPosition = 2;						// Gun's position in slot bucket...
	p->iFlags = 0;							// Special gun flags...
	p->iId = m_iId = WEAPON_Q1ROCKET;		// Gun's I.D
	p->iWeight = RPG_WEIGHT;			// Gun's weight for auto-switching

	return 1;
}

/*
======================================
Deploy
======================================
*/

BOOL C_Q1RocketLauncher::Deploy()
{
	return DefaultDeploy(
		"models/quake/v_rock2.mdl",
		"models/null.mdl",
		Q1_IDLE,
		"onehanded",
		0, 0, 0.0f);
}

/*
======================================
PrimaryAttack
======================================
*/

void C_Q1RocketLauncher::PrimaryAttack(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return;

	W_FireRocket();

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT(flags, m_pPlayer->edict(), m_usQ1RocketLauncher);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.7f;
}
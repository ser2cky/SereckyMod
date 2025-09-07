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

LINK_WEAPON_TO_CLASS( weapon_thompson, CThompson);

/*
======================================
Spawn
======================================
*/

void CThompson::Spawn()
{
	pev->classname = MAKE_STRING("weapon_thompson");
	Precache();
	SET_MODEL(ENT(pev), "models/w_9mmhandgun.mdl");
	m_iId = WEAPON_THOMPSON;

	m_iDefaultAmmo = THOMPSON_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

/*
======================================
Precache
======================================
*/

void CThompson::Precache(void)
{
	PRECACHE_MODEL("models/v_thompson.mdl");
	PRECACHE_MODEL("models/w_9mmhandgun.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shell

	PRECACHE_SOUND("weapons/thompson/thompson.wav");

	m_usFireThompson = PRECACHE_EVENT(1, "events/thompson1.sc");
}

/*
======================================
GetItemInfo
======================================
*/

int CThompson::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = THOMPSON_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_THOMPSON;
	p->iWeight = THOMPSON_WEIGHT;

	return 1;
}

/*
======================================
Deploy
======================================
*/

BOOL CThompson::Deploy()
{
	return DefaultDeploy("models/v_thompson.mdl", "models/p_9mmhandgun.mdl", GUN_RAISE, "onehanded");
}

/*
======================================
PrimaryAttack
======================================
*/

void CThompson::PrimaryAttack(void)
{
	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
			Reload();

		if (m_pPlayer->ammo_9mm <= 0)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
		}

		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags;

#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_9MM, 0, 8, m_pPlayer->pev, m_pPlayer->random_seed);

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireThompson, 0.0, (float*)&g_vecZero, (float*)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.12f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.4f;
}

/*
======================================
Reload
======================================
*/

void CThompson::Reload(void)
{
	if (m_pPlayer->ammo_9mm <= 0)
		return;

	DefaultReload(8, GUN_RELOAD1, 2.5f);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
}

/*
======================================
WeaponIdle
======================================
*/

void CThompson::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	SendWeaponAnim(0, 0);
}
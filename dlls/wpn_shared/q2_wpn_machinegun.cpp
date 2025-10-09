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

LINK_WEAPON_TO_CLASS(weapon_machinegun, CQuake2MachineGun);

/*
======================================
Spawn
======================================
*/

void CQuake2MachineGun::Spawn()
{
	pev->classname = MAKE_STRING("weapon_machinegun");
	Precache();
	m_iId = WEAPON_MACHINEGUN;
	SET_MODEL(ENT(pev), "models/w_9mmhandgun.mdl");

	m_iDefaultAmmo = MP5_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

/*
======================================
Precache
======================================
*/

void CQuake2MachineGun::Precache(void)
{
	PRECACHE_MODEL("models/v_machn.mdl");
	PRECACHE_MODEL("models/w_9mmhandgun.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");

	m_usFireMachinegun = PRECACHE_EVENT(1, "events/machn.sc");
}

/*
======================================
GetItemInfo
======================================
*/

int CQuake2MachineGun::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "_9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MACHINEGUN;
	p->iWeight = MACHINEGUN_WEIGHT;

	return 1;
}

/*
======================================
Deploy
======================================
*/

BOOL CQuake2MachineGun::Deploy()
{
	return DefaultDeploy("models/v_machn.mdl", "models/p_9mmhandgun.mdl", Q2_DRAW, "onehanded", 0);
}

/*
======================================
PrimaryAttack
======================================
*/
#define VectorSet(v, x, y, z)	(v[0]=(x), v[1]=(y), v[2]=(z))

/*
=================
fire_lead

This is an internal support routine used for bullet/pellet based weapons.
=================
*/

Vector fire_lead(edict_t* self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread)
{
	vec3_t		dir, end;
	vec3_t		forward = gpGlobals->v_forward;
	vec3_t		right = gpGlobals->v_right;
	vec3_t		up = gpGlobals->v_up;
	float		r, u;

#ifndef CLIENT_DLL

	TraceResult tr;

	r = RANDOM_FLOAT(-1.0f, 1.0f) * hspread;
	u = RANDOM_FLOAT(-1.0f, 1.0f) * vspread;

	end = start + aimdir + (8192 * forward)
		+ (r * right) + (u * up);

	UTIL_TraceLine(start, end, dont_ignore_monsters, self, &tr);

	if (tr.flFraction < 1.0)
	{
		if ((tr.pHit != nullptr) && (tr.pHit->v.takedamage))
		{
			ClearMultiDamage();
			gMultiDamage.type = DMG_BULLET;
			CBaseEntity::Instance(tr.pHit)->TraceAttack(VARS(self), damage, aimdir, &tr, DMG_BULLET);
			ApplyMultiDamage(VARS(self), VARS(self));
		}
	}
#endif // !CLIENT_DLL

	return Vector(r, u, 0.0);
}

#define DEFAULT_BULLET_HSPREAD	300
#define DEFAULT_BULLET_VSPREAD	500
void CQuake2MachineGun::PrimaryAttack(void)
{
	Vector offset, start;
	int flags, i;

#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	VectorSet(offset, 0, 8, m_pPlayer->pev->view_ofs[2] - 8);

	P_ProjectSource(m_pPlayer->pev->origin, offset, gpGlobals->v_forward, gpGlobals->v_right, start);
	Vector spread = fire_lead(m_pPlayer->edict(), start, gpGlobals->v_forward, 8, 2, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD);

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireMachinegun, 0.0, (float*)&g_vecZero, (float*)&g_vecZero, spread.x, spread.y, machinegun_shots, 0, 0, 0);

	for (i = 1; i < 3; i++)
		m_pPlayer->pev->punchangle[i] = RANDOM_FLOAT(-1.0f, 1.0f) * 0.7f;
	m_pPlayer->pev->punchangle[0] = machinegun_shots * -1.5f;

	// raise the gun as it is firing
	if (!gpGlobals->deathmatch)
	{
		machinegun_shots++;
		if (machinegun_shots > 9)
			machinegun_shots = 9;
	}

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.2f;
}

/*
======================================
ItemPostFrame
======================================
*/

void CQuake2MachineGun::ItemPostFrame(void)
{
	if (!(m_pPlayer->pev->button & IN_ATTACK))
	{
		machinegun_shots = 0;
		m_pPlayer->pev->punchangle = g_vecZero;
	}

	CBasePlayerWeapon::ItemPostFrame();
}

/*
======================================
WeaponIdle
======================================
*/

void CQuake2MachineGun::WeaponIdle(void)
{
	if (m_flTimeWeaponIdle >= UTIL_WeaponTimeBase())
		return;

	SendWeaponAnim(Q2_IDLE);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.0f;
}
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

#if 0

LINK_WEAPON_TO_CLASS(weapon_stubgun, CStubGun);

/*
======================================
Spawn

Spawn function for my gun.
======================================
*/

void CStubGun::Spawn()
{
	pev->classname = MAKE_STRING("weapon_stubgun"); // hack to allow for old names
	Precache();
	m_iId = WEAPON_STUBGUN;
	SET_MODEL(ENT(pev), "models/w_stubgun.mdl");

	m_iDefaultAmmo = STUBGUN_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

/*
======================================
Precache

Precache all assets need for my gun.
======================================
*/

void CStubGun::Precache(void)
{
	PRECACHE_MODEL("models/v_stubgun.mdl");
	PRECACHE_MODEL("models/w_stubgun.mdl");
	PRECACHE_MODEL("models/p_stubgun.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");
}

/*
======================================
GetItemInfo

Send my gun's info to the client.
======================================
*/

int CStubGun::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);	// Gun's name...
	p->pszAmmo1 = "9mm";					// Ammo type...
	p->iMaxAmmo1 = _9MM_MAX_CARRY;			// Max primary ammo...
	p->pszAmmo2 = NULL;						// Secondary Ammo type...
	p->iMaxAmmo2 = -1;						// Max secondary ammo...
	p->iMaxClip = STUBGUN_MAX_CLIP;			// Max clip for your gun...
	p->iSlot = 1;							// Gun's slot position
	p->iPosition = 3;						// Gun's position in slot bucket...
	p->iFlags = 0;							// Special gun flags...
	p->iId = m_iId = WEAPON_STUBGUN;		// Gun's I.D
	p->iWeight = STUBGUN_WEIGHT;			// Gun's weight for auto-switching

	return 1;
}

/*
======================================
Deploy

Bring up my gun, and play a little
animation.
======================================
*/

BOOL CStubGun::Deploy()
{
	return DefaultDeploy(
		"models/v_stubgun.mdl", 
		"models/p_stubgun.mdl", 
		GLOCK_DRAW, 
		"onehanded", 
		0);
}

/*
======================================
PrimaryAttack

My gun's primary attack function.
======================================
*/

void CStubGun::PrimaryAttack(void)
{

}

/*
======================================
SecondaryAttack

My gun's secondary attack function.
======================================
*/

void CStubGun::SecondaryAttack(void)
{

}

/*
======================================
Reload

My gun's reload function.
======================================
*/

void CStubGun::Reload(void)
{
	
}

/*
======================================
ItemPostFrame

My gun's "PostFrame" function that
gets called each and every frame.
======================================
*/

void CStubGun::ItemPostFrame(void)
{

}

/*
======================================
WeaponIdle

My gun's "WeaponIdle" function.
======================================
*/

void CStubGun::WeaponIdle(void)
{

}

#endif
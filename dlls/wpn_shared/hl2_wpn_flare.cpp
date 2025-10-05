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
#include "decals.h"

/*
======================================
	hl_wpn_flare.cpp
	Half-Life 2 beta flaregun..

	History:

	9/14/25:
	Started.
======================================
*/

/*
======================================
Flare projectile.
======================================
*/

#ifndef CLIENT_DLL

#define	SF_FLARE_NO_DLIGHT	0x00000001
#define	SF_FLARE_NO_SMOKE	0x00000002
#define	SF_FLARE_INFINITE	0x00000004
#define	SF_FLARE_START_OFF	0x00000008

#define	FLARE_DURATION		30.0f
#define FLARE_DECAY_TIME	10.0f
#define FLARE_BLIND_TIME	6.0f

class CFlare : public CBaseEntity 
{
private:
	void	Spawn(void);
	void	Precache(void);
	void	EXPORT FlareTouch(CBaseEntity* pOther);
	void	EXPORT FlareBurnTouch(CBaseEntity* pOther);
	void	EXPORT FlareThink(void);
	void	Die (float fadeTime);

	float	m_flNextDamage;
	float	m_flTimeBurnOut;
	int		m_nBounces;			// how many times has this flare bounced?
	BOOL	m_bSmoke;
	EHANDLE m_hOwner;
public:
	static CFlare* CreateFlare(Vector vecVel, Vector vecAngles, edict_t* pOwner, float lifetime);
};

LINK_ENTITY_TO_CLASS(flare, CFlare);

/*
======================================
Flare projectile: 
Precache.
======================================
*/

void CFlare::Precache(void)
{
	PRECACHE_MODEL("models/flare.mdl");
	PRECACHE_SOUND("weapons/flaregun/burn.wav");
}

/*
======================================
Flare projectile: 
Spawn.
======================================
*/

void CFlare::Spawn(void)
{
	SET_MODEL(ENT(pev), "models/flare.mdl");
	pev->classname = ALLOC_STRING("flare");

	SET_SIZE(ENT(pev), Vector(-2, -2, -2), Vector(2, 2, 2));
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	pev->friction = 0.6f;
	pev->gravity = 0.5f;

	//Don't start sparking immediately
	pev->nextthink = gpGlobals->time + 0.5f;
	m_flNextDamage = gpGlobals->time;

	if (pev->owner)
		m_hOwner = Instance(pev->owner);

	SetTouch(&CFlare::FlareTouch);
	SetThink(&CFlare::FlareThink);
}

/*
======================================
Flare projectile: 
CreateFlare.
======================================
*/

CFlare* CFlare::CreateFlare(Vector vecOrg, Vector vecAngles, edict_t* pOwner, float lifetime)
{
	CFlare* pFlare = GetClassPtr((CFlare*)NULL);

	UTIL_SetOrigin(pFlare->pev, vecOrg);

	pFlare->pev->angles = vecAngles;
	pFlare->Spawn();

	pFlare->pev->owner = pOwner;

	//Burn out time
	pFlare->m_flTimeBurnOut = gpGlobals->time + lifetime;

	return pFlare;
}

/*
======================================
Flare projectile: 
FlareThink.
======================================
*/

void CFlare::FlareThink(void)
{
	float	deltaTime = (m_flTimeBurnOut - gpGlobals->time);

	if (m_flTimeBurnOut != -1.0f)
	{
		//Fading away
		if ((deltaTime <= FLARE_DECAY_TIME))
		{
			//m_bFading = true;
			//CSoundEnvelopeController::GetController().SoundChangePitch(m_pBurnSound, 60, deltaTime);
			//CSoundEnvelopeController::GetController().SoundFadeOut(m_pBurnSound, deltaTime);
		}

		//Burned out
		if (m_flTimeBurnOut < gpGlobals->time)
		{
			UTIL_Remove(this);
			return;
		}
	}

	//Act differently underwater
	if (pev->waterlevel > 1)
	{
		UTIL_Bubbles(pev->origin + Vector(-2, -2, -2), pev->origin + Vector(2, 2, 2), 1);
		m_bSmoke = false;
	}
	else
	{
		//Shoot sparks
		if (RANDOM_LONG(0, 8) == 1)
		{
			UTIL_Sparks(pev->origin);
		}
	}

	//Next update
	pev->nextthink = gpGlobals->time + 0.1f;
}

/*
======================================
Flare projectile: 
FlareTouch.
======================================
*/

void CFlare::FlareTouch(CBaseEntity* pOther)
{
	if (!pOther->pev->solid)
		return;

	if ((m_nBounces < 10) && (pev->waterlevel < 1))
	{
		// Throw some real chunks here
		UTIL_Sparks(pev->origin);
	}

	if (pOther && pOther->pev->takedamage)
	{
		if (m_hOwner != NULL)
			CFlame::FlameSpawn(m_hOwner, pOther);
		else
			CFlame::FlameSpawn(NULL, pOther);

		pev->velocity.x *= 0.1f;
		pev->velocity.y *= 0.1f;
		pev->velocity.z *= 0.1f;

		pev->gravity = 1.0f;

		Die(0.5f);
		return;
	}
	else
	{
		// hit the world, check the material type here, see if the flare should stick.
		TraceResult tr = UTIL_GetGlobalTrace();

		//Only do this on the first bounce
		if (m_nBounces == 0)
		{
			float	surfDot = DotProduct(pev->velocity, tr.vecPlaneNormal);

			//Do not stick to ceilings or on shallow impacts
			if ((tr.vecPlaneNormal.z > -0.5f) && (surfDot < -0.9f))
			{
				pev->solid = SOLID_TRIGGER;
				SET_ORIGIN(ENT(pev), tr.vecEndPos + (tr.vecPlaneNormal * 2.0f));
				pev->velocity = g_vecZero;
				pev->movetype = MOVETYPE_NONE;

				m_flNextDamage = 0.0f;
				SetTouch(&CFlare::FlareBurnTouch);
				UTIL_DecalTrace(&tr, DECAL_SCORCH1);
			}
		}

		//Scorch decal
		if (sqrt(pev->velocity.Length()) > (250 * 250))
			UTIL_DecalTrace( &tr, DECAL_SCORCH1 );

		// Change our flight characteristics
		pev->gravity = 0.8f;

		m_nBounces++;

		//After the first bounce, smacking into whoever fired the flare is fair game
		pev->owner = NULL;

		// Slow down
		pev->velocity.x *= 0.8f;
		pev->velocity.y *= 0.8f;

		//Stopped?
		if (pev->velocity.Length() < 64.0f)
		{
			m_flNextDamage = 0.0f;
			pev->velocity = g_vecZero;
			pev->movetype = MOVETYPE_NONE;
			pev->solid = SOLID_TRIGGER;
			SET_ORIGIN(ENT(pev), pev->origin);
			SetTouch(&CFlare::FlareBurnTouch);
		}
	}
}

/*
======================================
Flare projectile: 
FlareBurnTouch.
======================================
*/

void CFlare::FlareBurnTouch(CBaseEntity* pOther)
{
	if (pOther && pOther->pev->takedamage && (m_flNextDamage < gpGlobals->time))
	{
		if (m_hOwner != NULL)
			pOther->TakeDamage(pev, m_hOwner->pev, 1, (DMG_BULLET | DMG_BURN));
		else
			pOther->TakeDamage(pev, pev, 1, (DMG_BULLET | DMG_BURN));

		m_flNextDamage = gpGlobals->time + 1.0f;
	}
}


/*
======================================
Flare projectile:
Die.
======================================
*/

void CFlare::Die(float fadeTime)
{
	m_flTimeBurnOut = gpGlobals->time + fadeTime;

	SetThink(&CFlare::FlareThink);
	pev->nextthink = gpGlobals->time + 0.1f;
}

#endif

/*
======================================
The Actual Flaregun
======================================
*/

LINK_WEAPON_TO_CLASS(weapon_flaregun, CFlareGun);

enum flaregun_e {
	FLAREGUN_IDLE1 = 0,
	FLAREGUN_IDLE2,
	FLAREGUN_IDLE3,
	FLAREGUN_IDLE4,
	FLAREGUN_FIRE1,
	FLAREGUN_FIRE2,
	FLAREGUN_FIRE3,
	FLAREGUN_RELOAD,
	FLAREGUN_DRAW1,
	FLAREGUN_DRAW2
};

/*
======================================
Spawn
======================================
*/

void CFlareGun::Spawn()
{
	pev->classname = MAKE_STRING("weapon_flaregun");
	Precache();
	m_iId = WEAPON_FLAREGUN;
	SET_MODEL(ENT(pev), "models/w_flaregun.mdl");

	m_iDefaultAmmo = FLAREGUN_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

/*
======================================
Precache
======================================
*/

void CFlareGun::Precache(void)
{
	PRECACHE_MODEL("models/v_flaregun.mdl");
	PRECACHE_MODEL("models/w_flaregun.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");

	PRECACHE_SOUND("weapons/flaregun/fire.wav");
	PRECACHE_SOUND("weapons/flaregun/reload.wav");

	m_usFireFlare = PRECACHE_EVENT(1, "events/flare1.sc");
	UTIL_PrecacheOther("flare");
}

/*
======================================
GetItemInfo
======================================
*/

int CFlareGun::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "flares";
	p->iMaxAmmo1 = FLARES_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = FLAREGUN_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_FLAREGUN;
	p->iWeight = FLAREGUN_WEIGHT;

	return 1;
}

/*
======================================
Deploy
======================================
*/

BOOL CFlareGun::Deploy()
{
	return DefaultDeploy("models/v_flaregun.mdl", "models/p_9mmhandgun.mdl", 
		RANDOM_FLOAT(0.0f, 1.0f) < 0.5f ? FLAREGUN_DRAW1 : FLAREGUN_DRAW2, // randomize!!!
		"onehanded", 0);
}

/*
======================================
PrimaryAttack
======================================
*/

void CFlareGun::PrimaryAttack(void)
{
	if (m_iClip == 0)
	{
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= 0)
		{
			Reload();
		}
		else 
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
		}
		return;
	}

#ifndef CLIENT_DLL
	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
	Vector vecVel = gpGlobals->v_forward * 1500;

	CFlare* pFlare = CFlare::CreateFlare(m_pPlayer->GetGunPosition(), UTIL_VecToAngles(vecVel), 
		m_pPlayer->edict(), FLARE_DURATION);
	pFlare->pev->velocity = vecVel;
#endif
	m_iClip--;

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireFlare, 0.0, (float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 0, 0, 0, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 3.0f, 6.0f);
}

/*
======================================
Reload
======================================
*/

void CFlareGun::Reload(void)
{
	DefaultReload(1, FLAREGUN_RELOAD, 1.0f);
}

/*
======================================
ItemPostFrame
======================================
*/

//void CFlareGun::ItemPostFrame(void)
//{
//
//}

/*
======================================
WeaponIdle
======================================
*/

void CFlareGun::WeaponIdle(void)
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	SendWeaponAnim(FLAREGUN_IDLE1 + 
		UTIL_SharedRandomLong(m_pPlayer->random_seed, 0, 3));

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.5f;
}
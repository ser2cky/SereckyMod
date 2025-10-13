/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
=============================================
	q1_weapons.cpp

	Stores all functions needed for "QUAKE1"
	style weapons...

	Pretty much everything you'll see here has
	been converted from the orignal Quake
	"weapons.qc"

	-serecky 10.11.25

	History:
	10/10/25: Created.
=============================================
*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "quakemonster.h"

extern int gmsgQ1TempEnts;
extern int gmsgQParticle;

LINK_WEAPON_TO_CLASS(quakegun, CBaseQuakeWeapon);

void CBaseQuakeWeapon::Precache(void)
{
	PRECACHE_SOUND("weapons/quake/r_exp3.wav"); // new rocket explosion
	PRECACHE_SOUND("weapons/quake/rocket1i.wav"); // spike gun
	PRECACHE_SOUND("weapons/quake/sgun1.wav");
	PRECACHE_SOUND("weapons/quake/guncock.wav"); // player shotgun
	PRECACHE_SOUND("weapons/quake/ric1.wav"); // ricochet (used in c code)
	PRECACHE_SOUND("weapons/quake/ric2.wav"); // ricochet (used in c code)
	PRECACHE_SOUND("weapons/quake/ric3.wav"); // ricochet (used in c code)
	PRECACHE_SOUND("weapons/quake/spike2.wav"); // super spikes
	PRECACHE_SOUND("weapons/quake/tink1.wav"); // spikes tink(used in c code)
	PRECACHE_SOUND("weapons/quake/grenade.wav"); // grenade launcher
	PRECACHE_SOUND("weapons/quake/bounce.wav"); // grenade bounce
	PRECACHE_SOUND("weapons/quake/shotgn2.wav"); // super shotgun
}

/*
================
W_FireAxe
================
*/
void CBaseQuakeWeapon::W_FireAxe( void )
{
	TraceResult tr;
    Vector source;
    Vector org;

    UTIL_MakeVectors(pev->v_angle);
    source = pev->origin + Vector(0, 0, 16);
	UTIL_TraceLine(source, source + gpGlobals->v_forward * 64, dont_ignore_monsters, ENT(pev), &tr);

    if (tr.flFraction == 1.0)
    {
        return;
    }

    org = tr.vecEndPos - gpGlobals->v_forward * 4;

    if (VARS(tr.pHit)->takedamage)
    {
		//tr.pHit->axhitme = TRUE;
        CQuakeMonster::SpawnBlood(org, g_vecZero, 20);
        CBaseEntity::Instance(tr.pHit)->TakeDamage(pev, pev, 20, DMG_GENERIC);
    }
    else
    {
        // hit wall
        EMIT_SOUND(ENT(pev), CHAN_WEAPON, "player/axhit2.wav", 1, ATTN_NORM);
		MESSAGE_BEGIN(MSG_BROADCAST, gmsgQ1TempEnts, NULL);
		WRITE_BYTE(Q1_TE_GUNSHOT);
		WRITE_COORD(org.x);
		WRITE_COORD(org.y);
		WRITE_COORD(org.z);
		MESSAGE_END();
    }
	return;
}

void particle(const float* org, const float* dir, float color, float count)
{
	MESSAGE_BEGIN(MSG_BROADCAST, gmsgQParticle, NULL);
	WRITE_COORD(org[0]);
	WRITE_COORD(org[1]);
	WRITE_COORD(org[2]);
	WRITE_CHAR(org[0]);
	WRITE_CHAR(org[1]);
	WRITE_CHAR(org[2]);
	WRITE_BYTE(count);
	WRITE_BYTE(color);
	MESSAGE_END();
}


//============================================================================

Vector CQuakeMonster::wall_velocity( void )
{
	Vector vel;

	vel = pev->velocity.Normalize();
	vel = (vel + gpGlobals->v_up * (RANDOM_FLOAT(0, 1) - 0.5) + gpGlobals->v_right * (RANDOM_FLOAT(0, 1) - 0.5)).Normalize();
	vel = vel + 2 * gpGlobals->trace_plane_normal;
	vel = vel * 200;

	return vel;
}

/*
================
SpawnMeatSpray
================
*/

void CQuakeMonster::SpawnMeatSpray(Vector org, Vector vel)
{
	CBaseEntity* missile = GetClassPtr((CBaseEntity*)NULL);

	missile->Spawn();
	missile->pev->owner = ENT(pev);
	missile->pev->movetype = MOVETYPE_BOUNCE;
	missile->pev->solid = SOLID_NOT;

	UTIL_MakeVectors(pev->angles);

	missile->pev->velocity = vel;
	missile->pev->velocity.z = missile->pev->velocity.z + 250 + 50 * RANDOM_FLOAT(0, 1);

	missile->pev->avelocity = Vector(3000, 1000, 2000);

	// set missile duration
	missile->pev->nextthink = gpGlobals->time + 1;
	missile->SetThink(&CBaseEntity::SUB_Remove);

	SET_MODEL(ENT(missile->pev), "progs/zom_gib.mdl");
	SET_SIZE(ENT(missile->pev), g_vecZero, g_vecZero);
	SET_ORIGIN(ENT(missile->pev), org);
}


/*
================
SpawnBlood
================
*/
void CQuakeMonster::SpawnBlood(Vector org, Vector vel, float damage)
{
	particle(org, vel * 0.1, 73, damage * 2);
}

/*
================
spawn_touchblood
================
*/
void CQuakeMonster::spawn_touchblood(float damage)
{
	Vector vel;

	vel = wall_velocity() * 0.2;
	SpawnBlood(pev->origin + vel * 0.01, vel, damage);
}

/*
================
SpawnChunk
================
*/
void CQuakeMonster::SpawnChunk(Vector org, Vector vel)
{
	particle(org, vel * 0.02, 0, 10);
}

/*
==============================================================================

BULLETS

==============================================================================
*/

/*
================
TraceAttack
================
*/



void CQuakeMonster::TraceAttack(float damage, Vector dir)
{
	Vector  vel, org;

	org = gpGlobals->trace_endpos - dir * 4;
	vel = (dir + gpGlobals->v_up * RANDOM_FLOAT(-1, 1) + gpGlobals->v_right * RANDOM_FLOAT(-1, 1)).Normalize();
	vel = vel + 2 * gpGlobals->trace_plane_normal;
	vel = vel * 200;

	if (VARS(gpGlobals->trace_ent)->takedamage)
	{
		SpawnBlood(org, vel * 0.2, damage);
		AddMultiDamage(pev, CBaseEntity::Instance(gpGlobals->trace_ent), damage, DMG_GENERIC);
		PARTICLE_EFFECT(org, vel * 0.1, 73, damage * 2);
	
	}
	else
	{
		MESSAGE_BEGIN(MSG_BROADCAST, gmsgQ1TempEnts, NULL);
		WRITE_BYTE(Q1_TE_GUNSHOT);
		WRITE_COORD(org.x);
		WRITE_COORD(org.y);
		WRITE_COORD(org.z);
		MESSAGE_END();

	}
}

/*
================
FireBullets

Used by shotgun, super shotgun, and enemy soldier firing
Go to the trouble of combining multiple pellets into a single damage call.
================
*/

void CQuakeMonster::FireBullets(int shotcount, Vector dir, Vector spread)
{
	TraceResult tr;
	Vector direction;
	Vector src;

	UTIL_MakeVectors(pev->v_angle);

	src = pev->origin + gpGlobals->v_forward * 10;
	src.z = pev->absmin.z + pev->size.z * 0.7;

	ClearMultiDamage();
	while (shotcount > 0)
	{
		direction = dir + RANDOM_FLOAT(-1, 1) * spread.x * gpGlobals->v_right + RANDOM_FLOAT(-1, 1) * spread.y * gpGlobals->v_up;

		UTIL_TraceLine(src, src + direction * 2048, dont_ignore_monsters, ENT(pev), &tr);
		if (tr.flFraction != 1.0)
		{
			TraceAttack(4, direction);
		}

		shotcount = shotcount - 1;
	}
	ApplyMultiDamage(pev, pev);
}

/*
================
W_FireShotgun
================
*/

void CBaseQuakeWeapon::W_FireShotgun(void)
{
	CQuakeMonster* pPlayer = (CQuakeMonster*)GET_PRIVATE(m_pPlayer->edict());
	Vector dir;

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	dir = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	pPlayer->FireBullets(6, dir, Vector(0.04, 0.04, 0) );
} 

/*
================
W_FireSuperShotgun
================
*/

void CBaseQuakeWeapon::W_FireSuperShotgun(void)
{
	CQuakeMonster* pPlayer = (CQuakeMonster*)GET_PRIVATE(m_pPlayer->edict());
	Vector dir;

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 1)
	{
		W_FireShotgun();
		return;
	}

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 2;
	dir = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	pPlayer->FireBullets(14, dir, Vector(0.14, 0.08, 0));
}

/*
==============================================================================

ROCKETS

==============================================================================
*/

void CBaseQuakeWeapon::BecomeExplosion()
{
	pev->movetype = MOVETYPE_NONE;
	pev->velocity = g_vecZero;
	SetThink(&CBaseEntity::SUB_Remove);
	pev->solid = SOLID_NOT;
	pev->nextthink = gpGlobals->time;
};

void CBaseQuakeWeapon::T_MissileTouch(CBaseEntity* pOther)
{
	float damg;

	if (pOther->edict() == pev->owner)
	{
		return; // don't explode on owner
	}

	if (UTIL_PointContents(pev->origin) == CONTENT_SKY)
	{
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	damg = 100 + RANDOM_FLOAT(0.0f, 1.0f) * 20;

	if (pOther->pev->health)
	{
		if (!FClassnameIs(pOther->pev, "shambler"))
		{
			damg = damg * 0.5; // mostly immune
		}
		//TakeDamage(other, pev, pev->owner, damg);
	}

	// don't do radius damage to the other, because all the damage
	// was done in the impact
	RadiusDamage(pev->origin, pev, VARS(pev->owner), 120, 128, 0, DMG_BLAST);

	//sound(pev, CHAN_WEAPON, "weapons/r_exp3.wav", 1, ATTN_NORM);
	pev->origin = pev->origin - 8 * pev->velocity.Normalize();

	MESSAGE_BEGIN(MSG_BROADCAST, gmsgQ1TempEnts, NULL);
	WRITE_BYTE(Q1_TE_EXPLOSION);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	MESSAGE_END();

	BecomeExplosion();
};

/*
================
W_FireRocket
================
*/
void CBaseQuakeWeapon::W_FireRocket(void)
{
	CBaseEntity* missile = GetClassPtr((CBaseEntity*)NULL);

	missile->Spawn();

	missile->pev->owner = m_pPlayer->edict();
	missile->pev->movetype = MOVETYPE_FLYMISSILE;
	missile->pev->solid = SOLID_BBOX;
	missile->pev->classname = ALLOC_STRING("missile");

	// set missile speed
	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	missile->pev->velocity = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	missile->pev->velocity = missile->pev->velocity * 1000;
	missile->pev->angles = UTIL_VecToAngles(missile->pev->velocity);

	missile->SetTouch(&CBaseQuakeWeapon::T_MissileTouch);

	// set missile duration
	missile->SetThink(&CBaseEntity::SUB_Remove);

	SET_MODEL(ENT(missile->pev), "models/rpgrocket.mdl");
	SET_SIZE(ENT(missile->pev), g_vecZero, g_vecZero);
	SET_ORIGIN(ENT(missile->pev), pev->origin + gpGlobals->v_forward * 8 + Vector(0,0,16));

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.8f;
};
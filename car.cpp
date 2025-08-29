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
car.cpp

code for cars you can drive just about everywhere

very very very slightly based on code from gta-quake by 
phlamethrower@quote-egnufeb-quote-greaterthan-colon-hash-comma-underscore-at.info

==================================================================
*/

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"saverestore.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"decals.h"

#define		SF_LOCKED 1 // Car's locked!

class CDriveableCar : public CBaseMonster
{
public:
	void	Precache(void) override;
	void	Spawn(void) override;
	//void	KeyValue(KeyValueData* pkvd);
	virtual int	ObjectCaps(void) override { return CBaseMonster::ObjectCaps() | FCAP_IMPULSE_USE; }

	void	EXPORT Use(CBaseMonster* pActivator, CBaseMonster* pCaller, USE_TYPE useType, float value);
	void	EXPORT Think(void);
	void	EXPORT Explode_Think(void);
	void	EXPORT Smoke(void);
	void	EXPORT Killed(entvars_t* pevAttacker, int iGib);

	void	Explode(TraceResult* pTrace, int bitsDamageType);

	int		TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	void	Car_Tilt(void);

	virtual int		Save(CSave& save) override;
	virtual int		Restore(CRestore& restore) override;

	static	TYPEDESCRIPTION m_SaveData[];

	edict_t*			m_eDriver;
	float				m_flSpeed;
	float				m_flGrip;
	float				m_flIdealGrip;
	int					m_iGear;
};
LINK_ENTITY_TO_CLASS(car, CDriveableCar);

TYPEDESCRIPTION	CDriveableCar::m_SaveData[] =
{
	DEFINE_FIELD(CDriveableCar, m_iGear, FIELD_INTEGER),
	DEFINE_FIELD(CDriveableCar, m_flSpeed, FIELD_FLOAT),
};
IMPLEMENT_SAVERESTORE(CDriveableCar, CBaseEntity);


void CDriveableCar::Killed(entvars_t* pevAttacker, int iGib)
{
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DEAD;

	pev->body = 1;
	pev->flags &= ~FL_ONGROUND;
	pev->movetype = MOVETYPE_TOSS;
	pev->velocity = CGib::VecVelocityForDamage(pev->health);
	pev->velocity.z = pev->velocity.z * 2;

	pev->avelocity.x = (2 * (RANDOM_LONG(0,1) - 0.5)) * 10.0f;
	pev->avelocity.y = (2 * (RANDOM_LONG(0, 1) - 0.5)) * 10.0f;
	pev->avelocity.z = 0.0f;

	SetThink(&CDriveableCar::Explode_Think);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CDriveableCar::Explode_Think ( void )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	vecSpot = pev->origin + Vector(0, 0, 8);
	UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), ignore_monsters, ENT(pev), &tr);

	Explode(&tr, DMG_BLAST);
}

void CDriveableCar::Explode(TraceResult* pTrace, int bitsDamageType)
{
	// Pull out of the wall a bit
	if (pTrace->flFraction != 1.0)
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * (pev->dmg - 24) * 0.6);

	int iContents = UTIL_PointContents(pev->origin);

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_EXPLOSION);		// This makes a dynamic light and the explosion sprites/sound
	WRITE_COORD(pev->origin.x);	// Send to PAS because of the sound
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z); 
	if (iContents != CONTENTS_WATER)
	{
		WRITE_SHORT(g_sModelIndexFireball);
	}
	else
	{
		WRITE_SHORT(g_sModelIndexWExplosion);
	}
	WRITE_BYTE((pev->dmg - 50) * .60); // scale * 10
	WRITE_BYTE(15); // framerate
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();

	RadiusDamage(pev->origin, pev, pev, pev->dmg, 64, CLASS_NONE, bitsDamageType);

	if (RANDOM_FLOAT(0, 1) < 0.5)
		UTIL_DecalTrace(pTrace, DECAL_SCORCH1);
	else
		UTIL_DecalTrace(pTrace, DECAL_SCORCH2);

	SetThink(&CDriveableCar::Smoke);
	pev->nextthink = gpGlobals->time + 0.3;

	if (iContents != CONTENTS_WATER)
	{
		int sparkCount = RANDOM_LONG(0, 3);
		for (int i = 0; i < sparkCount; i++)
			Create("spark_shower", pev->origin, pTrace->vecPlaneNormal, NULL);
	}
}

void CDriveableCar::Smoke(void)
{
	if (UTIL_PointContents(pev->origin) == CONTENTS_WATER)
		UTIL_Bubbles(pev->origin - Vector(64, 64, 64), pev->origin + Vector(64, 64, 64), 100);
	else {
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE((pev->dmg - 50) * 0.80); // scale * 10
		WRITE_BYTE(12); // framerate
		MESSAGE_END();
	}
	SetThink( NULL );
}

int CDriveableCar::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	Vector			vecTemp;
	if (!pev->takedamage)
		return 0;

	if (pevAttacker == pevInflictor) vecTemp = pevInflictor->origin - (VecBModelOrigin(pev));
	else vecTemp = pevInflictor->origin - (VecBModelOrigin(pev));

	g_vecAttackDir = vecTemp.Normalize();

	// do the damage
	pev->health -= flDamage;
	if (pev->health <= 0) {
		Killed(pevAttacker, GIB_NEVER);
		return 0;
	}
	return 1;
}

void CDriveableCar::Car_Tilt(void)
{
	// Some code knicked from Air Quake :)
	TraceResult tr1, tr2, tr3, tr4;
	Vector vec1, vec2, vec3, vec4;
	float fl_1, fl_2, fl_3, fl_4;
	float fl_r1, fl_r2, w1, w2;

	vec1 = pev->angles.y * Vector(0, 1, 0);
	UTIL_MakeVectors(vec1);

	vec1 = pev->origin + (16 * gpGlobals->v_forward) - (16 * gpGlobals->v_right);
	vec2 = pev->origin + (16 * gpGlobals->v_forward) + (16 * gpGlobals->v_right);
	vec3 = pev->origin - (16 * gpGlobals->v_forward) - (16 * gpGlobals->v_right);
	vec4 = pev->origin - (16 * gpGlobals->v_forward) + (16 * gpGlobals->v_right);

	UTIL_TraceLine(vec1 - 16 * gpGlobals->v_up, vec1 - 80 * gpGlobals->v_up, ignore_monsters,
		ENT(pev), &tr1);
	fl_1 = tr1.flFraction;
	UTIL_TraceLine(vec2 - 16 * gpGlobals->v_up, vec2 - 80 * gpGlobals->v_up, ignore_monsters,
		ENT(pev), &tr2);
	fl_2 = tr2.flFraction;
	UTIL_TraceLine(vec3 - 16 * gpGlobals->v_up, vec3 - 80 * gpGlobals->v_up, ignore_monsters,
		ENT(pev), &tr3);
	fl_3 = tr3.flFraction;
	UTIL_TraceLine(vec4 - 16 * gpGlobals->v_up, vec4 - 80 * gpGlobals->v_up, ignore_monsters,
		ENT(pev), &tr4);
	fl_4 = tr4.flFraction;

	if ((fl_1 >= 1.0f) && (fl_2 >= 1.0f) && (fl_3 >= 1.0f) && (fl_4 >= 1.0f)) {
		if (pev->angles.x > 0 && pev->angles.x < 180)
			pev->angles.x = pev->angles.x - 5;
		return;
	}

	fl_r1 = fl_1 - fl_2 + fl_3 - fl_4;
	fl_r2 = fl_1 - fl_3 + fl_2 - fl_4;

	vec1 = Vector(0, 0.5, 0);
	vec1.x = fl_r1;
	vec1.z = fl_r2;
	vec1 = vec1.Normalize();

	vec2.x = vec1.x;
	vec2.y = vec1.z;
	vec2.z = vec1.y;

	vec3 = gpGlobals->v_up * vec2.z + gpGlobals->v_forward * vec2.y - gpGlobals->v_right * vec2.x;
	vec3 = vec3.Normalize();

	if (!w1) {
		w2 = (4 - fl_1 - fl_2 - fl_3 - fl_4) * 64;
		if (w2 > w1)
			w1 = w2;
	}

	UTIL_MakeVectors(pev->angles);

	vec2 = UTIL_VecToAngles(vec1);
	vec3 = Vector(0, 2, 0);
	vec3.x = UTIL_AngleMod(0 - vec2.x);
	vec3.y = pev->angles.y;
	vec3.z = UTIL_AngleMod(vec2.y - 90);

	vec4 = vec3 - pev->angles;
	fl_1 = UTIL_AngleMod(vec4.x);
	if (fl_1 > 180)
		fl_1 = fl_1 - 360;
	fl_2 = UTIL_AngleMod(vec4.z);
	if (fl_2 > 180)
		fl_2 = fl_2 - 360;

	pev->avelocity.x = fl_1 * 4;
	pev->avelocity.z = fl_2 * 4;
};
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
=================================================
	Car.cpp
	Basically a driveable car in Quake. Based off
	bits and pieces of GTAQUAKE's car.qc code!

	History:

	8/16/25:
	Restarted! Only kept Car_Tilt code!

	8/20/25:
	After multiple failed attempts, the car
	now has decent movement code.
=================================================
*/

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"saverestore.h"
#include	"soundent.h"
#include	"weapons.h"
#include	"decals.h"

#define		SF_LOCKED 1 // Car's locked!

/*
=================================================
	Class def. goes here.
=================================================
*/

class CDriveableCar : public CBaseMonster
{
public:
	void Precache ( void );
	void Spawn( void );
	virtual int	ObjectCaps ( void ) { return CBaseMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	void Explode(TraceResult* pTrace, int bitsDamageType);
	int	TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	void EXPORT	Car_Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT	MonsterThink ( void );
	void EXPORT Explode_Think(void);
	void EXPORT Smoke(void);
	void EXPORT Killed(entvars_t* pevAttacker, int iGib);

	void Move ( float flInterval );
	void Car_WalkMove(float yaw, float dist);
	BOOL Car_MoveStep(const Vector& vecMove);
	int CarCheckImpact ( Vector vecStart, Vector vecEnd );
	void Car_Impact ( void );
	void Car_Tilt ( void );
	void Engine_Sound ( void );

	int Classify ( void ) { return CLASS_NONE; };
	int ISoundMask ( void ) { return bits_SOUND_NONE; };

private:
	edict_t*			m_pentDriver;
	entvars_t*			m_pevDriver;
	float				m_flNextEngineSound = 0.0f;
	float				m_flNextExit = 0.0f;
	float				m_flNextImpact;
	float				m_flSpeed;
	float				m_flAccel;
	float				m_flSlide;
	int					m_iDir;
	int					m_flMaxSpeed;
};
LINK_ENTITY_TO_CLASS(car, CDriveableCar);

/*
=================================================
	Precache
=================================================
*/

void CDriveableCar::Precache( void )
{
	PRECACHE_MODEL("models/m44.mdl"); 

	for (int i = 0; i < 10, i++;)
		PRECACHE_SOUND(UTIL_VarArgs("engine/6_%d.wav\n", i));
}

/*
=================================================
	Spawn
=================================================
*/

void CDriveableCar::Spawn( void )
{
	Precache();

	SET_MODEL(ENT(pev), "models/m44.mdl");
	UTIL_SetSize(pev, Vector(-48, -48, -36), Vector(48, 48, 36));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	pev->health = 100;
	pev->takedamage = DAMAGE_AIM;
	pev->friction = 0.55f;

	m_bloodColor = DONT_BLEED;
	m_MonsterState = MONSTERSTATE_NONE;

	m_flSpeed = 0.0f;
	m_flAccel = 0.25f;
	m_flSlide = 0.0f;

	MonsterInit();

	SetUse(&CDriveableCar::Car_Use);
	pev->nextthink = gpGlobals->time;
}

/*
=================================================
	TakeDamage
	Ouch...
=================================================
*/

int CDriveableCar::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if (!pev->takedamage || ENT(pevInflictor) == m_pentDriver || ENT(pevAttacker) == m_pentDriver )
		return 0;

	// do the damage
	pev->health -= flDamage;

	if (pev->health <= 0)
	{
		Killed(pevAttacker, GIB_NEVER);
		return 0;
	}
	return 1;
}

/*
=================================================
	Killed
	Creates the carcass of a dead car that flies
	up into the air.
=================================================
*/

void CDriveableCar::Killed(entvars_t* pevAttacker, int iGib)
{
	if (!FNullEnt(m_pentDriver) && fabs(m_flSpeed) >= 0.65f)
	{
		CLIENT_COMMAND(m_pentDriver, "firstperson\n");

		// We can clear the driver now.
		m_pevDriver = nullptr;
		m_pentDriver = nullptr;
	}

	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DEAD;

	pev->body = 1;
	pev->flags &= ~FL_ONGROUND;
	pev->movetype = MOVETYPE_TOSS;
	pev->velocity = CGib::VecVelocityForDamage(pev->health);
	pev->velocity.z = pev->velocity.z * 2;

	pev->avelocity.x = (2 * (RANDOM_LONG(0, 1) - 0.5)) * 10.0f;
	pev->avelocity.y = (2 * (RANDOM_LONG(0, 1) - 0.5)) * 10.0f;
	pev->avelocity.z = 0.0f;

	SetThink(&CDriveableCar::Explode_Think);
	pev->nextthink = gpGlobals->time + 0.1;
}

/*
=================================================
	Explode_Think
	Taken from ggrenade.cpp
=================================================
*/

void CDriveableCar::Explode_Think(void)
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	vecSpot = pev->origin + Vector(0, 0, 8);
	UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), ignore_monsters, ENT(pev), &tr);

	Explode(&tr, DMG_BLAST);
}

/*
=================================================
	Explode
	Taken from ggrenade.cpp
=================================================
*/

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

	::RadiusDamage(pev->origin, pev, pev, pev->dmg, 64, CLASS_NONE, bitsDamageType);

	if (RANDOM_FLOAT(0, 1) < 0.5)
		UTIL_DecalTrace(pTrace, DECAL_SCORCH1);
	else
		UTIL_DecalTrace(pTrace, DECAL_SCORCH2);

	SetThink(&CDriveableCar::Smoke);
	pev->nextthink = gpGlobals->time + 0.3;
}

/*
=================================================
	Smoke
	Taken from ggrenade.cpp
=================================================
*/

void CDriveableCar::Smoke(void)
{
	if (UTIL_PointContents(pev->origin) == CONTENTS_WATER)
		UTIL_Bubbles(pev->origin - Vector(64, 64, 64), pev->origin + Vector(64, 64, 64), 100);
	else
	{
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
	SetThink(NULL);
}

/*
=================================================
	Car_Tilt
	Handles the car's angles. Stolen from
	Gta Quake, which also takes from AirQuake
=================================================
*/

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

	if (pev->flags & FL_ONGROUND)
	{
		pev->angles.x = (pev->angles.x + (pev->avelocity.x * 0.1));
		pev->angles.z = (pev->angles.z + (pev->avelocity.z * 0.1));
	}
	else
	{
		pev->angles.x = (pev->angles.x + (pev->avelocity.x * 0.025));
		pev->angles.z = (pev->angles.z + (pev->avelocity.z * 0.025));
	}
}

/*
=================================================
	Engine_Sound
	Creates exactly what it says.
=================================================
*/

void CDriveableCar::Engine_Sound(void)
{
	int iIdealsound;

	iIdealsound = round((fabs(m_flSpeed) + 0.015f) * (10 / 1.485f));

	if (m_flNextEngineSound < gpGlobals->time)
	{
		if (iIdealsound <= 9) EMIT_SOUND(ENT(pev), CHAN_BODY, UTIL_VarArgs("engine/6_%d.wav", iIdealsound == 0 ? 1 : iIdealsound), 1.0f, ATTN_NORM);
		else EMIT_SOUND(ENT(pev), CHAN_BODY, "engine/6_10.wav", 1.0f, ATTN_NORM);
		m_flNextEngineSound = gpGlobals->time + 0.09f;
	}
}

/*
=================================================
	MonsterThink
	Handles car physics and player inputs.
=================================================
*/

void CDriveableCar::MonsterThink(void)
{
	m_pevDriver = VARS(m_pentDriver);
	float flInterval = StudioFrameAdvance(); // animate

	pev->nextthink = gpGlobals->time;

	if (!FNullEnt(m_pentDriver))
	{
		UTIL_SetOrigin(m_pevDriver, pev->origin + Vector(0, 0, 48));

		m_pevDriver->solid = SOLID_NOT;
		m_pevDriver->movetype = MOVETYPE_FLY;
		//m_pevDriver->takedamage = DAMAGE_NO;

		// TODO: Create impulse commands that cycle through the player's camera
		// - serecky 8.16.25

		if (m_pevDriver->button & IN_USE && m_flNextExit < gpGlobals->time)
		{
			CLIENT_COMMAND(m_pentDriver, "firstperson\n");

			UTIL_MakeVectors(pev->angles);
			UTIL_SetOrigin(m_pevDriver, pev->origin - gpGlobals->v_right * 64);

			m_pevDriver->solid = SOLID_SLIDEBOX;
			m_pevDriver->movetype = MOVETYPE_WALK;
			m_pevDriver->takedamage = DAMAGE_YES;

			// We can clear the driver now.
			m_pevDriver = nullptr;
			m_pentDriver = nullptr;
		}
	}

	Engine_Sound(); // vroom
	Move(flInterval); // For some reason I can't put all of this into MonsterThink so this is probably
	// the only way to handle movement stuff - serecky 8.16.25
}

/*
=================================================
	Car_Impact
	Handles car accidents. Ugly ones at that.
=================================================
*/

int CDriveableCar::CarCheckImpact(Vector vecStart, Vector vecEnd)
{
	m_pevDriver = VARS(m_pentDriver);
	CBaseEntity* victim = NULL;
	TraceResult tr;

	UTIL_TraceLine(vecStart, vecEnd, ignore_monsters, ENT(pev), &tr);

#if 0
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMPOINTS );
		WRITE_COORD(vecStart.x);
		WRITE_COORD(vecStart.y);
		WRITE_COORD(vecStart.z);
		WRITE_COORD(vecEnd.x);
		WRITE_COORD(vecEnd.y);
		WRITE_COORD(vecEnd.z);
		WRITE_SHORT(g_sModelIndexLaser);
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 1 ); // life
		WRITE_BYTE( 40 );  // width
		WRITE_BYTE( 0 );   // noise
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 160 );   // r, g, b
		WRITE_BYTE( 100 );   // r, g, b
		WRITE_BYTE( 128 );	// brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();
#endif

	if (tr.pHit && tr.pHit->v.takedamage)
	{
		victim = CBaseEntity::Instance(tr.pHit);

		UTIL_MakeVectors(pev->angles);

		victim->pev->flags &= ~FL_ONGROUND;
		victim->pev->origin.z += 1;
		victim->pev->velocity = gpGlobals->v_forward * (200.0f * m_flSpeed);

		ALERT(at_console, "hit entity!\n");
	}

	if (tr.flFraction >= 1.0f)
		return 0;

	// Deflect.
	m_flSpeed *= -0.8f;

	if (!FNullEnt(m_pentDriver) && fabs(m_flSpeed) >= 0.65f)
	{
		CBaseEntity::Instance(m_pevDriver)->TakeDamage(pev, pev, 2.0f * fabs(m_flSpeed), DMG_GENERIC);

		// Cars, being made of metal, take less damage! - serecky 8.18.25
		CBaseEntity::Instance(pev)->TakeDamage(pev, pev, 1.0f * fabs(m_flSpeed), DMG_GENERIC);
	}

	ALERT(at_console, "hit wall!\n");

	return 1;
}


void CDriveableCar::Car_Impact(void)
{
	Vector org, front, right, back, left;

	// The car has a high chance of getting stuck inside walls
	// due to this weird traceline stuff, so I'm gonna add this
	// cooldown timer here to prevent that. - serecky 8.20.25
	if (m_flNextImpact > gpGlobals->time)
		return;

	if (fabs(m_flSpeed) < 0.1f)
		return;

	UTIL_MakeVectors(Vector(-pev->angles.x, pev->angles.y, pev->angles.z));
	org = pev->origin + gpGlobals->v_up * 18.0f;

	front = org + gpGlobals->v_forward * 48.0f;
	back = org - gpGlobals->v_forward * 48.0f;

	// Front.
	if (CarCheckImpact(org, front)) { m_flNextImpact = gpGlobals->time + 0.2f; return; }
	// Rear.
	if (CarCheckImpact(org, back)) { m_flNextImpact = gpGlobals->time + 0.2f; return; }

	// Front right.
	if (CarCheckImpact(org, front + (gpGlobals->v_right * 36.0f))) { m_flNextImpact = gpGlobals->time + 0.4f; return; }
	// Front left.
	if (CarCheckImpact(org, front - (gpGlobals->v_right * 36.0f))) { m_flNextImpact = gpGlobals->time + 0.4f; return; }

	// Rear right.
	if (CarCheckImpact(org, back + (gpGlobals->v_right * 36.0f))) { m_flNextImpact = gpGlobals->time + 0.4f; return; }
	// Rear left.
	if (CarCheckImpact(org, back - (gpGlobals->v_right * 36.0f))) { m_flNextImpact = gpGlobals->time + 0.4f; return; }

	m_flNextImpact = 0.0f;
}

/*
=================================================
	Car_WalkMove
	Taken from physics.cpp in QuakeRemake.

	Replacement for the regular "WALK_MOVE"
	that allows for cars to drive off cliffs
	and all of that goodness.
=================================================
*/

void CDriveableCar::Car_WalkMove(float yaw, float dist)
{
	Vector	move;

	yaw = yaw * M_PI * 2 / 360;

	move.x = cos(yaw) * dist;
	move.y = sin(yaw) * dist;
	move.z = 0.0f;

	Car_MoveStep(move);
}

BOOL CDriveableCar::Car_MoveStep(const Vector& vecMove)
{
	TraceResult	trace;

	// try the move	
	Vector oldorg = pev->origin;
	Vector neworg = pev->origin + vecMove;

	float stepSize = 18.0f;

	// push down from a step height above the wished position
	neworg.z += stepSize;
	Vector end = neworg;
	end.z -= stepSize * 2;

	TRACE_MONSTER_HULL(edict(), neworg, end, dont_ignore_monsters, edict(), &trace);

	if (trace.fAllSolid)
		return FALSE;

	if (trace.fStartSolid)
	{
		neworg.z -= stepSize;
		TRACE_MONSTER_HULL(edict(), neworg, end, dont_ignore_monsters, edict(), &trace);

		if (trace.fAllSolid || trace.fStartSolid)
			return FALSE;
	}

	if (trace.flFraction == 1.0f)
	{
		pev->origin = pev->origin + vecMove;
		UTIL_SetOrigin(pev, pev->origin);

		pev->flags &= ~FL_ONGROUND;
		return TRUE;
	}

	// check point traces down for dangling corners
	pev->origin = trace.vecEndPos;

	if (!ENT_IS_ON_FLOOR(edict()))
	{
		// entity had floor mostly pulled out from underneath it
		// and is trying to correct
		UTIL_SetOrigin(pev, pev->origin);
		return TRUE;
	}

	if (pev->flags & FL_PARTIALGROUND)
		pev->flags |= FL_PARTIALGROUND;

	pev->groundentity = trace.pHit;

	// the move is ok
	UTIL_SetOrigin(pev, pev->origin);

	return TRUE;
}

/*
=================================================
	MonsterThink
	Handles car physics and player inputs.

	Accel stuff taken from:
	https://github.com/bacontsu/halflife-car
=================================================
*/

float Interpolate(float a, float b, float t)
{
	return a + t * (b - a);
}

void CDriveableCar::Move(float flInterval)
{
	static Vector ideal_angles = g_vecZero;
	m_pevDriver = VARS(m_pentDriver);

	UTIL_MakeVectors(pev->angles);

	if (!FNullEnt(m_pentDriver))
	{
		pev->ideal_yaw = pev->angles.y;

		if (m_pevDriver->button & IN_FORWARD)
			m_flSpeed = Interpolate(m_flSpeed, 4.5f, m_flAccel * gpGlobals->frametime);
		else if (m_pevDriver->button & IN_BACK)
			m_flSpeed = Interpolate(m_flSpeed, -4.5f, m_flAccel * gpGlobals->frametime);
		else
			m_flSpeed = Interpolate(m_flSpeed, 0.0f, gpGlobals->frametime);

		if (m_pevDriver->button & IN_MOVELEFT)
		{
			m_flSlide = 0.8f * m_flSpeed / 1.5f;
			m_iDir = 0;
		}
		else if (m_pevDriver->button & IN_MOVERIGHT)
		{
			m_flSlide = 0.8f * m_flSpeed / 1.5f;
			m_iDir = 1;
		}
		else
			m_flSlide = Interpolate(m_flSlide, 0.0f, 12.0f * gpGlobals->frametime);

		if (m_pevDriver->button)
			ALERT(at_console, "Button: %d\nSpeed: %f\nFric: %f\n", m_pevDriver->button, m_flSpeed, m_flSlide);
	}
	else
	{
		m_flSpeed = Interpolate(m_flSpeed, 0.0f, gpGlobals->frametime);
		m_flSlide = Interpolate(m_flSlide, 0.0f, 12.0f * gpGlobals->frametime);
	}

//	This doesn't work well unfortunately :(
//	if (pev->flags & FL_ONGROUND && pev->waterlevel != 3)
//		pev->velocity = gpGlobals->v_forward * m_flSpeed * 750;

	Car_WalkMove(pev->ideal_yaw, m_flSpeed);

	if (m_iDir) ideal_angles.y -= m_flSlide;
	else ideal_angles.y += m_flSlide;

	pev->angles.y = Interpolate(pev->angles.y, ideal_angles.y, 4.0f * gpGlobals->frametime);

	Car_Impact();

	Car_Tilt();
	pev->angles = pev->angles + (pev->avelocity * gpGlobals->frametime);
}

/*
=================================================
	Use
	Sets the car's driver.
=================================================
*/

void CDriveableCar::Car_Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (pCaller != NULL && pCaller->IsPlayer())
	{
		m_flNextExit = gpGlobals->time + 0.5f;

		m_pentDriver = ENT(pCaller->pev);
		ALERT(at_console, "%Driver's entindex: d\n", CBaseEntity::Instance(m_pentDriver)->entindex());

		CLIENT_COMMAND(m_pentDriver, "thirdperson\n");
		CLIENT_COMMAND(m_pentDriver, "cam_idealdist 256\n");
		CLIENT_COMMAND(m_pentDriver, "cam_idealyaw 0\n");
	}

}
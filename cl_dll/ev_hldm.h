//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#pragma once
#include "pm_defs.h"

// bullet types
typedef	enum
{
	BULLET_NONE = 0,
	BULLET_PLAYER_9MM, // glock
	BULLET_PLAYER_MP5, // mp5
	BULLET_PLAYER_357, // python
	BULLET_PLAYER_BUCKSHOT, // shotgun
	BULLET_PLAYER_CROWBAR, // crowbar swipe

	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM,
} Bullet;

#define	GAUSS_PRIMARY_CHARGE_VOLUME	256// how loud gauss is while charging
#define GAUSS_PRIMARY_FIRE_VOLUME	450// how loud gauss is when discharged

#define VECTOR_CONE_1DEGREES Vector( 0.00873, 0.00873, 0.00873 )
#define VECTOR_CONE_2DEGREES Vector( 0.01745, 0.01745, 0.01745 )
#define VECTOR_CONE_3DEGREES Vector( 0.02618, 0.02618, 0.02618 )
#define VECTOR_CONE_4DEGREES Vector( 0.03490, 0.03490, 0.03490 )
#define VECTOR_CONE_5DEGREES Vector( 0.04362, 0.04362, 0.04362 )
#define VECTOR_CONE_6DEGREES Vector( 0.05234, 0.05234, 0.05234 )
#define VECTOR_CONE_7DEGREES Vector( 0.06105, 0.06105, 0.06105 )	
#define VECTOR_CONE_8DEGREES Vector( 0.06976, 0.06976, 0.06976 )
#define VECTOR_CONE_9DEGREES Vector( 0.07846, 0.07846, 0.07846 )
#define VECTOR_CONE_10DEGREES Vector( 0.08716, 0.08716, 0.08716 )
#define VECTOR_CONE_15DEGREES Vector( 0.13053, 0.13053, 0.13053 )
#define VECTOR_CONE_20DEGREES Vector( 0.17365, 0.17365, 0.17365 )

// Based off weapon.cfg from all of the
// weapons in RTCW. - serecky 9.2.25
enum rtcw_anims_e {
	GUN_IDLE = 0,
	GUN_IDLE1,
	GUN_ATTACK1,
	GUN_ATTACK2,
	GUN_ATTACK3,
	GUN_DROP,
	GUN_RAISE,
	GUN_RELOAD1,
	GUN_RELOAD2,
	GUN_RELOAD3,
	GUN_ALT_SWITCH_FROM,
	GUN_ALT_SWITCH_TO
};

enum q2_anims_e {
	Q2_DRAW = 0,
	Q2_FIRE,
	Q2_IDLE,
	Q2_HOLSTER
};

bool CheckPVS(int playerIndex);
void EV_HLDM_GunshotDecalTrace( pmtrace_t *pTrace, char *decalName );
void EV_HLDM_DecalGunshot( pmtrace_t *pTrace, int iBulletType );
int EV_HLDM_CheckTracer( int idx, float *vecSrc, float *end, float *forward, float *right, int iBulletType, int iTracerFreq, int *tracerCount );
void EV_HLDM_FireBullets( int idx, float *forward, float *right, float *up, int cShots, float *vecSrc, float *vecDirShooting, float flDistance, int iBulletType, int iTracerFreq, int *tracerCount, float flSpreadX, float flSpreadY );
void EV_Q2_FireBullets(int idx, float* start, float* aimdir, float* forward, float* right, float* up, int te_impact, int hspread, int vspread);

void EV_FireGlock1(event_args_s* args);
void EV_FireGlock2(event_args_s* args);
void EV_FireShotGunSingle(event_args_s* args);
void EV_FireShotGunDouble(event_args_s* args);
void EV_FireMP5(event_args_s* args);
void EV_FireMP52(event_args_s* args);
void EV_FirePython(event_args_s* args);
void EV_FireGauss(event_args_s* args);
void EV_SpinGauss(event_args_s* args);
void EV_Crowbar(event_args_s* args);
void EV_FireCrossbow(event_args_s* args);
void EV_FireCrossbow2(event_args_s* args);
void EV_FireRpg(event_args_s* args);
void EV_EgonFire(event_args_s* args);
void EV_EgonStop(event_args_s* args);
void EV_HornetGunFire(event_args_s* args);
void EV_TripmineFire(event_args_s* args);
void EV_SnarkFire(event_args_s* args);
void EV_FireRailgun(event_args_s* args);
void EV_FireFlare(event_args_s* args);
void EV_FireMachinegun(event_args_t* args);

void EV_TrainPitchAdjust(event_args_s* args);

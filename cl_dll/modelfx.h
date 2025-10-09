#pragma once

//
// muzzle flashes / player effects
//
#define	MZ_BLASTER			0
#define MZ_MACHINEGUN		1
#define	MZ_SHOTGUN			2
#define	MZ_CHAINGUN1		3
#define	MZ_CHAINGUN2		4
#define	MZ_CHAINGUN3		5
#define	MZ_RAILGUN			6
#define	MZ_ROCKET			7
#define	MZ_GRENADE			8
#define	MZ_LOGIN			9
#define	MZ_LOGOUT			10
#define	MZ_RESPAWN			11
#define	MZ_BFG				12
#define	MZ_SSHOTGUN			13
#define	MZ_HYPERBLASTER		14
#define	MZ_ITEMRESPAWN		15

void R_RunParticleEffect(vec3_t org, vec3_t dir, int color, int count);
void R_RocketTrail(vec3_t start, vec3_t end, int type);

void R_BlobExplosion(vec3_t org);
void R_ParticleExplosion(vec3_t org);
void R_LavaSplash(vec3_t org);
void R_TeleportSplash(vec3_t org);
void R_ParticleEffect(vec3_t org, vec3_t dir, int color, int count);
void R_RailTrail(vec3_t start, vec3_t end, vec3_t angles);
void R_ParseMuzzleFlash(int weapon, vec3_t angles, vec3_t origin);

void CL_ClearTEnts(void);
void CL_SmokeAndFlash(vec3_t origin);
void CL_AddTEnts(void);
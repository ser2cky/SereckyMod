#pragma once

#define	Q1_TE_SPIKE				0
#define	Q1_TE_SUPERSPIKE		1
#define	Q1_TE_GUNSHOT			2
#define	Q1_TE_EXPLOSION			3
#define	Q1_TE_TAREXPLOSION		4
#define	Q1_TE_LIGHTNING1		5
#define	Q1_TE_LIGHTNING2		6
#define	Q1_TE_WIZSPIKE			7
#define	Q1_TE_KNIGHTSPIKE		8
#define	Q1_TE_LIGHTNING3		9
#define	Q1_TE_LAVASPLASH		10
#define	Q1_TE_TELEPORT			11
#define Q1_TE_EXPLOSION2		12

class CQuakeMonster : public CBaseMonster
{
public:
	edict_t* multi_ent;
	float multi_damage;

	Vector wall_velocity(void);
	void SpawnMeatSpray(Vector org, Vector vel);
	static void SpawnBlood(Vector org, Vector vel, float damage);
	void spawn_touchblood(float damage);
	void SpawnChunk(Vector org, Vector vel);
	void TraceAttack(float damage, Vector dir);
	void FireBullets(int shotcount, Vector dir, Vector spread);
};
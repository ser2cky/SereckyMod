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
//
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"
#include "modelfx.h"

#define MAX_CLIENTS 32

extern BEAM *pBeam;
extern BEAM *pBeam2;

/// USER-DEFINED SERVER MESSAGE HANDLERS

int CHud :: MsgFunc_ResetHUD(const char *pszName, int iSize, void *pbuf )
{
	ASSERT( iSize == 0 );

	// clear all hud data
	HUDLIST *pList = m_pHudList;

	while ( pList )
	{
		if ( pList->p )
			pList->p->Reset();
		pList = pList->pNext;
	}

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	return 1;
}

void CAM_ToFirstPerson(void);

void CHud :: MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf )
{
	CAM_ToFirstPerson();
}

void CHud :: MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf )
{
	// prepare all hud data
	HUDLIST *pList = m_pHudList;

	while (pList)
	{
		if ( pList->p )
			pList->p->InitHUDData();
		pList = pList->pNext;
	}

	//Probably not a good place to put this.
	pBeam = pBeam2 = NULL;
}


int CHud :: MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_Teamplay = READ_BYTE();

	return 1;
}


int CHud :: MsgFunc_Damage(const char *pszName, int iSize, void *pbuf )
{
	int		armor, blood;
	Vector	from;
	int		i;
	float	count;
	
	BEGIN_READ( pbuf, iSize );
	armor = READ_BYTE();
	blood = READ_BYTE();

	for (i=0 ; i<3 ; i++)
		from[i] = READ_COORD();

	count = (blood * 0.5) + (armor * 0.5);

	if (count < 10)
		count = 10;

	// TODO: kick viewangles,  show damage visually

	return 1;
}

int CHud :: MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iConcussionEffect = READ_BYTE();
	if (m_iConcussionEffect)
		this->m_StatusIcons.EnableIcon("dmg_concuss",255,160,0);
	else
		this->m_StatusIcons.DisableIcon("dmg_concuss");
	return 1;
}

int CHud::MsgFunc_SendAnim(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int iAnim = READ_BYTE();
	int body = READ_BYTE();

	cl_entity_t* view = gEngfuncs.GetViewModel();

	// Tell animation system new info
	view->latched.prevsequence = view->curstate.sequence;
	gEngfuncs.pfnWeaponAnim(iAnim, body);

	return 1;
}

/*
=================
MsgFunc_Q1ParseTEnt
=================
*/

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

int CHud::MsgFunc_Q1ParseTEnt(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int		type;
	vec3_t	pos;
	dlight_t* dl;
	particledan_t* p;
	int		rnd;
	int		colorStart, colorLength;

	type = READ_BYTE();
	switch (type)
	{
	case Q1_TE_WIZSPIKE:			// spike hitting wall
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		R_RunParticleEffect(pos, vec3_origin, 20, 30);
		gEngfuncs.pfnPlaySoundByNameAtLocation("wizard/hit.wav", 1.0f, pos);
		break;

	case Q1_TE_KNIGHTSPIKE:			// spike hitting wall
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		R_RunParticleEffect(pos, vec3_origin, 226, 20);
		gEngfuncs.pfnPlaySoundByNameAtLocation("hknight/hit.wav", 1.0f, pos);
		break;

	case Q1_TE_SPIKE:			// spike hitting wall
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		R_RunParticleEffect(pos, vec3_origin, 0, 10);
		if (rand() % 5)
			gEngfuncs.pfnPlaySoundByNameAtLocation("weapons/quake/tink1.wav", 1.0f, pos);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				gEngfuncs.pfnPlaySoundByNameAtLocation("weapons/quake/ric1.wav", 1.0f, pos);
			else if (rnd == 2)
				gEngfuncs.pfnPlaySoundByNameAtLocation("weapons/quake/ric2.wav", 1.0f, pos);
			else
				gEngfuncs.pfnPlaySoundByNameAtLocation("weapons/quake/ric3.wav", 1.0f, pos);
		}
		break;
	case Q1_TE_SUPERSPIKE:			// super spike hitting wall
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		R_RunParticleEffect(pos, vec3_origin, 0, 20);

		if (rand() % 5)
			gEngfuncs.pfnPlaySoundByNameAtLocation("weapons/quake/tink1.wav", 1.0f, pos);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				gEngfuncs.pfnPlaySoundByNameAtLocation("weapons/quake/ric1.wav", 1.0f, pos);
			else if (rnd == 2)
				gEngfuncs.pfnPlaySoundByNameAtLocation("weapons/quake/ric2.wav", 1.0f, pos);
			else
				gEngfuncs.pfnPlaySoundByNameAtLocation("weapons/quake/ric3.wav", 1.0f, pos);
		}
		break;

	case Q1_TE_GUNSHOT:			// bullet hitting wall
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		R_RunParticleEffect(pos, vec3_origin, 0, 20);
		break;

	case Q1_TE_EXPLOSION:			// rocket explosion
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		R_ParticleExplosion(pos);
		dl = gEngfuncs.pEfxAPI->CL_AllocDlight(1);
		VectorCopy(pos, dl->origin);
		dl->radius = 350;
		dl->die = gEngfuncs.GetClientTime() + 0.5;
		dl->decay = 300;
		dl->color.r = dl->color.g = dl->color.b = 255;
		gEngfuncs.pfnPlaySoundByNameAtLocation("weapons/quake/r_exp3.wav", 1.0f, pos);

		p = gHUD.m_ParticleDan.AllocParticle();
		VectorCopy(pos, p->org);
		p->model = "sprites/s_explod.spr";
		p->color = Vector(1.0f, 1.0f, 1.0f);
		p->rendermode = kRenderTransTexture;
		p->alpha = 1.0f;
		p->frame = 0;
		p->max_frames = 6;
		p->framerate = 10.0f;
		p->die = gEngfuncs.GetClientTime() + 1.0f;
		p->scale = { 24.0f, 24.0f };
		p->flags = PDAN_ANIMATE_DIE;
		break;

	case Q1_TE_TAREXPLOSION:			// tarbaby explosion
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		R_BlobExplosion(pos);

		gEngfuncs.pfnPlaySoundByNameAtLocation("weapons/quake/r_exp3.wav", 1.0f, pos);
		break;

	case Q1_TE_LIGHTNING1:				// lightning bolts
		//CL_ParseBeam(Mod_ForName("progs/bolt.mdl", true));
		break;

	case Q1_TE_LIGHTNING2:				// lightning bolts
		//CL_ParseBeam(Mod_ForName("progs/bolt2.mdl", true));
		break;

	case Q1_TE_LIGHTNING3:				// lightning bolts
		//CL_ParseBeam(Mod_ForName("progs/bolt3.mdl", true));
		break;

		// PGM 01/21/97 
	//case Q1_TE_BEAM:				// grappling hook beam
		//CL_ParseBeam(Mod_ForName("progs/beam.mdl", true));
		break;
		// PGM 01/21/97

	case Q1_TE_LAVASPLASH:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		R_LavaSplash(pos);
		break;

	case Q1_TE_TELEPORT:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		R_TeleportSplash(pos);
		break;

	case Q1_TE_EXPLOSION2:				// color mapped explosion
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		colorStart = READ_BYTE();
		colorLength = READ_BYTE();
		//R_ParticleExplosion2(pos, colorStart, colorLength);
		dl = gEngfuncs.pEfxAPI->CL_AllocDlight(1);
		VectorCopy(pos, dl->origin);
		dl->radius = 350;
		dl->die = gEngfuncs.GetClientTime() + 0.5;
		dl->decay = 300;
		dl->color.r = dl->color.g = dl->color.b = 255;
		gEngfuncs.pfnPlaySoundByNameAtLocation("weapons/quake/r_exp3.wav", 1.0f, pos);
		break;

#ifdef QUAKE2

#endif
	default:
		break;
	}
	return 1;
}

/*
=================
ParseParticleEffect
=================
*/

int CHud::MsgFunc_ParseParticleEffect(const char* pszName, int iSize, void* pbuf)
{
	vec3_t		org, dir;
	int			i, count, msgcount, color;

	for (i = 0; i < 3; i++)
		org[i] = READ_COORD();
	for (i = 0; i < 3; i++)
		dir[i] = READ_CHAR() * (1.0 / 16);
	msgcount = READ_BYTE();
	color = READ_BYTE();

	if (msgcount == 255)
		count = 1024;
	else
		count = msgcount;

	R_RunParticleEffect(org, dir, color, count);
	return 1;
}
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
#pragma once

// Struct for Doom-styled sprite tables!

typedef struct state_s
{
	HSPRITE				sprite;
	int					frame;
	float				tics;
	void				(*action);
	int					nextstate;
} state_t;

class CSpriteObject
{
public:
	void Init(void);
	void VidInit(void);
	void SpriteThink(float frametime, float time);
	void DrawSprite(float frame, HSPRITE model, vec3_t origin);
};
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

/*
=============================================
	sprites.cpp
	For handling of Doom-Styled sprites.
	- serecky 9.27.25

	Todo:
	Tidy this file up.
	Add rotating sprite objects for monsters!

	History:

	9/27/25:
	Created.

	10/1/25:
	Added weapon struct.

	10/2/25:
	Made weapon sprites like ParticleDans
	on crack.
=============================================
*/

#include "hud.h"
#include "cl_util.h"
#include "triangleapi.h"
#include "com_model.h"
#include "my_sprites.h"

#include <iostream>
#include <vector>

static spr_object_t* active_sprites, * free_sprites;
static spr_object_t* sprites = NULL;
spr_object_t* gun = NULL;
spr_object_t* flash = NULL;

int r_numsprites = MAX_SPRITE_OBJS;

extern vec3_t v_angles;

//============================================
//	CSpriteObject::Init
//============================================

int CSpriteObject::Init(void)
{
	InitSprites();
	return 1;
}

//============================================
//	CSpriteObject::Init
//============================================

int CSpriteObject::VidInit(void)
{
	ClearSprites();
	return 1;
}

//============================================
//	CSpriteObject::InitSprites
//============================================

void CSpriteObject::InitSprites(void)
{
	sprites = (spr_object_t*)malloc(r_numsprites * sizeof(spr_object_t));
	ClearSprites();
}

//============================================
//	CSpriteObject::ClearSprites
//============================================

void CSpriteObject::ClearSprites(void)
{
	int		i;

	free_sprites = &sprites[0];
	active_sprites = NULL;
	gun = NULL;
	flash = NULL;

	for (i = 0; i < r_numsprites; i++)
		sprites[i].next = &sprites[i + 1];
	sprites[r_numsprites - 1].next = NULL;
}

//============================================
//	CSpriteObject::Shutdown
//============================================

void CSpriteObject::Shutdown(void)
{
	ClearSprites();
	if (sprites)
		free(sprites);
	sprites = NULL;
}

//============================================
//	CSpriteObject::Init
//============================================

float DoomTic(float tic)
{
	return tic / 35.0f;
}

spr_object_t* CSpriteObject::R_AllocSpriteObject(void)
{
	spr_object_t* spr;

	if (!free_sprites)
	{
		gEngfuncs.Con_Printf("Sorry! No free sprites! Limit is: 4096\nPlease optimize particle usage! Thanks!\n");
		return NULL;
	}

	spr = free_sprites;
	free_sprites = spr->next;
	spr->next = active_sprites;
	active_sprites = spr;

	VectorClear(spr->org);
	VectorClear(spr->color);

	spr->active_sprite = nullptr;
	spr->nextthink = 0.0f;
	spr->brightness = 0.0f;
	spr->frame = 0;
	spr->flags = 0;
	spr->type = spr_obj_world;

	spr->mode = 0;
	spr->fire_frame = { 0, 0 };
	spr->raise_frame = { 0, 0 };
	spr->lower_frame = { 0, 0 };
	spr->idle_frame = { 0, 0 };
	spr->flash_frame = { -1, 0 };

	return spr;
}

//============================================
//	CSpriteObject::SpriteThink
//============================================

void CSpriteObject::SpriteThink(float frametime, float time)
{
	spr_object_t *spr, *kill;
	cl_entity_t* view;

	view = gEngfuncs.GetViewModel();

	if (frametime <= 0.0f)
		return;

	for (;; )
	{
		kill = active_sprites;
		if (kill && (kill->flags & SPR_DIE_NOW) != 0)
		{
			active_sprites = kill->next;
			kill->next = free_sprites;
			free_sprites = kill;
			gEngfuncs.Con_Printf("killed a sprite!\n");
			continue;
		}
		break;
	}

	for (spr = active_sprites; spr; spr = spr->next)
	{
		for (;; )
		{
			kill = spr->next;
			if (kill && (kill->flags & SPR_DIE_NOW) != 0 < time)
			{
				spr->next = kill->next;
				kill->next = free_sprites;
				free_sprites = kill;
				gEngfuncs.Con_Printf("killed a sprite!\n");
				continue;
			}
			break;
		}

		if (spr->type == spr_obj_weapon)
		{
			if ((gHUD.m_iKeyBits & IN_ATTACK) && (spr->mode != SPR_GUN_FIRING))
			{
				gEngfuncs.Con_Printf("test!\n");
				spr->frame = spr->fire_frame[0];
				spr->mode = SPR_GUN_FIRING;
			}

			if ((time >= spr->nextthink) || (spr->nextthink - time) > spr->active_sprite[spr->frame].tics)
			{
				if (spr->active_sprite[spr->active_sprite[spr->frame].nextstate].action != nullptr)
					spr->active_sprite[spr->active_sprite[spr->frame].nextstate].action();
				if (spr->frame == spr->flash_frame[1])
					spr->flags = SPR_DIE_NOW;
				spr->frame = spr->active_sprite[spr->frame].nextstate;
				spr->nextthink = time + spr->active_sprite[spr->frame].tics;
			}

			// Pretty iffy...
			if (CheckRangeInt(spr->frame, spr->fire_frame[0], spr->fire_frame[1]) == TRUE)
				spr->mode = SPR_GUN_FIRING;
			if (CheckRangeInt(spr->frame, spr->raise_frame[0], spr->raise_frame[1]) == TRUE)
				spr->mode = SPR_GUN_RAISING;
			if (CheckRangeInt(spr->frame, spr->lower_frame[0], spr->lower_frame[1]) == TRUE)
				spr->mode = SPR_GUN_LOWERING;
			if (CheckRangeInt(spr->frame, spr->idle_frame[0], spr->idle_frame[1]) == TRUE)
				spr->mode = SPR_GUN_IDLE;

			// Only firing gets it's own "if" statement!!!
			// - serecky 10.2.25
			if (spr->mode == SPR_GUN_FIRING)
			{
				// Create Muzzle-Flash sprite.
				if (spr->flash_frame[0] != -1)
				{
					//if (!flash)
					//	flash = R_AllocSpriteObject();
					//else
					//{
					//	flash->type = spr_obj_weapon;
					//	flash->frame = spr->flash_frame[0];
					//}
				}
			}

			if (view)
				spr->org = view->origin;

			gEngfuncs.pfnConsolePrint(UTIL_VarArgs("\n\n\nCur. Frame: %d, Tics: %.2f, Next Frame: %d", 
				spr->active_sprite[spr->frame].frame, spr->active_sprite[spr->frame].tics, spr->active_sprite[spr->frame].nextstate));

			DrawSprite(spr, spr->org);
		}
	}
}

//============================================
//	CSpriteObject::Init
//============================================

void CSpriteObject::DrawSprite(spr_object_t* spr, vec3_t origin)
{
	if (!spr)
		return;

	vec3_t angles, forward, right, up, screen;
	float scale[2] = 
	{ 
		SPR_Width(spr->active_sprite[spr->frame].sprite, spr->active_sprite[spr->frame].frame),
		SPR_Height(spr->active_sprite[spr->frame].sprite, spr->active_sprite[spr->frame].frame) * 1.2f
	};
	float sprite_scale = 0.025f;

	angles = v_angles;
	AngleVectors(angles, forward, right, up);

	VectorMA(origin, 8.0f, forward, origin);
	VectorScale(right, scale[0] * sprite_scale, right);
	VectorScale(up, scale[1] * sprite_scale, up);

	VectorMA(origin, spr->active_sprite[spr->frame].x, right, origin);
	VectorMA(origin, spr->active_sprite[spr->frame].y, up, origin);

	// START RENDERING!!!
	gEngfuncs.pTriAPI->RenderMode(kRenderTransTexture);
	gEngfuncs.pTriAPI->Color4f(spr->color[0], spr->color[1], spr->color[2], 1.0f);
	gEngfuncs.pTriAPI->Brightness(spr->brightness);

	if (!gEngfuncs.GetSpritePointer(spr->active_sprite[spr->frame].sprite))
		return;

	gEngfuncs.pTriAPI->SpriteTexture((struct model_s*)gEngfuncs.GetSpritePointer(
		spr->active_sprite[spr->frame].sprite), spr->active_sprite[spr->frame].frame);

	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad

	//top left
	gEngfuncs.pTriAPI->TexCoord2f(0, 0);
	gEngfuncs.pTriAPI->Vertex3f(origin[0] - right[0] + up[0], origin[1] - right[1] + up[1], origin[2] - right[2] + up[2]);

	//bottom left
	gEngfuncs.pTriAPI->TexCoord2f(0, 1);
	gEngfuncs.pTriAPI->Vertex3f(origin[0] - right[0] - up[0], origin[1] - right[1] - up[1], origin[2] - right[2] - up[2]);

	//bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1, 1);
	gEngfuncs.pTriAPI->Vertex3f(origin[0] + right[0] - up[0], origin[1] + right[1] - up[1], origin[2] + right[2] - up[2]);

	//top right
	gEngfuncs.pTriAPI->TexCoord2f(1, 0);
	gEngfuncs.pTriAPI->Vertex3f(origin[0] + right[0] + up[0], origin[1] + right[1] + up[1], origin[2] + right[2] + up[2]);

	gEngfuncs.pTriAPI->End(); //end our list of vertexes
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
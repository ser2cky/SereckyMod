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
	spr->frame = 0;
	spr->nextthink = 0.0f;
	spr->brightness = 0.0f;
	spr->flags = 0;

	VectorClear(spr->color2);
	spr->active_overlay = nullptr;
	spr->frame2 = 0;
	spr->nextthink2 = 0.0f;
	spr->brightness2 = 0.0f;
	spr->flags2 = 0;

	spr->type = spr_obj_world;

	spr->mode = 0;
	spr->old_ofs = { 0.0f, 0.0f };
	spr->ofs = { 0.0f, 0.0f };
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

		if (spr->type >= spr_obj_weapon)
		{
			if (view)
				spr->org = view->origin;

			if ((gHUD.m_iKeyBits & IN_USE) && (spr->mode == SPR_GUN_IDLE))
			{
				spr->frame = spr->lower_frame[0];
				spr->nextthink = time + spr->active_sprite[spr->lower_frame[0]].tics;

				spr->mode = SPR_GUN_LOWERING;
				DrawSprite(spr);
				return;
			}

			if ((gHUD.m_iKeyBits & IN_ATTACK2) && (spr->mode == SPR_GUN_IDLE))
			{
				spr->frame = spr->raise_frame[0];
				spr->nextthink = time + spr->active_sprite[spr->raise_frame[0]].tics;

				spr->mode = SPR_GUN_RAISING;
				DrawSprite(spr);
				return;
			}

			if (spr->mode == SPR_GUN_LOWERING || spr->mode == SPR_GUN_RAISING)
			{
				spr->old_ofs[0] = Interpolate(spr->old_ofs[0], spr->ofs[0], frametime * 12.0f);
				spr->old_ofs[1] = Interpolate(spr->old_ofs[1], spr->ofs[1], frametime * 12.0f);
			}

			if ((gHUD.m_iKeyBits & IN_ATTACK) && (spr->mode == SPR_GUN_IDLE))
			{
				// Set Muzzle-Flash frame.
				spr->frame2 = spr->flash_frame[0];
				spr->nextthink2 = time + spr->active_overlay[spr->flash_frame[0]].tics;

				// Set actual gun frame.
				spr->frame = spr->fire_frame[0];
				spr->nextthink = time + spr->active_sprite[spr->fire_frame[0]].tics;

				spr->mode = SPR_GUN_FIRING;

				DrawSprite(spr);
				return;
			}

			if ((time >= spr->nextthink) || (spr->nextthink - time) > spr->active_sprite[spr->frame].tics)
			{
				if (spr->active_sprite[spr->active_sprite[spr->frame].nextstate].action != nullptr)
					spr->active_sprite[spr->active_sprite[spr->frame].nextstate].action(spr, frametime, time);
				spr->frame = spr->active_sprite[spr->frame].nextstate;
				spr->nextthink = time + spr->active_sprite[spr->frame].tics;
			}

			// Thinker for evil overlays.
			if ((time >= spr->nextthink2) || (spr->nextthink2 - time) > spr->active_overlay[spr->frame2].tics)
			{
				if (spr->active_overlay[spr->active_overlay[spr->frame2].nextstate].action != nullptr)
					spr->active_overlay[spr->active_overlay[spr->frame2].nextstate].action(spr, frametime, time);
				spr->frame2 = spr->active_overlay[spr->frame2].nextstate;
				spr->nextthink2 = time + spr->active_overlay[spr->frame2].tics;
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

			gEngfuncs.pfnConsolePrint(UTIL_VarArgs("\n\n\nCur. Frame: %d, Tics: %.2f, Next Frame: %d", 
				spr->active_sprite[spr->frame].frame, spr->active_sprite[spr->frame].tics, spr->active_sprite[spr->frame].nextstate));

			DrawSprite(spr);
		}
	}
}

//============================================
//	CSpriteObject::Init
//============================================

void CSpriteObject::DrawSprite(spr_object_t* spr)
{
	vec3_t angles, forward, right, up, screen, overlay_org;
	float sprite_scale = 0.025f;
	float scale[2];

	angles = v_angles;

	VectorCopy(spr->org, overlay_org);

	if (!spr)
		return;

	//=========================================================
	// Active sprite (the actual gun sprite...)
	//=========================================================

	if (spr->active_sprite != nullptr)
	{
		AngleVectors(angles, forward, right, up);

		scale[0] = SPR_Width(spr->active_sprite[spr->frame].sprite, spr->active_sprite[spr->frame].frame);
		scale[1] = SPR_Height(spr->active_sprite[spr->frame].sprite, spr->active_sprite[spr->frame].frame) * 1.2f;

		VectorMA(spr->org, 8.0f, forward, spr->org);
		VectorScale(right, scale[0] * sprite_scale, right);
		VectorScale(up, scale[1] * sprite_scale, up);

		VectorMA(spr->org, spr->active_sprite[spr->frame].x + spr->old_ofs[0], right, spr->org);
		VectorMA(spr->org, spr->active_sprite[spr->frame].y + spr->old_ofs[1], up, spr->org);

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
		gEngfuncs.pTriAPI->Vertex3f(spr->org[0] - right[0] + up[0], spr->org[1] - right[1] + up[1], spr->org[2] - right[2] + up[2]);

		//bottom left
		gEngfuncs.pTriAPI->TexCoord2f(0, 1);
		gEngfuncs.pTriAPI->Vertex3f(spr->org[0] - right[0] - up[0], spr->org[1] - right[1] - up[1], spr->org[2] - right[2] - up[2]);

		//bottom right
		gEngfuncs.pTriAPI->TexCoord2f(1, 1);
		gEngfuncs.pTriAPI->Vertex3f(spr->org[0] + right[0] - up[0], spr->org[1] + right[1] - up[1], spr->org[2] + right[2] - up[2]);

		//top right
		gEngfuncs.pTriAPI->TexCoord2f(1, 0);
		gEngfuncs.pTriAPI->Vertex3f(spr->org[0] + right[0] + up[0], spr->org[1] + right[1] + up[1], spr->org[2] + right[2] + up[2]);

		gEngfuncs.pTriAPI->End(); //end our list of vertexes
		gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
	}
	else
		return; // don't draw overlay if we don't have active sprite!!!

	//=========================================================
	// Overlay (muzzleflashes etc etc.)
	//=========================================================

	if (spr->active_overlay != nullptr)
	{
		AngleVectors(angles, forward, right, up);

		scale[0] = SPR_Width(spr->active_overlay[spr->frame2].sprite, spr->active_overlay[spr->frame2].frame);
		scale[1] = SPR_Height(spr->active_overlay[spr->frame2].sprite, spr->active_overlay[spr->frame2].frame) * 1.2f;

		VectorMA(overlay_org, 7.995f, forward, overlay_org);
		VectorScale(right, scale[0] * sprite_scale, right);
		VectorScale(up, scale[1] * sprite_scale, up);

		VectorMA(overlay_org, spr->active_overlay[spr->frame2].x + spr->old_ofs[0], right, overlay_org);
		VectorMA(overlay_org, spr->active_overlay[spr->frame2].y + spr->old_ofs[1], up, overlay_org);

		// START RENDERING!!!
		gEngfuncs.pTriAPI->RenderMode(kRenderTransTexture);
		gEngfuncs.pTriAPI->Color4f(spr->color2[0], spr->color2[1], spr->color2[2], 1.0f);
		gEngfuncs.pTriAPI->Brightness(spr->brightness2);

		if (!gEngfuncs.GetSpritePointer(spr->active_overlay[spr->frame2].sprite))
			return;

		gEngfuncs.pTriAPI->SpriteTexture((struct model_s*)gEngfuncs.GetSpritePointer(
			spr->active_overlay[spr->frame2].sprite), spr->active_overlay[spr->frame2].frame);

		gEngfuncs.pTriAPI->CullFace(TRI_NONE);
		gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad

		//top left
		gEngfuncs.pTriAPI->TexCoord2f(0, 0);
		gEngfuncs.pTriAPI->Vertex3f(overlay_org[0] - right[0] + up[0], overlay_org[1] - right[1] + up[1], overlay_org[2] - right[2] + up[2]);

		//bottom left
		gEngfuncs.pTriAPI->TexCoord2f(0, 1);
		gEngfuncs.pTriAPI->Vertex3f(overlay_org[0] - right[0] - up[0], overlay_org[1] - right[1] - up[1], overlay_org[2] - right[2] - up[2]);

		//bottom right
		gEngfuncs.pTriAPI->TexCoord2f(1, 1);
		gEngfuncs.pTriAPI->Vertex3f(overlay_org[0] + right[0] - up[0], overlay_org[1] + right[1] - up[1], overlay_org[2] + right[2] - up[2]);

		//top right
		gEngfuncs.pTriAPI->TexCoord2f(1, 0);
		gEngfuncs.pTriAPI->Vertex3f(overlay_org[0] + right[0] + up[0], overlay_org[1] + right[1] + up[1], overlay_org[2] + right[2] + up[2]);

		gEngfuncs.pTriAPI->End(); //end our list of vertexes
		gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
	}
}
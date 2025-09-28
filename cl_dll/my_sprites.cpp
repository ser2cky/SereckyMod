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
=============================================
*/

#include "hud.h"
#include "cl_util.h"
#include "triangleapi.h"
#include "com_model.h"
#include "my_sprites.h"

extern vec3_t v_angles;
struct model_s* SPR_SHTG = NULL;

void CSpriteObject::Init(void)
{

}

void CSpriteObject::VidInit(void)
{

}

void CSpriteObject::SpriteThink(float frametime, float time)
{
	HSPRITE SPR_SHTG = SPR_Load("sprites/doom/SPR_SHTG.spr");

	static state_t test[] =
	{
		{ SPR_SHTG, 0, 7.0f / 35.0f , NULL, 1},	// S_SGUN2
		{ SPR_SHTG, 1, 5.0f / 35.0f, NULL, 2},	// S_SGUN3
		{ SPR_SHTG, 2, 5.0f / 35.0f, NULL, 3},	// S_SGUN4
		{ SPR_SHTG, 3, 4.0f / 35.0f, NULL, 4},	// S_SGUN5
		{ SPR_SHTG, 2, 5.0f / 35.0f, NULL, 5},	// S_SGUN6
		{ SPR_SHTG, 1, 5.0f / 35.0f, NULL, 6},	// S_SGUN7
		{ SPR_SHTG, 0, 3.0f / 35.0f, NULL, 7},	// S_SGUN8
		{ SPR_SHTG, 0, 7.0f / 35.0f, NULL, 7},	// S_SGUN9
	};

	static int curstate = 0;
	static float nextthink = 0.0f;
	cl_entity_t* view;

	view = gEngfuncs.GetViewModel();

	if (!view)
		return;

	if (time > nextthink)
	{
		curstate = test[curstate].nextstate;
		nextthink = time + test[curstate].tics;
	}

	gEngfuncs.pfnConsolePrint(UTIL_VarArgs("\n\n\nCur. Frame: %d, Tics: %.2f, Next Frame: %d", test[curstate].frame, test[curstate].tics, test[curstate].nextstate));

	DrawSprite(test[curstate].frame, test[curstate].sprite, view->curstate.origin);
}

void CSpriteObject::DrawSprite(float frame, HSPRITE model, vec3_t origin)
{
	if (!model)
		return;

	float scale[2] = { SPR_Width(model, frame), SPR_Height(model, frame) };
	vec3_t angles, forward, right, up;

	angles = v_angles;
	AngleVectors(angles, forward, right, up);

	VectorScale(right, scale[0] * 0.05f, right);
	VectorScale(up, scale[1] * 0.05f, up);

	VectorMA(origin, 12.0f, forward, origin);
	VectorMA(origin, -2.0f, up, origin);

	// START RENDERING!!!
	gEngfuncs.pTriAPI->RenderMode(kRenderTransTexture);
	gEngfuncs.pTriAPI->Color4f(1.0f, 1.0f, 1.0f, 1.0f);
	gEngfuncs.pTriAPI->Brightness(1.0f);

	gEngfuncs.pTriAPI->SpriteTexture((struct model_s*)gEngfuncs.GetSpritePointer(model), frame);
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
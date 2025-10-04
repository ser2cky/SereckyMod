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
#include <iostream>
#include <vector>
#include <array>

extern float DoomTic(float tic);

//============================================
//	my_sprites.h
//	for handling of doom styled sprites.
//============================================

#define SPR_GUN_IDLE		0		// Gun is doing nothing.
#define SPR_GUN_RAISING		1		// Gun is being brought up.
#define SPR_GUN_LOWERING	2		// Gun is being switched out.
#define SPR_GUN_FIRING		3		// Gun is firing.

#define MAX_SPRITE_OBJS		4096	// Absolute max. amt of sprites.

//============================================
//	basic state struct.
//============================================

struct spr_object_s;
typedef struct spr_object_s spr_object_t;

typedef struct state_s
{
	HSPRITE				sprite;				// Current sprite for this state.
	int					frame;				// Current sprite frame for this state.
	float				tics;				// Length that this tic lasts for.
	void				(*action)(spr_object_t* spr, float frametime, float time);	// Action function that's being called at this moment.
	int					nextstate;			// Next state to jump to.
	float				x;					// offset for X
	float				y;					// offset for Y
} state_t;

//============================================
//	basic sprite struct
//============================================

#define	SPR_DIE_NOW				0x00000001 // Flag to kill sprite.
#define	SPR_OVERRIDE_LIGHT		0x00000002 // Flag to override sprite lighting.

typedef enum {
	spr_obj_world,		// Like monster sprites
	spr_obj_weapon,		// Like weapon view-sprites
} spr_type_t;

typedef struct spr_object_s
{
	struct				spr_object_s* next;	// dont touch

	state_t				*active_sprite;		// Pointer to active sprite table!!
	float				nextthink;			// Thinker for weapon sprites.
	int					frame;				// Current sprite frame.
	float				brightness;			// Sets current brightness of sprite.
	int					flags;				// Sets flags for sprite so they can DIE.
	vec3_t				color;				// Sets current color of sprite.

	state_t				*active_overlay;	// Pointer to active sprite table (overlay)!!
	float				nextthink2;			// Thinker for weapon sprites (overlay).
	int					frame2;				// Current sprite frame (overlay).
	float				brightness2;		// Sets current brightness of sprite (overlay).
	int					flags2;				// Sets flags for sprite so they can DIE (overlay).
	vec3_t				color2;				// Sets current color of sprite (overlay).

	spr_type_t			type;				// Sets sprite type.
	vec3_t				org;				// Current origin
	std::array<float, 2> ofs;				// overall offset.
	std::array<float, 2> old_ofs;			// OLD offset.

	// Weapon only fields!
	int					mode;				// Current gun mode. Can be Raising, Firing, Lowering, and Idling.
	std::array<int, 2>	raise_frame;		// Lets you define range of "raising" frames.
	std::array<int, 2>	lower_frame;		// Lets you define range of "lowering" frames.
	std::array<int, 2>	idle_frame;			// Lets you define range of "idle" frames.
	std::array<int, 2>	fire_frame;			// Lets you define range of "firing" frames.
	std::array<int, 2>	flash_frame;		// Lets you define range of gun flash frames.
} spr_object_t;

extern spr_object_t* gun;

//============================================
//	sprite object class def.
//============================================

class CSpriteObject
{
public:
	int Init(void);
	int VidInit(void);
	void Shutdown(void);
	void InitSprites(void);
	void ClearSprites(void);
	spr_object_t* R_AllocSpriteObject( void );
	void SpriteThink(float frametime, float time);
	void DrawSprite(spr_object_t* spr);
};
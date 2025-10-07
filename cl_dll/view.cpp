//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// view/refresh setup functions

#include "hud.h"
#include "cl_util.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"

#include "entity_state.h"
#include "cl_entity.h"
#include "ref_params.h"
#include "in_defs.h" // PITCH YAW ROLL
#include "pm_movevars.h"
#include "pm_shared.h"
#include "pm_defs.h"
#include "event_api.h"
#include "pmtrace.h"
#include "screenfade.h"
#include "shake.h"
#include "hltv.h"
#include "kbutton.h"
#include "my_sprites.h"

// Spectator Mode
extern "C" 
{
	float	vecNewViewAngles[3];
	int		iHasNewViewAngles;
	float	vecNewViewOrigin[3];
	int		iHasNewViewOrigin;
	int		iIsSpectator;
}

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

extern "C" 
{
	int CL_IsThirdPerson( void );
	void CL_CameraOffset( float *ofs );

	void DLLEXPORT V_CalcRefdef( struct ref_params_s *pparams );

	void PM_ParticleLine( float *start, float *end, int pcolor, float life, float vert);
	int		PM_GetVisEntInfo( int ent );
	int		PM_GetPhysEntInfo( int ent );
	void	InterpolateAngles(  float * start, float * end, float * output, float frac );
	void	NormalizeAngles( float * angles );
	float	Distance(const float * v1, const float * v2);
	float	AngleBetweenVectors(  const float * v1,  const float * v2 );

	float	vJumpOrigin[3];
	float	vJumpAngles[3];
}

void V_DropPunchAngle ( float frametime, float *ev_punchangle );
void VectorAngles( const float *forward, float *angles );

#include "r_studioint.h"
#include "com_model.h"

extern engine_studio_api_t IEngineStudio;

/*
The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.
*/

extern cvar_t	*cl_forwardspeed;
extern cvar_t	*chase_active;
extern cvar_t	*scr_ofsx, *scr_ofsy, *scr_ofsz;
extern cvar_t	*cl_vsmoothing;
playermove_t* pmove;

#define	CAM_MODE_RELAX		1
#define CAM_MODE_FOCUS		2

vec3_t		v_origin, v_angles, v_cl_angles, v_sim_org, v_lastAngles;
float		v_frametime, v_lastDistance;	
float		v_cameraRelaxAngle	= 5.0f;
float		v_cameraFocusAngle	= 35.0f;
int			v_cameraMode = CAM_MODE_FOCUS;
qboolean	v_resetCamera = 1;

vec3_t ev_punchangle;

vec3_t kick_origin;
vec3_t kick_angles;

cvar_t	*scr_ofsx;
cvar_t	*scr_ofsy;
cvar_t	*scr_ofsz;

cvar_t	*v_centermove;
cvar_t	*v_centerspeed;

cvar_t	*cl_waterdist;
cvar_t	*cl_chasedist;

// These cvars are not registered (so users can't cheat), so set the ->value field directly
// Register these cvars in V_Init() if needed for easy tweaking
cvar_t	v_iyaw_cycle		= {"v_iyaw_cycle", "2", 0, 2};
cvar_t	v_iroll_cycle		= {"v_iroll_cycle", "0.5", 0, 0.5};
cvar_t	v_ipitch_cycle		= {"v_ipitch_cycle", "1", 0, 1};
cvar_t	v_iyaw_level		= {"v_iyaw_level", "0.3", 0, 0.3};
cvar_t	v_iroll_level		= {"v_iroll_level", "0.1", 0, 0.1};
cvar_t	v_ipitch_level		= {"v_ipitch_level", "0.3", 0, 0.3};

float	v_idlescale;  // used by TFC for concussion grenade effect

// temp working variables for player view
float		bobfracsin = 0.0f;
int			bobcycle = 0;
int			bobCycle = 0;
float		xyspeed = 0.0f;
float		bobtime = 0.0f;
float		bobmove = 0.0f;
extern		kbutton_t in_duck;
extern		kbutton_t in_speed;

float		landTime = 0.0f;
float		groundTime = 0.0f;
float		view_ofs = 0.0f;
static float landChange = -8.0f;
float		client_bobtime = 0.0f;
float		v_dmg_roll, v_dmg_pitch, v_dmg_time;	// damage kicks
static float fall_time, fall_value;		// for view drop on fall

cvar_t		*cl_gun_roll;
cvar_t		*cl_gun_yaw;
cvar_t		*cl_gun_pitch;

cvar_t		*cl_run_pitch;
cvar_t		*cl_run_roll;
cvar_t		*cl_bob_up;
cvar_t		*cl_bob_pitch;
cvar_t		*cl_bob_roll;
cvar_t		*cl_bob_style;

#define	LAND_DEFLECT_TIME	150
#define	LAND_RETURN_TIME	300

//=============================================================================
/*
void V_NormalizeAngles( float *angles )
{
	int i;
	// Normalize angles
	for ( i = 0; i < 3; i++ )
	{
		if ( angles[i] > 180.0 )
		{
			angles[i] -= 360.0;
		}
		else if ( angles[i] < -180.0 )
		{
			angles[i] += 360.0;
		}
	}
}

/*
===================
V_InterpolateAngles

Interpolate Euler angles.
FIXME:  Use Quaternions to avoid discontinuities
Frac is 0.0 to 1.0 ( i.e., should probably be clamped, but doesn't have to be )
===================

void V_InterpolateAngles( float *start, float *end, float *output, float frac )
{
	int i;
	float ang1, ang2;
	float d;
	
	V_NormalizeAngles( start );
	V_NormalizeAngles( end );

	for ( i = 0 ; i < 3 ; i++ )
	{
		ang1 = start[i];
		ang2 = end[i];

		d = ang2 - ang1;
		if ( d > 180 )
		{
			d -= 360;
		}
		else if ( d < -180 )
		{	
			d += 360;
		}

		output[i] = ang1 + d * frac;
	}

	V_NormalizeAngles( output );
} */

/*
===============
V_Q3_CalcBobNew
Quake3 Viewbob.
===============
*/

void V_Q3_CalcBobNew(struct ref_params_s* pparams)
{
	cl_entity_t* view;
	static float offset = 0.0f;
	float	scale, delta, fracsin;

	//ent = gEngfuncs.GetLocalPlayer();
	view = gEngfuncs.GetViewModel();

	if (!view)
		return;

	// on odd legs, invert some angles
	if (bobcycle & 1)
		scale = -xyspeed;
	else
		scale = xyspeed;

	// gun angles from bobbing
	view->angles[ROLL] += scale * bobfracsin * cl_gun_roll->value;
	view->angles[YAW] += scale * bobfracsin * cl_gun_yaw->value;
	view->angles[PITCH] += xyspeed * bobfracsin * cl_gun_pitch->value;

	// drop the weapon when landing
	delta = (pparams->time - landTime) * 1000.0f;
	if (delta < LAND_DEFLECT_TIME)
		offset = landChange * 0.25f * delta / LAND_DEFLECT_TIME;
	else if (delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME)
		offset = landChange * 0.25f * (LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;

	//gEngfuncs.pfnConsolePrint(UTIL_VarArgs("gun offset %.2f\n", offset));
	if (offset > 0.0f)
		offset = 0.0f;
	view->origin[2] += offset;

	// idle drift
	scale = xyspeed + 40;
	fracsin = sin((pparams->time * 1000) * 0.001);
	view->angles[ROLL] += scale * fracsin * 0.01f;
	view->angles[YAW] += scale * fracsin * 0.01f;
	view->angles[PITCH] += scale * fracsin * 0.01f;

	VectorCopy(view->origin, view->curstate.origin);
	VectorCopy(view->angles, view->curstate.angles);
}

/*
===================
V_Q3_CalcViewOffset
Quake3 Camerabob.
===================
*/

void V_Q3_CalcViewOffset(struct ref_params_s* pparams)
{
	float		speed, bob, delta, f;

	// add angles based on velocity

	delta = DotProduct(pparams->simvel, pparams->forward);
	pparams->viewangles[PITCH] += delta * cl_run_pitch->value;

	delta = DotProduct(pparams->simvel, pparams->right);
	pparams->viewangles[ROLL] -= delta * cl_run_roll->value;

	// add angles based on bob

	// make sure the bob is visible even at low speeds
	speed = xyspeed > 200 ? xyspeed : 200;

	delta = bobfracsin * cl_bob_pitch->value * speed;
	if (in_duck.state & 1)
		delta *= 3;		// crouching
	pparams->viewangles[PITCH] -= delta;
	delta = bobfracsin * cl_bob_roll->value * speed;
	if (in_duck.state & 1)
		delta *= 3;		// crouching accentuates roll
	if (bobcycle & 1)
		delta = -delta;
	pparams->viewangles[ROLL] += delta;

	//===================================

	// add bob height
	bob = bobfracsin * xyspeed * cl_bob_up->value;
	if (bob > 6)
		bob = 6;
	pparams->viewheight[2] += bob;

	// add fall height
	delta = (pparams->time - landTime) * 1000.0f;
	if (delta < LAND_DEFLECT_TIME) {
		f = delta / LAND_DEFLECT_TIME;
		view_ofs = landChange * f;
	}
	else if (delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME) {
		delta -= LAND_DEFLECT_TIME;
		f = 1.0 - (delta / LAND_RETURN_TIME);
		view_ofs = landChange * f;
	}

	if (view_ofs > 0.0f)
		view_ofs = 0.0f;

	// don't uncomment. looks weird...
	//gEngfuncs.pfnConsolePrint(UTIL_VarArgs("view offset %.2f\n", view_ofs));
	//view_ofs = Interpolate(view_ofs, 0.0f, pparams->frametime * 100.0f);
}

/*
============================
V_Q3_CalcBobValues
Calc Quake3 Viewbob Values
============================
*/
void V_Q3_CalcBobValues( struct ref_params_s* pparams )
{
	static float real_landtime = 0.0f;
	int old;

	if (pparams->frametime <= 0.0f)
		return;

	xyspeed = sqrt(pparams->simvel[0] * pparams->simvel[0] + pparams->simvel[1] * pparams->simvel[1]);

	if (!pparams->onground && pparams->simvel[2] < -150.0f) {
		// Putting actual landtime somewhere else so it doesn't mess up dropping.
		real_landtime = pparams->time;

		if (pparams->simvel[2] < -350.0f)
			landChange = -16.0f;
		else if (pparams->simvel[2] < -580.0f)
			landChange = -24.0f;
		else
			landChange = -8.0f;
	}
	else {
		if (real_landtime) {
			landTime = real_landtime;
			real_landtime = 0.0f;
			bobtime = 0; // start at beginning of cycle again
		}

		// Redid the cycle generation code to use the Quake3-Arena method
		// instead of the Quake 2 one, which had very poor interpolation!!
		// - serecky 9.2.25
		if (xyspeed < 5) 
			bobCycle = 0;

		if (in_duck.state & 1) {
			bobmove = 0.5;	// ducked characters bob much faster
		}
		else {
			if (!(in_speed.state & 1))
				bobmove = 0.4f;	// faster speeds bob faster
			else
				bobmove = 0.3f;	// walking bobs slow
		}

		old = bobCycle;
		bobCycle = (int)(old + bobmove * (pparams->frametime * 1000.0f)) & 255;
		groundTime = pparams->time;
	}
	bobcycle = (bobCycle & 128) >> 7;
	bobfracsin = fabs(sin((bobCycle & 127) / 127.0 * M_PI)) * -1;

	// determine the view offsets
	V_Q3_CalcViewOffset(pparams);

	// determine the gun offsets
	V_Q3_CalcBobNew(pparams);
}

/*
===============
LerpAngle

===============
*/
float LerpAngle(float a2, float a1, float frac)
{
	if (a1 - a2 > 180)
		a1 -= 360;
	if (a1 - a2 < -180)
		a1 += 360;
	return a2 + frac * (a1 - a2);
}

/*
=================
P_FallingDamage
=================
*/

#define	FALL_TIME		0.3

void P_FallingDamage(struct ref_params_s* pparams, double time)
{
	static float nextthink = 0.0f;
	float	delta;
	int		damage;
	vec3_t	dir;
	static vec3_t oldvel = { 0, 0, 0 };

	if (nextthink <= time || nextthink - time > 0.1f)
	{
		VectorCopy(pparams->simvel, oldvel);
		nextthink = time + 0.1f;
	}

	if ((oldvel[2] < 0) && (pparams->simvel[2] > oldvel[2]) && (!pparams->onground))
		delta = oldvel[2];
	else
	{
		if (!pparams->onground)
			return;
		delta = pparams->simvel[2] - oldvel[2];
	}
	delta = delta * delta * 0.0001;

	// never take falling damage if completely underwater
	if (pparams->waterlevel == 3)
		return;
	if (pparams->waterlevel == 2)
		delta *= 0.25;
	if (pparams->waterlevel == 1)
		delta *= 0.5;

	if (delta < 1)
		return;

	if (delta < 15)
	{
	//	ent->s.event = EV_FOOTSTEP;
		return;
	}

	fall_value = delta * 0.5;
	if (fall_value > 40)
		fall_value = 40;
	fall_time = time + FALL_TIME;
}

/*
===================
V_Q2_CalcViewOffset
Quake2 Camerabob.
===================
*/

void V_Q2_CalcViewOffset(struct ref_params_s* pparams, double time)
{
	float		bob, delta, ratio;
	static float nextthink = 0.0f;
	int i;

	static vec3_t oldv = { 0, 0, 0 };
	static vec3_t v = { 0, 0, 0 };
	static vec3_t oldangles = { 0, 0, 0 };
	static vec3_t angles = { 0, 0, 0 };

	if ( nextthink <= time || nextthink - time > 0.1f )
	{
		// add angles based on weapon kick
		oldangles = angles;

		// base angles
		angles = kick_angles;

		// add angles based on damage kick
		//ratio = (v_dmg_time - time) / DAMAGE_TIME;
		//if (ratio < 0)
		//{
		//	ratio = 0;
		//	ent->client->v_dmg_pitch = 0;
		//	ent->client->v_dmg_roll = 0;
		//}
		//angles[PITCH] += ratio * v_dmg_pitch;
		//angles[ROLL] += ratio * v_dmg_roll;

		//// add pitch based on fall kick

		ratio = (fall_time - time) / FALL_TIME;
		if (ratio < 0)
			ratio = 0;
		angles[PITCH] += ratio * fall_value;

		// add angles based on velocity

		delta = DotProduct(pparams->simvel, pparams->forward);
		angles[PITCH] += delta * cl_run_pitch->value;

		delta = DotProduct(pparams->simvel, pparams->right);
		angles[ROLL] += delta * cl_run_roll->value;

		// add angles based on bob

		delta = bobfracsin * cl_bob_pitch->value * xyspeed;
		if (in_duck.state & 1)
			delta *= 6;		// crouching
		angles[PITCH] += delta;
		delta = bobfracsin * cl_bob_roll->value * xyspeed;
		if (in_duck.state & 1)
			delta *= 6;		// crouching
		if (bobcycle & 1)
			delta = -delta;
		angles[ROLL] += delta;

		//===================================

		// base origin

		oldv = v;
		VectorClear(v);

		// add fall height

		ratio = (fall_time - time) / FALL_TIME;
		if (ratio < 0)
			ratio = 0;
		v[2] -= ratio * fall_value * 0.4;

		// add bob height

		bob = bobfracsin * xyspeed * cl_bob_up->value;
		if (bob > 6)
			bob = 6;
		//gi.DebugGraph (bob *2, 255);
		v[2] += bob;

		// add kick offset

		VectorAdd(v, kick_origin, v);

		// absolutely bound offsets
		// so the view can never be outside the player box

		if (v[0] < -14)
			v[0] = -14;
		else if (v[0] > 14)
			v[0] = 14;
		if (v[1] < -14)
			v[1] = -14;
		else if (v[1] > 14)
			v[1] = 14;
		if (v[2] < -22)
			v[2] = -22;
		else if (v[2] > 30)
			v[2] = 30;

		// clear weapon kicks
		VectorClear(kick_origin);
		VectorClear(kick_angles);

		nextthink = time + 0.1f;
	}

	for (i = 0; i < 3; i++)
	{
		oldangles[i] = LerpAngle(oldangles[i], angles[i], pparams->frametime * 16.0f);
		oldv[i] = LerpAngle(oldv[i], v[i], pparams->frametime * 16.0f);
	}

	VectorAdd(oldangles, pparams->viewangles, pparams->viewangles);
	VectorAdd(oldv, pparams->viewheight, pparams->viewheight);
}

/*
===============
V_Q2_CalcBobNew
Quake2 Viewbob.
===============
*/

void V_Q2_CalcBobNew(struct ref_params_s* pparams, double time)
{
	int		i;
	float	delta;
	cl_entity_t* view, * ent;
	static vec3_t oldv = { 0, 0, 0 };
	static vec3_t v = { 0, 0, 0 };
	static vec3_t prev_angles = { 0, 0, 0 };
	static float nextthink;

	ent = gEngfuncs.GetLocalPlayer();
	view = gEngfuncs.GetViewModel();

	if (!view)
		return;

	if (nextthink <= time || nextthink - time > 0.1f)
	{
		oldv = v; // Interp. helps to add that weird jankiness
		// Quake2's viewbob had... - serecky 9.8.25

		// gun angles from bobbing
		v[ROLL] = -xyspeed * bobfracsin * cl_gun_roll->value;
		v[YAW] = xyspeed * bobfracsin * cl_gun_yaw->value;
		if (bobcycle & 1)
		{
			v[ROLL] = -v[ROLL];
			v[YAW] = -v[YAW];
		}
		v[PITCH] = -xyspeed * bobfracsin * cl_gun_pitch->value;

		// gun angles from delta movement
		for (i = 0; i < 3; i++)
		{
			delta = (prev_angles[i] - ent->curstate.angles[i]);
			if (delta > 180)
				delta -= 360;
			if (delta < -180)
				delta += 360;
			if (delta > 45)
				delta = 45;
			if (delta < -45)
				delta = -45;
			if (i == YAW)
				v[ROLL] += -0.1 * delta;
			v[i] += 0.2 * delta;
		}
		VectorCopy(ent->angles, prev_angles);
		nextthink = time + 0.1f;
	}

	//gEngfuncs.pfnConsolePrint(UTIL_VarArgs("OLD: %.2f %.2f %.2f\n", oldv[0], oldv[1], oldv[2]));
	//gEngfuncs.pfnConsolePrint(UTIL_VarArgs("new: %.2f %.2f %.2f\n", v[0], v[1], v[2]));

	for (i = 0; i < 3; i++)
		oldv[i] = LerpAngle(oldv[i], v[i], pparams->frametime * 16.0f);

	vec3_t viewangles = { -pparams->viewangles[0], pparams->viewangles[1], pparams->viewangles[2] };
	VectorAdd(oldv, viewangles, view->angles);
	VectorCopy(view->angles, view->curstate.angles);
}

/*
============================
V_Q2_CalcBobValues
Calc Quake2 Viewbob Values
============================
*/

void V_Q2_CalcBobValues(struct ref_params_s* pparams)
{
	float bobtime = 0.0f; 
	static float nextthink;
	static double time;

	if (pparams->frametime <= 0.0f)
		return;

	time += pparams->frametime;

	cl_entity_t* view;
	view = gEngfuncs.GetViewModel();

	if ( nextthink <= time || nextthink - time > 0.1f)
	{
		xyspeed = sqrt(pparams->simvel[0] * pparams->simvel[0] + pparams->simvel[1] * pparams->simvel[1]);

		if (xyspeed < 5)
		{
			bobmove = 0.0f;
			client_bobtime = 0.0f;	// start at beginning of cycle again
		}
		else if (pparams->onground)
		{	// so bobbing only cycles when on ground
			if (xyspeed > 210.0f)
				bobmove = 0.25f;
			else if (xyspeed > 100.0f)
				bobmove = 0.125f;
			else
				bobmove = 0.0625f;
		}

		bobtime = (client_bobtime += bobmove);

		if (in_duck.state & 1)
			bobtime *= 4.0f;

		bobcycle = (int)bobtime;
		bobfracsin = fabs(sin(bobtime * M_PI));

		nextthink = time + 0.1f;
	}

	// detect hitting the floor
	P_FallingDamage(pparams, time);

	// determine the view offsets
	V_Q2_CalcViewOffset(pparams, time);

	// determine the gun offsets
	V_Q2_CalcBobNew(pparams, time);
}
/*
============================
V_DoomCalcBob
Calc Doom1/2 Viewbob Values
============================
*/

// A lot of the Doom viewbob code used bitshifting things
// that I have little-to-no understanding of it, so bobbing
// will probably be a little bit off here... also, 
// doom & quake have different ticrates, but it's
// whatever, this looks close enough. - serecky 9.27.25

#define MAXBOB			16
void V_DoomCalcBob(struct ref_params_s* pparams)
{
	static float bob, swingx, swingy, playerbob, angle;
	static	double	bobtime, bobtime2;
	vec3_t vel;
	int i;

	cl_entity_t* view;
	view = gEngfuncs.GetViewModel();

	bobtime += pparams->frametime;

	VectorCopy(pparams->simvel, vel);
	playerbob = sqrt((vel[0] * vel[0]) + (vel[1] * vel[1]));
	playerbob /= 48; // Originally a bitshift of 2 to the left...

	if (playerbob > MAXBOB)
		playerbob = MAXBOB;

	if (!pparams->onground)
	{
		view_ofs = Interpolate(view_ofs, vel[2] / 48.0f, pparams->frametime * 4.0f);
	}
	else
	{
		view_ofs = Interpolate(view_ofs, 0.0f, pparams->frametime * 4.0f);
		angle = fmod((2 * M_PI) * (bobtime), 2 * M_PI);
		bob = (playerbob / 2) * sin(angle);
	}
	pparams->vieworg[2] += bob;

	// I'm applying Doom's viewbob to 3D viewmodels
	// as well, because the TriApi gun SpriteObject
	// copies the viewmodel's origin and viewangles.
	// - serecky 9.27.25

	if (!view)
		return;

	view->origin[2] += bob;
	// Little check to pause bobbing if we're firing a gun!!
	// - serecky 10/2/25
	if (gun)
	{
		if (gun->mode == SPR_GUN_IDLE)
			bobtime2 = bobtime;
	}
	else
		bobtime2 = bobtime;

	// bob the weapon based on movement speed
	angle = fmod((bobtime2 * 4), 2 * M_PI);
	swingx = playerbob * cos(angle);
	angle = fmod((bobtime2 * 4), M_PI);
	swingy = playerbob * sin(angle);

	//gEngfuncs.pfnConsolePrint(UTIL_VarArgs("%.2f\n", playerbob));
	for (i = 0; i < 3; i++)
	{
		view->origin[i] += pparams->right[i] * swingx * 0.1f;
		view->origin[i] -= pparams->up[i] * swingy * 0.1f;
	}
}

/*
===============
V_CalcRoll
Used by view and sv_user
===============
*/
float V_CalcRoll (vec3_t angles, vec3_t velocity, float rollangle, float rollspeed )
{
    float   sign;
    float   side;
    float   value;
	vec3_t  forward, right, up;
    
	AngleVectors ( angles, forward, right, up );
    
	side = DotProduct (velocity, right);
    sign = side < 0 ? -1 : 1;
    side = fabs( side );
    
	value = rollangle;
    if (side < rollspeed)
	{
		side = side * value / rollspeed;
	}
    else
	{
		side = value;
	}
	return side * sign;
}

typedef struct pitchdrift_s
{
	float		pitchvel;
	int			nodrift;
	float		driftmove;
	double		laststop;
} pitchdrift_t;

static pitchdrift_t pd;

void V_StartPitchDrift( void )
{
	if ( pd.laststop == gEngfuncs.GetClientTime() )
	{
		return;		// something else is keeping it from drifting
	}

	if ( pd.nodrift || !pd.pitchvel )
	{
		pd.pitchvel = v_centerspeed->value;
		pd.nodrift = 0;
		pd.driftmove = 0;
	}
}

void V_StopPitchDrift ( void )
{
	pd.laststop = gEngfuncs.GetClientTime();
	pd.nodrift = 1;
	pd.pitchvel = 0;
}

/*
===============
V_DriftPitch

Moves the client pitch angle towards idealpitch sent by the server.

If the user is adjusting pitch manually, either with lookup/lookdown,
mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.
===============
*/
void V_DriftPitch ( struct ref_params_s *pparams )
{
	float		delta, move;

	if ( gEngfuncs.IsNoClipping() || !pparams->onground || pparams->demoplayback || pparams->spectator )
	{
		pd.driftmove = 0;
		pd.pitchvel = 0;
		return;
	}

	// don't count small mouse motion
	if (pd.nodrift)
	{
		if ( fabs( pparams->cmd->forwardmove ) < cl_forwardspeed->value )
			pd.driftmove = 0;
		else
			pd.driftmove += pparams->frametime;
	
		if ( pd.driftmove > v_centermove->value)
		{
			V_StartPitchDrift ();
		}
		return;
	}
	
	delta = pparams->idealpitch - pparams->cl_viewangles[PITCH];

	if (!delta)
	{
		pd.pitchvel = 0;
		return;
	}

	move = pparams->frametime * pd.pitchvel;
	pd.pitchvel += pparams->frametime * v_centerspeed->value;
	
	if (delta > 0)
	{
		if (move > delta)
		{
			pd.pitchvel = 0;
			move = delta;
		}
		pparams->cl_viewangles[PITCH] += move;
	}
	else if (delta < 0)
	{
		if (move > -delta)
		{
			pd.pitchvel = 0;
			move = -delta;
		}
		pparams->cl_viewangles[PITCH] -= move;
	}
}

/* 
============================================================================== 
						VIEW RENDERING 
============================================================================== 
*/ 

/*
==================
V_CalcGunAngle
==================
*/
void V_CalcGunAngle ( struct ref_params_s *pparams )
{	
	cl_entity_t *viewent;
	
	viewent = gEngfuncs.GetViewModel();
	if ( !viewent )
		return;

	viewent->angles[YAW]   =  pparams->viewangles[YAW]   + pparams->crosshairangle[YAW];
	viewent->angles[PITCH] = -pparams->viewangles[PITCH] + pparams->crosshairangle[PITCH] * 0.25;
	viewent->angles[ROLL]  -= v_idlescale * sin(pparams->time*v_iroll_cycle.value) * v_iroll_level.value;
	
	// don't apply all of the v_ipitch to prevent normally unseen parts of viewmodel from coming into view.
	viewent->angles[PITCH] -= v_idlescale * sin(pparams->time*v_ipitch_cycle.value) * (v_ipitch_level.value * 0.5);
	viewent->angles[YAW]   -= v_idlescale * sin(pparams->time*v_iyaw_cycle.value) * v_iyaw_level.value;

	VectorCopy( viewent->angles, viewent->curstate.angles );
	VectorCopy( viewent->angles, viewent->latched.prevangles );
}

/*
==============
V_AddIdle

Idle swaying
==============
*/
void V_AddIdle ( struct ref_params_s *pparams )
{
	pparams->viewangles[ROLL] += v_idlescale * sin(pparams->time*v_iroll_cycle.value) * v_iroll_level.value;
	pparams->viewangles[PITCH] += v_idlescale * sin(pparams->time*v_ipitch_cycle.value) * v_ipitch_level.value;
	pparams->viewangles[YAW] += v_idlescale * sin(pparams->time*v_iyaw_cycle.value) * v_iyaw_level.value;
}


/*
==============
V_CalcViewRoll

Roll is induced by movement and damage
==============
*/
void V_CalcViewRoll ( struct ref_params_s *pparams )
{
	//float		side;
	cl_entity_t *viewentity;
	
	viewentity = gEngfuncs.GetEntityByIndex( pparams->viewentity );
	if ( !viewentity )
		return;

	//side = V_CalcRoll ( viewentity->angles, pparams->simvel, pparams->movevars->rollangle, pparams->movevars->rollspeed );

	//pparams->viewangles[ROLL] += side;

	if ( pparams->health <= 0 && ( pparams->viewheight[2] != 0 ) )
	{
		// only roll the view if the player is dead and the viewheight[2] is nonzero 
		// this is so deadcam in multiplayer will work.
		pparams->viewangles[ROLL] = 80;	// dead view angle
		return;
	}
}


/*
==================
V_CalcIntermissionRefdef

==================
*/
void V_CalcIntermissionRefdef ( struct ref_params_s *pparams )
{
	cl_entity_t	*ent, *view;
	float		old;

	// ent is the player model ( visible when out of body )
	ent = gEngfuncs.GetLocalPlayer();
	
	// view is the weapon model (only visible from inside body )
	view = gEngfuncs.GetViewModel();

	VectorCopy ( pparams->simorg, pparams->vieworg );
	VectorCopy ( pparams->cl_viewangles, pparams->viewangles );

	view->model = NULL;

	// allways idle in intermission
	old = v_idlescale;
	v_idlescale = 1;

	V_AddIdle ( pparams );

	if ( gEngfuncs.IsSpectateOnly() )
	{
		// in HLTV we must go to 'intermission' position by ourself
		VectorCopy( gHUD.m_Spectator.m_cameraOrigin, pparams->vieworg );
		VectorCopy( gHUD.m_Spectator.m_cameraAngles, pparams->viewangles );
	}

	v_idlescale = old;

	v_cl_angles = pparams->cl_viewangles;
	v_origin = pparams->vieworg;
	v_angles = pparams->viewangles;
}

#define ORIGIN_BACKUP 64
#define ORIGIN_MASK ( ORIGIN_BACKUP - 1 )

typedef struct 
{
	float Origins[ ORIGIN_BACKUP ][3];
	float OriginTime[ ORIGIN_BACKUP ];

	float Angles[ ORIGIN_BACKUP ][3];
	float AngleTime[ ORIGIN_BACKUP ];

	int CurrentOrigin;
	int CurrentAngle;
} viewinterp_t;

/*
==================
V_CalcRefdef

==================
*/
void V_CalcNormalRefdef ( struct ref_params_s *pparams )
{
	cl_entity_t		*ent, *view;
	int				i;
	vec3_t			angles;
	float			waterOffset;
	static viewinterp_t		ViewInterp;

	static float oldz = 0;
	static float lasttime;

	vec3_t camAngles, camForward, camRight, camUp;
	cl_entity_t *pwater;

	V_DriftPitch ( pparams );

	if ( gEngfuncs.IsSpectateOnly() )
	{
		ent = gEngfuncs.GetEntityByIndex( g_iUser2 );
	}
	else
	{
		// ent is the player model ( visible when out of body )
		ent = gEngfuncs.GetLocalPlayer();
	}
	
	// view is the weapon model (only visible from inside body )
	view = gEngfuncs.GetViewModel();

	// transform the view offset by the model's matrix to get the offset from
	// model origin for the view

	// refresh position
	VectorCopy ( pparams->simorg, pparams->vieworg );

	VectorAdd( pparams->vieworg, pparams->viewheight, pparams->vieworg );

	VectorCopy ( pparams->cl_viewangles, pparams->viewangles );

	gEngfuncs.V_CalcShake();
	gEngfuncs.V_ApplyShake( pparams->vieworg, pparams->viewangles, 1.0 );

	// never let view origin sit exactly on a node line, because a water plane can
	// dissapear when viewed with the eye exactly on it.
	// FIXME, we send origin at 1/128 now, change this?
	// the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis
	
	pparams->vieworg[0] += 1.0/32;
	pparams->vieworg[1] += 1.0/32;
	pparams->vieworg[2] += 1.0/32;

	// Check for problems around water, move the viewer artificially if necessary 
	// -- this prevents drawing errors in GL due to waves

	waterOffset = 0;

	if ( pparams->waterlevel >= 2 )
	{
		int		i, contents, waterDist, waterEntity;
		vec3_t	point;
		waterDist = cl_waterdist->value;

		if ( pparams->hardware )
		{
			waterEntity = gEngfuncs.PM_WaterEntity( pparams->simorg );
			if ( waterEntity >= 0 && waterEntity < pparams->max_entities )
			{
				pwater = gEngfuncs.GetEntityByIndex( waterEntity );
				if ( pwater && ( pwater->model != NULL ) )
				{
					waterDist += ( pwater->curstate.scale * 16 );	// Add in wave height
				}
			}
		}
		else
		{
			waterEntity = 0;	// Don't need this in software
		}
		
		VectorCopy( pparams->vieworg, point );

		// Eyes are above water, make sure we're above the waves
		if ( pparams->waterlevel == 2 )	
		{
			point[2] -= waterDist;
			for ( i = 0; i < waterDist; i++ )
			{
				contents = gEngfuncs.PM_PointContents( point, NULL );
				if ( contents > CONTENTS_WATER )
					break;
				point[2] += 1;
			}
			waterOffset = (point[2] + waterDist) - pparams->vieworg[2];
		}
		else
		{
			// eyes are under water.  Make sure we're far enough under
			point[2] += waterDist;

			for ( i = 0; i < waterDist; i++ )
			{
				contents = gEngfuncs.PM_PointContents( point, NULL );
				if ( contents <= CONTENTS_WATER )
					break;
				point[2] -= 1;
			}
			waterOffset = (point[2] - waterDist) - pparams->vieworg[2];
		}
	}

	pparams->vieworg[2] += waterOffset + view_ofs;
	
	V_CalcViewRoll ( pparams );
	
	V_AddIdle ( pparams );

	// offsets
	VectorCopy( pparams->cl_viewangles, angles );

	AngleVectors ( angles, pparams->forward, pparams->right, pparams->up );

	// don't allow cheats in multiplayer
	if ( pparams->maxclients <= 1 )
	{
		for ( i=0 ; i<3 ; i++ )
		{
			pparams->vieworg[i] += scr_ofsx->value*pparams->forward[i] + scr_ofsy->value*pparams->right[i] + scr_ofsz->value*pparams->up[i];
		}
	}
	
	// Treating cam_ofs[2] as the distance
	if( CL_IsThirdPerson() )
	{
		vec3_t ofs;

		ofs[0] = ofs[1] = ofs[2] = 0.0;

		CL_CameraOffset( (float *)&ofs );

		VectorCopy( ofs, camAngles );
		camAngles[ ROLL ]	= 0;

		AngleVectors( camAngles, camForward, camRight, camUp );

		for ( i = 0; i < 3; i++ )
		{
			pparams->vieworg[ i ] += -ofs[2] * camForward[ i ];
		}
	}

	VectorCopy(pparams->simorg, view->origin);
	view->origin[2] += pparams->viewheight[2];
	view->origin[2] += 0.0f + view_ofs;

	// Give gun our viewangles
	VectorCopy(pparams->cl_viewangles, view->angles);

	// set up gun position
	V_CalcGunAngle(pparams);

	// Let the viewmodel shake at about 10% of the amplitude
	gEngfuncs.V_ApplyShake( view->origin, view->angles, 0.9 );

// Q2/Q3 View-bob and Gun-bob start.

	// calc values. moved for cleanliness
	if (cl_bob_style->value == 0)
		V_Q2_CalcBobValues(pparams);
	else if (cl_bob_style->value == 1)
		V_Q3_CalcBobValues(pparams);
	else
		V_DoomCalcBob(pparams);


// Q2/Q3 View-bob and Gun-bob end.

	// Add in the punchangle, if any

	static float lerpPunch[3] = { 0, 0, 0 };
	static float ev_lerpPunch[3] = { 0, 0, 0 };

	if (pparams->punchangle || ev_punchangle)
	{
		for (i = 0; i < 3; i++)
		{
			lerpPunch[i] = Interpolate(lerpPunch[i], pparams->punchangle[i], min(pparams->frametime, 1 / 72.0f) * 10.0f);
			ev_lerpPunch[i] = Interpolate(ev_lerpPunch[i], ev_punchangle[i], min(pparams->frametime, 1 / 72.0f) * 10.0f);
		}

		VectorAdd(pparams->viewangles, lerpPunch, pparams->viewangles);

		// Include client side punch, too
		VectorAdd(pparams->viewangles, ev_lerpPunch, pparams->viewangles);

		V_DropPunchAngle(pparams->frametime, (float*)&ev_punchangle);
	}

	// smooth out stair step ups
#if 1
	if ( !pparams->smoothing && pparams->onground && pparams->simorg[2] - oldz > 0)
	{
		float steptime;
		
		steptime = pparams->time - lasttime;
		if (steptime < 0)
	//FIXME		I_Error ("steptime < 0");
			steptime = 0;

		oldz += steptime * 150;
		if (oldz > pparams->simorg[2])
			oldz = pparams->simorg[2];
		if (pparams->simorg[2] - oldz > 18)
			oldz = pparams->simorg[2]- 18;
		pparams->vieworg[2] += oldz - pparams->simorg[2];
		view->origin[2] += oldz - pparams->simorg[2];
	}
	else
	{
		oldz = pparams->simorg[2];
	}
#endif

	{
		static float lastorg[3];
		vec3_t delta;

		VectorSubtract( pparams->simorg, lastorg, delta );

		if ( Length( delta ) != 0.0 )
		{
			VectorCopy( pparams->simorg, ViewInterp.Origins[ ViewInterp.CurrentOrigin & ORIGIN_MASK ] );
			ViewInterp.OriginTime[ ViewInterp.CurrentOrigin & ORIGIN_MASK ] = pparams->time;
			ViewInterp.CurrentOrigin++;

			VectorCopy( pparams->simorg, lastorg );
		}
	}

	// Smooth out whole view in multiplayer when on trains, lifts
	if ( cl_vsmoothing && cl_vsmoothing->value &&
		( pparams->smoothing && ( pparams->maxclients > 1 ) ) )
	{
		int foundidx;
		int i;
		float t;

		if ( cl_vsmoothing->value < 0.0 )
		{
			gEngfuncs.Cvar_SetValue( "cl_vsmoothing", 0.0 );
		}

		t = pparams->time - cl_vsmoothing->value;

		for ( i = 1; i < ORIGIN_MASK; i++ )
		{
			foundidx = ViewInterp.CurrentOrigin - 1 - i;
			if ( ViewInterp.OriginTime[ foundidx & ORIGIN_MASK ] <= t )
				break;
		}

		if ( i < ORIGIN_MASK &&  ViewInterp.OriginTime[ foundidx & ORIGIN_MASK ] != 0.0 )
		{
			// Interpolate
			vec3_t delta;
			double frac;
			double dt;
			vec3_t neworg;

			dt = ViewInterp.OriginTime[ (foundidx + 1) & ORIGIN_MASK ] - ViewInterp.OriginTime[ foundidx & ORIGIN_MASK ];
			if ( dt > 0.0 )
			{
				frac = ( t - ViewInterp.OriginTime[ foundidx & ORIGIN_MASK] ) / dt;
				frac = min( 1.0, frac );
				VectorSubtract( ViewInterp.Origins[ ( foundidx + 1 ) & ORIGIN_MASK ], ViewInterp.Origins[ foundidx & ORIGIN_MASK ], delta );
				VectorMA( ViewInterp.Origins[ foundidx & ORIGIN_MASK ], frac, delta, neworg );

				// Dont interpolate large changes
				if ( Length( delta ) < 64 )
				{
					VectorSubtract( neworg, pparams->simorg, delta );

					VectorAdd( pparams->simorg, delta, pparams->simorg );
					VectorAdd( pparams->vieworg, delta, pparams->vieworg );
					VectorAdd( view->origin, delta, view->origin );

				}
			}
		}
	}

	// Store off v_angles before munging for third person
	v_angles = pparams->viewangles;
	v_lastAngles = pparams->viewangles;
//	v_cl_angles = pparams->cl_viewangles;	// keep old user mouse angles !
	if ( CL_IsThirdPerson() )
	{
		VectorCopy( camAngles, pparams->viewangles);
		float pitch = camAngles[ 0 ];

		// Normalize angles
		if ( pitch > 180 ) 
			pitch -= 360.0;
		else if ( pitch < -180 )
			pitch += 360;

		// Player pitch is inverted
		pitch /= -3.0;

		// Slam local player's pitch value
		ent->angles[ 0 ] = pitch;
		ent->curstate.angles[ 0 ] = pitch;
		ent->prevstate.angles[ 0 ] = pitch;
		ent->latched.prevangles[ 0 ] = pitch;
	}

	// override all previous settings if the viewent isn't the client
	if ( pparams->viewentity > pparams->maxclients )
	{
		cl_entity_t *viewentity;
		viewentity = gEngfuncs.GetEntityByIndex( pparams->viewentity );
		if ( viewentity )
		{
			VectorCopy( viewentity->origin, pparams->vieworg );
			VectorCopy( viewentity->angles, pparams->viewangles );

			// Store off overridden viewangles
			v_angles = pparams->viewangles;
		}
	}

	lasttime = pparams->time;

	v_origin = pparams->vieworg;
}

void V_SmoothInterpolateAngles( float * startAngle, float * endAngle, float * finalAngle, float degreesPerSec )
{
	float absd,frac,d,threshhold;
	
	NormalizeAngles( startAngle );
	NormalizeAngles( endAngle );

	for ( int i = 0 ; i < 3 ; i++ )
	{
		d = endAngle[i] - startAngle[i];

		if ( d > 180.0f )
		{
			d -= 360.0f;
		}
		else if ( d < -180.0f )
		{	
			d += 360.0f;
		}

		absd = fabs(d);

		if ( absd > 0.01f )
		{
			frac = degreesPerSec * v_frametime;

			threshhold= degreesPerSec / 4;

			if ( absd < threshhold )
			{
				float h = absd / threshhold;
				h *= h;
				frac*= h;  // slow down last degrees
			}

			if ( frac >  absd )
			{
				finalAngle[i] = endAngle[i];
			}
			else
			{
				if ( d>0)
					finalAngle[i] = startAngle[i] + frac;
				else
					finalAngle[i] = startAngle[i] - frac;
			}
		}
		else
		{
			finalAngle[i] = endAngle[i];
		}

	}

	NormalizeAngles( finalAngle );
}

// Get the origin of the Observer based around the target's position and angles
void V_GetChaseOrigin( float * angles, float * origin, float distance, float * returnvec )
{
	vec3_t	vecEnd;
	vec3_t	forward;
	vec3_t	vecStart;
	pmtrace_t * trace;
	int maxLoops = 8;

	int ignoreent = -1;	// first, ignore no entity
	
	cl_entity_t	 *	ent = NULL;
	
	// Trace back from the target using the player's view angles
	AngleVectors(angles, forward, NULL, NULL);
	
	VectorScale(forward,-1,forward);

	VectorCopy( origin, vecStart );

	VectorMA(vecStart, distance , forward, vecEnd);

	while ( maxLoops > 0)
	{
		trace = gEngfuncs.PM_TraceLine( vecStart, vecEnd, PM_TRACELINE_PHYSENTSONLY, 2, ignoreent );

		// WARNING! trace->ent is is the number in physent list not the normal entity number

		if ( trace->ent <= 0)
			break;	// we hit the world or nothing, stop trace

		ent = gEngfuncs.GetEntityByIndex( PM_GetPhysEntInfo( trace->ent ) );

		if ( ent == NULL )
			break;

		// hit non-player solid BSP , stop here
		if ( ent->curstate.solid == SOLID_BSP && !ent->player ) 
			break;

		// if close enought to end pos, stop, otherwise continue trace
		if( Distance(trace->endpos, vecEnd ) < 1.0f )
		{
			break;
		}
		else
		{
			ignoreent = trace->ent;	// ignore last hit entity
			VectorCopy( trace->endpos, vecStart);
		}

		maxLoops--;
	}  

/*	if ( ent )
	{
		gEngfuncs.Con_Printf("Trace loops %i , entity %i, model %s, solid %i\n",(8-maxLoops),ent->curstate.number, ent->model->name , ent->curstate.solid ); 
	} */

	VectorMA( trace->endpos, 4, trace->plane.normal, returnvec );

	v_lastDistance = Distance(trace->endpos, origin);	// real distance without offset
}

/*void V_GetDeathCam(cl_entity_t * ent1, cl_entity_t * ent2, float * angle, float * origin)
{
	float newAngle[3]; float newOrigin[3]; 

	float distance = 168.0f;

	v_lastDistance+= v_frametime * 96.0f;	// move unit per seconds back

	if ( v_resetCamera )
		v_lastDistance = 64.0f;

	if ( distance > v_lastDistance )
		distance = v_lastDistance;

	VectorCopy(ent1->origin, newOrigin);

	if ( ent1->player )
		newOrigin[2]+= 17; // head level of living player

	// get new angle towards second target
	if ( ent2 )
	{
		VectorSubtract( ent2->origin, ent1->origin, newAngle );
		VectorAngles( newAngle, newAngle );
		newAngle[0] = -newAngle[0];
	}
	else
	{
		// if no second target is given, look down to dead player
		newAngle[0] = 90.0f;
		newAngle[1] = 0.0f;
		newAngle[2] = 0;
	}

	// and smooth view
	V_SmoothInterpolateAngles( v_lastAngles, newAngle, angle, 120.0f );
			
	V_GetChaseOrigin( angle, newOrigin, distance, origin );

	VectorCopy(angle, v_lastAngles);
}*/

void V_GetSingleTargetCam(cl_entity_t * ent1, float * angle, float * origin)
{
	float newAngle[3]; float newOrigin[3]; 
	
	int flags 	   = gHUD.m_Spectator.m_iObserverFlags;

	// see is target is a dead player
	qboolean deadPlayer = ent1->player && (ent1->curstate.solid == SOLID_NOT);
	
	float dfactor   = ( flags & DRC_FLAG_DRAMATIC )? -1.0f : 1.0f;

	float distance = 112.0f + ( 16.0f * dfactor ); // get close if dramatic;
	
	// go away in final scenes or if player just died
	if ( flags & DRC_FLAG_FINAL )
		distance*=2.0f;	
	else if ( deadPlayer )
		distance*=1.5f;	

	// let v_lastDistance float smoothly away
	v_lastDistance+= v_frametime * 32.0f;	// move unit per seconds back

	if ( distance > v_lastDistance )
		distance = v_lastDistance;
	
	VectorCopy(ent1->origin, newOrigin);

	if ( ent1->player )
	{
		if ( deadPlayer )  
			newOrigin[2]+= 2;	//laying on ground
		else
			newOrigin[2]+= 17; // head level of living player
			
	}
	else
		newOrigin[2]+= 8;	// object, tricky, must be above bomb in CS

	// we have no second target, choose view direction based on
	// show front of primary target
	VectorCopy(ent1->angles, newAngle);

	// show dead players from front, normal players back
	if ( flags & DRC_FLAG_FACEPLAYER )
		newAngle[1]+= 180.0f;


	newAngle[0]+= 12.5f * dfactor; // lower angle if dramatic

	// if final scene (bomb), show from real high pos
	if ( flags & DRC_FLAG_FINAL )
		newAngle[0] = 22.5f; 

	// choose side of object/player			
	if ( flags & DRC_FLAG_SIDE )
		newAngle[1]+=22.5f;
	else
		newAngle[1]-=22.5f;

	V_SmoothInterpolateAngles( v_lastAngles, newAngle, angle, 120.0f );

	// HACK, if player is dead don't clip against his dead body, can't check this
	V_GetChaseOrigin( angle, newOrigin, distance, origin );
}

float MaxAngleBetweenAngles(  float * a1, float * a2 )
{
	float d, maxd = 0.0f;

	NormalizeAngles( a1 );
	NormalizeAngles( a2 );

	for ( int i = 0 ; i < 3 ; i++ )
	{
		d = a2[i] - a1[i];
		if ( d > 180 )
		{
			d -= 360;
		}
		else if ( d < -180 )
		{	
			d += 360;
		}

		d = fabs(d);

		if ( d > maxd )
			maxd=d;
	}

	return maxd;
}

void V_GetDoubleTargetsCam(cl_entity_t	 * ent1, cl_entity_t * ent2,float * angle, float * origin)
{
	float newAngle[3]; float newOrigin[3]; float tempVec[3];

	int flags 	   = gHUD.m_Spectator.m_iObserverFlags;

	float dfactor   = ( flags & DRC_FLAG_DRAMATIC )? -1.0f : 1.0f;

	float distance = 112.0f + ( 16.0f * dfactor ); // get close if dramatic;
	
	// go away in final scenes or if player just died
	if ( flags & DRC_FLAG_FINAL )
		distance*=2.0f;	
	
	// let v_lastDistance float smoothly away
	v_lastDistance+= v_frametime * 32.0f;	// move unit per seconds back

	if ( distance > v_lastDistance )
		distance = v_lastDistance;

	VectorCopy(ent1->origin, newOrigin);

	if ( ent1->player )
		newOrigin[2]+= 17; // head level of living player
	else
		newOrigin[2]+= 8;	// object, tricky, must be above bomb in CS

	// get new angle towards second target
	VectorSubtract( ent2->origin, ent1->origin, newAngle );

	VectorAngles( newAngle, newAngle );
	newAngle[0] = -newAngle[0];

	// set angle diffrent in Dramtaic scenes
	newAngle[0]+= 12.5f * dfactor; // lower angle if dramatic
			
	if ( flags & DRC_FLAG_SIDE )
		newAngle[1]+=22.5f;
	else
		newAngle[1]-=22.5f;

	float d = MaxAngleBetweenAngles( v_lastAngles, newAngle );

	if ( ( d < v_cameraFocusAngle) && ( v_cameraMode == CAM_MODE_RELAX ) )
	{
		// difference is to small and we are in relax camera mode, keep viewangles
		VectorCopy(v_lastAngles, newAngle );
	}
	else if ( (d < v_cameraRelaxAngle) && (v_cameraMode == CAM_MODE_FOCUS) )
	{
		// we catched up with our target, relax again
		v_cameraMode = CAM_MODE_RELAX;
	}
	else
	{
		// target move too far away, focus camera again
		v_cameraMode = CAM_MODE_FOCUS;
	}

	// and smooth view, if not a scene cut
	if ( v_resetCamera || (v_cameraMode == CAM_MODE_RELAX) )
	{
		VectorCopy( newAngle, angle );
	}
	else
	{
		V_SmoothInterpolateAngles( v_lastAngles, newAngle, angle, 180.0f );
	}

	V_GetChaseOrigin( newAngle, newOrigin, distance, origin );

	// move position up, if very close at target
	if ( v_lastDistance < 64.0f )
		origin[2]+= 16.0f*( 1.0f - (v_lastDistance / 64.0f ) );

	// calculate angle to second target
	VectorSubtract( ent2->origin, origin, tempVec );
	VectorAngles( tempVec, tempVec );
	tempVec[0] = -tempVec[0];

	/* take middle between two viewangles
	InterpolateAngles( newAngle, tempVec, newAngle, 0.5f); */
	
	

}

void V_GetDirectedChasePosition(cl_entity_t	 * ent1, cl_entity_t * ent2,float * angle, float * origin)
{

	if ( v_resetCamera )
	{
		v_lastDistance = 4096.0f;
		// v_cameraMode = CAM_MODE_FOCUS;
	}

	if ( ( ent2 == (cl_entity_t*)0xFFFFFFFF ) || ( ent1->player && (ent1->curstate.solid == SOLID_NOT) ) )
	{
		// we have no second target or player just died
		V_GetSingleTargetCam(ent1, angle, origin);
	}
	else if ( ent2 )
	{
		// keep both target in view
		V_GetDoubleTargetsCam( ent1, ent2, angle, origin );
	}
	else
	{
		// second target disappeard somehow (dead)

		// keep last good viewangle
		float newOrigin[3];

		int flags 	   = gHUD.m_Spectator.m_iObserverFlags;

		float dfactor   = ( flags & DRC_FLAG_DRAMATIC )? -1.0f : 1.0f;

		float distance = 112.0f + ( 16.0f * dfactor ); // get close if dramatic;
	
		// go away in final scenes or if player just died
		if ( flags & DRC_FLAG_FINAL )
			distance*=2.0f;	
	
		// let v_lastDistance float smoothly away
		v_lastDistance+= v_frametime * 32.0f;	// move unit per seconds back

		if ( distance > v_lastDistance )
			distance = v_lastDistance;
		
		VectorCopy(ent1->origin, newOrigin);

		if ( ent1->player )
			newOrigin[2]+= 17; // head level of living player
		else
			newOrigin[2]+= 8;	// object, tricky, must be above bomb in CS

		V_GetChaseOrigin( angle, newOrigin, distance, origin );
	}

	VectorCopy(angle, v_lastAngles);
}

void V_GetChasePos(int target, float * cl_angles, float * origin, float * angles)
{
	cl_entity_t	 *	ent = NULL;
	
	if ( target ) 
	{
		ent = gEngfuncs.GetEntityByIndex( target );
	};
	
	if (!ent)
	{
		// just copy a save in-map position
		VectorCopy ( vJumpAngles, angles );
		VectorCopy ( vJumpOrigin, origin );
		return;
	}
	
	
	
	if ( gHUD.m_Spectator.m_autoDirector->value )
	{
		if ( g_iUser3 )
			V_GetDirectedChasePosition( ent, gEngfuncs.GetEntityByIndex( g_iUser3 ),
				angles, origin );
		else
			V_GetDirectedChasePosition( ent, ( cl_entity_t*)0xFFFFFFFF,
				angles, origin );
	}
	else
	{
		if ( cl_angles == NULL )	// no mouse angles given, use entity angles ( locked mode )
		{
			VectorCopy ( ent->angles, angles);
			angles[0]*=-1;
		}
		else
			VectorCopy ( cl_angles, angles);


		VectorCopy ( ent->origin, origin);
		
		origin[2]+= 28; // DEFAULT_VIEWHEIGHT - some offset

		V_GetChaseOrigin( angles, origin, cl_chasedist->value, origin );
	}

	v_resetCamera = false;	
}

void V_ResetChaseCam()
{
	v_resetCamera = true;
}


void V_GetInEyePos(int target, float * origin, float * angles )
{
	if ( !target)
	{
		// just copy a save in-map position
		VectorCopy ( vJumpAngles, angles );
		VectorCopy ( vJumpOrigin, origin );
		return;
	};


	cl_entity_t	 * ent = gEngfuncs.GetEntityByIndex( target );

	if ( !ent )
		return;

	VectorCopy ( ent->origin, origin );
	VectorCopy ( ent->angles, angles );

	angles[PITCH]*=-3.0f;	// see CL_ProcessEntityUpdate()

	if ( ent->curstate.solid == SOLID_NOT )
	{
		angles[ROLL] = 80;	// dead view angle
		origin[2]+= -8 ; // PM_DEAD_VIEWHEIGHT
	}
	else if (ent->curstate.usehull == 1 )
		origin[2]+= 12; // VEC_DUCK_VIEW;
	else
		// exacty eye position can't be caluculated since it depends on
		// client values like cl_bobcycle, this offset matches the default values
		origin[2]+= 28; // DEFAULT_VIEWHEIGHT
}

void V_GetMapFreePosition( float * cl_angles, float * origin, float * angles )
{
	vec3_t forward;
	vec3_t zScaledTarget;

	VectorCopy(cl_angles, angles);

	// modify angles since we don't wanna see map's bottom
	angles[0] = 51.25f + 38.75f*(angles[0]/90.0f);

	zScaledTarget[0] = gHUD.m_Spectator.m_mapOrigin[0];
	zScaledTarget[1] = gHUD.m_Spectator.m_mapOrigin[1];
	zScaledTarget[2] = gHUD.m_Spectator.m_mapOrigin[2] * (( 90.0f - angles[0] ) / 90.0f );
	

	AngleVectors(angles, forward, NULL, NULL);

	VectorNormalize(forward);

	VectorMA(zScaledTarget, -( 4096.0f / gHUD.m_Spectator.m_mapZoom ), forward , origin);
}

void V_GetMapChasePosition(int target, float * cl_angles, float * origin, float * angles)
{
	vec3_t forward;

	if ( target )
	{
		cl_entity_t	 *	ent = gEngfuncs.GetEntityByIndex( target );

		if ( gHUD.m_Spectator.m_autoDirector->value )
		{
			// this is done to get the angles made by director mode
			V_GetChasePos(target, cl_angles, origin, angles);
			VectorCopy(ent->origin, origin);
			
			// keep fix chase angle horizontal
			angles[0] = 45.0f;
		}
		else
		{
			VectorCopy(cl_angles, angles);
			VectorCopy(ent->origin, origin);

			// modify angles since we don't wanna see map's bottom
			angles[0] = 51.25f + 38.75f*(angles[0]/90.0f);
		}
	}
	else
	{
		// keep out roaming position, but modify angles
		VectorCopy(cl_angles, angles);
		angles[0] = 51.25f + 38.75f*(angles[0]/90.0f);
	}

	origin[2] *= (( 90.0f - angles[0] ) / 90.0f );
	angles[2] = 0.0f;	// don't roll angle (if chased player is dead)

	AngleVectors(angles, forward, NULL, NULL);

	VectorNormalize(forward);

	VectorMA(origin, -1536, forward, origin); 
}

int V_FindViewModelByWeaponModel(int weaponindex)
{

	static char * modelmap[][2] =	{
		{ "models/p_crossbow.mdl",		"models/v_crossbow.mdl"		},
		{ "models/p_crowbar.mdl",		"models/v_crowbar.mdl"		},
		{ "models/p_egon.mdl",			"models/v_egon.mdl"			},
		{ "models/p_gauss.mdl",			"models/v_gauss.mdl"		},
		{ "models/p_9mmhandgun.mdl",	"models/v_9mmhandgun.mdl"	},
		{ "models/p_grenade.mdl",		"models/v_grenade.mdl"		},
		{ "models/p_hgun.mdl",			"models/v_hgun.mdl"			},
		{ "models/p_9mmAR.mdl",			"models/v_9mmAR.mdl"		},
		{ "models/p_357.mdl",			"models/v_357.mdl"			},
		{ "models/p_rpg.mdl",			"models/v_rpg.mdl"			},
		{ "models/p_shotgun.mdl",		"models/v_shotgun.mdl"		},
		{ "models/p_squeak.mdl",		"models/v_squeak.mdl"		},
		{ "models/p_tripmine.mdl",		"models/v_tripmine.mdl"		},
		{ "models/p_satchel_radio.mdl",	"models/v_satchel_radio.mdl"},
		{ "models/p_satchel.mdl",		"models/v_satchel.mdl"		},
		{ NULL, NULL } };

	struct model_s * weaponModel = IEngineStudio.GetModelByIndex( weaponindex );

	if ( weaponModel )
	{
		int len = strlen( weaponModel->name );
		int i = 0;

		while ( modelmap[i] != NULL )
		{
			if ( !strnicmp( weaponModel->name, modelmap[i][0], len ) )
			{
				return gEngfuncs.pEventAPI->EV_FindModelIndex( modelmap[i][1] );
			}
			i++;
		}

		return 0;
	}
	else
		return 0;

}


/*
==================
V_CalcSpectatorRefdef

==================
*/
void V_CalcSpectatorRefdef ( struct ref_params_s * pparams )
{
	static vec3_t			velocity ( 0.0f, 0.0f, 0.0f);

	static int lastWeaponModelIndex = 0;
	static int lastViewModelIndex = 0;
		
	cl_entity_t	 * ent = gEngfuncs.GetEntityByIndex( g_iUser2 );
	
	pparams->onlyClientDraw = false;

	// refresh position
	VectorCopy ( pparams->simorg, v_sim_org );

	// get old values
	VectorCopy ( pparams->cl_viewangles, v_cl_angles );
	VectorCopy ( pparams->viewangles, v_angles );
	VectorCopy ( pparams->vieworg, v_origin );

	if (  ( g_iUser1 == OBS_IN_EYE || gHUD.m_Spectator.m_pip->value == INSET_IN_EYE ) && ent )
	{
		// calculate player velocity
		float timeDiff = ent->curstate.msg_time - ent->prevstate.msg_time;

		if ( timeDiff > 0 )
		{
			vec3_t distance;
			VectorSubtract(ent->prevstate.origin, ent->curstate.origin, distance);
			VectorScale(distance, 1/timeDiff, distance );

			velocity[0] = velocity[0]*0.9f + distance[0]*0.1f;
			velocity[1] = velocity[1]*0.9f + distance[1]*0.1f;
			velocity[2] = velocity[2]*0.9f + distance[2]*0.1f;
			
			VectorCopy(velocity, pparams->simvel);
		}

		// predict missing client data and set weapon model ( in HLTV mode or inset in eye mode )
		if ( gEngfuncs.IsSpectateOnly() )
		{
			V_GetInEyePos( g_iUser2, pparams->simorg, pparams->cl_viewangles );

			pparams->health = 1;

			cl_entity_t	 * gunModel = gEngfuncs.GetViewModel();

			if ( lastWeaponModelIndex != ent->curstate.weaponmodel )
			{
				// weapon model changed

				lastWeaponModelIndex = ent->curstate.weaponmodel;
				lastViewModelIndex = V_FindViewModelByWeaponModel( lastWeaponModelIndex );
				if ( lastViewModelIndex )
				{
					gEngfuncs.pfnWeaponAnim(0,0);	// reset weapon animation
				}
				else
				{
					// model not found
					gunModel->model = NULL;	// disable weapon model
					lastWeaponModelIndex = lastViewModelIndex = 0;
				}
			}

			if ( lastViewModelIndex )
			{
				gunModel->model = IEngineStudio.GetModelByIndex( lastViewModelIndex );
				gunModel->curstate.modelindex = lastViewModelIndex;
				gunModel->curstate.frame = 0;
				gunModel->curstate.colormap = 0; 
				gunModel->index = g_iUser2;
			}
			else
			{
				gunModel->model = NULL;	// disable weaopn model
			}
		}
		else
		{
			// only get viewangles from entity
			VectorCopy ( ent->angles, pparams->cl_viewangles );
			pparams->cl_viewangles[PITCH]*=-3.0f;	// see CL_ProcessEntityUpdate()
		}
	}

	v_frametime = pparams->frametime;

	if ( pparams->nextView == 0 )
	{
		// first renderer cycle, full screen

		switch ( g_iUser1 )
		{
			case OBS_CHASE_LOCKED:	V_GetChasePos( g_iUser2, NULL, v_origin, v_angles );
									break;

			case OBS_CHASE_FREE:	V_GetChasePos( g_iUser2, v_cl_angles, v_origin, v_angles );
									break;

			case OBS_ROAMING	:	VectorCopy (v_cl_angles, v_angles);
									VectorCopy (v_sim_org, v_origin);
									break;

			case OBS_IN_EYE		:   V_CalcNormalRefdef ( pparams );
									break;
				
			case OBS_MAP_FREE  :	pparams->onlyClientDraw = true;
									V_GetMapFreePosition( v_cl_angles, v_origin, v_angles );
									break;

			case OBS_MAP_CHASE  :	pparams->onlyClientDraw = true;
									V_GetMapChasePosition( g_iUser2, v_cl_angles, v_origin, v_angles );
									break;
		}

		if ( gHUD.m_Spectator.m_pip->value )
			pparams->nextView = 1;	// force a second renderer view

		gHUD.m_Spectator.m_iDrawCycle = 0;

	}
	else
	{
		// second renderer cycle, inset window

		// set inset parameters
		pparams->viewport[0] = XRES(gHUD.m_Spectator.m_OverviewData.insetWindowX);	// change viewport to inset window
		pparams->viewport[1] = YRES(gHUD.m_Spectator.m_OverviewData.insetWindowY);
		pparams->viewport[2] = XRES(gHUD.m_Spectator.m_OverviewData.insetWindowWidth);
		pparams->viewport[3] = YRES(gHUD.m_Spectator.m_OverviewData.insetWindowHeight);
		pparams->nextView	 = 0;	// on further view

		// override some settings in certain modes
		switch ( (int)gHUD.m_Spectator.m_pip->value )
		{
			case INSET_CHASE_FREE : V_GetChasePos( g_iUser2, v_cl_angles, v_origin, v_angles );
									break;	

			case INSET_IN_EYE	 :	V_CalcNormalRefdef ( pparams );
									break;

			case INSET_MAP_FREE  :	pparams->onlyClientDraw = true;
									V_GetMapFreePosition( v_cl_angles, v_origin, v_angles );
									break;

			case INSET_MAP_CHASE  :	pparams->onlyClientDraw = true;

									if ( g_iUser1 == OBS_ROAMING )
										V_GetMapChasePosition( 0, v_cl_angles, v_origin, v_angles );
									else
										V_GetMapChasePosition( g_iUser2, v_cl_angles, v_origin, v_angles );

									break;
		}

		gHUD.m_Spectator.m_iDrawCycle = 1;
	}

	// write back new values into pparams
	VectorCopy ( v_cl_angles, pparams->cl_viewangles );
	VectorCopy ( v_angles, pparams->viewangles )
	VectorCopy ( v_origin, pparams->vieworg );

}



void DLLEXPORT V_CalcRefdef( struct ref_params_s *pparams )
{
	// intermission / finale rendering
	if ( pparams->intermission )
	{	
		V_CalcIntermissionRefdef ( pparams );	
	}
	else if ( pparams->spectator || g_iUser1 )	// g_iUser true if in spectator mode
	{
		V_CalcSpectatorRefdef ( pparams );	
	}
	else if ( !pparams->paused )
	{
		V_CalcNormalRefdef ( pparams );
	}

/*
// Example of how to overlay the whole screen with red at 50 % alpha
#define SF_TEST
#if defined SF_TEST
	{
		screenfade_t sf;
		gEngfuncs.pfnGetScreenFade( &sf );

		sf.fader = 255;
		sf.fadeg = 0;
		sf.fadeb = 0;
		sf.fadealpha = 128;
		sf.fadeFlags = FFADE_STAYOUT | FFADE_OUT;

		gEngfuncs.pfnSetScreenFade( &sf );
	}
#endif
*/
}

/*
=============
V_DropPunchAngle

=============
*/
void V_DropPunchAngle ( float frametime, float *ev_punchangle )
{
	float	len;
	
	len = VectorNormalize ( ev_punchangle );
	len -= (10.0 + len * 0.5) * frametime;
	len = max( len, 0.0 );
	VectorScale ( ev_punchangle, len, ev_punchangle );
}

/*
=============
V_PunchAxis

Client side punch effect
=============
*/
void V_PunchAxis( int axis, float punch )
{
	ev_punchangle[ axis ] = punch;
}

/*
=============
V_Q2Punch

Client side punch effect
for Quake2!!!
=============
*/
void V_Q2Punch(float* kick_org, float* kick_ang)
{
	kick_origin[0] = kick_org[0];
	kick_origin[1] = kick_org[1];
	kick_origin[2] = kick_org[2];

	kick_angles[0] = kick_ang[0];
	kick_angles[1] = kick_ang[1];
	kick_angles[2] = kick_ang[2];
}

/*
=============
V_Init
=============
*/
void V_Init (void)
{
	gEngfuncs.pfnAddCommand ("centerview", V_StartPitchDrift );

	scr_ofsx			= gEngfuncs.pfnRegisterVariable( "scr_ofsx","0", 0 );
	scr_ofsy			= gEngfuncs.pfnRegisterVariable( "scr_ofsy","0", 0 );
	scr_ofsz			= gEngfuncs.pfnRegisterVariable( "scr_ofsz","0", 0 );

	v_centermove		= gEngfuncs.pfnRegisterVariable( "v_centermove", "0.15", 0 );
	v_centerspeed		= gEngfuncs.pfnRegisterVariable( "v_centerspeed","500", 0 );

	cl_gun_roll			= gEngfuncs.pfnRegisterVariable( "cl_gun_roll", "0.005", 0 );
	cl_gun_yaw			= gEngfuncs.pfnRegisterVariable( "cl_gun_yaw", "0.01", 0 );
	cl_gun_pitch		= gEngfuncs.pfnRegisterVariable( "cl_gun_pitch", "0.005", 0 );

	cl_run_pitch		= gEngfuncs.pfnRegisterVariable( "cl_run_pitch", "0.0002", 0 );
	cl_run_roll			= gEngfuncs.pfnRegisterVariable( "cl_run_roll", "0.0005", 0 );
	cl_bob_up			= gEngfuncs.pfnRegisterVariable( "cl_bob_up", "0.005", 0 );
	cl_bob_pitch		= gEngfuncs.pfnRegisterVariable( "cl_bob_pitch", "0.002", 0 );
	cl_bob_roll			= gEngfuncs.pfnRegisterVariable( "cl_bob_roll", "0.002", 0 );
	cl_bob_style		= gEngfuncs.pfnRegisterVariable( "cl_bob_style", "0", FCVAR_ARCHIVE);

	cl_waterdist		= gEngfuncs.pfnRegisterVariable( "cl_waterdist","4", 0 );
	cl_chasedist		= gEngfuncs.pfnRegisterVariable( "cl_chasedist","112", 0 );
}


//#define TRACE_TEST
#if defined( TRACE_TEST )

extern float in_fov;
/*
====================
CalcFov
====================
*/
float CalcFov (float fov_x, float width, float height)
{
	float	a;
	float	x;

	if (fov_x < 1 || fov_x > 179)
		fov_x = 90;	// error, set to 90

	x = width/tan(fov_x/360*M_PI);

	a = atan (height/x);

	a = a*360/M_PI;

	return a;
}

int hitent = -1;

void V_Move( int mx, int my )
{
	float fov;
	float fx, fy;
	float dx, dy;
	float c_x, c_y;
	float dX, dY;
	vec3_t forward, up, right;
	vec3_t newangles;

	vec3_t farpoint;
	pmtrace_t tr;

	fov = CalcFov( in_fov, (float)ScreenWidth, (float)ScreenHeight );

	c_x = (float)ScreenWidth / 2.0;
	c_y = (float)ScreenHeight / 2.0;

	dx = (float)mx - c_x;
	dy = (float)my - c_y;

	// Proportion we moved in each direction
	fx = dx / c_x;
	fy = dy / c_y;

	dX = fx * in_fov / 2.0 ;
	dY = fy * fov / 2.0;

	newangles = v_angles;

	newangles[ YAW ] -= dX;
	newangles[ PITCH ] += dY;

	// Now rotate v_forward around that point
	AngleVectors ( newangles, forward, right, up );

	farpoint = v_origin + 8192 * forward;

	// Trace
	tr = *(gEngfuncs.PM_TraceLine( (float *)&v_origin, (float *)&farpoint, PM_TRACELINE_PHYSENTSONLY, 2 /*point sized hull*/, -1 ));

	if ( tr.fraction != 1.0 && tr.ent != 0 )
	{
		hitent = PM_GetPhysEntInfo( tr.ent );
		PM_ParticleLine( (float *)&v_origin, (float *)&tr.endpos, 5, 1.0, 0.0 );
	}
	else
	{
		hitent = -1;
	}
}

#endif

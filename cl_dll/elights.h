//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#pragma once
#include "dlight.h"

// Goldsrc supports a max of 128 elights,
// however I think 32 should suffice for
// a client
#define MAX_ELIGHTS	32

//==========================
// CELightManager
//
//==========================
class CELightManager
{
public:
	struct elight_t
	{
		dlight_t* pelight;
		cl_entity_t* pentity;
	};

public:
	CELightManager( void );
	~CELightManager( void );

public:
	void Init( void );
	void VidInit( void );

	void Think( void );

private:
	// Array of env_elights
	elight_t m_elightsArray[MAX_ELIGHTS];
	// Nb of active lights
	int m_nbActiveELights;

	// Toggles debug messages
	cvar_t* m_pCvarELightsDebug;
};
extern CELightManager gELightManager;
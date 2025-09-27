//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#pragma once

//-----------------------------------------------------------------------------
// Purpose: Handles the drawing of the top-down map and all the things on it
//-----------------------------------------------------------------------------
class CHudOverview : public CHudBase
{
public:
	int Init();
	int VidInit();

	int Draw(float flTime);
	void InitHUDData( void );

private:
	HSPRITE m_hsprPlayer;
	HSPRITE m_hsprViewcone;
};

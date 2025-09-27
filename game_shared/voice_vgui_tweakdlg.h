//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#pragma once

class CMenuPanel;


// Returns true if the tweak dialog is currently up.
bool IsTweakDlgOpen();

// Returns a global instance of the tweak dialog.
CMenuPanel* GetVoiceTweakDlg();
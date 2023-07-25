/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2023, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "main.h"

static BOOL __buymenu_firsttime_render;
static BOOL __DEBUG__ = FALSE;

WORD GetSavedTimes(int iSaveSlot)
{
	FILE* fp = UTIL_OpenFileAtPath(gConfig.pszSavePath, PAL_va(0, "%d.rpg", iSaveSlot));
	WORD wSavedTimes = 0;
	if (fp != NULL)
	{
		if (fread(&wSavedTimes, sizeof(WORD), 1, fp) == 1)
			wSavedTimes = SDL_SwapLE16(wSavedTimes);
		else
			wSavedTimes = 0;
		fclose(fp);
	}
	return wSavedTimes;
}

VOID
PAL_DrawOpeningMenuBackground(
	VOID
)
/*++
  Purpose:

	Draw the background of the main menu.

  Parameters:

	None.

  Return value:

	None.

--*/
{
	LPBYTE        buf;

	buf = (LPBYTE)malloc(320 * 200);
	if (buf == NULL)
	{
		return;
	}

	//
	// Read the picture from fbp.mkf.
	//
	PAL_MKFDecompressChunk(buf, 320 * 200, MAINMENU_BACKGROUND_FBPNUM, gpGlobals->f.fpFBP);

	//
	// ...and blit it to the screen buffer.
	//
	PAL_FBPBlitToSurface(buf, gpScreen);
	VIDEO_UpdateScreen(NULL);

	free(buf);
}

INT
PAL_OpeningMenu(
	VOID
)
/*++
  Purpose:

	Show the opening menu.

  Parameters:

	None.

  Return value:

	Which saved slot to load from (1-5). 0 to start a new game.

--*/
{
	WORD          wItemSelected, wDifficultyNum;
	WORD          wDefaultItem = 0;
	INT           w[2] = { PAL_WordWidth(MAINMENU_LABEL_NEWGAME), PAL_WordWidth(MAINMENU_LABEL_LOADGAME) };

	MENUITEM      rgMainMenuItem[2] = {
		// value   label                     enabled   position
		{  0,      MAINMENU_LABEL_NEWGAME,   TRUE,     PAL_XY(125 - (w[0] > 4 ? (w[0] - 4) * 8 : 0), 95)  },
		{  1,      MAINMENU_LABEL_LOADGAME,  TRUE,     PAL_XY(125 - (w[1] > 4 ? (w[1] - 4) * 8 : 0), 112) }
	};

	//
	// Play the background music
	//
	AUDIO_PlayMusic(RIX_NUM_OPENINGMENU, TRUE, 1);

	//
	// Draw the background
	//
	PAL_DrawOpeningMenuBackground();
	PAL_FadeIn(0, FALSE, 1);

	while (TRUE)
	{
		//
		// Activate the menu
		//
		wItemSelected = PAL_ReadMenu(NULL, rgMainMenuItem, 2, wDefaultItem, MENUITEM_COLOR);

		if (wItemSelected == 0)
		{
			//
			// Start a new game
			//
			wDifficultyNum = PAL_New_GameDifficultyMenu();
			if (wDifficultyNum != 0xFFFF)
			{
				wItemSelected = 0;
				gpGlobals->wDifficultyNum = wDifficultyNum;
				break;
			}
		}
		else if (wItemSelected == 1)
		{
			//
			// Load game
			//

			VIDEO_BackupScreen(gpScreen);
			wItemSelected = PAL_SaveSlotMenu(1, FALSE);
			VIDEO_RestoreScreen(gpScreen);
			VIDEO_UpdateScreen(NULL);
			if (wItemSelected != MENUITEM_VALUE_CANCELLED)
			{
				//
				// __DEBUG__空存档下选择难度，否则不进入
				//
				FILE* fp = UTIL_OpenFileAtPath(gConfig.pszSavePath, PAL_va(1, "%d.rpg", wItemSelected));
				if (fp == NULL)
				{
					wDifficultyNum = PAL_New_GameDifficultyMenu();

					if (wDifficultyNum != 0xFFFF)
					{
						gpGlobals->wDifficultyNum = wDifficultyNum;
						break;
					}
				}
				else
				{
					fclose(fp);
					break;
				}
			}

			wDefaultItem = 0;
		}
	}

	//
	// Fade out the screen and the music
	//
	AUDIO_PlayMusic(0, FALSE, 1);
	PAL_FadeOut(1);

	if (wItemSelected == 0)
	{
		PAL_PlayAVI("3.avi");
	}

	return (INT)wItemSelected;
}

INT
PAL_SaveSlotMenu(
	WORD        wDefaultSlot,
	BOOL        fIsSave
)
/*++
  Purpose:

	Show the load game menu.

  Parameters:

	[IN]  wDefaultSlot - default save slot number (1-5).

  Return value:

	Which saved slot to load from (1-5). MENUITEM_VALUE_CANCELLED if cancelled.

--*/
{
	//LPBOX           rgpBox[5];
	LPBOX               lpMenuBox;
	int             i, w = PAL_WordMaxWidth(LOADMENU_LABEL_SLOT_1, 10);
	int             dx = (w > 4) ? (w - 4) * 16 : 0;
	WORD            wItemSelected;

	MENUITEM        rgMenuItem[10];

	const SDL_Rect  rect = { 195 - dx, 7, 120 + dx, 190 };

	lpMenuBox = PAL_CreateBox(PAL_XY(200 - dx, 12), 8, 4, 1, FALSE);

	//
	// Create the boxes and create the menu items
	//
	for (i = 0; i < 10; i++)
	{
		rgMenuItem[i].wValue = i + 1;
		rgMenuItem[i].fEnabled = TRUE;
		rgMenuItem[0].wNumWord = LOADMENU_LABEL_SLOT_1;
		rgMenuItem[1].wNumWord = LOADMENU_LABEL_SLOT_2;
		rgMenuItem[2].wNumWord = LOADMENU_LABEL_SLOT_3;
		rgMenuItem[3].wNumWord = LOADMENU_LABEL_SLOT_4;
		rgMenuItem[4].wNumWord = LOADMENU_LABEL_SLOT_5;
		rgMenuItem[5].wNumWord = LOADMENU_LABEL_SLOT_6;
		rgMenuItem[6].wNumWord = LOADMENU_LABEL_SLOT_7;
		rgMenuItem[7].wNumWord = LOADMENU_LABEL_SLOT_8;
		rgMenuItem[8].wNumWord = LOADMENU_LABEL_SLOT_10;
		rgMenuItem[9].wNumWord = LOADMENU_LABEL_SLOT_11;
		rgMenuItem[i].pos = PAL_XY(210 - dx, 20 + 17 * i);
	}

	//
	// Draw the numbers of saved times
	//
	PAL_DrawNumber((UINT)GetSavedTimes(10), 5, PAL_XY(270, 6 + 170),
		kNumColorBlue, kNumAlignRight);
	PAL_DrawNumber((UINT)GetSavedTimes(9), 5, PAL_XY(270, 6 + 153),
		kNumColorCyan, kNumAlignRight);

	for (i = 1; i <= 8; i++)
	{
		//
		// Draw the number
		//
		PAL_DrawNumber((UINT)GetSavedTimes(i), 5, PAL_XY(270, 6 + 17 * i),
			kNumColorYellow, kNumAlignRight);
	}

	//
	// Activate the menu
	//
	VIDEO_UpdateScreen(&rect);
	wItemSelected = PAL_ReadMenu(NULL, rgMenuItem, 10, wDefaultSlot - 1, MENUITEM_COLOR);

	//
	// Delete the boxes
	//
	PAL_DeleteBox(lpMenuBox);

	return wItemSelected;
}

static
WORD
PAL_SelectionMenu(
	int   nWords,
	int   nDefault,
	WORD  wItems[]
)
/*++
  Purpose:

	Show a common selection box.

  Parameters:

	[IN]  nWords - number of emnu items.
	[IN]  nDefault - index of default item.
	[IN]  wItems - item word array.

  Return value:

	User-selected index.

--*/
{
	LPBOX           rgpBox[4];
	MENUITEM        rgMenuItem[4];
	int             w[4] = {
		(nWords >= 1 && wItems[0]) ? PAL_WordWidth(wItems[0]) : 1,
		(nWords >= 2 && wItems[1]) ? PAL_WordWidth(wItems[1]) : 1,
		(nWords >= 3 && wItems[2]) ? PAL_WordWidth(wItems[2]) : 1,
		(nWords >= 4 && wItems[3]) ? PAL_WordWidth(wItems[3]) : 1 };
	int             dx[4] = { (w[0] - 1) * 16, (w[1] - 1) * 16, (w[2] - 1) * 16, (w[3] - 1) * 16 }, i;
	PAL_POS         pos[4] = { PAL_XY(145, 110), PAL_XY(220 + dx[0], 110), PAL_XY(145, 160), PAL_XY(220 + dx[2], 160) };
	WORD            wReturnValue;

	const SDL_Rect  rect = { 130, 100, 125 + max(dx[0] + dx[1], dx[2] + dx[3]), 100 };

	for (i = 0; i < nWords; i++)
		if (nWords > i && !wItems[i])
			return MENUITEM_VALUE_CANCELLED;

	//
	// Create menu items
	//
	for (i = 0; i < nWords; i++)
	{
		rgMenuItem[i].fEnabled = TRUE;
		rgMenuItem[i].pos = pos[i];
		rgMenuItem[i].wValue = i;
		rgMenuItem[i].wNumWord = wItems[i];
	}

	//
	// Create the boxes
	//
	dx[1] = dx[0]; dx[3] = dx[2]; dx[0] = dx[2] = 0;
	for (i = 0; i < nWords; i++)
	{
		rgpBox[i] = PAL_CreateSingleLineBox(PAL_XY(130 + 75 * (i % 2) + dx[i], 100 + 50 * (i / 2)), w[i] + 1, TRUE);
	}

	//
	// Activate the menu
	//
	wReturnValue = PAL_ReadMenu(NULL, rgMenuItem, nWords, nDefault, MENUITEM_COLOR);

	//
	// Delete the boxes
	//
	for (i = 0; i < nWords; i++)
	{
		PAL_DeleteBox(rgpBox[i]);
	}

	VIDEO_UpdateScreen(&rect);

	return wReturnValue;
}

static
WORD
PAL_New_SelectionMenu(
	int   nWords,
	int   nDefault,
	WORD  wItems[]
)
/*++
  Purpose:

	Show a common selection box.

  Parameters:

	[IN]  nWords - number of emnu items.
	[IN]  nDefault - index of default item.
	[IN]  wItems - item word array.

  Return value:

	User-selected index.

--*/
{
	LPBOX           rgpBox[4];
	MENUITEM        rgMenuItem[4];
	int             w[4] = {
		(nWords >= 1 && wItems[0]) ? PAL_WordWidth(wItems[0]) : 1,
		(nWords >= 2 && wItems[1]) ? PAL_WordWidth(wItems[1]) : 1,
		(nWords >= 3 && wItems[2]) ? PAL_WordWidth(wItems[2]) : 1,
		(nWords >= 4 && wItems[3]) ? PAL_WordWidth(wItems[3]) : 1 };
	int             dx[4] = { (w[0] - 1) * 16, (w[1] - 1) * 16, (w[2] - 1) * 16, (w[3] - 1) * 16 }, i;
	PAL_POS         pos[4] = { PAL_XY(145, 110), PAL_XY(237, 110), PAL_XY(145, 160), PAL_XY(237, 160) };
	WORD            wReturnValue;

	const SDL_Rect  rect = { 130, 100, 125 + max(dx[0] + dx[1], dx[2] + dx[3]), 100 };

	for (i = 0; i < nWords; i++)
		if (nWords > i && !wItems[i])
			return MENUITEM_VALUE_CANCELLED;

	//
	// Create menu items
	//
	for (i = 0; i < nWords; i++)
	{
		rgMenuItem[i].fEnabled = TRUE;
		rgMenuItem[i].pos = pos[i];
		rgMenuItem[i].wValue = i;
		rgMenuItem[i].wNumWord = wItems[i];
	}

	//
	// Create the boxes
	// 创建背景框
	dx[1] = dx[0]; dx[3] = dx[2]; dx[0] = dx[2] = 0;
	for (i = 0; i < nWords; i++)
	{
		rgpBox[i] = PAL_CreateSingleLineBox(PAL_XY(130 + 91 * (i % 2), 100 + 50 * (i / 2)), w[i] + 1, TRUE);
	}

	//
	// Activate the menu
	// 激活菜单，这里是一个条件式死循环，满足条件时才会自动跳出循环
	wReturnValue = PAL_New_ReadMenu(NULL, rgMenuItem, nWords, nDefault, MENUITEM_COLOR);

	//
	// Delete the boxes
	// 清除背景框
	for (i = 0; i < nWords; i++)
	{
		PAL_DeleteBox(rgpBox[i]);
	}

	VIDEO_UpdateScreen(&rect);

	return wReturnValue;
}

WORD
PAL_TripleMenu(
	WORD  wThirdWord
)
/*++
  Purpose:

	Show a triple-selection box.

  Parameters:

	None.

  Return value:

	User-selected index.

--*/
{
	WORD wItems[3] = { CONFIRMMENU_LABEL_NO, CONFIRMMENU_LABEL_YES, wThirdWord };
	return PAL_SelectionMenu(3, 0, wItems);
}

BOOL
PAL_ConfirmMenu(
	VOID
)
/*++
  Purpose:

	Show a "Yes or No?" confirm box.

  Parameters:

	None.

  Return value:

	TRUE if user selected Yes, FALSE if selected No.

--*/
{
	WORD wItems[2] = { CONFIRMMENU_LABEL_NO, CONFIRMMENU_LABEL_YES };
	WORD wReturnValue = PAL_SelectionMenu(2, 1, wItems);

	return (wReturnValue == MENUITEM_VALUE_CANCELLED || wReturnValue == 0) ? FALSE : TRUE;
}

WORD
PAL_New_ConfirmMenu(
	VOID
)
/*++
  Purpose:

	Show a "Yes or No?" confirm box.

  Parameters:

	None.

  Return value:

	TRUE if user selected Yes, FALSE if selected No.

--*/
{
	WORD wItems[4] = { CONFIRMMENU_LABEL_YES, CONFIRMMENU_LABEL_NO, CONFIRMMENU_LABEL_BUYFIVE ,CONFIRMMENU_LABEL_BUYTEN };
	WORD wReturnValue = PAL_New_SelectionMenu(4, 0, wItems);

	return wReturnValue;

	//WORD wItems[2] = { CONFIRMMENU_LABEL_NO, CONFIRMMENU_LABEL_YES };
	//WORD wReturnValue = PAL_SelectionMenu(2, 0, wItems);

	//return (wReturnValue == MENUITEM_VALUE_CANCELLED || wReturnValue == 0) ? FALSE : TRUE;
}

BOOL
PAL_SwitchMenu(
	BOOL      fEnabled
)
/*++
  Purpose:

	Show a "Enable/Disable" selection box.

  Parameters:

	[IN]  fEnabled - whether the option is originally enabled or not.

  Return value:

	TRUE if user selected "Enable", FALSE if selected "Disable".

--*/
{
	WORD wItems[2] = { SWITCHMENU_LABEL_DISABLE, SWITCHMENU_LABEL_ENABLE };
	WORD wReturnValue = PAL_SelectionMenu(2, fEnabled ? 1 : 0, wItems);
	return (wReturnValue == MENUITEM_VALUE_CANCELLED) ? fEnabled : ((wReturnValue == 0) ? FALSE : TRUE);
}

#ifndef PAL_CLASSIC

static VOID
PAL_BattleSpeedMenu(
	VOID
)
/*++
  Purpose:

	Show the Battle Speed selection box.

  Parameters:

	None.

  Return value:

	None.

--*/
{
	LPBOX           lpBox;
	WORD            wReturnValue;
	const SDL_Rect  rect = { 131, 100, 165, 50 };

	MENUITEM        rgMenuItem[5] = {
	   { 1,   BATTLESPEEDMENU_LABEL_1,       TRUE,   PAL_XY(145, 110) },
	   { 2,   BATTLESPEEDMENU_LABEL_2,       TRUE,   PAL_XY(170, 110) },
	   { 3,   BATTLESPEEDMENU_LABEL_3,       TRUE,   PAL_XY(195, 110) },
	   { 4,   BATTLESPEEDMENU_LABEL_4,       TRUE,   PAL_XY(220, 110) },
	   { 5,   BATTLESPEEDMENU_LABEL_5,       TRUE,   PAL_XY(245, 110) },
	};

	//
	// Create the boxes
	//
	lpBox = PAL_CreateSingleLineBox(PAL_XY(131, 100), 8, TRUE);

	//
	// Activate the menu
	//
	wReturnValue = PAL_ReadMenu(NULL, rgMenuItem, 5, gpGlobals->bBattleSpeed - 1,
		MENUITEM_COLOR);

	//
	// Delete the boxes
	//
	PAL_DeleteBox(lpBox);

	VIDEO_UpdateScreen(&rect);

	if (wReturnValue != MENUITEM_VALUE_CANCELLED)
	{
		gpGlobals->bBattleSpeed = wReturnValue;
	}
}

#endif

LPBOX
PAL_ShowCash(
	DWORD      dwCash
)
/*++
  Purpose:

	Show the cash amount at the top left corner of the screen.

  Parameters:

	[IN]  dwCash - amount of cash.

  Return value:

	pointer to the saved screen part.

--*/
{
	LPBOX     lpBox;

	//
	// Create the box.
	//
	lpBox = PAL_CreateSingleLineBox(PAL_XY(0, 0), 5, TRUE);
	if (lpBox == NULL)
	{
		return NULL;
	}

	//
	// Draw the text label.
	//
	PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(10, 10), 0, FALSE, FALSE, FALSE);

	//
	// Draw the cash amount.
	//
	PAL_DrawNumber(dwCash, 7, PAL_XY(47, 14), kNumColorYellow, kNumAlignRight);

	return lpBox;
}

static VOID
PAL_SystemMenu_OnItemChange(
	WORD        wCurrentItem
)
/*++
  Purpose:

	Callback function when user selected another item in the system menu.

  Parameters:

	[IN]  wCurrentItem - current selected item.

  Return value:

	None.

--*/
{
	gpGlobals->iCurSystemMenuItem = wCurrentItem - 1;
}

static BOOL
PAL_SystemMenu(
	VOID
)
/*++
  Purpose:

	Show the system menu.

  Parameters:

	None.

  Return value:

	TRUE if user made some operations in the menu, FALSE if user cancelled.

--*/
{
	LPBOX               lpMenuBox;
	WORD                wDifficultyNum, wReturnValue;
	int                 iSlot, i;
	const SDL_Rect      rect = { 40, 60, 280, 135 };

	//
	// Create menu items
	//
	const MENUITEM      rgSystemMenuItem[] =
	{
		// value  label                        enabled   pos
		{ 1,      SYSMENU_LABEL_SAVE,          TRUE,     PAL_XY(53, 72) },
		{ 2,      SYSMENU_LABEL_LOAD,          TRUE,     PAL_XY(53, 72 + 18) },
		{ 3,      SYSMENU_LABEL_MUSIC,         TRUE,     PAL_XY(53, 72 + 36) },
		{ 4,      SYSMENU_LABEL_SOUND,         TRUE,     PAL_XY(53, 72 + 54) },
		{ 5,      SYSMENU_LABEL_QUIT,          TRUE,     PAL_XY(53, 72 + 72) }
  #if !defined(PAL_CLASSIC)
		{ 6,      SYSMENU_LABEL_BATTLEMODE,    TRUE,     PAL_XY(53, 72 + 90) },
  #endif
	};
	const int           nSystemMenuItem = sizeof(rgSystemMenuItem) / sizeof(MENUITEM);

	//
	// Create the menu box.
	//
	lpMenuBox = PAL_CreateBox(PAL_XY(40, 60), nSystemMenuItem - 1, PAL_MenuTextMaxWidth(rgSystemMenuItem, nSystemMenuItem) - 1, 0, TRUE);

	//
	// Perform the menu.
	//
	wReturnValue = PAL_ReadMenu(PAL_SystemMenu_OnItemChange, rgSystemMenuItem, nSystemMenuItem, gpGlobals->iCurSystemMenuItem, MENUITEM_COLOR);

	if (wReturnValue == MENUITEM_VALUE_CANCELLED)
	{
		//
		// User cancelled the menu
		//
		PAL_DeleteBox(lpMenuBox);
		VIDEO_UpdateScreen(&rect);
		return FALSE;
	}

	switch (wReturnValue)
	{
	case 1:
		//
		// Save game
		//
		iSlot = PAL_SaveSlotMenu(gpGlobals->bCurrentSaveSlot, TRUE);

		if (iSlot != MENUITEM_VALUE_CANCELLED && (iSlot != 9 || iSlot != 10))
		{
			WORD wSavedTimes = 0;
			gpGlobals->bCurrentSaveSlot = (BYTE)iSlot;

			for (i = 1; i <= 8; i++)
			{
				WORD curSavedTimes = GetSavedTimes(i);
				if (curSavedTimes > wSavedTimes)
				{
					wSavedTimes = curSavedTimes;
				}
			}
			PAL_SaveGame(iSlot, wSavedTimes + 1);
		}
		break;

	case 2:
		//
		// Load game
		//
		iSlot = PAL_SaveSlotMenu(gpGlobals->bCurrentSaveSlot, FALSE);
		if (iSlot != MENUITEM_VALUE_CANCELLED)
		{
			//
			// 空存档下选择难度，否则不进入
			//
			FILE* fp = UTIL_OpenFileAtPath(gConfig.pszSavePath, PAL_va(1, "%d.rpg", iSlot));
			if (fp == NULL)
			{
				wDifficultyNum = PAL_New_GameDifficultyMenu();

				if (wDifficultyNum != 0xFFFF)
				{
					gpGlobals->wDifficultyNum = wDifficultyNum;
				}
				else
				{
					break;
				}
			}
			else
			{
				fclose(fp);
			}

			AUDIO_PlayMusic(0, FALSE, 1);
			PAL_FadeOut(1);
			PAL_ReloadInNextTick(iSlot);
		}
		break;

	case 3:
		//
		// Music
		//
		AUDIO_EnableMusic(PAL_SwitchMenu(AUDIO_MusicEnabled()));
		if (gConfig.eMIDISynth == SYNTH_NATIVE && gConfig.eMusicType == MUSIC_MIDI)
		{
			AUDIO_PlayMusic(AUDIO_MusicEnabled() ? gpGlobals->wNumMusic : 0, AUDIO_MusicEnabled(), 0);
		}
		break;

	case 4:
		//
		// Sound
		//
		AUDIO_EnableSound(PAL_SwitchMenu(AUDIO_SoundEnabled()));
		break;

	case 5:
		//
		// Quit
		//
		PAL_QuitGame();
		break;

#if !defined(PAL_CLASSIC)
	case 6:
		//
		// Battle Mode
		//
		PAL_BattleSpeedMenu();
		break;
#endif
	}

	PAL_DeleteBox(lpMenuBox);
	return TRUE;
}

VOID
PAL_InGameMagicMenu(
	VOID
)
/*++
  Purpose:

	Show the magic menu.

  Parameters:

	None.

  Return value:

	None.

--*/
{
	MENUITEM         rgMenuItem[MAX_PLAYERS_IN_PARTY];
	int              i, y;
	static WORD      w;
	DWORD             wMagic;

	//
	// Draw the player info boxes
	//
	y = 45;

	if (gpGlobals->wMaxPartyMemberIndex >= 3)
	{
		y = 7;
	}

	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		PAL_PlayerInfoBox(PAL_XY(y, 165), gpGlobals->rgParty[i].wPlayerRole, 100,
			TIMEMETER_COLOR_DEFAULT, TRUE);
		y += 78;
	}

	y = 75;

	//
	// Generate one menu items for each player in the party
	//
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		assert(i <= MAX_PLAYERS_IN_PARTY);

		rgMenuItem[i].wValue = i;
		rgMenuItem[i].wNumWord =
			gpGlobals->g.PlayerRoles.rgwName[gpGlobals->rgParty[i].wPlayerRole];
		rgMenuItem[i].fEnabled =
			(gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[i].wPlayerRole] > 0);
		rgMenuItem[i].pos = PAL_XY(48, y);

		y += 18;
	}

	//
	// Draw the box
	//
	PAL_CreateBox(PAL_XY(35, 62), gpGlobals->wMaxPartyMemberIndex, 2, 0, FALSE);

	w = PAL_ReadMenu(NULL, rgMenuItem, gpGlobals->wMaxPartyMemberIndex + 1, w, MENUITEM_COLOR);

	if (w == MENUITEM_VALUE_CANCELLED)
	{
		return;
	}

	wMagic = 0;

	while (TRUE)
	{
		wMagic = PAL_MagicSelectionMenu(gpGlobals->rgParty[w].wPlayerRole, FALSE, wMagic);
		if (wMagic == 0)
		{
			break;
		}

		if (gpGlobals->g.rgObject[wMagic].magic.wFlags & kMagicFlagApplyToAll)
		{
			//
			// __DEBUG__暂时隐藏附加地图元素
			//
			gpGlobals->fIsTriggerScriptRun = TRUE;
			PAL_MakeScene();

			gpGlobals->g.rgObject[wMagic].magic.wScriptOnUse =
				PAL_RunTriggerScript(gpGlobals->g.rgObject[wMagic].magic.wScriptOnUse, 0);

			if (g_fScriptSuccess)
			{
				gpGlobals->g.rgObject[wMagic].magic.wScriptOnSuccess =
					PAL_RunTriggerScript(gpGlobals->g.rgObject[wMagic].magic.wScriptOnSuccess, 0);

				if (g_fScriptSuccess)
					gpGlobals->g.PlayerRoles.rgwMP[gpGlobals->rgParty[w].wPlayerRole] -=
					gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[wMagic].magic.wMagicNumber].wCostMP;
			}

			//
			// __DEBUG__显示附加地图元素
			//
			gpGlobals->fIsTriggerScriptRun = FALSE;

			if (gpGlobals->fNeedToFadeIn)
			{
				PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
				gpGlobals->fNeedToFadeIn = FALSE;
			}
		}
		else
		{
			//
			// Need to select which player to use the magic on.
			//
			WORD       wPlayer = 0;
			SDL_Rect   rect;

			while (wPlayer != MENUITEM_VALUE_CANCELLED)
			{
				//
				// Redraw the player info boxes first
				//
				y = 45;

				if (gpGlobals->wMaxPartyMemberIndex >= 3)
				{
					y = 7;
				}

				for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
				{
					PAL_PlayerInfoBox(PAL_XY(y, 165), gpGlobals->rgParty[i].wPlayerRole, 100,
						TIMEMETER_COLOR_DEFAULT, TRUE);
					y += 78;
				}

				//
				// Draw the cursor on the selected item
				//
				rect.x = 70 + 78 * wPlayer;
				rect.y = 193;
				rect.w = 9;
				rect.h = 6;

				PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR),
					gpScreen, PAL_XY(rect.x, rect.y));

				VIDEO_UpdateScreen(&rect);

				while (TRUE)
				{
					PAL_ClearKeyState();
					PAL_ProcessEvent();

					if (g_InputState.dwKeyPress & kKeyMenu)
					{
						wPlayer = MENUITEM_VALUE_CANCELLED;
						break;
					}
					else if (g_InputState.dwKeyPress & kKeySearch)
					{
						//
						// __DEBUG__暂时隐藏附加地图元素
						//
						gpGlobals->fIsTriggerScriptRun = TRUE;
						PAL_MakeScene();

						gpGlobals->g.rgObject[wMagic].magic.wScriptOnUse =
							PAL_RunTriggerScript(gpGlobals->g.rgObject[wMagic].magic.wScriptOnUse,
								gpGlobals->rgParty[wPlayer].wPlayerRole);

						if (g_fScriptSuccess)
						{
							gpGlobals->g.rgObject[wMagic].magic.wScriptOnSuccess =
								PAL_RunTriggerScript(gpGlobals->g.rgObject[wMagic].magic.wScriptOnSuccess,
									gpGlobals->rgParty[wPlayer].wPlayerRole);

							if (g_fScriptSuccess)
							{
								gpGlobals->g.PlayerRoles.rgwMP[gpGlobals->rgParty[w].wPlayerRole] -=
									gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[wMagic].magic.wMagicNumber].wCostMP;

								//
								// Check if we have run out of MP
								//
								if (gpGlobals->g.PlayerRoles.rgwMP[gpGlobals->rgParty[w].wPlayerRole] <
									gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[wMagic].magic.wMagicNumber].wCostMP)
								{
									//
									// Don't go further if run out of MP
									//
									wPlayer = MENUITEM_VALUE_CANCELLED;
								}
							}
						}

						//
						// __DEBUG__显示附加地图元素
						//
						gpGlobals->fIsTriggerScriptRun = FALSE;

						break;
					}
					else if (g_InputState.dwKeyPress & (kKeyLeft | kKeyUp))
					{
						if (wPlayer > 0)
						{
							wPlayer--;
							break;
						}
					}
					else if (g_InputState.dwKeyPress & (kKeyRight | kKeyDown))
					{
						if (wPlayer < gpGlobals->wMaxPartyMemberIndex)
						{
							wPlayer++;
							break;
						}
					}

					SDL_Delay(1);
				}
			}
		}

		//
		// Redraw the player info boxes
		//
		y = 45;

		if (gpGlobals->wMaxPartyMemberIndex >= 3)
		{
			y = 7;
		}

		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			PAL_PlayerInfoBox(PAL_XY(y, 165), gpGlobals->rgParty[i].wPlayerRole, 100,
				TIMEMETER_COLOR_DEFAULT, TRUE);
			y += 78;
		}
	}
}

static VOID
PAL_InventoryMenu(
	VOID
)
/*++
  Purpose:

	Show the inventory menu.

  Parameters:

	None.

  Return value:

	None.

--*/
{
	static WORD      w = 0;

	MENUITEM        rgMenuItem[2] =
	{
		// value  label                     enabled   pos
		{ 1,      INVMENU_LABEL_EQUIP,      TRUE,     PAL_XY(43, 73) },
		{ 2,      INVMENU_LABEL_USE,        TRUE,     PAL_XY(43, 73 + 18) },
	};

	PAL_CreateBox(PAL_XY(30, 60), 1, PAL_MenuTextMaxWidth(rgMenuItem, sizeof(rgMenuItem) / sizeof(MENUITEM)) - 1, 0, FALSE);

	w = PAL_ReadMenu(NULL, rgMenuItem, 2, w - 1, MENUITEM_COLOR);

	switch (w)
	{
	case 1:
		PAL_GameEquipItem();
		break;

	case 2:
		PAL_GameUseItem();
		break;
	}
}

static VOID
PAL_InGameMenu_OnItemChange(
	WORD        wCurrentItem
)
/*++
  Purpose:

	Callback function when user selected another item in the in-game menu.

  Parameters:

	[IN]  wCurrentItem - current selected item.

  Return value:

	None.

--*/
{
	gpGlobals->iCurMainMenuItem = wCurrentItem - 1;
}

VOID
PAL_InGameMenu(
	VOID
)
/*++
  Purpose:

	Show the in-game main menu.

  Parameters:

	None.

  Return value:

	None.

--*/
{
	LPBOX                lpCashBox, lpMenuBox;
	WORD                 wReturnValue;

	// Fix render problem with shadow
	VIDEO_BackupScreen(gpScreen);

	//
	// Create menu items
	//
	MENUITEM        rgMainMenuItem[] =
	{
		// value  label                      enabled   pos
		{ 1,      GAMEMENU_LABEL_STATUS,     TRUE,     PAL_XY(16, 50) },
		{ 2,      GAMEMENU_LABEL_MAGIC,      TRUE,     PAL_XY(16, 50 + 18) },
		{ 3,      GAMEMENU_LABEL_INVENTORY,  TRUE,     PAL_XY(16, 50 + 36) },
		{ 4,      GAMEMENU_LABEL_SYSTEM,     TRUE,     PAL_XY(16, 50 + 54) },
		{ 5,      BATTLEUI_LABEL_LEVELMAGIC,     TRUE,     PAL_XY(16, 50 + 72) },
	};

	//
	// Display the cash amount.
	//
	lpCashBox = PAL_ShowCash(gpGlobals->dwCash);

	// 显示坐标
	PAL_DrawNumber(gpGlobals->rgTrail[0].x, 5, PAL_XY(288 - 6 * 8, 0), kNumColorYellow, kNumAlignRight);
	PAL_DrawNumber(gpGlobals->rgTrail[0].y, 5, PAL_XY(288, 0), kNumColorCyan, kNumAlignRight);

	// 显示场景号
	PAL_DrawNumber(gpGlobals->wNumScene, 5, PAL_XY(288, 10), kNumColorBlue, kNumAlignRight);

	// 显示难度
	PAL_DrawText(PAL_GetWord(LABEL_DIFFICULTY_ARDER + gpGlobals->wDifficultyNum), PAL_XY(10, 183), MENUITEM_COLOR, TRUE, TRUE, FALSE);

	//
	// Create the menu box.
	//
	// Fix render problem with shadow
	lpMenuBox = PAL_CreateBox(PAL_XY(3, 37), sizeof(rgMainMenuItem) / sizeof(MENUITEM) - 1, PAL_MenuTextMaxWidth(rgMainMenuItem, sizeof(rgMainMenuItem) / sizeof(MENUITEM)) - 1, 0, FALSE);

	//
	// Process the menu
	//
	while (TRUE)
	{
		wReturnValue = PAL_ReadMenu(PAL_InGameMenu_OnItemChange, rgMainMenuItem, sizeof(rgMainMenuItem) / sizeof(MENUITEM),
			gpGlobals->iCurMainMenuItem, MENUITEM_COLOR);

		if (wReturnValue == MENUITEM_VALUE_CANCELLED)
		{
			break;
		}

		switch (wReturnValue)
		{
		case 1:
			//
			// Status
			//
			PAL_PlayerStatus();
			goto out;

		case 2:
			//
			// Magic
			//
			PAL_InGameMagicMenu();
			goto out;

		case 3:
			//
			// Inventory
			//
			PAL_InventoryMenu();
			goto out;

		case 4:
			//
			// System
			//
			if (PAL_SystemMenu())
			{
				goto out;
			}
			break;

		case 5:
			//
			// Inventory
			//
			PAL_New_AllMagicList();
			goto out;
		}
	}

out:
	//
	// Remove the boxes.
	//
	PAL_DeleteBox(lpCashBox);
	PAL_DeleteBox(lpMenuBox);

	// Fix render problem with shadow
	VIDEO_RestoreScreen(gpScreen);
}

VOID
PAL_PlayerStatus(
	VOID
)
/*++
  Purpose:

	Show the player status.

  Parameters:

	None.

  Return value:

	None.

--*/
{
	PAL_LARGE BYTE   bufBackground[320 * 200];
	PAL_LARGE BYTE   bufImage[PAL_RLEBUFSIZE];
	PAL_LARGE BYTE   bufImageBox[50 * 49];
	int              labels0[] = {
	   STATUS_LABEL_EXP, STATUS_LABEL_LEVEL, STATUS_LABEL_HP,
	   STATUS_LABEL_MP
	};
	int              labels1[] = {
	   STATUS_LABEL_EXP_LAYOUT, STATUS_LABEL_LEVEL_LAYOUT, STATUS_LABEL_HP_LAYOUT,
	   STATUS_LABEL_MP_LAYOUT
	};
	int              labels[] = {
	   STATUS_LABEL_ATTACKPOWER, STATUS_LABEL_MAGICPOWER, STATUS_LABEL_RESISTANCE,
	   STATUS_LABEL_DEXTERITY, STATUS_LABEL_FLEERATE
	};
	int              iCurrent;
	int              iPlayerRole;
	int              i, y, j;
	WORD             w;
	WCHAR            s[256];

	PAL_MKFDecompressChunk(bufBackground, 320 * 200, STATUS_BACKGROUND_FBPNUM, gpGlobals->f.fpFBP);
	iCurrent = 0;

	if (gConfig.fUseCustomScreenLayout)
	{
		for (i = 0; i < 49; i++)
		{
			memcpy(&bufImageBox[i * 50], &bufBackground[(i + 39) * 320 + 247], 50);
		}
		for (i = 0; i < 49; i++)
		{
			memcpy(&bufBackground[(i + 125) * 320 + 81], &bufBackground[(i + 125) * 320 + 81 - 50], 50);
			memcpy(&bufBackground[(i + 141) * 320 + 141], &bufBackground[(i + 141) * 320 + 81 - 50], 50);
			memcpy(&bufBackground[(i + 133) * 320 + 201], &bufBackground[(i + 133) * 320 + 81 - 50], 50);
			memcpy(&bufBackground[(i + 101) * 320 + 251], &bufBackground[(i + 101) * 320 + 81 - 50], 50);
			memcpy(&bufBackground[(i + 39) * 320 + 247], &bufBackground[(i + 39) * 320 + 189 - 50], 50);
			if (i > 0) memcpy(&bufBackground[(i - 1) * 320 + 189], &bufBackground[(i - 1) * 320 + 189 - 50], 50);
		}
		for (i = 0; i < MAX_PLAYER_EQUIPMENTS; i++)
		{
			short x = PAL_X(gConfig.ScreenLayout.RoleEquipImageBoxes[i]);
			short y = PAL_Y(gConfig.ScreenLayout.RoleEquipImageBoxes[i]);
			short sx = (x < 0) ? -x : 0, sy = (y < 0) ? -y : 0, d = (x > 270) ? x - 270 : 0;
			if (sx >= 50 || sy >= 49 || x >= 320 || y >= 200) continue;
			for (; sy < 49 && y + sy < 200; sy++)
			{
				memcpy(&bufBackground[(y + sy) * 320 + x + sx], &bufImageBox[sy * 50 + sx], 50 - sx - d);
			}
		}
	}

	while (iCurrent >= 0 && iCurrent <= gpGlobals->wMaxPartyMemberIndex)
	{
		iPlayerRole = gpGlobals->rgParty[iCurrent].wPlayerRole;

		//
		// Draw the background image
		//
		PAL_FBPBlitToSurface(bufBackground, gpScreen);

		//
		// Draw the image of player role
		//
		if (PAL_MKFReadChunk(bufImage, PAL_RLEBUFSIZE, gpGlobals->g.PlayerRoles.rgwAvatar[iPlayerRole], gpGlobals->f.fpRGM) > 0)
		{
			PAL_RLEBlitToSurface(bufImage, gpScreen, gConfig.ScreenLayout.RoleImage);
		}

		//
		// Draw the equipments
		//
		for (i = 0; i < MAX_PLAYER_EQUIPMENTS; i++)
		{
			int offset;

			w = gpGlobals->g.PlayerRoles.rgwEquipment[i][iPlayerRole];

			if (w == 0)
			{
				continue;
			}

			//
			// Draw the image
			//
			if (PAL_MKFReadChunk(bufImage, PAL_RLEBUFSIZE,
				gpGlobals->g.rgObject[w].item.wBitmap, gpGlobals->f.fpBALL) > 0)
			{
				PAL_RLEBlitToSurface(bufImage, gpScreen,
					PAL_XY_OFFSET(gConfig.ScreenLayout.RoleEquipImageBoxes[i], 1, 1));
			}

			//
			// Draw the text label
			//
			offset = PAL_WordWidth(w) * 16;
			if (PAL_X(gConfig.ScreenLayout.RoleEquipNames[i]) + offset > 320)
			{
				offset = 320 - PAL_X(gConfig.ScreenLayout.RoleEquipNames[i]) - offset;
			}
			else
			{
				offset = 0;
			}
			int index = &gConfig.ScreenLayout.RoleEquipNames[i] - gConfig.ScreenLayoutArray;
			BOOL fShadow = (gConfig.ScreenLayoutFlag[index] & DISABLE_SHADOW) ? FALSE : TRUE;
			BOOL fUse8x8Font = (gConfig.ScreenLayoutFlag[index] & USE_8x8_FONT) ? TRUE : FALSE;
			PAL_DrawText(PAL_GetWord(w), PAL_XY_OFFSET(gConfig.ScreenLayout.RoleEquipNames[i], offset, 0), STATUS_COLOR_EQUIPMENT, fShadow, FALSE, fUse8x8Font);
		}

		//
		// Draw the text labels
		//
		for (i = 0; i < sizeof(labels0) / sizeof(int); i++)
		{
			int index = labels1[i];
			BOOL fShadow = (gConfig.ScreenLayoutFlag[index] & DISABLE_SHADOW) ? FALSE : TRUE;
			BOOL fUse8x8Font = (gConfig.ScreenLayoutFlag[index] & USE_8x8_FONT) ? TRUE : FALSE;
			PAL_DrawText(PAL_GetWord(labels0[i]), *(&gConfig.ScreenLayout.RoleExpLabel + i), MENUITEM_COLOR, fShadow, FALSE, fUse8x8Font);
		}
		for (i = 0; i < sizeof(labels) / sizeof(int); i++)
		{
			int index = &gConfig.ScreenLayout.RoleStatusLabels[i] - gConfig.ScreenLayoutArray;
			BOOL fShadow = (gConfig.ScreenLayoutFlag[index] & DISABLE_SHADOW) ? FALSE : TRUE;
			BOOL fUse8x8Font = (gConfig.ScreenLayoutFlag[index] & USE_8x8_FONT) ? TRUE : FALSE;
			PAL_DrawText(PAL_GetWord(labels[i]), gConfig.ScreenLayout.RoleStatusLabels[i], MENUITEM_COLOR, fShadow, FALSE, fUse8x8Font);
		}

		PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[iPlayerRole]),
			gConfig.ScreenLayout.RoleName, MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);

		//
		// Draw the stats
		//
		if (gConfig.ScreenLayout.RoleExpSlash != 0)
		{
			PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, gConfig.ScreenLayout.RoleExpSlash);
		}
		if (gConfig.ScreenLayout.RoleHPSlash != 0)
		{
			PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, PAL_XY(78, 58));
		}
		if (gConfig.ScreenLayout.RoleMPSlash != 0)
		{
			PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, PAL_XY(78, 80));
		}

		if (gpGlobals->g.PlayerRoles.rgwHP[iPlayerRole] > PAL_GetPlayerMaxHP(iPlayerRole))
		{
			gpGlobals->g.PlayerRoles.rgwHP[iPlayerRole] = PAL_GetPlayerMaxHP(iPlayerRole);
		}
		if (gpGlobals->g.PlayerRoles.rgwMP[iPlayerRole] > PAL_GetPlayerMaxMP(iPlayerRole))
		{
			gpGlobals->g.PlayerRoles.rgwMP[iPlayerRole] = PAL_GetPlayerMaxMP(iPlayerRole);
		}

		j = 0;

		PAL_DrawNumber(gpGlobals->Exp.rgPrimaryExp[iPlayerRole].wExp, 6,
			gConfig.ScreenLayout.RoleCurrExp, kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(PAL_GetLevelUpBaseExp(gpGlobals->g.PlayerRoles.rgwLevel[iPlayerRole]), 6,
			gConfig.ScreenLayout.RoleNextExp, kNumColorCyan, kNumAlignRight);
		PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwLevel[iPlayerRole], 6,
			gConfig.ScreenLayout.RoleLevel, kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwHP[iPlayerRole], 6,
			PAL_XY(42, 56), kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerMaxHP(iPlayerRole), 6,
			PAL_XY(83, 61), kNumColorBlue, kNumAlignRight);
		PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMP[iPlayerRole], 6,
			PAL_XY(42, 78), kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerMaxMP(iPlayerRole), 6,
			PAL_XY(83, 83), kNumColorBlue, kNumAlignRight);

		PAL_DrawNumber((INT)PAL_GetPlayerActualAttackStrength(iPlayerRole), 6,
			gConfig.ScreenLayout.RoleStatusValues[0], kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber((INT)PAL_GetPlayerActualMagicStrength(iPlayerRole), 6,
			gConfig.ScreenLayout.RoleStatusValues[1], kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber((INT)PAL_GetPlayerActualDefense(iPlayerRole), 6,
			gConfig.ScreenLayout.RoleStatusValues[2], kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber((INT)PAL_GetPlayerActualDexterity(iPlayerRole), 6,
			gConfig.ScreenLayout.RoleStatusValues[3], kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber((INT)PAL_GetPlayerActualFleeRate(iPlayerRole), 6,
			gConfig.ScreenLayout.RoleStatusValues[4], kNumColorYellow, kNumAlignRight);

		PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"力道");
		PAL_DrawText(s, PAL_XY(84, 99), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"气劲");
		PAL_DrawText(s, PAL_XY(84, 119), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		PAL_DrawNumber((INT)PAL_GetPlayerPower(iPlayerRole), 5,
			PAL_XY(118, 103), kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber((INT)PAL_GetPlayerWisdom(iPlayerRole), 5,
			PAL_XY(118, 123), kNumColorYellow, kNumAlignRight);
		//
		// Draw all poisons
		//将毒的名称写在状态栏里
		y = 58;

#ifdef POISON_STATUS_EXPAND
		int wPoisonIntensity = 0;
#endif

		//
		// Draw all poisons
		//
		for (i = 0; i < MAX_POISONS; i++)
		{
			w = gpGlobals->rgPoisonStatus[i][iCurrent].wPoisonID;

			if (w != 0 && gpGlobals->g.rgObject[w].poison.wPoisonLevel <= 999)
			{
				if (i > 7)
				{
					if (i == 8)
					{
						y = 58;
					}
					PAL_DrawText(PAL_GetWord(w), PAL_XY(260, y),
						(BYTE)(gpGlobals->g.rgObject[w].poison.wColor + 10), TRUE, FALSE, FALSE);

#ifdef POISON_STATUS_EXPAND
					wPoisonIntensity = gpGlobals->rgPoisonStatus[i][iCurrent].wPoisonIntensity;
					if (wPoisonIntensity != 0)
					{
						PAL_DrawNumber(wPoisonIntensity, 2,
							PAL_XY(310, y + 4), kNumColorYellow, kNumAlignRight);
					}
#endif

					y += 18;
				}
				else
				{
					PAL_DrawText(PAL_GetWord(w), PAL_XY(185, y),
						(BYTE)(gpGlobals->g.rgObject[w].poison.wColor + 10), TRUE, FALSE, FALSE);

#ifdef POISON_STATUS_EXPAND
					wPoisonIntensity = gpGlobals->rgPoisonStatus[i][iCurrent].wPoisonIntensity;
					if (wPoisonIntensity != 0)
					{
						PAL_DrawNumber(wPoisonIntensity, 2,
							PAL_XY(235, y + 4), kNumColorYellow, kNumAlignRight);
					}
#endif
					y += 18;
				}
			}
		}

		//
		// Update the screen
		//
		VIDEO_UpdateScreen(NULL);

		//
		// Wait for input
		//
		PAL_ClearKeyState();

		while (TRUE)
		{
			UTIL_Delay(1);

			if (g_InputState.dwKeyPress & kKeyMenu)
			{
				iCurrent = -1;
				break;
			}
			else if (g_InputState.dwKeyPress & (kKeyLeft | kKeyUp))
			{
				iCurrent--;
				break;
			}
			else if (g_InputState.dwKeyPress & (kKeyRight | kKeyDown | kKeySearch))
			{
				iCurrent++;
				break;
			}
		}
	}
}

WORD
PAL_ItemUseMenu(
	WORD           wItemToUse
)
/*++
  Purpose:

	Show the use item menu.

  Parameters:

	[IN]  wItemToUse - the object ID of the item to use.

  Return value:

	The selected player to use the item onto.
	MENUITEM_VALUE_CANCELLED if user cancelled.

--*/
{
	BYTE           bColor, bSelectedColor;
	PAL_LARGE BYTE bufImage[2048];
	DWORD          dwColorChangeTime;
	static WORD    wSelectedPlayer = 0;
	SDL_Rect       rect = { 110, 2, 200, 200 };
	int            i;
	WCHAR          s[256];

	bSelectedColor = MENUITEM_COLOR_SELECTED_FIRST;
	dwColorChangeTime = 0;

	while (TRUE)
	{
		if (wSelectedPlayer > gpGlobals->wMaxPartyMemberIndex)
		{
			wSelectedPlayer = 0;
		}

		//
		// Draw the box
		//
		//PAL_CreateBox(PAL_XY(120, 2), 9, 9, 0, FALSE);
		PAL_CreateBox(PAL_XY(85, 2), 9, 12, 0, FALSE);

		//
		// Draw the stats of the selected player
		//
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_LEVEL), PAL_XY(201, 16),
			ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_HP), PAL_XY(201, 34),
			ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_MP), PAL_XY(201, 52),
			ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_ATTACKPOWER), PAL_XY(201, 70),
			ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_MAGICPOWER), PAL_XY(201, 88),
			ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_RESISTANCE), PAL_XY(201, 106),
			ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_DEXTERITY), PAL_XY(201, 124),
			ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_FLEERATE), PAL_XY(201, 142),
			ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);

		PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"力道");
		PAL_DrawText(s, PAL_XY(201, 160), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"气劲");
		PAL_DrawText(s, PAL_XY(201, 178), MENUITEM_COLOR, TRUE, FALSE, FALSE);

		i = gpGlobals->rgParty[wSelectedPlayer].wPlayerRole;

		PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwLevel[i], 5, PAL_XY(250, 20),
			kNumColorYellow, kNumAlignRight);

		PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen,
			PAL_XY(268, 38));
		PAL_DrawNumber(PAL_GetPlayerMaxHP(i), 5,
			PAL_XY(271, 40), kNumColorBlue, kNumAlignRight);
		PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwHP[i], 5,
			PAL_XY(236, 37), kNumColorYellow, kNumAlignRight);

		PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen,
			PAL_XY(269, 56));
		PAL_DrawNumber(PAL_GetPlayerMaxMP(i), 5,
			PAL_XY(271, 58), kNumColorBlue, kNumAlignRight);
		PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMP[i], 5,
			PAL_XY(236, 55), kNumColorYellow, kNumAlignRight);

		PAL_DrawNumber(PAL_GetPlayerActualAttackStrength(i), 5, PAL_XY(260, 74),
			kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerActualMagicStrength(i), 5, PAL_XY(260, 92),
			kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerActualDefense(i), 5, PAL_XY(260, 110),
			kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerActualDexterity(i), 5, PAL_XY(260, 128),
			kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerActualFleeRate(i), 5, PAL_XY(260, 146),
			kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerPower(i), 5,
			PAL_XY(260, 164), kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerWisdom(i), 5,
			PAL_XY(260, 182), kNumColorYellow, kNumAlignRight);

		//
		// Draw the names of the players in the party
		//
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			if (i == wSelectedPlayer)
			{
				bColor = bSelectedColor;
			}
			else
			{
				bColor = MENUITEM_COLOR;
			}

			//PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[gpGlobals->rgParty[i].wPlayerRole]), PAL_XY(135, 16 + 20 * i), bColor, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[gpGlobals->rgParty[i].wPlayerRole]), PAL_XY(95, 16 + 20 * i), bColor, TRUE, FALSE, FALSE);
		}

		//PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen, PAL_XY(130, 118));
		PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen, PAL_XY(90, 118));

		i = PAL_GetItemAmount(wItemToUse);

		if (i > 0)
		{
			//
			// Draw the picture of the item
			//
			if (PAL_MKFReadChunk(bufImage, 2048, gpGlobals->g.rgObject[wItemToUse].item.wBitmap, gpGlobals->f.fpBALL) > 0)
			{
				//PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(140, 128));
				PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(98, 125));
			}

			//
			// Draw the amount and label of the item
			//
			//PAL_DrawText(PAL_GetWord(wItemToUse), PAL_XY(136, 163), STATUS_COLOR_EQUIPMENT, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(wItemToUse), PAL_XY(92, 183), STATUS_COLOR_EQUIPMENT, TRUE, FALSE, FALSE);
			//PAL_DrawNumber(i, 3, PAL_XY(180, 173), kNumColorCyan, kNumAlignRight);
			PAL_DrawNumber(i, 4, PAL_XY(PAL_TextWidth(PAL_GetWord(wItemToUse)) + 95, 192), kNumColorCyan, kNumAlignLeft);
		}

		//
		// Update the screen area
		//
		//VIDEO_UpdateScreen(&rect);
		VIDEO_UpdateScreen(NULL);

		//
		// Wait for key
		//
		PAL_ClearKeyState();

		while (TRUE)
		{
			//
			// See if we should change the highlight color
			//
			if (SDL_TICKS_PASSED(SDL_GetTicks(), dwColorChangeTime))
			{
				if ((WORD)bSelectedColor + 1 >=
					(WORD)MENUITEM_COLOR_SELECTED_FIRST + MENUITEM_COLOR_SELECTED_TOTALNUM)
				{
					bSelectedColor = MENUITEM_COLOR_SELECTED_FIRST;
				}
				else
				{
					bSelectedColor++;
				}

				dwColorChangeTime = SDL_GetTicks() + (600 / MENUITEM_COLOR_SELECTED_TOTALNUM);

				//
				// Redraw the selected item.
				//
				//PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[gpGlobals->rgParty[wSelectedPlayer].wPlayerRole]), PAL_XY(135, 16 + 20 * wSelectedPlayer), bSelectedColor, FALSE, TRUE, FALSE);
				PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[gpGlobals->rgParty[wSelectedPlayer].wPlayerRole]), PAL_XY(95, 16 + 20 * wSelectedPlayer), bSelectedColor, FALSE, TRUE, FALSE);
			}

			PAL_ProcessEvent();

			if (g_InputState.dwKeyPress != 0)
			{
				break;
			}

			SDL_Delay(1);
		}

		if (i <= 0)
		{
			return MENUITEM_VALUE_CANCELLED;
		}

		if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
		{
			wSelectedPlayer--;
		}
		else if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
		{
			if (wSelectedPlayer < gpGlobals->wMaxPartyMemberIndex)
			{
				wSelectedPlayer++;
			}
		}
		else if (g_InputState.dwKeyPress & kKeyMenu)
		{
			break;
		}
		else if (g_InputState.dwKeyPress & kKeySearch)
		{
			return gpGlobals->rgParty[wSelectedPlayer].wPlayerRole;
		}
	}

	return MENUITEM_VALUE_CANCELLED;
}

static VOID
PAL_BuyMenu_OnItemChange(
	WORD           wCurrentItem
)
/*++
  Purpose:

	Callback function which is called when player selected another item
	in the buy menu.

  Parameters:

	[IN]  wCurrentItem - current item on the menu, indicates the object ID of
						 the currently selected item.

  Return value:

	None.

--*/
{
	const SDL_Rect      rect = { 20, 8, 300, 185 };
	int                 i, n;
	PAL_LARGE BYTE      bufImage[2048];

	if (__buymenu_firsttime_render)
		PAL_RLEBlitToSurfaceWithShadow(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen, PAL_XY(35 + 6, 8 + 6), TRUE);
	//
	// Draw the picture of current selected item
	//
	PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen,
		PAL_XY(35, 8));

	if (PAL_MKFReadChunk(bufImage, 2048,
		gpGlobals->g.rgObject[wCurrentItem].item.wBitmap, gpGlobals->f.fpBALL) > 0)
	{
		if (__buymenu_firsttime_render)
			PAL_RLEBlitToSurfaceWithShadow(bufImage, gpScreen, PAL_XY(42 + 6, 16 + 6), TRUE);
		PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(42, 16));
	}

	//
	// See how many of this item we have in the inventory
	//
	n = 0;

	for (i = 0; i < MAX_INVENTORY; i++)
	{
		if (gpGlobals->rgInventory[i].wItem == 0)
		{
			break;
		}
		else if (gpGlobals->rgInventory[i].wItem == wCurrentItem)
		{
			n = gpGlobals->rgInventory[i].nAmount;
			break;
		}
	}

	if (__buymenu_firsttime_render)
		PAL_CreateSingleLineBoxWithShadow(PAL_XY(20, 75), 5, FALSE, 6);
	else
		//
		// Draw the amount of this item in the inventory
		//
		PAL_CreateSingleLineBoxWithShadow(PAL_XY(20, 75), 5, FALSE, 0);
	PAL_DrawText(PAL_GetWord(BUYMENU_LABEL_CURRENT), PAL_XY(30, 84), 0, FALSE, FALSE, FALSE);
	PAL_DrawNumber(n, 6, PAL_XY(69, 88), kNumColorYellow, kNumAlignRight);

	if (__buymenu_firsttime_render)
		PAL_CreateSingleLineBoxWithShadow(PAL_XY(20, 114), 5, FALSE, 6);
	else
		//
		// Draw the cash amount
		//
		PAL_CreateSingleLineBoxWithShadow(PAL_XY(20, 114), 5, FALSE, 0);
	PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(30, 124), 0, FALSE, FALSE, FALSE);
	PAL_DrawNumber(gpGlobals->dwCash, 6, PAL_XY(69, 128), kNumColorYellow, kNumAlignRight);

	if (__buymenu_firsttime_render)
		PAL_CreateSingleLineBoxWithShadow(PAL_XY(20, 154), 5, FALSE, 6);
	else
		//
		// Draw the LingHu
		//

		PAL_CreateSingleLineBoxWithShadow(PAL_XY(20, 154), 5, FALSE, 0);
	PAL_DrawText(PAL_GetWord(SHOP_LINGHU_LABEL), PAL_XY(30, 164), 0, FALSE, FALSE, FALSE);
	PAL_DrawNumber(gpGlobals->wCollectValue, 6, PAL_XY(69, 169), kNumColorCyan, kNumAlignRight);

	VIDEO_UpdateScreen(&rect);

	__buymenu_firsttime_render = FALSE;
}


VOID
PAL_BuyMenu(
	WORD           wStoreNum
)
/*++
  Purpose:

	Show the buy item menu.

  Parameters:

	[IN]  wStoreNum - number of the store to buy items from.

  Return value:

	None.

--*/
{
	MENUITEM        rgMenuItem[MAX_STORE_ITEM];
	int             i, y;
	WORD            w, x, n = 0;// *wConsumables;
	DWORD* iConsumables;

	//
	// create the menu items
	//
	y = 22;

	for (i = 0; i < MAX_STORE_ITEM; i++)
	{
		if (gpGlobals->g.lprgStore[wStoreNum].rgwItems[i] == 0)
		{
			break;
		}

		rgMenuItem[i].wValue = gpGlobals->g.lprgStore[wStoreNum].rgwItems[i];
		rgMenuItem[i].wNumWord = gpGlobals->g.lprgStore[wStoreNum].rgwItems[i];
		rgMenuItem[i].fEnabled = TRUE;
		rgMenuItem[i].pos = PAL_XY(150, y);

		y += 18;
	}

	//
	// Draw the box
	//
	PAL_CreateBox(PAL_XY(125, 8), 8, 8, 1, FALSE);

	//
	// Draw the number of prices
	//
	for (y = 0; y < i; y++)
	{
		w = gpGlobals->g.rgObject[rgMenuItem[y].wValue].item.wPrice;
		PAL_DrawNumber(w, 6, PAL_XY(235, 25 + y * 18), kNumColorCyan, kNumAlignRight);
	}

	w = 0;
	__buymenu_firsttime_render = TRUE;

	// 获取灵葫值或金钱
	iConsumables = (wStoreNum == 0) ? &gpGlobals->wCollectValue : &gpGlobals->dwCash;

	while (TRUE)
	{
		w = PAL_ReadMenu(PAL_BuyMenu_OnItemChange, rgMenuItem, i, w, MENUITEM_COLOR);

		if (w == MENUITEM_VALUE_CANCELLED)
		{
			break;
		}

		if (gpGlobals->g.rgObject[w].item.wPrice <= *iConsumables)
		{
			//
			// 打开购买选择框,判断玩家选择的结果
			//
			x = PAL_New_ConfirmMenu();
			if (x == 0)
			{
				//
				// Player bought an item
				// 玩家购买了 1 个道具
				n = 1;
			}
			else if (x == 2)
			{
				//
				// 玩家购买了 5 个道具
				// 
				n = 5;
			}
			else if (x == 3)
			{
				//
				// 玩家购买了 10 个道具
				// 
				n = 10;
			}
			else
			{
				// 玩家取消了购买
				x = 1;
			}

			//
			// 若用户未选择“否”并且钱够用则购买成功
			//
			if (x != 1 && gpGlobals->g.rgObject[w].item.wPrice * n <= *iConsumables)
			{
				*iConsumables -= gpGlobals->g.rgObject[w].item.wPrice * n;
				PAL_AddItemToInventory(w, n);
			}
		}

		//
		// Place the cursor to the current item on next loop
		// 在下一个循环中将光标放置到当前项
		for (y = 0; y < i; y++)
		{
			if (w == rgMenuItem[y].wValue)
			{
				w = y;
				break;
			}
		}
	}
}

static VOID
PAL_SellMenu_OnItemChange(
	WORD         wCurrentItem
)
/*++
  Purpose:

	Callback function which is called when player selected another item
	in the sell item menu.

  Parameters:

	[IN]  wCurrentItem - current item on the menu, indicates the object ID of
						 the currently selected item.

  Return value:

	None.

--*/
{
	//
	// Draw the cash amount
	//
	PAL_CreateSingleLineBoxWithShadow(PAL_XY(100, 150), 5, FALSE, 0);
	PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(110, 160), 0, FALSE, FALSE, FALSE);
	PAL_DrawNumber(gpGlobals->dwCash, 7, PAL_XY(149, 164), kNumColorYellow, kNumAlignRight);

	//
	// Draw the price
	//
	PAL_CreateSingleLineBoxWithShadow(PAL_XY(220, 150), 5, FALSE, 0);

	if (gpGlobals->g.rgObject[wCurrentItem].item.wFlags & kItemFlagSellable)
	{
		PAL_DrawText(PAL_GetWord(SELLMENU_LABEL_PRICE), PAL_XY(230, 160), 0, FALSE, FALSE, FALSE);
		PAL_DrawNumber(gpGlobals->g.rgObject[wCurrentItem].item.wPrice * 0.5, 6,
			PAL_XY(269, 164), kNumColorYellow, kNumAlignRight);
	}
}

VOID
PAL_SellMenu(
	VOID
)
/*++
  Purpose:

	Show the sell item menu.

  Parameters:

	None.

  Return value:

	None.

--*/
{
	WORD      w;

	while (TRUE)
	{
		w = PAL_ItemSelectMenu(PAL_SellMenu_OnItemChange, kItemFlagSellable);
		if (w == 0)
		{
			break;
		}

		if (PAL_ConfirmMenu())
		{
			if (PAL_AddItemToInventory(w, -1))
			{
				gpGlobals->dwCash += gpGlobals->g.rgObject[w].item.wPrice * 0.5;
			}
		}
	}
}

VOID
PAL_EquipItemMenu(
	WORD        wItem
)
/*++
  Purpose:

	Show the menu which allow players to equip the specified item.

  Parameters:

	[IN]  wItem - the object ID of the item.

  Return value:

	None.

--*/
{
	PAL_LARGE BYTE   bufBackground[320 * 200];
	PAL_LARGE BYTE   bufImageBox[72 * 72];
	PAL_LARGE BYTE   bufImage[2048];
	WORD             w;
	int              iCurrentPlayer, i;
	BYTE             bColor, bSelectedColor;
	DWORD            dwColorChangeTime;
	WCHAR            s[256];

	gpGlobals->wLastUnequippedItem = wItem;

	PAL_MKFDecompressChunk(bufBackground, 320 * 200, EQUIPMENU_BACKGROUND_FBPNUM,
		gpGlobals->f.fpFBP);

	if (gConfig.fUseCustomScreenLayout)
	{
		int x = PAL_X(gConfig.ScreenLayout.EquipImageBox);
		int y = PAL_Y(gConfig.ScreenLayout.EquipImageBox);
		for (i = 8; i < 72; i++)
		{
			memcpy(&bufBackground[i * 320 + 92], &bufBackground[(i + 128) * 320 + 92], 32);
			memcpy(&bufBackground[(i + 64) * 320 + 92], &bufBackground[(i + 128) * 320 + 92], 32);
		}
		for (i = 9; i < 90; i++)
		{
			memcpy(&bufBackground[i * 320 + 226], &bufBackground[(i + 104) * 320 + 226], 32);
		}
		for (i = 99; i < 113; i++)
		{
			memcpy(&bufBackground[i * 320 + 226], &bufBackground[(i + 16) * 320 + 226], 32);
		}
		for (i = 8; i < 80; i++)
		{
			memcpy(&bufImageBox[(i - 8) * 72], &bufBackground[i * 320 + 8], 72);
			memcpy(&bufBackground[i * 320 + 8], &bufBackground[(i + 72) * 320 + 8], 72);
		}
		for (i = 0; i < 72; i++)
		{
			memcpy(&bufBackground[(i + y) * 320 + x], &bufImageBox[i * 72], 72);
		}
	}

	iCurrentPlayer = 0;
	bSelectedColor = MENUITEM_COLOR_SELECTED_FIRST;
	dwColorChangeTime = SDL_GetTicks() + (600 / MENUITEM_COLOR_SELECTED_TOTALNUM);

	while (TRUE)
	{
		wItem = gpGlobals->wLastUnequippedItem;

		//
		// Draw the background
		//
		PAL_FBPBlitToSurface(bufBackground, gpScreen);

		//
		// Draw the item picture
		//
		if (PAL_MKFReadChunk(bufImage, 2048,
			gpGlobals->g.rgObject[wItem].item.wBitmap, gpGlobals->f.fpBALL) > 0)
		{
			PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY_OFFSET(gConfig.ScreenLayout.EquipImageBox, 8, 8));
		}

		if (gConfig.fUseCustomScreenLayout)
		{
			int labels1[] = { STATUS_LABEL_ATTACKPOWER, STATUS_LABEL_MAGICPOWER, STATUS_LABEL_RESISTANCE, STATUS_LABEL_DEXTERITY, STATUS_LABEL_FLEERATE };
			int labels2[] = { EQUIP_LABEL_HEAD, EQUIP_LABEL_SHOULDER, EQUIP_LABEL_BODY, EQUIP_LABEL_HAND, EQUIP_LABEL_FOOT, EQUIP_LABEL_NECK };
			for (i = 0; i < sizeof(labels1) / sizeof(int); i++)
			{
				int index = &gConfig.ScreenLayout.EquipStatusLabels[i] - gConfig.ScreenLayoutArray;
				BOOL fShadow = (gConfig.ScreenLayoutFlag[index] & DISABLE_SHADOW) ? FALSE : TRUE;
				BOOL fUse8x8Font = (gConfig.ScreenLayoutFlag[index] & USE_8x8_FONT) ? TRUE : FALSE;
				PAL_DrawText(PAL_GetWord(labels1[i]), gConfig.ScreenLayoutArray[index], MENUITEM_COLOR, fShadow, FALSE, fUse8x8Font);
			}
			for (i = 0; i < sizeof(labels2) / sizeof(int); i++)
			{
				int index = &gConfig.ScreenLayout.EquipLabels[i] - gConfig.ScreenLayoutArray;
				BOOL fShadow = (gConfig.ScreenLayoutFlag[index] & DISABLE_SHADOW) ? FALSE : TRUE;
				BOOL fUse8x8Font = (gConfig.ScreenLayoutFlag[index] & USE_8x8_FONT) ? TRUE : FALSE;
				PAL_DrawText(PAL_GetWord(labels2[i]), gConfig.ScreenLayoutArray[index], MENUITEM_COLOR, fShadow, FALSE, fUse8x8Font);
			}
		}

		//
		// Draw the current equipment of the selected player
		//
		w = gpGlobals->rgParty[iCurrentPlayer].wPlayerRole;
		for (i = 0; i < MAX_PLAYER_EQUIPMENTS; i++)
		{
			if (gpGlobals->g.PlayerRoles.rgwEquipment[i][w] != 0)
			{
				PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwEquipment[i][w]),
					gConfig.ScreenLayout.EquipNames[i], MENUITEM_COLOR, TRUE, FALSE, FALSE);
			}
		}

		PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"力道");
		PAL_DrawText(s, PAL_XY(227, 117), 0, FALSE, FALSE, FALSE);

		PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"气劲");
		PAL_DrawText(s, PAL_XY(227, 137), 0, FALSE, FALSE, FALSE);

		PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"体力上限");
		PAL_DrawText(s, PAL_XY(207, 157), 0, FALSE, FALSE, FALSE);

		PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"真气上限");
		PAL_DrawText(s, PAL_XY(207, 177), 0, FALSE, FALSE, FALSE);


		//
		// Draw the stats of the currently selected player
		//
		PAL_DrawNumber(PAL_GetPlayerActualAttackStrength(w), 6, PAL_XY(280, 14), kNumColorCyan, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerActualMagicStrength(w), 6, PAL_XY(280, 36), kNumColorCyan, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerActualDefense(w), 6, PAL_XY(280, 58), kNumColorCyan, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerActualDexterity(w), 6, PAL_XY(280, 80), kNumColorCyan, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerActualFleeRate(w), 6, PAL_XY(280, 102), kNumColorCyan, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerMaxHP(w), 6, PAL_XY(280, 162), kNumColorCyan, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerMaxMP(w), 6, PAL_XY(280, 182), kNumColorCyan, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerPower(w), 6, PAL_XY(280, 122), kNumColorCyan, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerWisdom(w), 6, PAL_XY(280, 142), kNumColorCyan, kNumAlignRight);

		//
		// Draw a box for player selection
		//
		PAL_CreateBox(PAL_XY(2, 95), gpGlobals->wMaxPartyMemberIndex, 2, 0, FALSE);

		//
		// Draw the label of players
		//
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			w = gpGlobals->rgParty[i].wPlayerRole;

			if (iCurrentPlayer == i)
			{
				if (gpGlobals->g.rgObject[wItem].item.wFlags & (kItemFlagEquipableByPlayerRole_First << w))
				{
					bColor = bSelectedColor;
				}
				else
				{
					bColor = MENUITEM_COLOR_SELECTED_INACTIVE;
				}
			}
			else
			{
				if (gpGlobals->g.rgObject[wItem].item.wFlags & (kItemFlagEquipableByPlayerRole_First << w))
				{
					bColor = MENUITEM_COLOR;
				}
				else
				{
					bColor = MENUITEM_COLOR_INACTIVE;
				}
			}

			PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]),
				PAL_XY(15, 108 + 18 * i), bColor, TRUE, FALSE, FALSE);
		}

		//
		// Draw the text label and amount of the item
		//
		if (wItem != 0)
		{
			PAL_DrawText(PAL_GetWord(wItem), gConfig.ScreenLayout.EquipItemName, MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
			PAL_DrawNumber(PAL_GetItemAmount(wItem), 2, gConfig.ScreenLayout.EquipItemAmount, kNumColorCyan, kNumAlignRight);
		}

		//
		// Update the screen
		//
		VIDEO_UpdateScreen(NULL);

		//
		// Accept input
		//
		PAL_ClearKeyState();

		while (TRUE)
		{
			PAL_ProcessEvent();

			//
			// See if we should change the highlight color
			//
			if (SDL_TICKS_PASSED(SDL_GetTicks(), dwColorChangeTime))
			{
				if ((WORD)bSelectedColor + 1 >=
					(WORD)MENUITEM_COLOR_SELECTED_FIRST + MENUITEM_COLOR_SELECTED_TOTALNUM)
				{
					bSelectedColor = MENUITEM_COLOR_SELECTED_FIRST;
				}
				else
				{
					bSelectedColor++;
				}

				dwColorChangeTime = SDL_GetTicks() + (600 / MENUITEM_COLOR_SELECTED_TOTALNUM);

				//
				// Redraw the selected item if needed.
				//
				w = gpGlobals->rgParty[iCurrentPlayer].wPlayerRole;

				if (gpGlobals->g.rgObject[wItem].item.wFlags & (kItemFlagEquipableByPlayerRole_First << w))
				{
					PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]),
						PAL_XY_OFFSET(gConfig.ScreenLayout.EquipRoleListBox, 13, 13 + 18 * iCurrentPlayer), bSelectedColor, TRUE, TRUE, FALSE);
				}
			}

			if (g_InputState.dwKeyPress != 0)
			{
				break;
			}

			SDL_Delay(1);
		}

		if (wItem == 0)
		{
			return;
		}

		if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
		{
			iCurrentPlayer--;
			if (iCurrentPlayer < 0)
			{
				iCurrentPlayer = 0;
			}
		}
		else if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
		{
			iCurrentPlayer++;
			if (iCurrentPlayer > gpGlobals->wMaxPartyMemberIndex)
			{
				iCurrentPlayer = gpGlobals->wMaxPartyMemberIndex;
			}
		}
		else if (g_InputState.dwKeyPress & kKeyMenu)
		{
			return;
		}
		else if (g_InputState.dwKeyPress & kKeySearch)
		{
			w = gpGlobals->rgParty[iCurrentPlayer].wPlayerRole;

			if (gpGlobals->g.rgObject[wItem].item.wFlags & (kItemFlagEquipableByPlayerRole_First << w))
			{
				//
				// __DEBUG__暂时隐藏附加地图元素
				//
				gpGlobals->fIsTriggerScriptRun = TRUE;
				PAL_MakeScene();

				//
				// Run the equip script
				//
				gpGlobals->g.rgObject[wItem].item.wScriptOnEquip =
					PAL_RunTriggerScript(gpGlobals->g.rgObject[wItem].item.wScriptOnEquip,
						gpGlobals->rgParty[iCurrentPlayer].wPlayerRole);

				//
				// __DEBUG__显示附加地图元素
				//
				gpGlobals->fIsTriggerScriptRun = FALSE;
			}
		}
	}
}

VOID
PAL_QuitGame(
	VOID
)
{
#if PAL_HAS_CONFIG_PAGE
	WORD wReturnValue = PAL_TripleMenu(SYSMENU_LABEL_LAUNCHSETTING);
#else
	WORD wReturnValue = PAL_ConfirmMenu(); // No config menu available
#endif
	if (wReturnValue == 1 || wReturnValue == 2)
	{
		if (wReturnValue == 2) gConfig.fLaunchSetting = TRUE;
		PAL_SaveConfig();		// Keep the fullscreen state
		AUDIO_PlayMusic(0, FALSE, 2);
		PAL_FadeOut(2);
		PAL_Shutdown(0);
	}
}

VOID
PAL_New_AllMagicList(
	VOID
)
/*++
Purpose:
Show the player status.
Parameters:
None.
Return value:
None.
--*/
{
	PAL_LARGE BYTE   bufBackground[320 * 200];
	PAL_LARGE BYTE   bufImage[16384];
	int              iCurrent;
	int              iPlayerRole;
	int              j, i;

	PAL_MKFDecompressChunk(bufBackground, 320 * 200, 60,
		gpGlobals->f.fpFBP);
	iCurrent = 0;

	//
	// __DEBUG__设置术谱页面关闭
	//
	g_Battle.UI.MenuState = kBattleMenuMagicList;

	while (iCurrent >= 0 && iCurrent <= gpGlobals->wMaxPartyMemberIndex)
	{
		iPlayerRole = gpGlobals->rgParty[iCurrent].wPlayerRole;

		//
		// Draw the background image
		//
		PAL_FBPBlitToSurface(bufBackground, gpScreen);

		//
		if (PAL_MKFReadChunk(bufImage, 16384,
			gpGlobals->g.PlayerRoles.rgwAvatar[iPlayerRole], gpGlobals->f.fpRGM) > 0)
		{
			PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(240, 100));
			VIDEO_UpdateScreen(NULL);
		}

		j = 0;
		i = 0;

		if (iPlayerRole == RoleID_LiXiaoYao)
		{
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i].m[iPlayerRole].wMagic),
				PAL_XY(1, 5), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 1].m[iPlayerRole].wMagic),
				PAL_XY(110, 5), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 2].m[iPlayerRole].wMagic),
				PAL_XY(220, 5), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);

			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 3].m[iPlayerRole].wMagic),
				PAL_XY(1, 25), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 4].m[iPlayerRole].wMagic),
				PAL_XY(110, 25), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 5].m[iPlayerRole].wMagic),
				PAL_XY(220, 25), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);

			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 6].m[iPlayerRole].wMagic),
				PAL_XY(1, 45), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 7].m[iPlayerRole].wMagic),
				PAL_XY(110, 45), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[j + 8].m[iPlayerRole].wMagic),
				PAL_XY(220, 45), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);

			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 9].m[iPlayerRole].wMagic),
				PAL_XY(1, 65), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 10].m[iPlayerRole].wMagic),
				PAL_XY(110, 65), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 11].m[iPlayerRole].wMagic),
				PAL_XY(220, 65), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);

			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 12].m[iPlayerRole].wMagic),
				PAL_XY(1, 85), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);

		}

		if (iPlayerRole == RoleID_ZhaoLingEr)
		{
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i].m[iPlayerRole].wMagic),
				PAL_XY(1, 5), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 1].m[iPlayerRole].wMagic),
				PAL_XY(110, 5), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 2].m[iPlayerRole].wMagic),
				PAL_XY(220, 5), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);

			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 3].m[iPlayerRole].wMagic),
				PAL_XY(1, 25), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 4].m[iPlayerRole].wMagic),
				PAL_XY(110, 25), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 5].m[iPlayerRole].wMagic),
				PAL_XY(220, 25), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);

			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 6].m[iPlayerRole].wMagic),
				PAL_XY(1, 45), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 7].m[iPlayerRole].wMagic),
				PAL_XY(110, 45), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[j + 8].m[iPlayerRole].wMagic),
				PAL_XY(220, 45), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);

			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 9].m[iPlayerRole].wMagic),
				PAL_XY(1, 65), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 10].m[iPlayerRole].wMagic),
				PAL_XY(110, 65), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 11].m[iPlayerRole].wMagic),
				PAL_XY(220, 65), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);

			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 12].m[iPlayerRole].wMagic),
				PAL_XY(1, 85), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 13].m[iPlayerRole].wMagic),
				PAL_XY(110, 85), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 14].m[iPlayerRole].wMagic),
				PAL_XY(220, 85), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);

			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 15].m[iPlayerRole].wMagic),
				PAL_XY(1, 105), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 16].m[iPlayerRole].wMagic),
				PAL_XY(110, 105), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 17].m[iPlayerRole].wMagic),
				PAL_XY(220, 105), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);

		}

		if (iPlayerRole == RoleID_LinYueRu)
		{
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i].m[iPlayerRole].wMagic),
				PAL_XY(1, 5), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 1].m[iPlayerRole].wMagic),
				PAL_XY(110, 5), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 2].m[iPlayerRole].wMagic),
				PAL_XY(220, 5), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);

			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 3].m[iPlayerRole].wMagic),
				PAL_XY(1, 25), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 4].m[iPlayerRole].wMagic),
				PAL_XY(110, 25), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 5].m[iPlayerRole].wMagic),
				PAL_XY(220, 25), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);

			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 6].m[iPlayerRole].wMagic),
				PAL_XY(1, 45), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 7].m[iPlayerRole].wMagic),
				PAL_XY(110, 45), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[j + 8].m[iPlayerRole].wMagic),
				PAL_XY(220, 45), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);

			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 9].m[iPlayerRole].wMagic),
				PAL_XY(1, 65), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 10].m[iPlayerRole].wMagic),
				PAL_XY(110, 65), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 11].m[iPlayerRole].wMagic),
				PAL_XY(220, 65), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);

			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 12].m[iPlayerRole].wMagic),
				PAL_XY(1, 85), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 13].m[iPlayerRole].wMagic),
				PAL_XY(110, 85), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
		}


		if (iPlayerRole == RoleID_ANu)
		{
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i].m[iPlayerRole].wMagic),
				PAL_XY(1, 5), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 1].m[iPlayerRole].wMagic),
				PAL_XY(110, 5), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 2].m[iPlayerRole].wMagic),
				PAL_XY(220, 5), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);

			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 3].m[iPlayerRole].wMagic),
				PAL_XY(1, 25), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 4].m[iPlayerRole].wMagic),
				PAL_XY(110, 25), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 5].m[iPlayerRole].wMagic),
				PAL_XY(220, 25), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);

			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 6].m[iPlayerRole].wMagic),
				PAL_XY(1, 45), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i + 7].m[iPlayerRole].wMagic),
				PAL_XY(110, 45), STATUS_COLOR_MAGIC, TRUE, FALSE, FALSE);
		}
		////分隔符/////////////////////////////////////////////

		if (iPlayerRole == RoleID_LiXiaoYao)

		{
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j].m[iPlayerRole].wLevel, 3,
				PAL_XY(92, 10), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 1].m[iPlayerRole].wLevel, 3,
				PAL_XY(202, 10), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 2].m[iPlayerRole].wLevel, 3,
				PAL_XY(302, 10), kNumColorBlue, kNumAlignRight);

			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 3].m[iPlayerRole].wLevel, 3,
				PAL_XY(92, 30), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 4].m[iPlayerRole].wLevel, 3,
				PAL_XY(202, 30), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 5].m[iPlayerRole].wLevel, 3,
				PAL_XY(302, 30), kNumColorBlue, kNumAlignRight);

			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 6].m[iPlayerRole].wLevel, 3,
				PAL_XY(92, 50), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 7].m[iPlayerRole].wLevel, 3,
				PAL_XY(202, 50), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 8].m[iPlayerRole].wLevel, 3,
				PAL_XY(302, 50), kNumColorBlue, kNumAlignRight);

			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 9].m[iPlayerRole].wLevel, 3,
				PAL_XY(92, 70), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 10].m[iPlayerRole].wLevel, 3,
				PAL_XY(202, 70), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 11].m[iPlayerRole].wLevel, 3,
				PAL_XY(302, 70), kNumColorBlue, kNumAlignRight);

			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 12].m[iPlayerRole].wLevel, 3,
				PAL_XY(92, 90), kNumColorBlue, kNumAlignRight);
		}


		if (iPlayerRole == RoleID_ZhaoLingEr)

		{
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j].m[iPlayerRole].wLevel, 3,
				PAL_XY(92, 10), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 1].m[iPlayerRole].wLevel, 3,
				PAL_XY(202, 10), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 2].m[iPlayerRole].wLevel, 3,
				PAL_XY(302, 10), kNumColorBlue, kNumAlignRight);

			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 3].m[iPlayerRole].wLevel, 3,
				PAL_XY(92, 30), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 4].m[iPlayerRole].wLevel, 3,
				PAL_XY(202, 30), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 5].m[iPlayerRole].wLevel, 3,
				PAL_XY(302, 30), kNumColorBlue, kNumAlignRight);

			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 6].m[iPlayerRole].wLevel, 3,
				PAL_XY(92, 50), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 7].m[iPlayerRole].wLevel, 3,
				PAL_XY(202, 50), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 8].m[iPlayerRole].wLevel, 3,
				PAL_XY(302, 50), kNumColorBlue, kNumAlignRight);

			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 9].m[iPlayerRole].wLevel, 3,
				PAL_XY(92, 70), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 10].m[iPlayerRole].wLevel, 3,
				PAL_XY(202, 70), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 11].m[iPlayerRole].wLevel, 3,
				PAL_XY(302, 70), kNumColorBlue, kNumAlignRight);

			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 12].m[iPlayerRole].wLevel, 3,
				PAL_XY(92, 90), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 13].m[iPlayerRole].wLevel, 3,
				PAL_XY(202, 90), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 14].m[iPlayerRole].wLevel, 3,
				PAL_XY(302, 90), kNumColorBlue, kNumAlignRight);

			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 15].m[iPlayerRole].wLevel, 3,
				PAL_XY(92, 110), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 16].m[iPlayerRole].wLevel, 3,
				PAL_XY(202, 110), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 17].m[iPlayerRole].wLevel, 3,
				PAL_XY(302, 110), kNumColorBlue, kNumAlignRight);

		}
		if (iPlayerRole == RoleID_LinYueRu)

		{

			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j].m[iPlayerRole].wLevel, 3,
				PAL_XY(92, 10), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 1].m[iPlayerRole].wLevel, 3,
				PAL_XY(202, 10), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 2].m[iPlayerRole].wLevel, 3,
				PAL_XY(302, 10), kNumColorBlue, kNumAlignRight);

			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 3].m[iPlayerRole].wLevel, 3,
				PAL_XY(92, 30), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 4].m[iPlayerRole].wLevel, 3,
				PAL_XY(202, 30), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 5].m[iPlayerRole].wLevel, 3,
				PAL_XY(302, 30), kNumColorBlue, kNumAlignRight);

			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 6].m[iPlayerRole].wLevel, 3,
				PAL_XY(92, 50), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 7].m[iPlayerRole].wLevel, 3,
				PAL_XY(202, 50), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 8].m[iPlayerRole].wLevel, 3,
				PAL_XY(302, 50), kNumColorBlue, kNumAlignRight);

			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 9].m[iPlayerRole].wLevel, 3,
				PAL_XY(92, 70), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 10].m[iPlayerRole].wLevel, 3,
				PAL_XY(202, 70), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 11].m[iPlayerRole].wLevel, 3,
				PAL_XY(302, 70), kNumColorBlue, kNumAlignRight);

			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 12].m[iPlayerRole].wLevel, 3,
				PAL_XY(92, 90), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 13].m[iPlayerRole].wLevel, 3,
				PAL_XY(202, 90), kNumColorBlue, kNumAlignRight);
		}


		if (iPlayerRole == RoleID_ANu)

		{
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j].m[iPlayerRole].wLevel, 3,
				PAL_XY(92, 10), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 1].m[iPlayerRole].wLevel, 3,
				PAL_XY(202, 10), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 2].m[iPlayerRole].wLevel, 3,
				PAL_XY(302, 10), kNumColorBlue, kNumAlignRight);

			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 3].m[iPlayerRole].wLevel, 3,
				PAL_XY(92, 30), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 4].m[iPlayerRole].wLevel, 3,
				PAL_XY(202, 30), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 5].m[iPlayerRole].wLevel, 3,
				PAL_XY(302, 30), kNumColorBlue, kNumAlignRight);

			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 6].m[iPlayerRole].wLevel, 3,
				PAL_XY(92, 50), kNumColorBlue, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[j + 7].m[iPlayerRole].wLevel, 3,
				PAL_XY(202, 50), kNumColorBlue, kNumAlignRight);
		}
		//
		// Draw the image of player role
		//


		//


		//
		// Update the screen
		//
		VIDEO_UpdateScreen(NULL);

		//
		// Wait for input
		//
		PAL_ClearKeyState();

		while (TRUE)
		{
			UTIL_Delay(1);

			if (g_InputState.dwKeyPress & kKeyMenu)
			{
				iCurrent = -1;
				break;
			}
			else if (g_InputState.dwKeyPress & (kKeyLeft | kKeyUp))
			{
				iCurrent--;
				break;
			}
			else if (g_InputState.dwKeyPress & (kKeyRight | kKeyDown | kKeySearch))
			{
				iCurrent++;
				break;
			}
		}
	}

	//
	// __DEBUG__设置术谱页面关闭
	//
	g_Battle.UI.MenuState = kBattleMenuMain;
}

VOID
PAL_BuyLingHu(
	WORD           wStoreNum
)
/*++
Purpose:
Show the buy item menu.
Parameters:
[IN]  wStoreNum - number of the store to buy items from.
Return value:
None.
--*/
{
	MENUITEM        rgMenuItem[MAX_STORE_ITEM];
	int             i, y;
	WORD            w;
	SDL_Rect        rect = { 125, 8, 190, 190 };

	//
	// create the menu items
	//
	y = 22;

	for (i = 0; i < MAX_STORE_ITEM; i++)
	{
		if (gpGlobals->g.lprgStore[wStoreNum].rgwItems[i] == 0)
		{
			break;
		}

		rgMenuItem[i].wValue = gpGlobals->g.lprgStore[wStoreNum].rgwItems[i];
		rgMenuItem[i].wNumWord = gpGlobals->g.lprgStore[wStoreNum].rgwItems[i];
		rgMenuItem[i].fEnabled = TRUE;
		rgMenuItem[i].pos = PAL_XY(150, y);

		y += 18;
	}

	//
	// Draw the box
	//
	PAL_CreateBox(PAL_XY(125, 8), 8, 8, 1, FALSE);

	//
	// Draw the number of prices
	//
	for (y = 0; y < i; y++)
	{
		w = gpGlobals->g.rgObject[rgMenuItem[y].wValue].item.wPrice;
		PAL_DrawNumber(w, 6, PAL_XY(235, 25 + y * 18), kNumColorCyan, kNumAlignRight);
	}

	VIDEO_UpdateScreen(&rect);

	w = 0;
	__buymenu_firsttime_render = TRUE;

	while (TRUE)
	{
		w = PAL_ReadMenu(PAL_BuyMenu_OnItemChange, rgMenuItem, i, w, MENUITEM_COLOR);

		if (w == MENUITEM_VALUE_CANCELLED)
		{
			break;
		}

		if (gpGlobals->g.rgObject[w].item.wPrice <= gpGlobals->wCollectValue)
		{
			if (PAL_ConfirmMenu())
			{
				//
				// Player bought an item
				//
				gpGlobals->wCollectValue -= gpGlobals->g.rgObject[w].item.wPrice;
				PAL_AddItemToInventory(w, 1);

			}
		}

		//
		// Place the cursor to the current item on next loop
		//
		for (y = 0; y < i; y++)
		{
			if (w == rgMenuItem[y].wValue)
			{
				w = y;
				break;
			}
		}
	}

	//
	// __DEBUG__关闭该菜单状态
	//
	g_Battle.UI.MenuState = kBattleMenuMain;
}

WORD
PAL_New_GameDifficultyMenu(
	VOID
)
{
	LPBOX           lpGameDifficultyMenuBox[MAX_DIFFICULTY_MAX];
	int             i;
	INT             iWidth = 8 * 2 + 16 * 5;
	INT             iMarginLeft = (320 - iWidth) / 2;
	INT             iMarginTop = 50;
	INT             iPaddingTop = 9;
	INT             iPaddingLeft = 31;
	INT             iRowHeight = 40;
	WORD            wItemSelected;

	MENUITEM        rgMenuItem[MAX_DIFFICULTY_MAX];

	const SDL_Rect  rect = { iMarginLeft, iMarginTop, iMarginLeft + iWidth + 6, iMarginTop + iRowHeight * (MAX_DIFFICULTY_MAX - 1) + iPaddingTop + 6 };

	for (i = 0; i < MAX_DIFFICULTY_MAX; i++)
	{
		// Fix render problem with shadow
		lpGameDifficultyMenuBox[i] = PAL_CreateSingleLineBox(PAL_XY(iMarginLeft, iMarginTop + iRowHeight * i), 5, TRUE);

		rgMenuItem[i].wValue = i;
		rgMenuItem[i].fEnabled = TRUE;
		rgMenuItem[i].wNumWord = LABEL_DIFFICULTY_ARDER + i;

		rgMenuItem[i].pos = PAL_XY(iMarginLeft + iPaddingLeft, iMarginTop + iRowHeight * i + iPaddingTop);
	}

	//
	// Activate the menu
	//
	wItemSelected = PAL_ReadMenu(NULL, rgMenuItem, MAX_DIFFICULTY_MAX, 0, MENUITEM_COLOR);

	//
	// Delete the boxes
	// 清除该框
	for (i = 0; i < MAX_DIFFICULTY_MAX; i++)
	{
		PAL_DeleteBox(lpGameDifficultyMenuBox[i]);
	}

	VIDEO_UpdateScreen(&rect);
	//VIDEO_UpdateScreen(NULL);

	return wItemSelected;
}

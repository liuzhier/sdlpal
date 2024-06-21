/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2022, SDLPAL development team.
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
#include "fight.h"

//多人战斗：选择三角标
extern WORD g_rgPlayerPos[5][5][2];

static int g_iCurMiscMenuItem = 0;
static int g_iCurSubMenuItem = 0;

VOID
PAL_PlayerInfoBox(
   PAL_POS         pos,
   WORD            wPlayerRole,
   INT             iTimeMeter,
   BYTE            bTimeMeterColor,
   BOOL            fUpdate
)
/*++
  Purpose:

    Show the player info box.

  Parameters:

    [IN]  pos - the top-left corner position of the box.

    [IN]  wPlayerRole - the player role ID to be shown.

    [IN]  iTimeMeter - the value of time meter. 0 = empty, 100 = full.

    [IN]  bTimeMeterColor - the color of time meter.

    [IN]  fUpdate - whether to update the screen area or not.

  Return value:

    None.

--*/
{
   SDL_Rect        rect;
   BYTE            bPoisonColor;
   int             i, iPartyIndex;
   WORD            wMaxLevel, w;

   const BYTE      rgStatusPos[kStatusAll][2] =
   {
      {35, 19},  // confused
      {44, 12},  // slow
      {54, 1},   // sleep
      {55, 20},  // silence
      {0, 0},    // puppet
      {0, 0},    // bravery
      {0, 0},    // protect
      {0, 0},    // haste
      {0, 0},    // dualattack
   };

   const WORD      rgwStatusWord[kStatusAll] =
   {
      0x1D,  // confused
      0x1B,  // slow
      0x1C,  // sleep
      0x1A,  // silence
      0x00,  // puppet
      0x00,  // bravery
      0x00,  // protect
      0x00,  // haste
      0x00,  // dualattack
   };

   const BYTE      rgbStatusColor[kStatusAll] =
   {
      0x5F,  // confused
      0xBF,  // slow
      0x0E,  // sleep
      0x3C,  // silence
      0x00,  // puppet
      0x00,  // bravery
      0x00,  // protect
      0x00,  // haste
      0x00,  // dualattack
   };

   //
   // Draw the box
   //
   PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_PLAYERINFOBOX),
      gpScreen, pos);

   //
   // Draw the player face
   //
   wMaxLevel = 0;
   bPoisonColor = 0xFF;

   for (iPartyIndex = 0; iPartyIndex <= gpGlobals->wMaxPartyMemberIndex; iPartyIndex++)
   {
      if (gpGlobals->rgParty[iPartyIndex].wPlayerRole == wPlayerRole)
      {
         break;
      }
   }

   if (iPartyIndex <= gpGlobals->wMaxPartyMemberIndex)
   {
      for (i = 0; i < MAX_POISONS; i++)
      {
         w = gpGlobals->rgPoisonStatus[i][iPartyIndex].wPoisonID;

         if (w != 0 &&
            gpGlobals->g.rgObject[w].poison.wPoisonLevel <= 3)
         {
            if (gpGlobals->g.rgObject[w].poison.wPoisonLevel >= wMaxLevel)
            {
               wMaxLevel = gpGlobals->g.rgObject[w].poison.wPoisonLevel;
               bPoisonColor = (BYTE)(gpGlobals->g.rgObject[w].poison.wColor);
            }
         }
      }
   }

   if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0)
   {
      //
      // Always use the black/white color for dead players
      // and do not use the time meter
      //
      bPoisonColor = 0;
      iTimeMeter = 0;
   }

   if (bPoisonColor == 0xFF)
   {
      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_PLAYERFACE_FIRST + wPlayerRole),
         gpScreen, PAL_XY(PAL_X(pos) - 2, PAL_Y(pos) - 4));
   }
   else
   {
      PAL_RLEBlitMonoColor(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_PLAYERFACE_FIRST + wPlayerRole),
         gpScreen, PAL_XY(PAL_X(pos) - 2, PAL_Y(pos) - 4), bPoisonColor, 0);
   }

#ifndef PAL_CLASSIC
   //
   // Draw a border for the Time Meter
   //
   rect.x = PAL_X(pos) + 31;
   rect.y = PAL_Y(pos) + 4;
   rect.w = 1;
   rect.h = 6;
   SDL_FillRect(gpScreen, &rect, 0xBD);

   rect.x += 39;
   SDL_FillRect(gpScreen, &rect, 0xBD);

   rect.x = PAL_X(pos) + 32;
   rect.y = PAL_Y(pos) + 3;
   rect.w = 38;
   rect.h = 1;
   SDL_FillRect(gpScreen, &rect, 0xBD);

   rect.y += 7;
   SDL_FillRect(gpScreen, &rect, 0xBD);

   //
   // Draw the Time meter bar
   //
   if (iTimeMeter >= 100)
   {
      rect.x = PAL_X(pos) + 33;
      rect.y = PAL_Y(pos) + 6;
      rect.w = 36;
      rect.h = 2;
      SDL_FillRect(gpScreen, &rect, 0x2C);
   }
   else if (iTimeMeter > 0)
   {
      rect.x = PAL_X(pos) + 33;
      rect.y = PAL_Y(pos) + 5;
      rect.w = iTimeMeter * 36 / 100;
      rect.h = 4;
      SDL_FillRect(gpScreen, &rect, bTimeMeterColor);
   }
#endif

   //
   // Draw the HP and MP value
   //
#ifdef PAL_CLASSIC
   PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen,
      PAL_XY(PAL_X(pos) + 49, PAL_Y(pos) + 6));
   PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole], 4,
      PAL_XY(PAL_X(pos) + 47, PAL_Y(pos) + 8), kNumColorYellow, kNumAlignRight);
   
   if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] < gpGlobals->fdunjia * 100 && PAL_IsPlayerPoisonedByKind(wPlayerRole, 0x023A) && gpGlobals->fdunjia * 100 < gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole])
         {
   PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole], 4,
      PAL_XY(PAL_X(pos) + 26, PAL_Y(pos) + 5), RandomLong(0, 2), kNumAlignRight);
		 }
		 else
		 {
   PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole], 4,
      PAL_XY(PAL_X(pos) + 26, PAL_Y(pos) + 5), kNumColorYellow, kNumAlignRight);
		 }

   PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen,
      PAL_XY(PAL_X(pos) + 49, PAL_Y(pos) + 22));
   PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole], 4,
      PAL_XY(PAL_X(pos) + 47, PAL_Y(pos) + 24), kNumColorCyan, kNumAlignRight);
	  
	if (gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] < gpGlobals->fdunjia * 100 && PAL_IsPlayerPoisonedByKind(wPlayerRole, 0x023A) && gpGlobals->fdunjia * 100 < gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole])
         {
   PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole], 4,
      PAL_XY(PAL_X(pos) + 26, PAL_Y(pos) + 21), RandomLong(0, 2), kNumAlignRight);
		 }
		 else
		 {
			 PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole], 4,
      PAL_XY(PAL_X(pos) + 26, PAL_Y(pos) + 21), kNumColorCyan, kNumAlignRight);
		 }
	  
	  
	  
#else
   PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen,
      PAL_XY(PAL_X(pos) + 49, PAL_Y(pos) + 14));
   PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole], 4,
      PAL_XY(PAL_X(pos) + 47, PAL_Y(pos) + 16), kNumColorYellow, kNumAlignRight);
   PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole], 4,
      PAL_XY(PAL_X(pos) + 26, PAL_Y(pos) + 13), kNumColorYellow, kNumAlignRight);

   PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen,
      PAL_XY(PAL_X(pos) + 49, PAL_Y(pos) + 24));
   PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole], 4,
      PAL_XY(PAL_X(pos) + 47, PAL_Y(pos) + 26), kNumColorCyan, kNumAlignRight);
   PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole], 4,
      PAL_XY(PAL_X(pos) + 26, PAL_Y(pos) + 23), kNumColorCyan, kNumAlignRight);
#endif

   //
   // Draw Statuses
   //
   if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] > 0)
   {
      for (i = 0; i < kStatusAll; i++)
      {
         if (gpGlobals->rgPlayerStatus[wPlayerRole][i] > 0 &&
            rgwStatusWord[i] != 0)
         {
            PAL_DrawText(PAL_GetWord(rgwStatusWord[i]),
               PAL_XY(PAL_X(pos) + rgStatusPos[i][0], PAL_Y(pos) + rgStatusPos[i][1]),
               rgbStatusColor[i], TRUE, FALSE, FALSE);
         }
      }
   }

   //
   // Update the screen area if needed
   //
   if (fUpdate)
   {
      rect.x = PAL_X(pos) - 2;
      rect.y = PAL_Y(pos) - 4;
      rect.w = 77;
      rect.h = 39;

      VIDEO_UpdateScreen(&rect);
   }
   
   if (gpGlobals->g.PlayerRoles.rgwHP[0] == 0)
      {
      gpGlobals->fdunjia = 0;
	  gpGlobals->ftiangang = 0;
	  PAL_CurePoisonByKind(wPlayerRole, 0x023A);
	  }
   if (gpGlobals->rgPlayerStatus[0][kStatusdunjia] == 0)
      {
	  gpGlobals->fdunjia = 0;
	  gpGlobals->ftiangang = 0;
	  PAL_CurePoisonByKind(wPlayerRole, 0x023A);
	  }
   if (gpGlobals->rgPlayerStatus[0][kStatusdunjia1] == 0 && gpGlobals->fdunjia1 == TRUE && gpGlobals->g.PlayerRoles.rgwHP[0] == 0)
      {
	  gpGlobals->g.PlayerRoles.rgwMaxHP[0] -= 1000;
	  gpGlobals->g.PlayerRoles.rgwMaxMP[0] -= 1000;
	  gpGlobals->g.PlayerRoles.rgwMP[0] = gpGlobals->g.PlayerRoles.rgwMaxMP[0];
	  gpGlobals->fdunjia1 = FALSE;
	  }
	else
	  {
	if (gpGlobals->rgPlayerStatus[0][kStatusdunjia1] == 0 && gpGlobals->fdunjia1 == TRUE)
      {
	  gpGlobals->g.PlayerRoles.rgwMaxHP[0] -= 1000;
	  gpGlobals->g.PlayerRoles.rgwMaxMP[0] -= 1000;
	  gpGlobals->g.PlayerRoles.rgwHP[0] = gpGlobals->g.PlayerRoles.rgwMaxHP[0];
	  gpGlobals->g.PlayerRoles.rgwMP[0] = gpGlobals->g.PlayerRoles.rgwMaxMP[0];
	  gpGlobals->fdunjia1 = FALSE;
	  }
	  }
		  
	  if (gpGlobals->fdunjia1 == TRUE || PAL_IsPlayerPoisonedByKind(0, 0x023A))
           {
           PAL_CurePoisonByKind(0, 0x0238);
		   gpGlobals->rgPlayerStatus[0][kStatusfujia] = 0;
           }
	  
   
   if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0)
      {
      gpGlobals->rgPlayerStatus[wPlayerRole][kStatusfujia] = 0;
	  gpGlobals->rgPlayerStatus[wPlayerRole][kStatusfujia1] = 0;
	  gpGlobals->rgPlayerStatus[wPlayerRole][kStatusdunjia] = 0;
	  gpGlobals->rgPlayerStatus[wPlayerRole][kStatusdunjia1] = 0;
      }
   
   if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusfujia1] == 0 && gpGlobals->rgPlayerStatus[wPlayerRole][kStatusdunjia] == 0)
      {
      PAL_RemoveEquipmentEffect(wPlayerRole, kBodyPartfujia);
      }
	  

	  
		  
}

static BOOL
PAL_BattleUIIsActionValid(
   BATTLEUIACTION         ActionType
)
/*++
  Purpose:

    Check if the specified action is valid.

  Parameters:

    [IN]  ActionType - the type of the action.

  Return value:

    TRUE if the action is valid, FALSE if not.

--*/
{
   WORD     wPlayerRole, w;
   int      i;

   wPlayerRole = gpGlobals->rgParty[g_Battle.UI.wCurPlayerIndex].wPlayerRole;

   switch (ActionType)
   {
   case kBattleUIActionAttack:
   case kBattleUIActionMisc:
      break;

   case kBattleUIActionMagic:
      if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSilence] != 0)
      {
         return FALSE;
      }
      break;

   case kBattleUIActionCoopMagic:
      if (gpGlobals->wMaxPartyMemberIndex == 0)
      {
         return FALSE;
      }
#ifndef PAL_CLASSIC
      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         w = gpGlobals->rgParty[i].wPlayerRole;

         if (gpGlobals->g.PlayerRoles.rgwHP[w] < gpGlobals->g.PlayerRoles.rgwMaxHP[w] / 5 ||
            gpGlobals->rgPlayerStatus[w][kStatusSleep] != 0 ||
            gpGlobals->rgPlayerStatus[w][kStatusConfused] != 0 ||
            gpGlobals->rgPlayerStatus[w][kStatusSilence] != 0 ||
            g_Battle.rgPlayer[i].flTimeMeter < 100 ||
            g_Battle.rgPlayer[i].state == kFighterAct)
         {
            return FALSE;
         }
      }
#else
      {
         int healthyNumber = 0;
         for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
            if (PAL_IsPlayerHealthy(gpGlobals->rgParty[i].wPlayerRole))
               healthyNumber++;
         return PAL_IsPlayerHealthy(wPlayerRole) && healthyNumber > 1;
      }
#endif
      break;
   }

   return TRUE;
}

static VOID
PAL_BattleUIDrawMiscMenu(
   WORD       wCurrentItem,
   BOOL       fConfirmed
)
/*++
  Purpose:

    Draw the misc menu.

  Parameters:

    [IN]  wCurrentItem - the current selected menu item.

    [IN]  fConfirmed - TRUE if confirmed, FALSE if not.

  Return value:

    None.

--*/
{
   int           i;
   BYTE          bColor;
   
#ifdef PAL_CLASSIC
   MENUITEM rgMenuItem[] = {
      // value   label                     enabled   position
      {  0,      BATTLEUI_LABEL_AUTO,      TRUE,     PAL_XY(16, 32)  },
      {  1,      BATTLEUI_LABEL_INVENTORY, TRUE,     PAL_XY(16, 50)  },
      {  2,      BATTLEUI_LABEL_DEFEND,    TRUE,     PAL_XY(16, 68)  },
      {  3,      BATTLEUI_LABEL_FLEE,      TRUE,     PAL_XY(16, 86)  },
      {  4,      BATTLEUI_LABEL_STATUS,    TRUE,     PAL_XY(16, 104) },
	  {  5,      29997,                    TRUE,     PAL_XY(16, 122) },//功能
   };
#else
   MENUITEM rgMenuItem[] = {
      // value   label                   enabled   position
      {  0,      BATTLEUI_LABEL_ITEM,    TRUE,     PAL_XY(16, 32)  },
      {  1,      BATTLEUI_LABEL_DEFEND,  TRUE,     PAL_XY(16, 50)  },
      {  2,      BATTLEUI_LABEL_AUTO,    TRUE,     PAL_XY(16, 68)  },
      {  3,      BATTLEUI_LABEL_FLEE,    TRUE,     PAL_XY(16, 86)  },
      {  4,      BATTLEUI_LABEL_STATUS,  TRUE,     PAL_XY(16, 104) }
   };
#endif

   //
   // Draw the box
   //
   PAL_CreateBox(PAL_XY(2, 20), 5, PAL_MenuTextMaxWidth(rgMenuItem, sizeof(rgMenuItem)/sizeof(MENUITEM)) - 1, 0, FALSE);

   //
   // Draw the menu items
   //
   for (i = 0; i < 6; i++)
   {
      bColor = MENUITEM_COLOR;

      if (i == wCurrentItem)
      {
         if (fConfirmed)
         {
            bColor = MENUITEM_COLOR_CONFIRMED;
         }
         else
         {
            bColor = MENUITEM_COLOR_SELECTED;
         }
      }

      PAL_DrawText(PAL_GetWord(rgMenuItem[i].wNumWord), rgMenuItem[i].pos, bColor, TRUE, FALSE, FALSE);
   }
}

static WORD
PAL_BattleUIMiscMenuUpdate(
   VOID
)
/*++
  Purpose:

    Update the misc menu.

  Parameters:

    None.

  Return value:

    The selected item number. 0 if cancelled, 0xFFFF if not confirmed.

--*/
{
   //
   // Draw the menu
   //
   PAL_BattleUIDrawMiscMenu(g_iCurMiscMenuItem, FALSE);

   //
   // Process inputs
   //
   if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
   {
      g_iCurMiscMenuItem--;
      if (g_iCurMiscMenuItem < 0)
      {
         g_iCurMiscMenuItem = 5;
      }
   }
   else if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
   {
      g_iCurMiscMenuItem++;
      if (g_iCurMiscMenuItem > 5)
      {
         g_iCurMiscMenuItem = 0;
      }
   }
   else if (g_InputState.dwKeyPress & kKeySearch)
   {
      return g_iCurMiscMenuItem + 1;
   }
   else if (g_InputState.dwKeyPress & kKeyMenu)
   {
      return 0;
   }

   return 0xFFFF;
}

static WORD
PAL_BattleUIMiscItemSubMenuUpdate(
   VOID
)
/*++
  Purpose:

    Update the item sub menu of the misc menu.

  Parameters:

    None.

  Return value:

    The selected item number. 0 if cancelled, 0xFFFF if not confirmed.

--*/
{
   int             i;
   BYTE            bColor;


   //战斗换装
   MENUITEM rgMenuItem[] = {
      // value   label                      enabled   position
      {  0,      BATTLEUI_LABEL_USEITEM,    TRUE,     PAL_XY(44, 62)  },
      {  1,      BATTLEUI_LABEL_THROWITEM,  TRUE,     PAL_XY(44, 80)  },
	  {  2,      INVMENU_LABEL_EQUIP,       TRUE,     PAL_XY(44, 98)  },
   };

   //
   // Draw the menu
   //
#ifdef PAL_CLASSIC
   PAL_BattleUIDrawMiscMenu(1, TRUE);
#else
   PAL_BattleUIDrawMiscMenu(0, TRUE);
#endif
   PAL_CreateBox(PAL_XY(30, 50), 2, PAL_MenuTextMaxWidth(rgMenuItem, 2) - 1, 0, FALSE);

   //
   // Draw the menu items
   //
   for (i = 0; i < 3; i++)
   {
      bColor = MENUITEM_COLOR;

      if (i == g_iCurSubMenuItem)
      {
         bColor = MENUITEM_COLOR_SELECTED;
      }

      PAL_DrawText(PAL_GetWord(rgMenuItem[i].wNumWord), rgMenuItem[i].pos, bColor, TRUE, FALSE, FALSE);
   }

   //
   // Process inputs
   //
   if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
   {
        g_iCurSubMenuItem--;
		if (g_iCurSubMenuItem < 0)
		{
		g_iCurSubMenuItem = 2;
		}
   }
   else if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
   {
        g_iCurSubMenuItem++;
		if (g_iCurSubMenuItem > 2)
		{
		g_iCurSubMenuItem = 0;
		}
   }
   else if (g_InputState.dwKeyPress & kKeySearch)
   {
      return g_iCurSubMenuItem + 1;
   }
   else if (g_InputState.dwKeyPress & kKeyMenu)
   {
        g_Battle.UI.MenuState = kBattleMenuMisc;
		g_iCurMiscMenuItem = 1;
   }

   return 0xFFFF;
}

static WORD
PAL_BattleUIMiscgongnengSubMenuUpdate(
VOID
)
/*++
  战斗功能菜单
  --*/
{
	int             i;
	BYTE            bColor;

	MENUITEM rgMenuItem[] = {
		// value   label                      enabled   position
			{0, 29991, TRUE, PAL_XY(55, 42)},
			{1, 29992, TRUE, PAL_XY(55, 60)},
			{2, 29993, TRUE, PAL_XY(55, 78)},
			{3, 29994, TRUE, PAL_XY(55, 96)},
			{4, 29995, TRUE, PAL_XY(55, 114)},
			{5, 29996, TRUE, PAL_XY(55, 132)},
	};

	//
	// Draw the menu
	//
#ifdef PAL_CLASSIC
	PAL_BattleUIDrawMiscMenu(5, TRUE);
#else
	PAL_BattleUIDrawMiscMenu(0, TRUE);
#endif
	PAL_CreateBox(PAL_XY(45, 32), 5, 4, 0, FALSE);

	//
	// Draw the menu items
	//
	for (i = 0; i < 6; i++)
	{
		bColor = MENUITEM_COLOR;

		if (i == g_iCurSubMenuItem)
		{
			bColor = MENUITEM_COLOR_SELECTED;
		}

		PAL_DrawText(PAL_GetWord(rgMenuItem[i].wNumWord), rgMenuItem[i].pos, bColor,
			TRUE, FALSE, FALSE);
	}

	//
	// Process inputs
	//
	if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
	{
		g_iCurSubMenuItem--;
		if (g_iCurSubMenuItem < 0)
		{
		g_iCurSubMenuItem = 5;
		}
	}
	else if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
	{
		g_iCurSubMenuItem++;
		if (g_iCurSubMenuItem > 5)
		{
		g_iCurSubMenuItem = 0;
		}
	}
	else if (g_InputState.dwKeyPress & kKeySearch)
	{
		return g_iCurSubMenuItem + 1;
	}
	else if (g_InputState.dwKeyPress & kKeyMenu)
	{
		g_Battle.UI.MenuState = kBattleMenuMisc;
		g_iCurMiscMenuItem = 5;
	}

	return 0xFFFF;
}

VOID
PAL_BattleUIShowText(
   LPCWSTR       lpszText,
   WORD          wDuration
)
/*++
  Purpose:

    Show a text message in the battle.

  Parameters:

    [IN]  lpszText - the text message to be shown.

    [IN]  wDuration - the duration of the message, in milliseconds.

  Return value:

    None.

--*/
{
   if (!SDL_TICKS_PASSED(SDL_GetTicks(), g_Battle.UI.dwMsgShowTime))
   {
      wcscpy(g_Battle.UI.szNextMsg, lpszText);
      g_Battle.UI.wNextMsgDuration = wDuration;
   }
   else
   {
      wcscpy(g_Battle.UI.szMsg, lpszText);
      g_Battle.UI.dwMsgShowTime = SDL_GetTicks() + wDuration;
   }
}

VOID
PAL_BattleUIPlayerReady(
   WORD          wPlayerIndex
)
/*++
  Purpose:

    Start the action selection menu of the specified player.

  Parameters:

    [IN]  wPlayerIndex - the player index.

  Return value:

    None.

--*/
{
#ifndef PAL_CLASSIC
   WORD w = gpGlobals->rgParty[wPlayerIndex].wPlayerRole;
#endif

   g_Battle.UI.wCurPlayerIndex = wPlayerIndex;
   g_Battle.UI.state = kBattleUISelectMove;
   g_Battle.UI.wSelectedAction = 0;
   g_Battle.UI.MenuState = kBattleMenuMain;

#ifndef PAL_CLASSIC
   //
   // Play a sound which indicates the player is ready
   //
   if (gpGlobals->rgPlayerStatus[w][kStatusPuppet] == 0 &&
      gpGlobals->rgPlayerStatus[w][kStatusSleep] == 0 &&
      gpGlobals->rgPlayerStatus[w][kStatusConfused] == 0 &&
      !g_Battle.UI.fAutoAttack && !gpGlobals->fAutoBattle)
   {
      AUDIO_PlaySound(78);
   }
#endif
}

static VOID
PAL_BattleUIUseItem(
   VOID
)
/*++
  Purpose:

    Use an item in the battle UI.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   WORD       wSelectedItem;

   wSelectedItem = PAL_ItemSelectMenuUpdate();

   if (wSelectedItem != 0xFFFF)
   {
      if (wSelectedItem != 0)
      {
         g_Battle.UI.wActionType = kBattleActionUseItem;
         g_Battle.UI.wObjectID = wSelectedItem;

         if (gpGlobals->g.rgObject[wSelectedItem].item.wFlags & kItemFlagApplyToAll)
         {
            g_Battle.UI.state = kBattleUISelectTargetPlayerAll;
         }
         else
         {
#ifdef PAL_CLASSIC
            g_Battle.UI.iSelectedIndex = 0;
#else
            g_Battle.UI.iSelectedIndex = g_Battle.UI.wCurPlayerIndex;
#endif
            g_Battle.UI.state = kBattleUISelectTargetPlayer;
         }
      }
      else
      {
         g_Battle.UI.MenuState = kBattleMenuMain;
      }
   }
}

static VOID
PAL_BattleUIThrowItem(
   VOID
)
/*++
  Purpose:

    Throw an item in the battle UI.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   WORD wSelectedItem = PAL_ItemSelectMenuUpdate();

   if (wSelectedItem != 0xFFFF)
   {
      if (wSelectedItem != 0)
      {
         g_Battle.UI.wActionType = kBattleActionThrowItem;
         g_Battle.UI.wObjectID = wSelectedItem;

         if (gpGlobals->g.rgObject[wSelectedItem].item.wFlags & kItemFlagApplyToAll)
         {
            g_Battle.UI.state = kBattleUISelectTargetEnemyAll;
         }
         else
         {
            if(g_Battle.UI.iPrevEnemyTarget != -1)
               g_Battle.UI.iSelectedIndex = g_Battle.UI.iPrevEnemyTarget;
            g_Battle.UI.state = kBattleUISelectTargetEnemy;
            g_Battle.UI.iSelectedIndex = 0;
         }
      }
      else
      {
         g_Battle.UI.MenuState = kBattleMenuMain;
      }
   }
}

static WORD
PAL_BattleUIPickAutoMagic(
   WORD          wPlayerRole,
   WORD          wRandomRange
)
/*++
  Purpose:

    Pick a magic for the specified player for automatic usage.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

    [IN]  wRandomRange - the range of the magic power.

  Return value:

    The object ID of the selected magic. 0 for physical attack.

--*/
{
   WORD             wMagic = 0, w, wMagicNum;
   int              i, iMaxPower = 0, iPower;

   if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSilence] != 0)
   {
      return 0;
   }

   for (i = 0; i < MAX_PLAYER_MAGICS; i++)
   {
      w = gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole];
      if (w == 0)
      {
         continue;
      }

      wMagicNum = gpGlobals->g.rgObject[w].magic.wMagicNumber;

      //
      // skip if the magic is an ultimate move or not enough MP
      //
      if (gpGlobals->g.lprgMagic[wMagicNum].wCostMP == 1 ||
         gpGlobals->g.lprgMagic[wMagicNum].wCostMP > gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] ||
         (SHORT)(gpGlobals->g.lprgMagic[wMagicNum].wBaseDamage) <= 0)
      {
         continue;
      }

      iPower = (SHORT)(gpGlobals->g.lprgMagic[wMagicNum].wBaseDamage) +
         RandomLong(0, wRandomRange);

      if (iPower > iMaxPower)
      {
         iMaxPower = iPower;
         wMagic = w;
      }
   }

   return wMagic;
}

VOID
PAL_BattleUIUpdate(
   VOID
)
/*++
  Purpose:

    Update the status of battle UI.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int              i, j, x, y;
   WORD             wPlayerRole, w;
   static int       s_iFrame = 0;

   s_iFrame++;

   //数据开关
    if (g_InputState.dwKeyPress & kKeyshuju)
   {
	   gpGlobals->fShowDataInBattle++;
	   goto end;
   }
    if (gpGlobals->fShowDataInBattle == 0)
   {
	   PAL_New_BattleUIShowData();
   }
	if (gpGlobals->fShowDataInBattle > 1)
   {
	   PAL_New_BattleUIShowData();
	   gpGlobals->fShowDataInBattle = 0;
   }
	
	//怪物数据开关
   if (g_InputState.dwKeyPress & kKeyqingbao)
   {
      PAL_New_EnemyStatus();
      goto end;
   }

   //人物等级法术开关
   if (g_InputState.dwKeyPress & kKeyfashu)
   {
      PAL_New_PlayerLevelmagic();
      goto end;
   }
   
   //战斗速度开关
    if (g_InputState.dwKeyPress & kKeysudu)
   {
	   PAL_DrawText(L"是否开启战斗加速？", PAL_XY(50,40), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
	   PAL_DrawText(L"你是否还在用变速齿轮？", PAL_XY(50,60), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
	   PAL_DrawText(L"不不不，该功能给你想要的！", PAL_XY(50,80), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
	   gpGlobals->fsudu = PAL_SwitchMenu1(gpGlobals->fsudu);
	   goto end;
   }
   //最快速度开关
   	if (gpGlobals->fsudu == FALSE && g_Battle.BattleResult == kBattleResultOnGoing)
	{
		if (g_InputState.dwKeyPress & kKeyLeft && g_InputState.dwKeyPress & kKeyDown && g_InputState.dwKeyPress & kKeyRight)
		{
	   PAL_DrawText(L"是否开启最快战斗速度？", PAL_XY(50,40), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
	   PAL_DrawText(L"嗯！速度是快了不少！！", PAL_XY(50,60), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
	   PAL_DrawText(L"就是....有点眼花......", PAL_XY(50,80), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
	   gpGlobals->fsudu2 = PAL_SwitchMenu(gpGlobals->fsudu2);
	   goto end;
		}
	}
	
   //帮助开关
   if (g_InputState.dwKeyPress & kKeybangzhu)
   {
      PAL_New_bangzhujiemian();
	  goto end;
   }
   
   //战斗换装
   if (g_InputState.dwKeyPress & kKeyhuanzhuang)
   {
	   //物品栏自动排序
	   PAL_New_SortInventory();
	   
      PAL_GameEquipItem();
	  goto end;
   }
   
   if (g_Battle.UI.fAutoAttack && !gpGlobals->fAutoBattle)
   {
      //
      // Draw the "auto attack" message if in the autoattack mode.
      //
      if (g_InputState.dwKeyPress & kKeyMenu)
      {
         g_Battle.UI.fAutoAttack = FALSE;
      }
      else
      {
         LPCWSTR itemText = PAL_GetWord(BATTLEUI_LABEL_AUTO);
         PAL_DrawText(itemText, PAL_XY(312-PAL_TextWidth(itemText), 10),
            MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
      }
   }

   if (gpGlobals->fAutoBattle)
   {
      PAL_BattlePlayerCheckReady();

      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         if (g_Battle.rgPlayer[i].state == kFighterCom)
         {
            PAL_BattleUIPlayerReady(i);
            break;
         }
      }

      if (g_Battle.UI.state != kBattleUIWait)
      {
         w = PAL_BattleUIPickAutoMagic(gpGlobals->rgParty[g_Battle.UI.wCurPlayerIndex].wPlayerRole, 9999);

         if (w == 0)
         {
            g_Battle.UI.wActionType = kBattleActionAttack;
            g_Battle.UI.iSelectedIndex = PAL_BattleSelectAutoTarget();
         }
         else
         {
            g_Battle.UI.wActionType = kBattleActionMagic;
            g_Battle.UI.wObjectID = w;

            if (gpGlobals->g.rgObject[w].magic.wFlags & kMagicFlagApplyToAll)
            {
               g_Battle.UI.iSelectedIndex = -1;
            }
            else
            {
               g_Battle.UI.iSelectedIndex = PAL_BattleSelectAutoTarget();
            }
         }

         PAL_BattleCommitAction(FALSE);
      }

      goto end;
   }

   if (g_InputState.dwKeyPress & kKeyAuto)
   {
      g_Battle.UI.fAutoAttack = !g_Battle.UI.fAutoAttack;
      g_Battle.UI.MenuState = kBattleMenuMain;
   }

#ifdef PAL_CLASSIC
   if (g_Battle.Phase == kBattlePhasePerformAction)
   {
      goto end;
   }

   if (!g_Battle.UI.fAutoAttack)
#endif
   {
      //
      // Draw the player info boxes.
      //
      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;
         w = (WORD)(g_Battle.rgPlayer[i].flTimeMeter);

         j = TIMEMETER_COLOR_DEFAULT;

#ifndef PAL_CLASSIC
         if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusHaste] > 0)
         {
            j = TIMEMETER_COLOR_HASTE;
         }
         else if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSlow] > 0)
         {
            j = TIMEMETER_COLOR_SLOW;
         }
#endif

         if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] != 0 ||
            gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] != 0 ||
            gpGlobals->rgPlayerStatus[wPlayerRole][kStatusPuppet] != 0)
         {
            w = 0;
         }
			
			// 更改角色信息栏的位置。在屏幕下方
			//多人战斗：人物状态栏
			if (gpGlobals->wMaxPartyMemberIndex >= 3)
			{
				if (i > 3)
				{
					PAL_PlayerInfoBox(PAL_XY(91 + 77 * 2, 165 - (i - 3) * 38), wPlayerRole, w, j, FALSE);
					continue;
				}

				PAL_PlayerInfoBox(PAL_XY(14 + 77 * i, 165), wPlayerRole, w, j, FALSE);
				continue;
			}

			PAL_PlayerInfoBox(PAL_XY(91 + 77 * i, 165), wPlayerRole,
				w, j, FALSE);
      }
   }



   if (g_InputState.dwKeyPress & kKeyStatus)
   {
      PAL_PlayerStatus();
      goto end;
   }

   if (g_Battle.UI.state != kBattleUIWait)
   {
      wPlayerRole = gpGlobals->rgParty[g_Battle.UI.wCurPlayerIndex].wPlayerRole;

      if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0 &&
         gpGlobals->rgPlayerStatus[wPlayerRole][kStatusPuppet])
      {
         g_Battle.UI.wActionType = kBattleActionAttack;

         if (PAL_PlayerCanAttackAll(gpGlobals->rgParty[g_Battle.UI.wCurPlayerIndex].wPlayerRole))
         {
            g_Battle.UI.iSelectedIndex = -1;
         }
         else
         {
            g_Battle.UI.iSelectedIndex = PAL_BattleSelectAutoTarget();
         }

         PAL_BattleCommitAction(FALSE);
         goto end; // don't go further
      }

      //
      // Cancel any actions if player is dead or sleeping.
      //
      if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0 ||
         gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] != 0 ||
         gpGlobals->rgPlayerStatus[wPlayerRole][kStatusParalyzed] != 0)
      {
         g_Battle.UI.wActionType = kBattleActionPass;
         PAL_BattleCommitAction(FALSE);
         goto end; // don't go further
      }

      if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] != 0)
      {
         g_Battle.UI.wActionType = kBattleActionAttackMate;
         PAL_BattleCommitAction(FALSE);
         goto end; // don't go further
      }

      if (g_Battle.UI.fAutoAttack)
      {
         g_Battle.UI.wActionType = kBattleActionAttack;

         if (PAL_PlayerCanAttackAll(gpGlobals->rgParty[g_Battle.UI.wCurPlayerIndex].wPlayerRole))
         {
            g_Battle.UI.iSelectedIndex = -1;
         }
         else
         {
            g_Battle.UI.iSelectedIndex = PAL_BattleSelectAutoTarget();
         }

         PAL_BattleCommitAction(FALSE);
         goto end; // don't go further
      }

      //
      // Draw the arrow on the player's head.
      //
      i = SPRITENUM_BATTLE_ARROW_CURRENTPLAYER_RED;
      if (s_iFrame & 1)
      {
         i = SPRITENUM_BATTLE_ARROW_CURRENTPLAYER;
      }

      x = g_rgPlayerPos[gpGlobals->wMaxPartyMemberIndex][g_Battle.UI.wCurPlayerIndex][0] - 8;
      y = g_rgPlayerPos[gpGlobals->wMaxPartyMemberIndex][g_Battle.UI.wCurPlayerIndex][1] - 74;

      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, i), gpScreen, PAL_XY(x, y));
   }

   switch (g_Battle.UI.state)
   {
   case kBattleUIWait:
      if (!g_Battle.fEnemyCleared)
      {
         PAL_BattlePlayerCheckReady();

         for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
         {
            if (g_Battle.rgPlayer[i].state == kFighterCom)
            {
               PAL_BattleUIPlayerReady(i);
               break;
            }
         }
      }
      break;

   case kBattleUISelectMove:
      //
      // Draw the icons
      //
      {
         struct {
            int               iSpriteNum;
            PAL_POS           pos;
            BATTLEUIACTION    action;
         } rgItems[] =
         {
            {SPRITENUM_BATTLEICON_ATTACK,    PAL_XY(27, 140), kBattleUIActionAttack},
            {SPRITENUM_BATTLEICON_MAGIC,     PAL_XY(0, 155),  kBattleUIActionMagic},
            {SPRITENUM_BATTLEICON_COOPMAGIC, PAL_XY(54, 155), kBattleUIActionCoopMagic},
            {SPRITENUM_BATTLEICON_MISCMENU,  PAL_XY(27, 170), kBattleUIActionMisc}
         };

			//多于4个人的时候调整战斗菜单盘的位置
			//多人战斗：战斗轮盘
			if (gpGlobals->wMaxPartyMemberIndex >= 3)
			{
				for (i = 0; i < 4; i++)
				{
					rgItems[i].pos -= (37 << 16);
				}
			}

         if (g_Battle.UI.MenuState == kBattleMenuMain)
         {
            if (g_InputState.dir == kDirNorth)
            {
               g_Battle.UI.wSelectedAction = 0;
            }
            else if (g_InputState.dir == kDirSouth)
            {
               g_Battle.UI.wSelectedAction = 3;
            }
            else if (g_InputState.dir == kDirWest)
            {
               if (PAL_BattleUIIsActionValid(kBattleUIActionMagic))
               {
                  g_Battle.UI.wSelectedAction = 1;
               }
            }
            else if (g_InputState.dir == kDirEast)
            {
               if (PAL_BattleUIIsActionValid(kBattleUIActionCoopMagic))
               {
                  g_Battle.UI.wSelectedAction = 2;
               }
            }
         }

         if (!PAL_BattleUIIsActionValid(rgItems[g_Battle.UI.wSelectedAction].action))
         {
            g_Battle.UI.wSelectedAction = 0;
         }

         for (i = 0; i < 4; i++)
         {
            if (g_Battle.UI.wSelectedAction == i)
            {
               PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, rgItems[i].iSpriteNum),
                  gpScreen, rgItems[i].pos);
            }
            else if (PAL_BattleUIIsActionValid(rgItems[i].action))
            {
               PAL_RLEBlitMonoColor(PAL_SpriteGetFrame(gpSpriteUI, rgItems[i].iSpriteNum),
                  gpScreen, rgItems[i].pos, 0, -4);
            }
            else
            {
               PAL_RLEBlitMonoColor(PAL_SpriteGetFrame(gpSpriteUI, rgItems[i].iSpriteNum),
                  gpScreen, rgItems[i].pos, 0x10, -4);
            }
         }

         switch (g_Battle.UI.MenuState)
         {
         case kBattleMenuMain:
            if (g_InputState.dwKeyPress & kKeySearch)
            {
               switch (g_Battle.UI.wSelectedAction)
               {
               case 0:
                  //
                  // Attack
                  //
                  g_Battle.UI.wActionType = kBattleActionAttack;
                  if (PAL_PlayerCanAttackAll(gpGlobals->rgParty[g_Battle.UI.wCurPlayerIndex].wPlayerRole))
                  {
                     g_Battle.UI.state = kBattleUISelectTargetEnemyAll;
                  }
                  else
                  {
                     if(g_Battle.UI.iPrevEnemyTarget != -1)
                        g_Battle.UI.iSelectedIndex = g_Battle.UI.iPrevEnemyTarget;
                     g_Battle.UI.state = kBattleUISelectTargetEnemy;
                     g_Battle.UI.iSelectedIndex = 0;
                  }
                  break;

               case 1:
                  //
                  // Magic
                  //
                  g_Battle.UI.MenuState = kBattleMenuMagicSelect;
                  PAL_MagicSelectionMenuInit(wPlayerRole, TRUE, 0);
                  break;

               case 2:
                  //
                  // Cooperative magic
                  //
                  w = gpGlobals->rgParty[g_Battle.UI.wCurPlayerIndex].wPlayerRole;
                  w = PAL_GetPlayerCooperativeMagic(w);

                  g_Battle.UI.wActionType = kBattleActionCoopMagic;
                  g_Battle.UI.wObjectID = w;

                  if (gpGlobals->g.rgObject[w].magic.wFlags & kMagicFlagUsableToEnemy)
                  {
                     if (gpGlobals->g.rgObject[w].magic.wFlags & kMagicFlagApplyToAll)
                     {
                        g_Battle.UI.state = kBattleUISelectTargetEnemyAll;
                     }
                     else
                     {
                        if(g_Battle.UI.iPrevEnemyTarget != -1)
                           g_Battle.UI.iSelectedIndex = g_Battle.UI.iPrevEnemyTarget;
                        g_Battle.UI.state = kBattleUISelectTargetEnemy;
                        g_Battle.UI.iSelectedIndex = 0;
                     }
                  }
                  else
                  {
                     if (gpGlobals->g.rgObject[w].magic.wFlags & kMagicFlagApplyToAll)
                     {
                        g_Battle.UI.state = kBattleUISelectTargetPlayerAll;
                     }
                     else
                     {
#ifdef PAL_CLASSIC
                        g_Battle.UI.iSelectedIndex = 0;
#else
                        g_Battle.UI.iSelectedIndex = g_Battle.UI.wCurPlayerIndex;
#endif
                        g_Battle.UI.state = kBattleUISelectTargetPlayer;
                     }
                  }
                  break;

               case 3:
                  //
                  // Misc menu
                  //
                  g_Battle.UI.MenuState = kBattleMenuMisc;
//                  g_iCurMiscMenuItem = 0; //disabled due to not same as both original version
                  break;
               }
            }
            else if (g_InputState.dwKeyPress & kKeyDefend)
            {
               g_Battle.UI.wActionType = kBattleActionDefend;
               PAL_BattleCommitAction(FALSE);
            }
            else if (g_InputState.dwKeyPress & kKeyForce)
            {
               w = PAL_BattleUIPickAutoMagic(gpGlobals->rgParty[g_Battle.UI.wCurPlayerIndex].wPlayerRole, 60);

               if (w == 0)
               {
                  g_Battle.UI.wActionType = kBattleActionAttack;

                  if (PAL_PlayerCanAttackAll(gpGlobals->rgParty[g_Battle.UI.wCurPlayerIndex].wPlayerRole))
                  {
                     g_Battle.UI.iSelectedIndex = -1;
                  }
                  else
                  {
                     g_Battle.UI.iSelectedIndex = PAL_BattleSelectAutoTarget();
                  }
               }
               else
               {
                  g_Battle.UI.wActionType = kBattleActionMagic;
                  g_Battle.UI.wObjectID = w;

                  if (gpGlobals->g.rgObject[w].magic.wFlags & kMagicFlagApplyToAll)
                  {
                     g_Battle.UI.iSelectedIndex = -1;
                  }
                  else
                  {
                     g_Battle.UI.iSelectedIndex = PAL_BattleSelectAutoTarget();
                  }
               }

               PAL_BattleCommitAction(FALSE);
            }
            else if (g_InputState.dwKeyPress & kKeyFlee)
            {
               g_Battle.UI.wActionType = kBattleActionFlee;
               PAL_BattleCommitAction(FALSE);
            }
            else if (g_InputState.dwKeyPress & kKeyUseItem)
            {
				//物品栏自动排序
		       PAL_New_SortInventory();
			   
			   //if (wPlayerRole == 0)
			   //{
               g_Battle.UI.MenuState = kBattleMenuUseItemSelect;
               PAL_ItemSelectMenuInit(kItemFlagUsable);
			   //}
            }
            else if (g_InputState.dwKeyPress & kKeyThrowItem)
            {
				//物品栏自动排序
		       PAL_New_SortInventory();
			   
			   //if (wPlayerRole == 2)
			   //{
               g_Battle.UI.MenuState = kBattleMenuThrowItemSelect;
               PAL_ItemSelectMenuInit(kItemFlagThrowable);
			   //}
            }
            else if (g_InputState.dwKeyPress & kKeyRepeat)
            {
               PAL_BattleCommitAction(TRUE);
            }
#ifdef PAL_CLASSIC
            else if (g_InputState.dwKeyPress & kKeyMenu)
            {
               g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].state = kFighterWait;
               g_Battle.UI.state = kBattleUIWait;

               if (g_Battle.UI.wCurPlayerIndex > 0)
               {
                  //
                  // Revert to the previous player
                  //
                  do
                  {
                     g_Battle.rgPlayer[--g_Battle.UI.wCurPlayerIndex].state = kFighterWait;

                     if (g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.ActionType == kBattleActionThrowItem)
                     {
                        for (i = 0; i < MAX_INVENTORY; i++)
                        {
                           if (gpGlobals->rgInventory[i].wItem ==
                              g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.wActionID)
                           {
                              gpGlobals->rgInventory[i].nAmountInUse--;
                              break;
                           }
                        }
                     }
                     else if (g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.ActionType == kBattleActionUseItem)
                     {
                        if (gpGlobals->g.rgObject[g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.wActionID].item.wFlags & kItemFlagConsuming)
                        {
                           for (i = 0; i < MAX_INVENTORY; i++)
                           {
                              if (gpGlobals->rgInventory[i].wItem ==
                                 g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.wActionID)
                              {
                                 gpGlobals->rgInventory[i].nAmountInUse--;
                                 break;
                              }
                           }
                        }
                     }
                  } while (g_Battle.UI.wCurPlayerIndex > 0 &&
                     (gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[g_Battle.UI.wCurPlayerIndex].wPlayerRole] == 0 ||
                      gpGlobals->rgPlayerStatus[gpGlobals->rgParty[g_Battle.UI.wCurPlayerIndex].wPlayerRole][kStatusConfused] > 0 ||
                      gpGlobals->rgPlayerStatus[gpGlobals->rgParty[g_Battle.UI.wCurPlayerIndex].wPlayerRole][kStatusSleep] > 0 ||
                      gpGlobals->rgPlayerStatus[gpGlobals->rgParty[g_Battle.UI.wCurPlayerIndex].wPlayerRole][kStatusParalyzed] > 0));
               }
            }
#else
            else if (g_InputState.dwKeyPress & kKeyMenu)
            {
               float flMin = -1;
               j = -1;

               for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
               {
                  if (g_Battle.rgPlayer[i].flTimeMeter >= 100)
                  {
                     g_Battle.rgPlayer[i].flTimeMeter += 100; // HACKHACK: Prevent the time meter from going below 100

                     if ((g_Battle.rgPlayer[i].flTimeMeter < flMin || flMin < 0) &&
                        i != (int)g_Battle.UI.wCurPlayerIndex &&
                        g_Battle.rgPlayer[i].state == kFighterWait)
                     {
                        flMin = g_Battle.rgPlayer[i].flTimeMeter;
                        j = i;
                     }
                  }
               }

               if (j != -1)
               {
                  g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].flTimeMeter = flMin - 99;
                  g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].state = kFighterWait;
                  g_Battle.UI.state = kBattleUIWait;
               }
            }
#endif
            break;

         case kBattleMenuMagicSelect:
            w = PAL_MagicSelectionMenuUpdate();

            if (w != 0xFFFF)
            {
               g_Battle.UI.MenuState = kBattleMenuMain;

               if (w != 0)
               {
                  g_Battle.UI.wActionType = kBattleActionMagic;
                  g_Battle.UI.wObjectID = w;

                  if (gpGlobals->g.rgObject[w].magic.wFlags & kMagicFlagUsableToEnemy)
                  {
                     if (gpGlobals->g.rgObject[w].magic.wFlags & kMagicFlagApplyToAll)
                     {
                        g_Battle.UI.state = kBattleUISelectTargetEnemyAll;
                     }
                     else
                     {
                        if(g_Battle.UI.iPrevEnemyTarget != -1)
                           g_Battle.UI.iSelectedIndex = g_Battle.UI.iPrevEnemyTarget;
                        g_Battle.UI.state = kBattleUISelectTargetEnemy;
                        g_Battle.UI.iSelectedIndex = 0;
                     }
                  }
                  else
                  {
                     if (gpGlobals->g.rgObject[w].magic.wFlags & kMagicFlagApplyToAll)
                     {
                        g_Battle.UI.state = kBattleUISelectTargetPlayerAll;
                     }
                     else
                     {
#ifdef PAL_CLASSIC
                        g_Battle.UI.iSelectedIndex = 0;
#else
                        g_Battle.UI.iSelectedIndex = g_Battle.UI.wCurPlayerIndex;
#endif
                        g_Battle.UI.state = kBattleUISelectTargetPlayer;
                     }
                  }
               }
            }
            break;

         case kBattleMenuUseItemSelect:
            PAL_BattleUIUseItem();
            break;

         case kBattleMenuThrowItemSelect:
            PAL_BattleUIThrowItem();
            break;

         case kBattleMenuMisc:
            w = PAL_BattleUIMiscMenuUpdate();

            if (w != 0xFFFF)
            {
               g_Battle.UI.MenuState = kBattleMenuMain;

               switch (w)
               {
#ifdef PAL_CLASSIC
               case 2: // item
#else
               case 1: // item
#endif
                  g_Battle.UI.MenuState = kBattleMenuMiscItemSubMenu;
//                  g_iCurSubMenuItem = 0; //disabled due to not same as both original version
                  break;

#ifdef PAL_CLASSIC
               case 3: // defend
#else
               case 2: // defend
#endif
                  g_Battle.UI.wActionType = kBattleActionDefend;
                  PAL_BattleCommitAction(FALSE);
                  break;

#ifdef PAL_CLASSIC
               case 1: // auto
#else
               case 3: // auto
#endif
                  g_Battle.UI.fAutoAttack = TRUE;
                  break;

               case 4: // flee
                  g_Battle.UI.wActionType = kBattleActionFlee;
                  PAL_BattleCommitAction(FALSE);
                  break;

               case 5: // status
                  PAL_PlayerStatus();
                  break;
				  
			   case 6: // 功能
				  g_Battle.UI.MenuState = kBattleMenugongneng;
				  g_iCurSubMenuItem = 0;
				  break;	
               }
            }
            break;

         case kBattleMenuMiscItemSubMenu:
            w = PAL_BattleUIMiscItemSubMenuUpdate();
			
			//物品栏自动排序
		    PAL_New_SortInventory();

            if (w != 0xFFFF)
            {
               g_Battle.UI.MenuState = kBattleMenuMain;

               switch (w)
               {
               case 1: // use
			      //if (wPlayerRole == 0)
			      //{
                  g_Battle.UI.MenuState = kBattleMenuUseItemSelect;
                  PAL_ItemSelectMenuInit(kItemFlagUsable);
				  //}
                  break;

               case 2: // throw
			      //if (wPlayerRole == 2)
			      //{
                  g_Battle.UI.MenuState = kBattleMenuThrowItemSelect;
                  PAL_ItemSelectMenuInit(kItemFlagThrowable);
				  //}
                  break;
				  
			   case 3:  // 战斗换装
				  PAL_GameEquipItem();
				  break;
               }
            }
            break;
			
			
			case kBattleMenugongneng:
				{
			   w = PAL_BattleUIMiscgongnengSubMenuUpdate();

			  if (w != 0xFFFF)
				{
				g_Battle.UI.MenuState = kBattleMenuMain;

				switch (w)
				{
				case 1: //数据
				    gpGlobals->fShowDataInBattle++;
				    break;

				case 2: //怪物属性
					PAL_New_EnemyStatus();
					break;
								
				case 3: //等级法术
					PAL_New_PlayerLevelmagic();
					break;
								
				case 4: //战斗加速
				    PAL_DrawText(L"已默认 开启 状态", PAL_XY(100,2), 0x1A, TRUE, TRUE, FALSE);
					PAL_DrawText(L"是否开启战斗加速？", PAL_XY(80,20), MENUITEM_COLOR_CONFIRMED, TRUE, TRUE, FALSE);
	                gpGlobals->fsudu = PAL_SwitchMenu1(gpGlobals->fsudu);
					break;
								
							
				case 5: //战斗最快速度
				if (gpGlobals->fsudu == FALSE && g_Battle.BattleResult == kBattleResultOnGoing)
	                {
					PAL_DrawText(L"是否开启战斗最快加速？", PAL_XY(80,20), MENUITEM_COLOR_CONFIRMED, TRUE, TRUE, FALSE);
	                gpGlobals->fsudu2 = PAL_SwitchMenu(gpGlobals->fsudu2);
					}
				else
					{
					PAL_DrawText(L"首先需要打开战斗加速~", PAL_XY(80,20), 
					0x1A, TRUE, TRUE, FALSE);
					PAL_WaitForKey(0);
					g_Battle.UI.MenuState = kBattleMenugongneng;
					g_iCurSubMenuItem = 4;
					}
					break;
								
				case 6: //特别说明
					PAL_New_bangzhujiemian();
					break;
						}
					}
					break;
				}
         }
      }
      break;

   case kBattleUISelectTargetEnemy:
      x = -1;
      y = 0;

      for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
      {
         if (g_Battle.rgEnemy[i].wObjectID != 0)
         {
            x = i;
            y++;
         }
      }

      if (x == -1)
      {
         g_Battle.UI.state = kBattleUISelectMove;
         break;
      }

      if (g_Battle.UI.wActionType == kBattleActionCoopMagic)
      {
         if (!PAL_BattleUIIsActionValid(kBattleUIActionCoopMagic))
         {
            g_Battle.UI.state = kBattleUISelectMove;
            break;
         }
      }

#ifdef PAL_CLASSIC
      //
      // Don't bother selecting when only 1 enemy left
      //
      if (y == 1)
      {
//         g_Battle.UI.iPrevEnemyTarget = x;  //disabled due to not same as both original version
         if( g_Battle.UI.iSelectedIndex == -1 )
            g_Battle.UI.iSelectedIndex = x;
         else
             for (g_Battle.UI.iSelectedIndex = 0; g_Battle.UI.iSelectedIndex < MAX_ENEMIES_IN_TEAM; g_Battle.UI.iSelectedIndex++)
                 if (g_Battle.rgEnemy[g_Battle.UI.iSelectedIndex].wObjectID != 0)
                     break;
         PAL_BattleCommitAction(FALSE);
         break;
      }
#endif
      if (g_Battle.UI.iSelectedIndex > x)
      {
         g_Battle.UI.iSelectedIndex = x;
      }
      else if (g_Battle.UI.iSelectedIndex < 0)
      {
         g_Battle.UI.iSelectedIndex = 0;
      }

      for (i = 0; i <= x; i++)
      {
         if (g_Battle.rgEnemy[g_Battle.UI.iSelectedIndex].wObjectID != 0)
         {
            break;
         }
         g_Battle.UI.iSelectedIndex++;
         g_Battle.UI.iSelectedIndex %= x + 1;
      }

      //
      // Highlight the selected enemy
      //
      if (s_iFrame & 1)
      {
         i = g_Battle.UI.iSelectedIndex;

         x = PAL_X(g_Battle.rgEnemy[i].pos);
         y = PAL_Y(g_Battle.rgEnemy[i].pos);

         x -= PAL_RLEGetWidth(PAL_SpriteGetFrame(g_Battle.rgEnemy[i].lpSprite, g_Battle.rgEnemy[i].wCurrentFrame)) / 2;
         y -= PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle.rgEnemy[i].lpSprite, g_Battle.rgEnemy[i].wCurrentFrame));

         PAL_RLEBlitWithColorShift(PAL_SpriteGetFrame(g_Battle.rgEnemy[i].lpSprite, g_Battle.rgEnemy[i].wCurrentFrame),
            gpScreen, PAL_XY(x, y), 7);
      }

      if (g_InputState.dwKeyPress & kKeyMenu)
      {
         g_Battle.UI.state = kBattleUISelectMove;
      }
      else if (g_InputState.dwKeyPress & kKeySearch)
      {
//         g_Battle.UI.iPrevEnemyTarget = g_Battle.UI.iSelectedIndex; //disabled due to not same as both original version
         PAL_BattleCommitAction(FALSE);
      }
      else if (g_InputState.dwKeyPress & (kKeyLeft | kKeyDown))
      {
         g_Battle.UI.iSelectedIndex--;
         if( g_Battle.UI.iSelectedIndex < 0 ) g_Battle.UI.iSelectedIndex = MAX_ENEMIES_IN_TEAM-1;
         while (g_Battle.UI.iSelectedIndex != 0 &&
            g_Battle.rgEnemy[g_Battle.UI.iSelectedIndex].wObjectID == 0)
         {
            g_Battle.UI.iSelectedIndex--;
            if( g_Battle.UI.iSelectedIndex < 0 ) g_Battle.UI.iSelectedIndex = MAX_ENEMIES_IN_TEAM-1;
         }
      }
      else if (g_InputState.dwKeyPress & (kKeyRight | kKeyUp))
      {
         g_Battle.UI.iSelectedIndex++;
         if( g_Battle.UI.iSelectedIndex >= MAX_ENEMIES_IN_TEAM ) g_Battle.UI.iSelectedIndex = 0;
         while (g_Battle.UI.iSelectedIndex < MAX_ENEMIES_IN_TEAM &&
            g_Battle.rgEnemy[g_Battle.UI.iSelectedIndex].wObjectID == 0)
         {
            g_Battle.UI.iSelectedIndex++;
            if( g_Battle.UI.iSelectedIndex >= MAX_ENEMIES_IN_TEAM ) g_Battle.UI.iSelectedIndex = 0;
         }
      }
      break;

   case kBattleUISelectTargetPlayer:
#ifdef PAL_CLASSIC
      //
      // Don't bother selecting when only 1 player is in the party
      //
      if (gpGlobals->wMaxPartyMemberIndex == 0)
      {
         g_Battle.UI.iSelectedIndex = 0;
         PAL_BattleCommitAction(FALSE);
      }
#endif

      j = SPRITENUM_BATTLE_ARROW_SELECTEDPLAYER;
      if (s_iFrame & 1)
      {
         j = SPRITENUM_BATTLE_ARROW_SELECTEDPLAYER_RED;
      }

      //
      // Draw arrows on the selected player
      //
      x = g_rgPlayerPos[gpGlobals->wMaxPartyMemberIndex][g_Battle.UI.iSelectedIndex][0] - 8;
      y = g_rgPlayerPos[gpGlobals->wMaxPartyMemberIndex][g_Battle.UI.iSelectedIndex][1] - 67;

      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, j), gpScreen, PAL_XY(x, y));

      if (g_InputState.dwKeyPress & kKeyMenu)
      {
         g_Battle.UI.state = kBattleUISelectMove;
      }
      else if (g_InputState.dwKeyPress & kKeySearch)
      {
         PAL_BattleCommitAction(FALSE);
      }
      else if (g_InputState.dwKeyPress & (kKeyLeft | kKeyDown))
      {
         if (g_Battle.UI.iSelectedIndex != 0)
         {
            g_Battle.UI.iSelectedIndex--;
         }
         else
         {
            g_Battle.UI.iSelectedIndex = gpGlobals->wMaxPartyMemberIndex;
         }
      }
      else if (g_InputState.dwKeyPress & (kKeyRight | kKeyUp))
      {
         if (g_Battle.UI.iSelectedIndex < gpGlobals->wMaxPartyMemberIndex)
         {
            g_Battle.UI.iSelectedIndex++;
         }
         else
         {
            g_Battle.UI.iSelectedIndex = 0;
         }
      }

      break;

   case kBattleUISelectTargetEnemyAll:
#ifdef PAL_CLASSIC
      //
      // Don't bother selecting
      //
      g_Battle.UI.iSelectedIndex = (WORD)-1;
      PAL_BattleCommitAction(FALSE);
#else
      if (g_Battle.UI.wActionType == kBattleActionCoopMagic)
      {
         if (!PAL_BattleUIIsActionValid(kBattleActionCoopMagic))
         {
            g_Battle.UI.state = kBattleUISelectMove;
            break;
         }
      }

      if (s_iFrame & 1)
      {
         //
         // Highlight all enemies
         //
         for (i = g_Battle.wMaxEnemyIndex; i >= 0; i--)
         {
            if (g_Battle.rgEnemy[i].wObjectID == 0)
            {
               continue;
            }

            x = PAL_X(g_Battle.rgEnemy[i].pos);
            y = PAL_Y(g_Battle.rgEnemy[i].pos);

            x -= PAL_RLEGetWidth(PAL_SpriteGetFrame(g_Battle.rgEnemy[i].lpSprite, g_Battle.rgEnemy[i].wCurrentFrame)) / 2;
            y -= PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle.rgEnemy[i].lpSprite, g_Battle.rgEnemy[i].wCurrentFrame));

            PAL_RLEBlitWithColorShift(PAL_SpriteGetFrame(g_Battle.rgEnemy[i].lpSprite, g_Battle.rgEnemy[i].wCurrentFrame),
               gpScreen, PAL_XY(x, y), 7);
         }
      }
      if (g_InputState.dwKeyPress & kKeyMenu)
      {
         g_Battle.UI.state = kBattleUISelectMove;
      }
      else if (g_InputState.dwKeyPress & kKeySearch)
      {
         g_Battle.UI.iSelectedIndex = -1;
         PAL_BattleCommitAction(FALSE);
      }
#endif
      break;

   case kBattleUISelectTargetPlayerAll:
#ifdef PAL_CLASSIC
      //
      // Don't bother selecting
      //
      g_Battle.UI.iSelectedIndex = (WORD)-1;
      PAL_BattleCommitAction(FALSE);
#else
      j = SPRITENUM_BATTLE_ARROW_SELECTEDPLAYER;
      if (s_iFrame & 1)
      {
         j = SPRITENUM_BATTLE_ARROW_SELECTEDPLAYER_RED;
      }
      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         if (g_Battle.UI.wActionType == kBattleActionMagic)
         {
            w = gpGlobals->g.rgObject[g_Battle.UI.wObjectID].magic.wMagicNumber;

            if (gpGlobals->g.lprgMagic[w].wType == kMagicTypeTrance)
            {
               if (i != g_Battle.UI.wCurPlayerIndex)
                  continue;
            }
         }

         //
         // Draw arrows on all players, despite of dead or not
         //
         x = g_rgPlayerPos[gpGlobals->wMaxPartyMemberIndex][i][0] - 8;
         y = g_rgPlayerPos[gpGlobals->wMaxPartyMemberIndex][i][1] - 67;

         PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, j), gpScreen, PAL_XY(x, y));
      }

      if (g_InputState.dwKeyPress & kKeyMenu)
      {
         g_Battle.UI.state = kBattleUISelectMove;
      }
      else if (g_InputState.dwKeyPress & kKeySearch)
      {
         g_Battle.UI.iSelectedIndex = -1;
         PAL_BattleCommitAction(FALSE);
      }
#endif
      break;
   }

end:
   //
   // Show the text message if there is one.
   //
#ifndef PAL_CLASSIC
   if (!SDL_TICKS_PASSED(SDL_GetTicks(), g_Battle.UI.dwMsgShowTime))
   {
      //
      // The text should be shown in a small window at the center of the screen
      //
      PAL_POS    pos;
	  int        i, w = wcslen(g_Battle.UI.szMsg), len = 0;

	  for (i = 0; i < w; i++)
		  len += PAL_CharWidth(g_Battle.UI.szMsg[i]) >> 3;

      //
      // Create the window box
      //
      pos = PAL_XY(160 - len * 4, 40);
      PAL_CreateSingleLineBox(pos, (len + 1) / 2, FALSE);

      //
      // Show the text on the screen
      //
      pos = PAL_XY(PAL_X(pos) + 8 + ((len & 1) << 2), PAL_Y(pos) + 10);
      PAL_DrawText(g_Battle.UI.szMsg, pos, 0, FALSE, FALSE, FALSE);
   }
   else if (g_Battle.UI.szNextMsg[0] != '\0')
   {
      wcscpy(g_Battle.UI.szMsg, g_Battle.UI.szNextMsg);
      g_Battle.UI.dwMsgShowTime = SDL_GetTicks() + g_Battle.UI.wNextMsgDuration;
      g_Battle.UI.szNextMsg[0] = '\0';
   }
#endif

   //
   // Draw the numbers
   //
   for (i = 0; i < BATTLEUI_MAX_SHOWNUM; i++)
   {
      if (g_Battle.UI.rgShowNum[i].wNum > 0)
      {
         if ((SDL_GetTicks() - g_Battle.UI.rgShowNum[i].dwTime) / BATTLE_FRAME_TIME > 10)
         {
            g_Battle.UI.rgShowNum[i].wNum = 0;
         }
         else
         {
            PAL_DrawNumber(g_Battle.UI.rgShowNum[i].wNum, 5,
               PAL_XY(PAL_X(g_Battle.UI.rgShowNum[i].pos), PAL_Y(g_Battle.UI.rgShowNum[i].pos) - (SDL_GetTicks() - g_Battle.UI.rgShowNum[i].dwTime) / BATTLE_FRAME_TIME),
               g_Battle.UI.rgShowNum[i].color, kNumAlignRight);
         }
      }
   }

   PAL_ClearKeyState();
}

VOID
PAL_BattleUIShowNum(
   WORD           wNum,
   PAL_POS        pos,
   NUMCOLOR       color
)
/*++
  Purpose:

    Show a number on battle screen (indicates HP/MP change).

  Parameters:

    [IN]  wNum - number to be shown.

    [IN]  pos - position of the number on the screen.

    [IN]  color - color of the number.

  Return value:

    None.

--*/
{
   int     i;

   for (i = 0; i < BATTLEUI_MAX_SHOWNUM; i++)
   {
      if (g_Battle.UI.rgShowNum[i].wNum == 0)
      {
         g_Battle.UI.rgShowNum[i].wNum = wNum;
         g_Battle.UI.rgShowNum[i].pos = PAL_XY(PAL_X(pos) - 15, PAL_Y(pos));
         g_Battle.UI.rgShowNum[i].color = color;
         g_Battle.UI.rgShowNum[i].dwTime = SDL_GetTicks();

         break;
      }
   }
}

VOID
PAL_New_BattleUIShowData(
VOID
)
/*++
Purpose: 在战斗中显示一些数据。
--*/
{
	int              i, j, o, x, y;
	WORD             wPlayerRole, w;

	//显示敌人血量
	for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
	{
		if (g_Battle.rgEnemy[i].wObjectID == 0)
		{
			continue;
		}
		
		x = PAL_X(g_Battle.rgEnemy[i].pos);
		y = PAL_Y(g_Battle.rgEnemy[i].pos) - 50;
		
		if (g_Battle.rgEnemy[i].dwActualHealth < g_Battle.rgEnemy[i].dwMaxHealth / 4 && g_Battle.rgEnemy[i].e.wCollectValue != 0 && PAL_GetItemAmount(0x010E) > 0)
		{
			o = RandomLong(0, 2);
			PAL_DrawText(L"收", PAL_XY(x - 30, y + 14), RandomLong(0x1A, 0x3C), TRUE, FALSE, FALSE);
		}
		else
		{
			//if (g_Battle.rgEnemy[i].dwActualHealth < g_Battle.rgEnemy[i].dwMaxHealth / 10 && g_Battle.rgEnemy[i].e.wCollectValue == 0 && PAL_GetItemAmount(0x0086) > 0)
		   //{
			//o = RandomLong(0, 2);
			//PAL_DrawText(L"化", PAL_XY(x - 30, y + 14), RandomLong(0x1A, 0x3C), TRUE, FALSE, FALSE);
		   //}
		  // else
		   //{
			o = kNumColorYellow;
		  // }
		}
		
		PAL_DrawNumber(g_Battle.rgEnemy[i].dwActualHealth, 6, PAL_XY(50 * i, 0), o, kNumAlignRight);
		
		//敌人可用灵葫值
		if (g_Battle.rgEnemy[i].e.wCollectValue > 0)
		{
		PAL_DrawNumber(g_Battle.rgEnemy[i].e.wCollectValue, 6, PAL_XY(50 * i, 10), kNumColorYellow, kNumAlignRight);
		}

        const WORD      rgwStatusWord[kStatusAll] =
	    {
		0x1D,  // confused
		0x1B,  // slow
		0x1C,  // sleep
		0x1A,  // silence
	    };
        const BYTE      rgbStatusColor[kStatusAll] =
	    {
		0x3C,  // confused
		0x1A,  // slow
		0x8D,  // sleep
		0x5F,  // silence
	    };

        
        //敌方负面状态，只有在有回合的情况下显示
        for (j = 0; j < kStatusPuppet; j++)
	    {
		  if (g_Battle.rgEnemy[i].rgwStatus[j] != 0)
		  {
		  PAL_DrawText(PAL_GetWord(rgwStatusWord[j]), PAL_XY(x - 10, y), rgbStatusColor[j], TRUE, FALSE, FALSE);
	       PAL_DrawNumber(g_Battle.rgEnemy[i].rgwStatus[j], 3, PAL_XY(x + 5, y + 5), kNumColorBlue, kNumAlignRight);
		   }
           y += 14;
	    }
	}

	//显示我方的对各属性仙术的抗性
	int startPos = 320 - 20 * (gpGlobals->wMaxPartyMemberIndex + 1);
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;
		for (j = 0; j < NUM_MAGIC_ELEMENTAL; j++)
		{
			w = PAL_GetPlayerElementalResistance(wPlayerRole, j);
			PAL_DrawNumber(w, 3, PAL_XY(startPos + 20 * i, 8 * j), kNumColorYellow, kNumAlignRight);
		}

		w = PAL_GetPlayerPoisonResistance(wPlayerRole);
		PAL_DrawNumber(w, 3, PAL_XY(startPos + 20 * i, 8 * j + 2), kNumColorYellow, kNumAlignRight);
	}

	//显示我方有益状态的剩余轮次
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;
		for (j = kStatusPuppet; j < kStatusfujia; j++)
		{
			w = gpGlobals->rgPlayerStatus[wPlayerRole][j];
			
			if (min(99, w) < 99 && min(99, w) > 0)
		    {
			PAL_DrawNumber(min(99, w), 3, PAL_XY(startPos + 20 * i,
				65 + 8 * (j - kStatusPuppet) - 12), RandomLong(0, 2), kNumAlignRight);
			}
			else
			{
			PAL_DrawNumber(min(99, w), 3, PAL_XY(startPos + 20 * i,
				65 + 8 * (j - kStatusPuppet) - 12), kNumColorYellow, kNumAlignRight);
			}
		}
		
		//圣灵祝福回合转换
		if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusfujia] > 0 && gpGlobals->fzhufu1 == TRUE)
		    {
		   gpGlobals->fzhufu = gpGlobals->rgPlayerStatus[wPlayerRole][kStatusfujia];
			}
		
	}

    //金钱
    PAL_DrawNumber(gpGlobals->dwCash, 7, PAL_XY(276, 115), kNumColorYellow, kNumAlignRight);

	//显示总的灵葫值
	w = gpGlobals->wCollectValue;
	PAL_DrawNumber(w, 5, PAL_XY(288, 100), kNumColorYellow, kNumAlignRight);
	
	

	//战场环境
	for (j = 0; j < NUM_MAGIC_ELEMENTAL; j++)
		{
		   SHORT p = gpGlobals->g.lprgBattleField[gpGlobals->wNumBattleField].rgsMagicEffect[j];
			
			if (p < 0)
			{
				o = kNumColorBlue;
			}
            else
			{
				o = kNumColorYellow;
			}
			
	PAL_DrawNumber(p, 1, PAL_XY(2, 10 * j + 70), o, kNumAlignRight);
		}
	
    //圣灵祝福显示
	if (gpGlobals->fzhufu > 0 && gpGlobals->fzhufu1 == TRUE)
	{
	PAL_DrawText(L"祝福", PAL_XY(265, 125), RandomLong(1, 255), TRUE, FALSE, FALSE);
	PAL_DrawNumber(gpGlobals->fzhufu, 1, PAL_XY(310, 131), RandomLong(0, 2), kNumAlignRight);
	}
	
	if (gpGlobals->fdunjia > 0)
	{
	PAL_DrawText(L"化劲", PAL_XY(265, 145), RandomLong(1, 255), TRUE, FALSE, FALSE);
	PAL_DrawNumber(gpGlobals->fdunjia - 1, 1, PAL_XY(310, 145), RandomLong(0, 2), kNumAlignRight);
	PAL_DrawNumber(gpGlobals->fdunjia * 100, 3, PAL_XY(300, 155), RandomLong(0, 2), kNumAlignRight);
	}
	
	if (gpGlobals->ftouqiefangtao1 == TRUE)
    {
	PAL_DrawText(L"失败！偷窃后不可逃...", PAL_XY(80, 100), RandomLong(1, 255), TRUE, FALSE, FALSE);
	}
}


VOID
PAL_New_EnemyStatus(
VOID
)
{
	PAL_LARGE BYTE		bufBackground[320 * 200];
	INT					iCurrent;
	BATTLEENEMY			be;
	INT					i, x, y, h, j;
	WORD				w;
	LPCBITMAPRLE		lBMR;
	PAL_POS				pos;

	const int        rgEquipPos[MAX_PLAYER_EQUIPMENTS][2] = {
			{190, 0}, {248, 40}, {252, 102}, {202, 134}, {142, 142}, {82, 126}
	};

	PAL_MKFDecompressChunk(bufBackground, 320 * 200, gpGlobals->wNumBattleField, gpGlobals->f.fpFBP);

	iCurrent = 0;

	while (iCurrent >= 0 && iCurrent < MAX_ENEMIES_IN_TEAM)
	{
		be = g_Battle.rgEnemy[iCurrent];
		if (be.wObjectID == 0 || be.dwActualHealth == 0)
		{
			iCurrent++;
			continue;
		}

		// Draw the background image
		PAL_FBPBlitToSurface(bufBackground, gpScreen);

		// 怪物图像
		lBMR = PAL_SpriteGetFrame(g_Battle.rgEnemy[iCurrent].lpSprite, g_Battle.rgEnemy[iCurrent].wCurrentFrame);
		pos = PAL_XY(180, 85);
		pos = PAL_XY(PAL_X(pos) - PAL_RLEGetWidth(lBMR) / 2, PAL_Y(pos) - PAL_RLEGetHeight(lBMR) / 2);
		PAL_RLEBlitToSurface(lBMR, gpScreen, pos);

		// Draw the text labels
		i = 0;
		x = 6;
		y = 6;
		h = 19;
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_HP), PAL_XY(x, y + (i++) * h), MENUITEM_COLOR, TRUE, FALSE, FALSE);//HP标签
        PAL_DrawText(L"灵抗", PAL_XY(x, y + (i++) * h), MENUITEM_COLOR, TRUE, FALSE, FALSE);//灵抗标签
		
		PAL_DrawText(L"巫抗", PAL_XY(x, y + (i++) * h), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		PAL_DrawText(L"毒抗", PAL_XY(x, y + (i++) * h), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		PAL_DrawText(L"物抗", PAL_XY(x, y + (i++) * h), MENUITEM_COLOR, TRUE, FALSE, FALSE);

		PAL_DrawText(L"灵葫值", PAL_XY(x, y + (i++) * h), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		
		PAL_DrawText(L"默认法术", PAL_XY(x, y + (i++) * h), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		
		PAL_DrawText(L"物攻附加", PAL_XY(x, y + (i++) * h), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		
		PAL_DrawText(L"偷窃", PAL_XY(x, y + (i++) * h), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		
		PAL_DrawText(L"掉落", PAL_XY(x, y + (i++) * h), MENUITEM_COLOR, TRUE, FALSE, FALSE);//掉落标签

		PAL_DrawText(PAL_GetWord(be.wObjectID), PAL_XY(130, 6), 0x4F, TRUE, FALSE, FALSE);


        i = 0;
		x = 250;
		y = 6;
		h = 19;
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_EXP), PAL_XY(x, y + (i++) * h), MENUITEM_COLOR, TRUE, FALSE, FALSE);//经验标签
		PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(x, y + (i++) * h), MENUITEM_COLOR, TRUE, FALSE, FALSE);//金钱标签
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_LEVEL), PAL_XY(x, y + (i++) * h), MENUITEM_COLOR, TRUE, FALSE, FALSE);//等级标签
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_ATTACKPOWER), PAL_XY(x, y + (i++) * h), MENUITEM_COLOR, TRUE, FALSE, FALSE);//攻击标签
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_MAGICPOWER), PAL_XY(x, y + (i++) * h), MENUITEM_COLOR, TRUE, FALSE, FALSE);//灵力标签
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_RESISTANCE), PAL_XY(x, y + (i++) * h), MENUITEM_COLOR, TRUE, FALSE, FALSE);//防御标签
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_DEXTERITY), PAL_XY(x, y + (i++) * h), MENUITEM_COLOR, TRUE, FALSE, FALSE);//身法标签
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_FLEERATE), PAL_XY(x, y + (i++) * h), MENUITEM_COLOR, TRUE, FALSE, FALSE);//吉运标签
		PAL_DrawText(L"攻击次数", PAL_XY(x - 10, y + (i++) * h), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		
		PAL_DrawText(L"NO.", PAL_XY(287, 185), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		
		
		// Draw the stats
		i = 0;
		x = 42;
		y = 11;
		h = 19;

		if (be.dwMaxHealth <= 99999)
		{
		PAL_DrawNumber(be.dwActualHealth, 5, PAL_XY(x, y + (i++) * h - 5), kNumColorYellow, kNumAlignRight);//体力
		//体力、体力最大值的分隔符“/”
		PAL_DrawText(L"----", PAL_XY(x, y + (i - 1) * h - 3), 0x1A, TRUE, FALSE, FALSE);
		PAL_DrawNumber(be.dwMaxHealth, 5, PAL_XY(x, y + (i - 1) * h + 7), kNumColorBlue, kNumAlignRight);//体力最大
		}
		
		else if (be.dwMaxHealth > 9999999)
		{
		PAL_DrawNumber(be.dwActualHealth, 10, PAL_XY(x, y + (i++) * h - 5), kNumColorYellow, kNumAlignRight);//体力
		//体力、体力最大值的分隔符“/”
		PAL_DrawText(L"--------", PAL_XY(x, y + (i - 1) * h - 3), 0x1A, TRUE, FALSE, FALSE);
		PAL_DrawNumber(be.dwMaxHealth, 10, PAL_XY(x, y + (i - 1) * h + 7), kNumColorBlue, kNumAlignRight);//体力最大
		}
		
		else if (be.dwMaxHealth > 99999 && be.dwMaxHealth <= 9999999)
		{
		PAL_DrawNumber(be.dwActualHealth, 7, PAL_XY(x, y + (i++) * h - 5), kNumColorYellow, kNumAlignRight);//体力
		//体力、体力最大值的分隔符“/”
		PAL_DrawText(L"-----", PAL_XY(x, y + (i - 1) * h - 3), 0x1A, TRUE, FALSE, FALSE);
		PAL_DrawNumber(be.dwMaxHealth, 7, PAL_XY(x, y + (i - 1) * h + 7), kNumColorBlue, kNumAlignRight);//体力最大
		}
		
		for (j = 0; j < NUM_MAGIC_ELEMENTAL; j++)
		{
			PAL_DrawNumber(PAL_New_GetEnemyElementalResistance(iCurrent,j), 13, PAL_XY(22 * (j - 1), y + h), kNumColorYellow, kNumAlignRight);//灵抗
		}
		
		i++;
		PAL_DrawNumber(PAL_New_GetEnemySorceryResistance(iCurrent), 4, PAL_XY(x, y + (i++) * h), kNumColorYellow, kNumAlignRight);//巫抗
		PAL_DrawNumber(PAL_New_GetEnemyPoisonResistance(iCurrent), 4, PAL_XY(x, y + (i++) * h), kNumColorYellow, kNumAlignRight);//毒抗
		PAL_DrawNumber(PAL_New_GetEnemyPhysicalResistance(iCurrent), 4, PAL_XY(x, y + (i++) * h), kNumColorYellow, kNumAlignRight);//物抗

		PAL_DrawNumber(be.e.wCollectValue, 5, PAL_XY(x, y + (i++) * h), kNumColorYellow, kNumAlignRight);//灵葫值
		
		WORD wMagic = be.e.wMagic != 0 ? be.e.wMagic : LABEL_NOTHING;
		PAL_DrawText(PAL_GetWord(wMagic), PAL_XY(x + 36, y + (i++) * h - 5), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);//默认法术
		
		WORD wAttackEquivItem = be.e.wAttackEquivItem != 0 ? be.e.wAttackEquivItem : LABEL_NOTHING;
		PAL_DrawText(PAL_GetWord(wAttackEquivItem), PAL_XY(x + 36, y + (i++) * h - 5), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);//物攻附加
		
		PAL_DrawNumber(be.e.nStealItem, 5, PAL_XY(x, y + (i++) * h), kNumColorYellow, kNumAlignRight);
		PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, PAL_XY(x + 30, y + (i - 1) * h));
		WORD wStealItem = be.e.wStealItem != 0 ? be.e.wStealItem : CASH_LABEL;
		PAL_DrawText(PAL_GetWord(wStealItem), PAL_XY(x + 35, y + (i - 1) * h - 5), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);//偷窃

		PAL_FindEnemyBooty(be.wScriptOnBattleEnd, iCurrent, PAL_XY(x, y + (i++) * h), PAL_XY(72, 182), PAL_XY(77, 178));//战后获得
		
		PAL_DrawNumber(iCurrent + 1, 1, PAL_XY(310, 190), kNumColorYellow, kNumAlignRight);//ID
		
		
		i = 0;
		x = 285;
		y = 11;
		h = 19;
		PAL_DrawNumber(be.e.wExp, 5, PAL_XY(x, y + (i++) * h), kNumColorYellow, kNumAlignRight);//经验
		PAL_DrawNumber(be.e.wCash, 5, PAL_XY(x, y + (i++) * h), kNumColorYellow, kNumAlignRight);//金钱
		PAL_DrawNumber(be.e.wLevel, 5, PAL_XY(x, y + (i++) * h), kNumColorYellow, kNumAlignRight);//等级
		PAL_DrawNumber(PAL_New_GetEnemyAttackStrength(iCurrent), 5, PAL_XY(x, y + (i++) * h), kNumColorYellow, kNumAlignRight);//攻击
		PAL_DrawNumber(PAL_New_GetEnemyMagicStrength(iCurrent), 5, PAL_XY(x, y + (i++) * h), kNumColorYellow, kNumAlignRight);//灵力
		PAL_DrawNumber(PAL_New_GetEnemyDefense(iCurrent), 5, PAL_XY(x, y + (i++) * h), kNumColorYellow, kNumAlignRight);//防御
		PAL_DrawNumber(PAL_New_GetEnemyDexterity(iCurrent), 5, PAL_XY(x, y + (i++) * h), kNumColorYellow, kNumAlignRight);//身法
		PAL_DrawNumber(PAL_New_GetEnemyFleeRate(iCurrent), 5, PAL_XY(x, y + (i++) * h), kNumColorYellow, kNumAlignRight);//吉运
		PAL_DrawNumber(PAL_New_GetEnemyDualMove(iCurrent), 5, PAL_XY(x, y + (i++) * h), kNumColorCyan, kNumAlignRight);//攻击次数
		

		// Draw all poisons
		//将毒的名称写在状态栏里
		y = 30;

#ifdef POISON_STATUS_EXPAND
		int wPoisonIntensity = 0;
#endif

		for (i = 0; i < MAX_POISONS; i++)
		{
			w = g_Battle.rgEnemy[iCurrent].rgPoisons[i].wPoisonID;

			if (w != 0)
			{
				PAL_DrawText(PAL_GetWord(w), PAL_XY(170,y),
					(BYTE)(gpGlobals->g.rgObject[w].poison.wColor + 10), TRUE, FALSE, FALSE);

#ifdef POISON_STATUS_EXPAND
				wPoisonIntensity = g_Battle.rgEnemy[iCurrent].rgPoisons[i].wPoisonIntensity;
				if (wPoisonIntensity != 0)
				{
					PAL_DrawNumber(wPoisonIntensity, 2, PAL_XY(245, y + 4), kNumColorYellow, kNumAlignRight);
				}
#endif
				y += 17;	
			}
		}
		
		const WORD      rgwStatusWord[kStatusAll] =
	    {
		0x1D,  // confused
		0x1B,  // slow
		0x1C,  // sleep
		0x1A,  // silence
	    };
        const BYTE      rgbStatusColor[kStatusAll] =
	    {
		0x3C,  // confused
		0x1A,  // slow
		0x8D,  // sleep
		0x5F,  // silence
	    };
        
		y = 60;
        //敌方负面状态
        for (j = 0; j < kStatusPuppet; j++)
	    {
		  if (g_Battle.rgEnemy[iCurrent].rgwStatus[j] != 0)
		  {
		  PAL_DrawText(PAL_GetWord(rgwStatusWord[j]), PAL_XY(110, y), rgbStatusColor[j], TRUE, FALSE, FALSE);
	       PAL_DrawNumber(g_Battle.rgEnemy[iCurrent].rgwStatus[j], 3, PAL_XY(125, y + 5), kNumColorBlue, kNumAlignRight);
		   }
           y += 14;
	    }
		
		if (g_Battle.rgEnemy[iCurrent].dwActualHealth < g_Battle.rgEnemy[iCurrent].dwMaxHealth / 4 && g_Battle.rgEnemy[iCurrent].e.wCollectValue != 0 && PAL_GetItemAmount(0x010E) > 0)
		{
		PAL_DrawText(L"收", PAL_XY(110, 40), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		}
		else
		{
		  if (g_Battle.rgEnemy[iCurrent].dwActualHealth < g_Battle.rgEnemy[iCurrent].dwMaxHealth / 10 && g_Battle.rgEnemy[iCurrent].e.wCollectValue == 0 && PAL_GetItemAmount(0x0086) > 0)
		   {
			PAL_DrawText(L"化", PAL_XY(110, 40), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
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
				do 
				{
					iCurrent--;
					if (iCurrent < 0)
					{
						break;
					}
					be = g_Battle.rgEnemy[iCurrent];
				} while (be.wObjectID == 0 && be.dwActualHealth == 0);
				break;
			}
			else if (g_InputState.dwKeyPress & (kKeyRight | kKeyDown | kKeySearch))
			{
				do
				{
					iCurrent++;
					if (iCurrent >= MAX_ENEMIES_IN_TEAM)
					{
						break;
					}
					be = g_Battle.rgEnemy[iCurrent];
				} while (be.wObjectID == 0 && be.dwActualHealth == 0);
				break;
			}
		}
	}
}


VOID
PAL_New_PlayerLevelmagic(
VOID
)
{
   PAL_LARGE BYTE       bufImage[16384], bufBackground[320 * 200];
   INT					iCurrent;
   int                  iPlayerRole, i, j, k;
   const int            iItemsPerLine = 32 / gConfig.dwWordLength;
   const int            iItemTextWidth = 9 * gConfig.dwWordLength + 13;
   const int            iLinesPerPage = 5 - gConfig.ScreenLayout.ExtraMagicDescLines;
   const int            iBoxYOffset = gConfig.ScreenLayout.ExtraMagicDescLines * 16;
   
   
   iCurrent = 0;
   PAL_MKFDecompressChunk(bufBackground, 320 * 200, 60, gpGlobals->f.fpFBP);
   
   while (iCurrent >= 0 && iCurrent <= MAX_PLAYABLE_PLAYER_ROLES)
   {
      if (iCurrent == 0)
	   {
   iPlayerRole = 0;
   PAL_MKFDecompressChunk(bufBackground, 320 * 200, 60, gpGlobals->f.fpFBP);
		   }
      if (iCurrent == 1)
	   {
   iPlayerRole = 1;
   PAL_MKFDecompressChunk(bufBackground, 320 * 200, 62, gpGlobals->f.fpFBP);
		   }
	   if (iCurrent == 2)
	   {
   iPlayerRole = 2;
   PAL_MKFDecompressChunk(bufBackground, 320 * 200, 64, gpGlobals->f.fpFBP);
		   }
      if (iCurrent == 3)
	   {
   iPlayerRole = 4;
   PAL_MKFDecompressChunk(bufBackground, 320 * 200, 66, gpGlobals->f.fpFBP);
		   }
	  if (iCurrent == 4)
	   {
   iPlayerRole = 3;
   PAL_MKFDecompressChunk(bufBackground, 320 * 200, 2, gpGlobals->f.fpFBP);
		   }
	  if (iCurrent == 5)
	   {
   iPlayerRole = 5;
   PAL_MKFDecompressChunk(bufBackground, 320 * 200, 2, gpGlobals->f.fpFBP);
		   }
		   
      // 背景图像
      PAL_FBPBlitToSurface(bufBackground, gpScreen);

	  
      // 人物图像
      if (PAL_MKFReadChunk(bufImage, 16384, gpGlobals->g.PlayerRoles.rgwAvatar[iPlayerRole], gpGlobals->f.fpRGM) > 0)
      {
         PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(230, 90));
      }
	
	  for (i = 0; i < gpGlobals->g.nLevelUpMagic; i++)
      {
	    for (j = 0; j < iLinesPerPage; j++)
        {
          for (k = 0; k < iItemsPerLine; k++)
          {
	  
	       //人物等级法术
           //盖罗娇没有法术空位，这里就只保留了头像。
	       if (iPlayerRole != 5)
		   {
			 if (gpGlobals->g.lprgLevelUpMagic[i].m[min(iPlayerRole, 5)].wLevel > 0)
			 { 
		     PAL_DrawNumber(gpGlobals->g.lprgLevelUpMagic[i].m[min(iPlayerRole, 5)].wLevel, 2, PAL_XY(10 + k * iItemTextWidth, 9 + j * 20 + iBoxYOffset), kNumColorYellow, kNumAlignRight);
		     PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[i].m[min(iPlayerRole, 5)].wMagic), PAL_XY(25 + k * iItemTextWidth, 2 + j * 20 + iBoxYOffset), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
             i++;
			}
		  }
	    }
	  }
	}

      // Update the screen
      VIDEO_UpdateScreen(NULL);


      // Wait for input
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

VOID
PAL_New_SortInventory(
VOID
)
{
	int         i, j;
	WORD        ItemID1, ItemID2;
	WORD		ItemNum;
	INVENTORY   TempItem;
	INVENTORY	TempInventory[MAX_INVENTORY];

	memset(TempInventory, 0, sizeof(TempInventory));

	for (i = 0, j = 0; i < MAX_INVENTORY; i++)
	{
		TempItem = gpGlobals->rgInventory[i];
		if (TempItem.wItem != 0 && TempItem.nAmount != 0)
		{
			TempInventory[j] = TempItem;
			j++;
		}
	}
	ItemNum = j;

	for (i = 0; i < ItemNum; i++)
	{
		for (j = 0; j < ItemNum - i - 1; j++)
		{
			ItemID1 = TempInventory[j].wItem;
			ItemID2 = TempInventory[j + 1].wItem;


			if (ItemID1 > ItemID2)
			{
				TempItem = TempInventory[j];
				TempInventory[j] = TempInventory[j + 1];
				TempInventory[j + 1] = TempItem;
			}
		}
	}

	for (i = 0; i < MAX_INVENTORY; i++)
	{
		gpGlobals->rgInventory[i] = TempInventory[i];
	}

	return;
}
	
VOID
PAL_New_bangzhujiemian(
VOID
)
{
   PAL_LARGE BYTE       bufImage[16384];
   INT					iCurrent, i;

   PAL_FadeOut(1);
   iCurrent = 0;
   i = 0;
   
   while (iCurrent >= 0 && iCurrent <= 5)
   {
	   
	   if (g_Battle.BattleResult != kBattleResultOnGoing)
         {
	   AUDIO_PlayMusic(16, TRUE, 1);
	     }
	   
	  //背景图像
	  PAL_CreateBoxWithShadow(PAL_XY(2 , 0), 9, 17, 1, FALSE, 0);

	  //第一页
	  if (iCurrent == 0)
         {
	  PAL_DrawText(L"<<帮助说明>>", PAL_XY(110, 5), MENUITEM_COLOR, TRUE, FALSE, FALSE);
	  PAL_DrawText(L"免费游戏，免费程序", PAL_XY(90, 30), MENUITEM_COLOR_SELECTED, TRUE, FALSE, FALSE);
	  PAL_DrawText(L"严禁售卖，谨防受骗", PAL_XY(90, 50), MENUITEM_COLOR_SELECTED, TRUE, FALSE, FALSE);
	  
	  //人物头像
	  if (PAL_MKFReadChunk(bufImage, 16384, gpGlobals->g.PlayerRoles.rgwAvatar[1], gpGlobals->f.fpRGM) > 0)
      {
         PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(30, 90));
      }
	  if (PAL_MKFReadChunk(bufImage, 16384, gpGlobals->g.PlayerRoles.rgwAvatar[0], gpGlobals->f.fpRGM) > 0)
      {
         PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(90, 85));
      }
	  if (PAL_MKFReadChunk(bufImage, 16384, gpGlobals->g.PlayerRoles.rgwAvatar[2], gpGlobals->f.fpRGM) > 0)
      {
         PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(150, 80));
      }
	  if (PAL_MKFReadChunk(bufImage, 16384, gpGlobals->g.PlayerRoles.rgwAvatar[4], gpGlobals->f.fpRGM) > 0)
      {
         PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(210, 80));
      }

      //下一页
      PAL_DrawText(L"下一页", PAL_XY(250, 180), 0x8D, TRUE, FALSE, FALSE);
	     }
		 
		 //第二页
		 if (iCurrent == 1)
         { 
		 PAL_DrawText(L"<<功能快捷按键介绍>>", PAL_XY(85, 5), MENUITEM_COLOR, TRUE, FALSE, FALSE);


		 PAL_DrawText(L"非战斗中", PAL_XY(80, 35), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"战斗中", PAL_XY(200, 35), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 
		 PAL_DrawText(L"Q键：", PAL_XY(15, 60), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"W键：", PAL_XY(15, 80), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"E键：", PAL_XY(15, 100), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"R键：", PAL_XY(15, 120), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"A键：", PAL_XY(15, 140), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"S键：", PAL_XY(15, 160), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 
		 PAL_DrawText(L"退出游戏", PAL_XY(80, 60), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"逃跑", PAL_XY(200, 60), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"装备", PAL_XY(80, 80), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"投掷", PAL_XY(200, 80), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"使用", PAL_XY(80, 100), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"使用", PAL_XY(200, 100), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"无", PAL_XY(80, 120), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"重复上一轮攻击", PAL_XY(200, 120), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"无", PAL_XY(80, 140), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"围攻", PAL_XY(200, 140), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"状态栏", PAL_XY(80, 160), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"状态栏", PAL_XY(200, 160), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 
		 PAL_DrawText(L"上一页", PAL_XY(25, 180), 0x8D, TRUE, FALSE, FALSE);
         PAL_DrawText(L"下一页", PAL_XY(250, 180), 0x8D, TRUE, FALSE, FALSE);
		 }
		 
		 if (iCurrent == 2)
         {
		 PAL_DrawText(L"非战斗中", PAL_XY(80, 5), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"战斗中", PAL_XY(200, 5), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 
		 PAL_DrawText(L"D键：", PAL_XY(15, 30), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"F键：", PAL_XY(15, 50), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"P键：", PAL_XY(15, 70), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"T键：", PAL_XY(15, 90), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"G键：", PAL_XY(15, 110), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"Y键：", PAL_XY(15, 130), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"U键：", PAL_XY(15, 150), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 
		 PAL_DrawText(L"无", PAL_XY(80, 30), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"防御", PAL_XY(200, 30), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"法术栏", PAL_XY(80, 50), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"释放最强法术", PAL_XY(200, 50), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"截图", PAL_XY(80, 70), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"截图", PAL_XY(200, 70), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"无", PAL_XY(80, 90), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"数据", PAL_XY(200, 90), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"无", PAL_XY(80, 110), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"怪物属性栏", PAL_XY(200, 110), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"法术与等级", PAL_XY(80, 130), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"法术与等级", PAL_XY(200, 130), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"无", PAL_XY(80, 150), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"战斗换装", PAL_XY(200, 150), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 
		 PAL_DrawText(L"上一页", PAL_XY(25, 180), 0x8D, TRUE, FALSE, FALSE);
         PAL_DrawText(L"下一页", PAL_XY(250, 180), 0x8D, TRUE, FALSE, FALSE);
		 }
		 
		 if (iCurrent == 3)
         {
		 PAL_DrawText(L"非战斗中", PAL_XY(80, 5), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"战斗中", PAL_XY(200, 5), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 
		 PAL_DrawText(L"I键：", PAL_XY(15, 30), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 
		 PAL_DrawText(L"按键连发", PAL_XY(80, 30), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"无", PAL_XY(200, 30), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 
		 PAL_DrawText(L"上一页", PAL_XY(25, 180), 0x8D, TRUE, FALSE, FALSE);
         PAL_DrawText(L"下一页", PAL_XY(250, 180), 0x8D, TRUE, FALSE, FALSE);
		 }

		 if (iCurrent == 4)
         {
		 PAL_DrawText(L"非战斗中", PAL_XY(80, 5), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"战斗中", PAL_XY(200, 5), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 
		 PAL_DrawText(L"Z键：", PAL_XY(15, 30), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"X键：", PAL_XY(15, 50), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"J键：", PAL_XY(15, 70), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
         PAL_DrawText(L"组合键： ", PAL_XY(15, 90), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 
		 PAL_DrawText(L"快捷存档", PAL_XY(80, 30), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"无", PAL_XY(200, 30), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"快捷读档", PAL_XY(80, 50), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"无", PAL_XY(200, 50), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"无", PAL_XY(80, 70), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"战斗加速", PAL_XY(200, 70), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 
		 PAL_DrawText(L"------------------------------------", PAL_XY(15, 80), 0x1A, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"←↓→", PAL_XY(80, 90), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"战斗最快速度", PAL_XY(200, 90), MENUITEM_COLOR, TRUE, FALSE, FALSE);
		 
		 PAL_DrawText(L"开启了战斗加速以后，同时按←↓→键", PAL_XY(15, 120), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"会显示开关，如果没有显示开关的话", PAL_XY(15, 140), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"那是你没按准，三个键同时多按几次！！", PAL_XY(15, 160), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 
		 PAL_DrawText(L"上一页", PAL_XY(25, 180), 0x8D, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"下一页", PAL_XY(250, 180), 0x8D, TRUE, FALSE, FALSE);
		 }
		 
		 if (iCurrent == 5)
         {
		 PAL_DrawText(L"<<特别声明>>", PAL_XY(110, 5), MENUITEM_COLOR, TRUE, FALSE, FALSE);
			
		 PAL_DrawText(L"本程序是自由免费软件", PAL_XY(50, 50), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 
		 PAL_DrawText(L"源程序发布于", PAL_XY(50, 80), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 PAL_DrawText(L"(http://sdlpal.github.io/)", PAL_XY(50, 100), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 
		 PAL_DrawText(L"   So lost im faded   修改编译", PAL_XY(30, 130), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		 
		 PAL_DrawText(L"上一页", PAL_XY(25, 180), 0x8D, TRUE, FALSE, FALSE);
		 }
	
	  if (iCurrent == 0)
      {
	  PAL_FadeIn(0, FALSE, 2);
	  }
	
	      // Update the screen
      VIDEO_UpdateScreen(NULL);


      // Wait for input
      PAL_ClearKeyState();

      while (TRUE)
      {
         UTIL_Delay(1);

         if (g_InputState.dwKeyPress & kKeyMenu)
         {
			i = 1;
            iCurrent = -1;
			if (g_Battle.BattleResult != kBattleResultOnGoing)
            {
			UTIL_Delay(200);
			AUDIO_PlayMusic(gpGlobals->wNumMusic, TRUE, 1);
            }
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
	  
	  if (i == 0 && iCurrent > 5 || iCurrent < 0)
         {
			 if (g_Battle.BattleResult != kBattleResultOnGoing)
             {
			iCurrent = -1;
			UTIL_Delay(200);
			AUDIO_PlayMusic(gpGlobals->wNumMusic, TRUE, 1);
            PAL_FadeOut(1);
			PAL_FadeIn(0, FALSE, 1);
            break;
			 }
         }
   }
}
	
/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2024, SDLPAL development team.
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

#if PD_MODInformation
VOID
PAL_New_MODInformation(
   VOID
)
/*++
  Purpose:

   Show the additional credits.

  Parameters:

   None.

  Return value:

   None.

--*/
{
   LPCWSTR rgszStrings[] = {
      L"                作者的话",
      L"WIN-95 版 竞速 MOD Ver"PD_ProgramVersion,
      L"作者：仪拓诗、风羽かざば_Official",
      L"              (本MOD完全免费，严禁售卖)",
      L"第一次为大家制作竞速版本，不足之处还望 ",
      L"大家不吝赐教～本程序目前仍有许多地方与 ",
      L"原版不同，待完善，感谢大家测试和反馈～ ",
      L"",
      L"特别感谢∶(排名不分先后)",
      L"Palalex(圆梦MOD程序代码供参考)",
      L"白昼寒露、-皮皮叶-、Houou",
      L"                    按 Enter 继续......",
   };

   int        i = 0;

   PAL_DrawOpeningMenuBackground();
   PAL_SetPalette(0, FALSE);
   AUDIO_PlayMusic(RIX_NUM_OPENINGMENU, TRUE, 1);

   for (i = 0; i < 12; i++)
   {
      WCHAR buffer[48];
      PAL_swprintf(buffer, sizeof(buffer) / sizeof(WCHAR), rgszStrings[i]);
      PAL_DrawText(buffer, PAL_XY(0, 2 + i * 16), DESCTEXT_COLOR, TRUE, FALSE, FALSE);
   }

   //
   // Wait for input
   //
   PAL_ClearKeyState();

   while (TRUE)
   {
      VIDEO_UpdateScreen(NULL);

      UTIL_Delay(1);

      if (g_InputState.dwKeyPress & (kKeyMenu | kKeySearch))
      {
         break;
      }
   }
}
#endif

VOID
PAL_GameMain(
   VOID
)
/*++
  Purpose:

    The game entry routine.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   DWORD       dwTime;

#if PD_MODInformation
   PAL_New_MODInformation();
#endif

   //
   // Show the opening menu.
   //
   gpGlobals->bCurrentSaveSlot = (BYTE)PAL_OpeningMenu();

#if PD_LoadSave_NoResetItemCursor
   gpGlobals->iCurInvMenuItem = 0;
#if PD_Menu_KeyLeftOrRight_NextLine
   gpGlobals->iCurSellMenuItem = 0;
#endif // PD_Menu_KeyLeftOrRight_NextLine
#endif // PD_LoadSave_NoResetItemCursor
   gpGlobals->fInMainGame = TRUE;

   //
   // Initialize game data and set the flags to load the game resources.
   //
   PAL_ReloadInNextTick(gpGlobals->bCurrentSaveSlot);

   //
   // Run the main game loop.
   //
   dwTime = SDL_GetTicks();
   while (TRUE)
   {
      //
      // Load the game resources if needed.
      //
      PAL_LoadResources();

      //
      // Clear the input state of previous frame.
      //
      PAL_ClearKeyState();

      //
      // Wait for the time of one frame. Accept input here.
      //
      PAL_DelayUntil(dwTime);

      //
      // Set the time of the next frame.
      //
      dwTime = SDL_GetTicks() + FRAME_TIME;

      //
      // Run the main frame routine.
      //
      PAL_StartFrame();
   }
}

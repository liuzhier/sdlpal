/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2019, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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
#include <setjmp.h>

#if defined(PAL_HAS_GIT_REVISION)
# undef PAL_GIT_REVISION
# include "generated.h"
#endif

static jmp_buf g_exit_jmp_buf;
static int g_exit_code = 0;

#define BITMAPNUM_SPLASH_UP         (gConfig.fIsWIN95 ? 0x03 : 0x26)
#define BITMAPNUM_SPLASH_DOWN       (gConfig.fIsWIN95 ? 0x04 : 0x27)
#define SPRITENUM_SPLASH_TITLE      0x47
#define SPRITENUM_SPLASH_CRANE      0x49
#define NUM_RIX_TITLE               0x05

VOID
PAL_AdditionalCredits2(
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
   LPCWSTR rgszcps[][CP_MAX] = {
      // Traditional Chinese, Simplified Chinese
      { L"", L"", /*L""*/ },
      { L"         經典特別篇   ",
        L"DOS版MOD 圆梦终曲",
        //L"   \x30AF\x30E9\x30B7\x30C3\x30AF\x7279\x5225\x7DE8  "
      },
      { L"",
      L"",
      /*L""*/ },
      { L"", L"", /*L""*/ },
      { L"", L"", /*L""*/ },
      { L"", L"", /*L""*/ },

      { L"", L"", /*L""*/ },

      { L"   本程式是自由軟體，按照 GNU General",
        L" ",
        //L" \x3053\x306E\x30D7\x30ED\x30B0\x30E9\x30E0\x306F\x81EA\x7531\x30BD\x30D5\x30C8\x30A6\x30A7\x30A2\x3067\x3059\x3001"
      },
      { L"Public License v3",
      L"转载请注明MOD作者：碎片天空(Alex)", /*L""*/ },

      { L"Public License v3 或更高版本發佈.",
        L"严禁用于商业用途，如果您是花钱买到本MOD",
        //L" GNU General Public License v3 \x306E\x4E0B\x3067"
      },
      { L"",
      L"那么您被骗了，作者不承担任何后果", },
      { L"                    ...按 Enter 結束",
        L"                  按回车开始游戏",
        //L"      ...Enter\x30AD\x30FC\x3092\x62BC\x3057\x3066\x7D42\x4E86\x3057\x307E\x3059"
      },
   };

   LPCWSTR rgszStrings[] = {
      L"                作者的话",
 #ifdef PAL_CLASSIC
      L"%ls(" WIDETEXT(__DATE__) L")",
 #else
      L"                        (" WIDETEXT(__DATE__) L")",
 #endif
      L"圆梦是我第一个修改的仙剑MOD，从一开始构",
      L"思到现在的终曲，发生了很多事情，感谢各位",
      L"仙迷的支持也希望以后会作出更好的仙剑MOD",
      L"带给大家，终曲代表着圆梦MOD的完结",
      L"但不是仙剑梦的一个完结",  // Porting information line 1
      L"%ls",  // Porting information line 2
      L"%ls",  // GNU line 1
      L"%ls",  // GNU line 2
      L"%ls",  // GNU line 3
      L"%ls",  // Press Enter to continue
   };

   int        i = 0;

   PAL_DrawOpeningMenuBackground();

   for (i = 0; i < 12; i++)
   {
      WCHAR buffer[48];
      PAL_swprintf(buffer, sizeof(buffer) / sizeof(WCHAR), rgszStrings[i], gConfig.pszMsgFile ? g_rcCredits[i] : rgszcps[i][PAL_GetCodePage()]);
      PAL_DrawText(buffer, PAL_XY(0, 2 + i * 16), DESCTEXT_COLOR, TRUE, FALSE, FALSE);
   }

   PAL_SetPalette(0, FALSE);
   VIDEO_UpdateScreen(NULL);
   AUDIO_PlayMusic(RIX_NUM_OPENINGMENU, TRUE, 1);
   PAL_WaitForKey(0);
}

static VOID
PAL_Init(
   VOID
)
/*++
  Purpose:

    Initialize everything needed by the game.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int           e;
#if PAL_HAS_GIT_REVISION
   UTIL_LogOutput(LOGLEVEL_DEBUG, "SDLPal build revision: %s\n", PAL_GIT_REVISION);
#endif

   //
   // Initialize subsystems.
   //
   e = PAL_InitGlobals();
   if (e != 0)
   {
	   TerminateOnError("Could not initialize global data: %d.\n", e);
   }

   e = VIDEO_Startup();
   if (e != 0)
   {
      TerminateOnError("Could not initialize Video: %d.\n", e);
   }

   VIDEO_SetWindowTitle("Loading...");

   e = PAL_InitUI();
   if (e != 0)
   {
      TerminateOnError("Could not initialize UI subsystem: %d.\n", e);
   }

   e = PAL_InitText();
   if (e != 0)
   {
      TerminateOnError("Could not initialize text subsystem: %d.\n", e);
   }

   e = PAL_InitFont(&gConfig);
   if (e != 0)
   {
      TerminateOnError("Could not load fonts: %d.\n", e);
   }
   PAL_InitInput();
   PAL_InitResources();
   AUDIO_OpenDevice();
   PAL_AVIInit();

   VIDEO_SetWindowTitle(UTIL_va(UTIL_GlobalBuffer(0), PAL_GLOBAL_BUFFER_SIZE,
      "仙剑奇侠传DOS版  MOD：圆梦终曲 碎片天空(Alex)  %s%s",
#if defined(_DEBUG) || defined(DEBUG)
	   " (Debug) ",
#else
	   "",
#endif
#if defined(PAL_HAS_GIT_REVISION) && defined(PAL_GIT_REVISION)
	   " ["  PAL_GIT_REVISION "] "
#else
	   ""
#endif
	   , (gConfig.fEnableGLSL && gConfig.pszShader ? gConfig.pszShader : "")
   ));
}

VOID
PAL_Shutdown(
   int exit_code
)
/*++
  Purpose:

    Free everything needed by the game.

  Parameters:

    exit_code -  The exit code return to OS.

  Return value:

    None.

--*/
{
   AUDIO_CloseDevice();
   PAL_AVIShutdown();
   PAL_FreeFont();
   PAL_FreeResources();
   PAL_FreeUI();
   PAL_FreeText();
   PAL_ShutdownInput();
   VIDEO_Shutdown();
   
   //
   // global needs be free in last
   // since subsystems may needs config content during destroy
   // which also cleared here
   //
   PAL_FreeGlobals();

   g_exit_code = exit_code;
#if !__EMSCRIPTEN__
   longjmp(g_exit_jmp_buf, 1);
#else
   SDL_Quit();
   UTIL_Platform_Quit();
   return;
#endif
}

VOID
PAL_TrademarkScreen(
   VOID
)
/*++
  Purpose:

    Show the trademark screen.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   if (PAL_PlayAVI("1.avi")) return;

   PAL_SetPalette(3, FALSE);
   AUDIO_PlayMusic(106, TRUE, 3);
   PAL_RNGPlay(6, 0, -1, 50);
   UTIL_Delay(1000);
   PAL_FadeOut(1);
}

VOID
PAL_SplashScreen(
   VOID
)
/*++
  Purpose:

    Show the splash screen.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   SDL_Color     *palette = PAL_GetPalette(1, FALSE);
   SDL_Color      rgCurrentPalette[256];
   SDL_Surface   *lpBitmapDown, *lpBitmapUp;
   SDL_Rect       srcrect, dstrect;
   LPSPRITE       lpSpriteCrane;
   LPBITMAPRLE    lpBitmapTitle;
   LPBYTE         buf, buf2;
   int            cranepos[9][3], i, iImgPos = 200, iCraneFrame = 0, iTitleHeight;
   DWORD          dwTime, dwBeginTime;
   BOOL           fUseCD = TRUE;

   if (PAL_PlayAVI("2.avi")) return;

   if (palette == NULL)
   {
      fprintf(stderr, "ERROR: PAL_SplashScreen(): palette == NULL\n");
      return;
   }

   //
   // Allocate all the needed memory at once for simplification
   //
   buf = (LPBYTE)UTIL_calloc(1, 320 * 200 * 2);
   buf2 = (LPBYTE)(buf + 320 * 200);
   lpSpriteCrane = (LPSPRITE)buf2 + 32000;

   //
   // Create the surfaces
   //
   lpBitmapDown = VIDEO_CreateCompatibleSurface(gpScreen);
   lpBitmapUp = VIDEO_CreateCompatibleSurface(gpScreen);

   //
   // Read the bitmaps
   //
   PAL_MKFReadChunk(buf, 320 * 200, BITMAPNUM_SPLASH_UP, gpGlobals->f.fpFBP);
   Decompress(buf, buf2, 320 * 200);
   PAL_FBPBlitToSurface(buf2, lpBitmapUp);
   PAL_MKFReadChunk(buf, 320 * 200, BITMAPNUM_SPLASH_DOWN, gpGlobals->f.fpFBP);
   Decompress(buf, buf2, 320 * 200);
   PAL_FBPBlitToSurface(buf2, lpBitmapDown);
   PAL_MKFReadChunk(buf, 32000, SPRITENUM_SPLASH_TITLE, gpGlobals->f.fpMGO);
   Decompress(buf, buf2, 32000);
   lpBitmapTitle = (LPBITMAPRLE)PAL_SpriteGetFrame(buf2, 0);
   PAL_MKFReadChunk(buf, 32000, SPRITENUM_SPLASH_CRANE, gpGlobals->f.fpMGO);
   Decompress(buf, lpSpriteCrane, 32000);

   iTitleHeight = PAL_RLEGetHeight(lpBitmapTitle);
   lpBitmapTitle[2] = 0;
   lpBitmapTitle[3] = 0; // HACKHACK

   //
   // Generate the positions of the cranes
   //
   for (i = 0; i < 9; i++)
   {
      cranepos[i][0] = RandomLong(300, 600);
      cranepos[i][1] = RandomLong(0, 80);
      cranepos[i][2] = RandomLong(0, 8);
   }

   //
   // Play the title music
   //
   if (!AUDIO_PlayCDTrack(7))
   {
      fUseCD = FALSE;
      AUDIO_PlayMusic(NUM_RIX_TITLE, TRUE, 2);
   }

   //
   // Clear all of the events and key states
   //
   PAL_ProcessEvent();
   PAL_ClearKeyState();

   dwBeginTime = SDL_GetTicks();

   srcrect.x = 0;
   srcrect.w = 320;
   dstrect.x = 0;
   dstrect.w = 320;

   while (TRUE)
   {
      PAL_ProcessEvent();
      dwTime = SDL_GetTicks() - dwBeginTime;

      //
      // Set the palette
      //
      if (dwTime < 15000)
      {
         for (i = 0; i < 256; i++)
         {
            rgCurrentPalette[i].r = (BYTE)(palette[i].r * ((float)dwTime / 15000));
            rgCurrentPalette[i].g = (BYTE)(palette[i].g * ((float)dwTime / 15000));
            rgCurrentPalette[i].b = (BYTE)(palette[i].b * ((float)dwTime / 15000));
         }
      }

      VIDEO_SetPalette(rgCurrentPalette);
      VIDEO_UpdateSurfacePalette(lpBitmapDown);
      VIDEO_UpdateSurfacePalette(lpBitmapUp);

      //
      // Draw the screen
      //
      if (iImgPos > 1)
      {
         iImgPos--;
      }

      //
      // The upper part...
      //
      srcrect.y = iImgPos;
      srcrect.h = 200 - iImgPos;

      dstrect.y = 0;
      dstrect.h = srcrect.h;

	  VIDEO_CopySurface(lpBitmapUp, &srcrect, gpScreen, &dstrect);

      //
      // The lower part...
      //
      srcrect.y = 0;
      srcrect.h = iImgPos;

      dstrect.y = 200 - iImgPos;
      dstrect.h = srcrect.h;

	  VIDEO_CopySurface(lpBitmapDown, &srcrect, gpScreen, &dstrect);

      //
      // Draw the cranes...
      //
      for (i = 0; i < 9; i++)
      {
         LPCBITMAPRLE lpFrame = PAL_SpriteGetFrame(lpSpriteCrane,
            cranepos[i][2] = (cranepos[i][2] + (iCraneFrame & 1)) % 8);
         cranepos[i][1] += ((iImgPos > 1) && (iImgPos & 1)) ? 1 : 0;
         PAL_RLEBlitToSurface(lpFrame, gpScreen,
            PAL_XY(cranepos[i][0], cranepos[i][1]));
         cranepos[i][0]--;
      }
      iCraneFrame++;

      //
      // Draw the title...
      //
      if (PAL_RLEGetHeight(lpBitmapTitle) < iTitleHeight)
      {
         //
         // HACKHACK
         //
         WORD w = lpBitmapTitle[2] | (lpBitmapTitle[3] << 8);
         w++;
         lpBitmapTitle[2] = (w & 0xFF);
         lpBitmapTitle[3] = (w >> 8);
      }

      PAL_RLEBlitToSurface(lpBitmapTitle, gpScreen, PAL_XY(255, 10));
      VIDEO_UpdateScreen(NULL);

      //
      // Check for keypress...
      //
      if (g_InputState.dwKeyPress & (kKeyMenu | kKeySearch))
      {
         //
         // User has pressed a key...
         //
         lpBitmapTitle[2] = iTitleHeight & 0xFF;
         lpBitmapTitle[3] = iTitleHeight >> 8; // HACKHACK

         PAL_RLEBlitToSurface(lpBitmapTitle, gpScreen, PAL_XY(255, 10));

         VIDEO_UpdateScreen(NULL);

         if (dwTime < 15000)
         {
            //
            // If the picture has not completed fading in, complete the rest
            //
            while (dwTime < 15000)
            {
               for (i = 0; i < 256; i++)
               {
                  rgCurrentPalette[i].r = (BYTE)(palette[i].r * ((float)dwTime / 15000));
                  rgCurrentPalette[i].g = (BYTE)(palette[i].g * ((float)dwTime / 15000));
                  rgCurrentPalette[i].b = (BYTE)(palette[i].b * ((float)dwTime / 15000));
               }
               VIDEO_SetPalette(rgCurrentPalette);
               VIDEO_UpdateSurfacePalette(lpBitmapDown);
               VIDEO_UpdateSurfacePalette(lpBitmapUp);
               UTIL_Delay(8);
               dwTime += 250;
            }
            UTIL_Delay(500);
         }

         //
         // Quit the splash screen
         //
         break;
      }

      //
      // Delay a while...
      //
      PAL_ProcessEvent();
      while (SDL_GetTicks() - dwBeginTime < dwTime + 85)
      {
         SDL_Delay(1);
         PAL_ProcessEvent();
      }
   }

   VIDEO_FreeSurface(lpBitmapDown);
   VIDEO_FreeSurface(lpBitmapUp);
   free(buf);

   if (!fUseCD)
   {
      AUDIO_PlayMusic(0, TRUE, 1);
   }

   PAL_FadeOut(1);
   PAL_AdditionalCredits2();
}



int
main(
   int      argc,
   char    *argv[]
)
/*++
  Purpose:

    Program entry.

  Parameters:

    argc - Number of arguments.

    argv - Array of arguments.

  Return value:

    Integer value.

--*/
{
#if PAL_HAS_PLATFORM_STARTUP
   UTIL_Platform_Startup(argc,argv);
#endif

#if !__EMSCRIPTEN__
   if (setjmp(g_exit_jmp_buf) != 0)
   {
	   // A longjmp is made, should exit here
	   SDL_Quit();
	   UTIL_Platform_Quit();
	   return g_exit_code;
   }
#endif

#if !defined(UNIT_TEST) || defined(UNIT_TEST_GAME_INIT)
   //
   // Initialize SDL
   //
   if (SDL_Init(PAL_SDL_INIT_FLAGS) == -1)
   {
	   TerminateOnError("Could not initialize SDL: %s.\n", SDL_GetError());
   }

   PAL_LoadConfig(TRUE);

   //
   // Platform-specific initialization
   //
   if (UTIL_Platform_Init(argc, argv) != 0)
	   return -1;

   //
   // Should launch setting?
   // Generally, the condition should never be TRUE as the UTIL_Platform_Init is assumed
   // to handle gConfig.fLaunchSetting correctly. However, it may actually be true due to
   // the activatation event on WinRT platform, so close the current process to make new
   // process go to setting.
   // For platforms without configuration page available, this condition will NEVER be true.
   //
   if (PAL_HAS_CONFIG_PAGE && gConfig.fLaunchSetting)
	   return 0;

   //
   // If user requests a file-based log, then add it after the system-specific one.
   //
   if (gConfig.pszLogFile)
	   UTIL_LogAddOutputCallback(UTIL_LogToFile, gConfig.iLogLevel);

   //
   // Initialize everything
   //
   PAL_Init();
#endif

#if !defined(UNIT_TEST)
   //
   // Show the trademark screen and splash screen
   //
//   PAL_TrademarkScreen();
   PAL_SplashScreen();

   //
   // Run the main game routine
   //
   PAL_GameMain();

   //
   // Should not really reach here...
   //
   assert(FALSE);
   return 255;
#else
   extern int testmain(int argc, char *argv[]);
   return testmain(argc, argv);
#endif
}

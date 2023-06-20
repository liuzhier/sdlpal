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
#include <setjmp.h>

#if defined(PAL_HAS_GIT_REVISION)
# undef PAL_GIT_REVISION
# include "generated.h"
#endif

static jmp_buf g_exit_jmp_buf;
static int g_exit_code = 0;

char gExecutablePath[PAL_MAX_PATH];

#define BITMAPNUM_SPLASH_UP         (gConfig.fIsWIN95 ? 0x03 : 0x26)
#define BITMAPNUM_SPLASH_DOWN       (gConfig.fIsWIN95 ? 0x04 : 0x27)
#define SPRITENUM_SPLASH_TITLE      0x47
#define SPRITENUM_SPLASH_CRANE      0x49
#define NUM_RIX_TITLE               0x05

static VOID
PAL_Init(
	VOID
)
/*++
  Purpose:

	Initialize everything needed by the game.
	初始化游戏所需的一切。

  Parameters:

	None.
	无

  Return value:

	None.
	无

--*/
{
	int           e;
#if PAL_HAS_GIT_REVISION && defined(PAL_GIT_REVISION)
	UTIL_LogOutput(LOGLEVEL_DEBUG, "SDLPal build revision: %s\n", PAL_GIT_REVISION);
#endif

	//
	// Initialize subsystems.
	// 初始化子系统。
	e = PAL_InitGlobals();
	if (e != 0)
	{
		//TerminateOnError("Could not initialize global data: %d.\n", e);
		TerminateOnError("初始化游戏数据时发生错误: %d.\n", e);
	}

	e = VIDEO_Startup();
	if (e != 0)
	{
		//TerminateOnError("Could not initialize Video: %d.\n", e);
		TerminateOnError("初始化视频时发生错误: %d.\n", e);
	}

	//VIDEO_SetWindowTitle("Loading...");
	VIDEO_SetWindowTitle("游戏载入中...");

	e = PAL_InitUI();
	if (e != 0)
	{
		//TerminateOnError("Could not initialize UI subsystem: %d.\n", e);
		TerminateOnError("初始化UI子系统时发生错误: %d.\n", e);
	}

	e = PAL_InitText();
	if (e != 0)
	{
		//TerminateOnError("Could not initialize text subsystem: %d.\n", e);
		TerminateOnError("初始化文本子系统时发生错误: %d.\n", e);
	}

	e = PAL_InitFont(&gConfig);
	if (e != 0)
	{
		TerminateOnError("Could not load fonts: %d.\n", e);
		TerminateOnError("加载字体时发生错误: %d.\n", e);
	}

	PAL_InitInput();
	PAL_InitResources();
	AUDIO_OpenDevice();
	PAL_AVIInit();

	VIDEO_SetWindowTitle(UTIL_va(UTIL_GlobalBuffer(0), PAL_GLOBAL_BUFFER_SIZE,
		"仙剑奇侠传DOS版 小改 Beta 娱乐版 %s%s",
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
	PAL_RNGPlay(6, 0, -1, 25);
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
	SDL_Color* palette = PAL_GetPalette(1, FALSE);
	SDL_Color      rgCurrentPalette[256];
	SDL_Surface* lpBitmapDown, * lpBitmapUp;
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
		AUDIO_PlayMusic(0, FALSE, 1);
	}

	PAL_FadeOut(1);
}



int
main(
	int      argc,
	char* argv[]
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
#if !defined( __EMSCRIPTEN__ ) && !defined(__WINRT__) && !defined(__N3DS__)
	memset(gExecutablePath, 0, PAL_MAX_PATH);
	strncpy(gExecutablePath, argv[0], PAL_MAX_PATH);
#endif

#if PAL_HAS_PLATFORM_STARTUP
	UTIL_Platform_Startup(argc, argv);
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
	// 如果用户请求基于文件的日志，则将其添加到系统特定日志之后。
	if (gConfig.pszLogFile)
		UTIL_LogAddOutputCallback(UTIL_LogToFile, gConfig.iLogLevel);

	//
	// Initialize everything
	// 初始化所有内容
	PAL_Init();
#endif

#if !defined(UNIT_TEST)
	//
	// Show the trademark screen and splash screen
	// 显示商标屏幕和启动屏幕
	//PAL_TrademarkScreen();
	PAL_SplashScreen();

	// 播放标题音乐
	AUDIO_PlayMusic(RIX_NUM_OPENINGMENU, TRUE, 1);

	// 展示作者的话
	PAL_AuthorWords();

	// 初始化存档目录
	strcat(gConfig.pszSavePath, "\\SAVE");

	//
	// Run the main game routine
	// 运行主游戏例程
	PAL_GameMain();

	//
	// Should not really reach here...
	// 不应该真的到达这里。。。？
	assert(FALSE);
	return 255;
#else
	extern int testmain(int argc, char* argv[]);
	return testmain(argc, argv);
#endif
}

VOID
PAL_AuthorWords(
	VOID
)
/*++
  Purpose:

	显示作者的话

  Parameters:

	None.
	无

  Return value:

	None.
	无

--*/
{
	//LPCWSTR rgszcps[] = {
	//	L"          作者的话（免费游戏）          ",
	//	L"仙剑奇侠传DOS版MOD 梦花园4.0 By 冻霜枕头",
	//	L"快捷键：               B站uid: 439413637",
	//	L"１ 快捷存档 透视敌方血量及负面 Buff 回合",
	//	L"２ 透视战场环境影响 白色增益 蓝色削弱",
	//	L"３ 查看敌方全属性页面",
	//	L"４ 查看队员领悟仙术所需修行",
	//	L"８ 透视我方五灵抗性",
	//	L"９ 透视我方增益 Buff 剩余回合",
	//	L"０ 透视我方负面 Buff 剩余回合",
	//	L"　　　 转载请注明MOD作者：七月菡萏胜似霜",
	//	L"我讨厌梦xx系列的改版 ．．．按 Enter 继续",
	//};

	//LPCWSTR rgszcps[] = {
	//	L"          温馨提示（免费游戏）          ",
	//	L"仙剑DOS版  MOD 圣灵传说1.0 By SDLPalLHYY",
	//	L"",
	//	L"快捷键：                               ",
	//	L"１ 快捷存档 透视战场敌方血量及各种 Buff",
	//	L"２ 查看敌方全属性页面",
	//	L"３ 查看队员领悟仙术所需修行",
	//	L"",
	//	L"     作者：灵幻ぬ月影（SDLPalLHYY）    ",
	//	L"  个人娱乐版，不提供下载，不喜勿喷哈～ ",
	//	L" 请勿相信一切出售改版者，全都是诈骗！！",
	//	L"　　　　　　　　　　．．．按 Enter　继续",
	//};

	LPCWSTR rgszcps[] = {
		L"          作者的话（免费游戏）",
		L"仙剑奇侠传DOS版 MOD 梦幻2.11 娱乐版小改",
		L"",
		L"主程序 HACK：",
		L"         柳尚书的二丫头（SDLPalLHYY）",
		L"",
		L"特别感谢：碎片天空（Alex）",
		L"",
		L"主播娱乐版，不提供下载，不喜勿喷哈～",
		L"",
		L"请勿相信一切出售改版者，谨防上当受骗。",
		L"                    ．．．按 Enter 继续",
	};

	//LPCWSTR rgszcps[] = {
	//	L"          作者的话（免费游戏）",
	//	L"仙剑奇侠传DOS版 Beta",
	//	L"音乐来自：圆梦1.1",
	//	L"大部分创意来自：圆梦终曲",
	//	L"部分素材来自：轩辕剑  霹雳奇侠传",
	//	L"图像及调色板来自：仙剑奇侠传SS版",
	//	L"",
	//	L"作者：柳氏尚书府二小姐（SDLPalLHYY）",
	//	L"特别感谢：碎片天空（Alex）",
	//	L"个人娱乐版，不提供下载，不喜勿喷哈～",
	//	L"请勿相信一切出售改版者，全是骗子。  ",
	//	L"                    ．．．按 Enter 继续",
	//};

	WCHAR      buffer[48];

	// 展示标题背景
	PAL_DrawOpeningMenuBackground();

	// 展示作者的话文本
	for (int i = 0; i < sizeof(rgszcps) / sizeof(rgszcps[0]); i++)
	{
		PAL_swprintf(buffer, sizeof(buffer) / sizeof(WCHAR), rgszcps[i]);
		PAL_DrawText(buffer, PAL_XY(0, 2 + i * 16), DESCTEXT_COLOR, TRUE, FALSE, FALSE);
	}
	PAL_DrawText(L"(" WIDETEXT(__DATE__) L")", PAL_XY(176, 34), DESCTEXT_COLOR, TRUE, FALSE, FALSE);

	// 设置游戏调色板
	PAL_SetPalette(0, FALSE);

	// 重绘整个画面
	VIDEO_UpdateScreen(NULL);

	// 等待玩家按下触发键
	PAL_WaitForKey(0);
}

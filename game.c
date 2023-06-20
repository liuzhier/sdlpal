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

VOID
PAL_GameMain(
	VOID
)
/*++
  Purpose:

	The game entry routine.
	正式进入游戏

  Parameters:

	None.
	无

  Return value:

	None.
	无

--*/
{
	DWORD       dwTime;

	//
	// Show the opening menu.
	// 展示游戏标题主画面
	gpGlobals->bCurrentSaveSlot = (BYTE)PAL_OpeningMenu();
	gpGlobals->fInMainGame = TRUE;

	//
	// Initialize game data and set the flags to load the game resources.
	// 初始化游戏数据并设置标志以加载游戏资源。
	PAL_ReloadInNextTick(gpGlobals->bCurrentSaveSlot);

#ifdef PAL_WalkDuringSceneSwitching
	// 开启进场前置身位
	gpGlobals->fGameStart = TRUE;
#endif // 前置身位

	//
	// Run the main game loop.
	// 运行主游戏循环
	dwTime = SDL_GetTicks();
	while (TRUE)
	{
		//
		// Load the game resources if needed.
		// 如果需要则加载游戏资源
		PAL_LoadResources();

		//
		// Clear the input state of previous frame.
		// 清除上一帧的输入状态
		PAL_ClearKeyState();

		//
		// Wait for the time of one frame. Accept input here.
		// 等待一帧的时间。在此接受输入
		PAL_DelayUntil(dwTime);

		//
		// Set the time of the next frame.
		// 设置下一帧的时间
		//dwTime = SDL_GetTicks() + FRAME_TIME;
		dwTime = SDL_GetTicks() + FRAME_TIME;

		//
		// Run the main frame routine.
		// 运行主帧例程
		PAL_StartFrame();
	}
}

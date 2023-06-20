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
PAL_GameUpdate(
	BOOL       fTrigger
)
/*++
  Purpose:

	The main game logic routine. Update the status of everything.
	主要的游戏逻辑例程。更新所有内容的状态。

  Parameters:

	[IN]  fTrigger - whether to process trigger events or not.
	是否处理触发事件

  Return value:

	None.
	无

--*/
{
	WORD            wEventObjectID, wDir;
	int             i;
	LPEVENTOBJECT   p;
	//WORD            wResult;

	//
	// Check for trigger events
	// 检查触发事件
	if (fTrigger)
	{
		//
		// Check if we are entering a new scene
		// 检查我们是否正在进入新场景
		if (gpGlobals->fEnteringScene)
		{
			//
			// Run the script for entering the scene
			// 运行脚本以进入场景
			gpGlobals->fEnteringScene = FALSE;

			i = gpGlobals->wNumScene - 1;
			gpGlobals->g.rgScene[i].wScriptOnEnter = PAL_RunTriggerScript(gpGlobals->g.rgScene[i].wScriptOnEnter, 0xFFFF, FALSE);

			if (gpGlobals->fEnteringScene)
			{
				//
				// Don't go further as we're switching to another scene
				// 当我们切换到另一个场景时，不要走得更远
				return;
			}

			PAL_ClearKeyState();
			PAL_MakeScene();
		}

		//
		// Update the vanish time for all event objects
		// 更新所有事件对象的消失时间
		for (wEventObjectID = 0; wEventObjectID < gpGlobals->g.nEventObject; wEventObjectID++)
		{
			p = &gpGlobals->g.lprgEventObject[wEventObjectID];

			if (p->sVanishTime != 0)
			{
				p->sVanishTime += ((p->sVanishTime < 0) ? 1 : -1);
			}
		}

		//
		// Loop through all event objects in the current scene
		// 循环当前场景中的所有事件对象
		for (wEventObjectID = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex + 1;
			wEventObjectID <= gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex;
			wEventObjectID++)
		{
			p = &gpGlobals->g.lprgEventObject[wEventObjectID - 1];

			if (p->sVanishTime != 0)
			{
				continue;
			}

			if (p->sState < 0)
			{
				if (p->x < PAL_X(gpGlobals->viewport) ||
					p->x > PAL_X(gpGlobals->viewport) + 320 ||
					p->y < PAL_Y(gpGlobals->viewport) ||
					p->y > PAL_Y(gpGlobals->viewport) + 320)
				{
					p->sState = abs(p->sState);
					p->wCurrentFrameNum = 0;
				}
			}
			else if (p->sState > 0 && p->wTriggerMode >= kTriggerTouchNear)
			{
				//
				// This event object can be triggered without manually exploring
				// 自动触发：此事件对象可以在不手动探索的情况下触发
				if (abs(PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset) - p->x) +
					abs(PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset) - p->y) * 2 <
					(p->wTriggerMode - kTriggerTouchNear) * 32 + 16)
				{
					//
					// Player is in the trigger zone.
					// 玩家处于触发区

					if (p->nSpriteFrames)
					{
						//
						// The sprite has multiple frames. Try to adjust the direction.
						// 事件图像有多个帧。试着调整方向
						int                xOffset, yOffset;

						p->wCurrentFrameNum = 0;

						xOffset = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset) - p->x;
						yOffset = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset) - p->y;

						if (xOffset > 0)
						{
							p->wDirection = ((yOffset > 0) ? kDirEast : kDirNorth);
						}
						else
						{
							p->wDirection = ((yOffset > 0) ? kDirSouth : kDirWest);
						}

						//
						// Redraw the scene
						// 重新绘制场景
						PAL_UpdatePartyGestures(FALSE);

						PAL_MakeScene();
						VIDEO_UpdateScreen(NULL);
					}

					//
					// Execute the script.
					// 执行脚本
					p->wTriggerScript = PAL_RunTriggerScript(p->wTriggerScript, wEventObjectID, FALSE);

					PAL_ClearKeyState();

					if (gpGlobals->fEnteringScene)
					{
						//
						// Don't go further on scene switching
						// 不要在场景切换上更进一步
						return;
					}
				}
			}
		}
	}

	//
	// Run autoscript for each event objects
	// 为每个事件对象运行自动脚本
	for (wEventObjectID = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex + 1;
		wEventObjectID <= gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex;
		wEventObjectID++)
	{
		p = &gpGlobals->g.lprgEventObject[wEventObjectID - 1];

		if (p->sState > 0 && p->sVanishTime == 0)
		{
			WORD wScriptEntry = p->wAutoScript;
			if (wScriptEntry != 0)
			{
				p->wAutoScript = PAL_RunAutoScript(wScriptEntry, wEventObjectID);
				if (gpGlobals->fEnteringScene)
				{
					//
					// Don't go further on scene switching
					// 不要在场景切换上更进一步
					return;
				}
			}
		}

		//
		// Check if the player is in the way
		// 检查队员是否挡路
		if (fTrigger && p->sState >= kObjStateBlocker && p->wSpriteNum != 0 &&
			abs(p->x - PAL_X(gpGlobals->viewport) - PAL_X(gpGlobals->partyoffset)) +
			abs(p->y - PAL_Y(gpGlobals->viewport) - PAL_Y(gpGlobals->partyoffset)) * 2 <= 12)
		{
			//
			// Player is in the way, try to move a step
			// 若队员挡道了，则试着移动一步
			wDir = (p->wDirection + 1) % 4;
			for (i = 0; i < 4; i++)
			{
				int              x, y;
				PAL_POS          pos;

				x = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
				y = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

				x += ((wDir == kDirWest || wDir == kDirSouth) ? -16 : 16);
				y += ((wDir == kDirWest || wDir == kDirNorth) ? -8 : 8);

				pos = PAL_XY(x, y);

				if (!PAL_CheckObstacle(pos, TRUE, 0))
				{
					//
					// move here
					// 移动到此处
					gpGlobals->viewport = PAL_XY(
						PAL_X(pos) - PAL_X(gpGlobals->partyoffset),
						PAL_Y(pos) - PAL_Y(gpGlobals->partyoffset));

					break;
				}

				wDir = (wDir + 1) % 4;
			}
		}
	}

	gpGlobals->dwFrameNum++;
}

VOID
PAL_GameUseItem(
	VOID
)
/*++
  Purpose:

	Allow player use an item in the game.
	允许队员在游戏中使用物品。

  Parameters:

	None.
	无

  Return value:

	None.
	无

--*/
{
	WORD         wObject;

	// 使用道具时关闭属性增减自动提示（注意：该开关也有其他意义，切换场景的前置身位开关）
	gpGlobals->fGameStart = FALSE;

	while (TRUE)
	{
		wObject = PAL_ItemSelectMenu(NULL, kItemFlagUsable);

		if (wObject == 0)
		{
			return;
		}

		//if (!(gpGlobals->g.rgObject[wObject].item.wFlags & kItemFlagApplyToAll))
		if (!(gpGlobals->g.rgObject[wObject].item.wFlags & kItemFlagApplyToPlayerAll))
		{
			//
			// Select the player to use the item on
			// 选择要在其上使用道具的队员
			WORD     wPlayer = 0;

			while (TRUE)
			{
				wPlayer = PAL_ItemUseMenu(wObject);

				if (wPlayer == MENUITEM_VALUE_CANCELLED)
				{
					break;
				}

				//
				// Run the script
				// 运行脚本
				gpGlobals->g.rgObject[wObject].item.wScriptOnUse =
					PAL_RunTriggerScript(gpGlobals->g.rgObject[wObject].item.wScriptOnUse, wPlayer, FALSE);

				//
				// Remove the item if the item is consuming and the script succeeded
				// 若项目正在使用且脚本执行成功，则删除该项目
				if ((gpGlobals->g.rgObject[wObject].item.wFlags & kItemFlagConsuming) &&
					g_fScriptSuccess)
				{
					PAL_AddItemToInventory(wObject, -1);
				}
			}
		}
		else
		{
			//
			// Run the script
			// 运行脚本
			gpGlobals->g.rgObject[wObject].item.wScriptOnUse =
				PAL_RunTriggerScript(gpGlobals->g.rgObject[wObject].item.wScriptOnUse, 0xFFFF, FALSE);

			//
			// Remove the item if the item is consuming and the script succeeded
			// 如果项目正在使用并且脚本成功，则删除该项目
			if ((gpGlobals->g.rgObject[wObject].item.wFlags & kItemFlagConsuming) &&
				g_fScriptSuccess)
			{
				PAL_AddItemToInventory(wObject, -1);
			}

			return;
		}
	}

	// 道具使用完毕，打开属性增减自动提示
	gpGlobals->fGameStart = TRUE;
}

VOID
PAL_GameEquipItem(
	VOID
)
/*++
  Purpose:

	Allow player equip an item in the game.
	允许队员在游戏中装备物品。

  Parameters:

	None.
	无

  Return value:

	None.
	无

--*/
{
	WORD      wObject;

	while (TRUE)
	{
		wObject = PAL_ItemSelectMenu(NULL, kItemFlagEquipable);

		if (wObject == 0)
		{
			return;
		}

		PAL_EquipItemMenu(wObject);
	}
}

VOID
PAL_Search(
	VOID
)
/*++
  Purpose:

	Process searching trigger events.
	处理调查触发事件。

  Parameters:

	None.
	无

  Return value:

	None.
	无

--*/
{
	int                x, y, xOffset, yOffset, dx, dy, dh, ex, ey, eh, i, k, l;
	LPEVENTOBJECT      p;
	PAL_POS            rgPos[13];

	//
	// Get the party location
	// 获取领队坐标？
	x = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
	y = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

	if (gpGlobals->wPartyDirection == kDirNorth || gpGlobals->wPartyDirection == kDirEast)
	{
		xOffset = 16;
	}
	else
	{
		xOffset = -16;
	}

	if (gpGlobals->wPartyDirection == kDirEast || gpGlobals->wPartyDirection == kDirSouth)
	{
		yOffset = 8;
	}
	else
	{
		yOffset = -8;
	}

	rgPos[0] = PAL_XY(x, y);

	for (i = 0; i < 4; i++)
	{
		rgPos[i * 3 + 1] = PAL_XY(x + xOffset, y + yOffset);
		rgPos[i * 3 + 2] = PAL_XY(x, y + yOffset * 2);
		rgPos[i * 3 + 3] = PAL_XY(x + xOffset, y);
		x += xOffset;
		y += yOffset;
	}

	for (i = 0; i < 13; i++)
	{
		//
		// Convert to map location
		// 转换为地图绝对坐标？
		dh = ((PAL_X(rgPos[i]) % 32) ? 1 : 0);
		dx = PAL_X(rgPos[i]) / 32;
		dy = PAL_Y(rgPos[i]) / 16;

		//
		// Loop through all event objects
		// 循环遍历所有事件对象
		for (k = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex;
			k < gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex; k++)
		{
			p = &(gpGlobals->g.lprgEventObject[k]);
			ex = p->x / 32;
			ey = p->y / 16;
			eh = ((p->x % 32) ? 1 : 0);

			if (p->sState <= 0 || p->wTriggerMode >= kTriggerTouchNear ||
				p->wTriggerMode * 6 - 4 < i || dx != ex || dy != ey || dh != eh)
			{
				continue;
			}

			//
			// Adjust direction/gesture for party members and the event object
			// 调整队员与事件对象的方向/姿势
			if (p->nSpriteFrames * 4 > p->wCurrentFrameNum)
			{
				p->wCurrentFrameNum = 0; // use standing gesture 使用站立姿势（即置帧0）
				p->wDirection = (gpGlobals->wPartyDirection + 2) % 4; // face the party 使该对象面对领队（重设面朝方向）

				for (l = 0; l <= gpGlobals->wMaxPartyMemberIndex; l++)
				{
					//
					// All party members should face the event object
					// 所有队员都应该面对事件对象
					gpGlobals->rgParty[l].wFrame = gpGlobals->wPartyDirection * 3;
				}

				//
				// Redraw everything
				// 重新绘制所有内容
				PAL_MakeScene();
				VIDEO_UpdateScreen(NULL);
			}

			//
			// Execute the script
			// 执行脚本
			p->wTriggerScript = PAL_RunTriggerScript(p->wTriggerScript, k + 1, FALSE);

			//
			// Clear inputs and delay for a short time
			// 清除输入并进行短时间的延迟
			UTIL_Delay(50);
			PAL_ClearKeyState();

			return; // don't go further 不要走得更远
		}
	}
}

VOID
PAL_StartFrame(
	VOID
)
/*++
  Purpose:

	Starts a video frame. Called once per video frame.
	启动视频帧。每个视频帧调用一次。

  Parameters:

	None.
	无

  Return value:

	None.
	无

--*/
{
	//
	// Run the game logic of one frame
	// 运行一帧的游戏逻辑
	PAL_GameUpdate(TRUE);

	// 若当前状态为切换场景
	if (gpGlobals->fEnteringScene)
#ifdef PAL_WalkDuringSceneSwitching
	{
		// 允许进场一个前置身位
		if (gpGlobals->fGameStart && gpGlobals->fEnteringScene)
		{
			//
			// Update the positions and gestures of party members
			// 更新队员的坐标和姿态（当前帧）
			PAL_UpdateParty();

			// 关闭进场前置身位
			gpGlobals->fGameStart = FALSE;
		}

		return;
	}

	// 开启进场前置身位
	gpGlobals->fGameStart = TRUE;
#else
		return;
#endif

	//
	// Update the positions and gestures of party members
	// 更新队员的坐标和姿态（当前帧）
	PAL_UpdateParty();

	//
	// Update the scene
	// 更新场景
	PAL_MakeScene();
	VIDEO_UpdateScreen(NULL);

	if (g_InputState.dwKeyPress & kKeyMenu)
	{
		// 按下对应键时
		// Show the in-game menu
		// 显示游戏内菜单
		PAL_InGameMenu();
	}
	else if (g_InputState.dwKeyPress & kKeyUseItem)
	{
		// 按下对应键时
		// Show the use item menu
		// 展示"使用"模式下的道具栏
		PAL_GameUseItem();
	}
	else if (g_InputState.dwKeyPress & kKeyQuipmentItem)
	{
		// 按下对应键时
		// Show the equipment menu
		// 展示"装备"模式下的道具栏
		PAL_GameEquipItem();
	}
	else if (g_InputState.dwKeyPress & kKeyForce)
	{
		// 按下对应键时
		// Show the magic menu
		// 展示"法术"的队员选择栏
		PAL_InGameMagicMenu();
	}
	else if (g_InputState.dwKeyPress & kKeyStatus)
	{
		// 按下对应键时
		// Show the player status
		// 展示队员状态
		PAL_PlayerStatus();
	}
	else if (g_InputState.dwKeyPress & kKeySearch)
	{
		// 按下对应键时
		// Process search events
		// 调查可触发的事件
		PAL_Search();
	}
	else if (g_InputState.dwKeyPress & kKeyFlee)
	{
		// 按下对应键时
		// Quit Game
		// 选择是否退出游戏
		PAL_QuitGame();
	}
	else if (g_InputState.dwKeyPress & kKeyFastSave)
	{
		// 按下对应键时
		// Convenient storage of game progress
		// 便捷储存游戏进度
		PAL_FastSaveGame(FALSE);
	}
	else if (g_InputState.dwKeyPress & kKeyPlayerLevelmagic)
	{
		// 按下对应键时
		// Show the level required for team members to learn magic
		// 查看队员领悟仙术所需修行
		PAL_New_PlayerLevelmagic();
	}

	// 驱魔香时间
	if (--gpGlobals->wChasespeedChangeCycles == 0)
	{
		gpGlobals->wChaseRange = 1;
	}
}

VOID
PAL_WaitForKey(
	WORD      wTimeOut
)
/*++
  Purpose:

	Wait for any key.
	等待有任意键按下

  Parameters:

	[IN]  wTimeOut - the maximum time of the waiting. 0 = wait forever.
	等待的最长时间。0=永远等待。

  Return value:

	None.
	无

--*/
{
	DWORD     dwTimeOut = SDL_GetTicks() + wTimeOut;

	PAL_ClearKeyState();

	while (wTimeOut == 0 || !SDL_TICKS_PASSED(SDL_GetTicks(), dwTimeOut))
	{
		UTIL_Delay(5);

		if (g_InputState.dwKeyPress & (kKeySearch | kKeyMenu))
		{
			break;
		}
	}
}

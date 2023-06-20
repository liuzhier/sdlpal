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

BATTLE          g_Battle;

WORD
g_rgPlayerPos[4][4][2] = {
   {{240, 170}},                                                     // one player
   {{200, 176}, {256, 152}},                                         // two players
   {{180, 180}, {234, 170}, {270, 146}},                             // three players
   //
   // 这里增加了四人战的队员战场的坐标，代码来自：仙剑奇侠传DOS版圆梦终曲MOD
   //
   {{160, 190}, {217, 185}, {255, 165}, {285, 145}}                  // four players
};

VOID
PAL_BattleMakeScene(
	VOID
)
/*++
  Purpose:

	Generate the battle scene into the scene buffer.
	将战斗场景生成到场景缓冲区中。

  Parameters:

	None.

  Return value:

	None.

--*/
{
	int          i, j;
	PAL_POS      pos;
	LPBYTE       pSrc, pDst;
	BYTE         b;
	INT          enemyDrawSeq[MAX_ENEMIES_IN_TEAM];

	//
	// Draw the background
	// 重绘战斗背景
	pSrc = g_Battle.lpBackground->pixels;
	pDst = g_Battle.lpSceneBuf->pixels;

	for (i = 0; i < g_Battle.lpSceneBuf->pitch * g_Battle.lpSceneBuf->h; i++)
	{
		b = (*pSrc & 0x0F);
		b += g_Battle.sBackgroundColorShift;

		if (b & 0x80)
		{
			b = 0;
		}
		else if (b & 0x70)
		{
			b = 0x0F;
		}

		*pDst = (b | (*pSrc & 0xF0));

		++pSrc;
		++pDst;
	}

	PAL_ApplyWave(g_Battle.lpSceneBuf);

	memset(&enemyDrawSeq, -1, sizeof(enemyDrawSeq));
	// sort by y
	for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
		enemyDrawSeq[i] = i;
	for (i = 0; i < g_Battle.wMaxEnemyIndex; i++)
		for (j = i + 1; j < g_Battle.wMaxEnemyIndex; j++)
			if (PAL_Y(g_Battle.rgEnemy[i].pos) < PAL_Y(g_Battle.rgEnemy[j].pos)) {
				INT tmp = enemyDrawSeq[i];
				enemyDrawSeq[i] = enemyDrawSeq[j];
				enemyDrawSeq[j] = tmp;
			}

	//
	// Draw the enemies
	// 重绘敌方单位
	for (j = g_Battle.wMaxEnemyIndex; j >= 0; j--)
	{
		i = enemyDrawSeq[j];
		pos = g_Battle.rgEnemy[i].pos;

		if (g_Battle.rgEnemy[i].rgwStatus[kStatusConfused] > 0 &&
			g_Battle.rgEnemy[i].rgwStatus[kStatusSleep] == 0 &&
			g_Battle.rgEnemy[i].rgwStatus[kStatusParalyzed] == 0)
		{
			//
			// Enemy is confused
			// 若敌人为混乱状态
			pos = PAL_XY(PAL_X(pos) + RandomLong(-1, 1), PAL_Y(pos));
		}

		pos = PAL_XY(PAL_X(pos) - PAL_RLEGetWidth(PAL_SpriteGetFrame(g_Battle.rgEnemy[i].lpSprite, g_Battle.rgEnemy[i].wCurrentFrame)) / 2,
			PAL_Y(pos) - PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle.rgEnemy[i].lpSprite, g_Battle.rgEnemy[i].wCurrentFrame)));

		if (g_Battle.rgEnemy[i].wObjectID != 0)
		{
			if (g_Battle.rgEnemy[i].iColorShift)
			{
				// 更改敌方颜色偏移并更改敌方帧数、位置？
				PAL_RLEBlitWithColorShift(PAL_SpriteGetFrame(g_Battle.rgEnemy[i].lpSprite, g_Battle.rgEnemy[i].wCurrentFrame),
					g_Battle.lpSceneBuf, pos, g_Battle.rgEnemy[i].iColorShift);
			}
			else
			{
				// 更改敌方帧数、位置？
				PAL_RLEBlitToSurface(PAL_SpriteGetFrame(g_Battle.rgEnemy[i].lpSprite, g_Battle.rgEnemy[i].wCurrentFrame),
					g_Battle.lpSceneBuf, pos);
			}
		}
	}

	if (g_Battle.lpSummonSprite != NULL)
	{
		//
		// Draw the summoned god
		// 若我方召唤了神时，重绘召唤神
		//pos = PAL_XY(PAL_X(g_Battle.posSummon) - PAL_RLEGetWidth(PAL_SpriteGetFrame(g_Battle.lpSummonSprite, g_Battle.iSummonFrame)) / 2,
		//	PAL_Y(g_Battle.posSummon) - PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle.lpSummonSprite, g_Battle.iSummonFrame)));

		// 设置召唤神位置
		pos = PAL_XY(PAL_X(g_Battle.posSummon) - PAL_RLEGetWidth(PAL_New_DoubleMKFGetFrame(g_Battle.lpSummonSprite, g_Battle.iSummonChinkNum, g_Battle.iSummonFrame, gpGlobals->f.fpFGOD)) / 2,
			PAL_Y(g_Battle.posSummon) - PAL_RLEGetHeight(PAL_New_DoubleMKFGetFrame(g_Battle.lpSummonSprite, g_Battle.iSummonChinkNum, g_Battle.iSummonFrame, gpGlobals->f.fpFGOD)));

		// 原始数据读取形象帧
		//PAL_RLEBlitToSurface(PAL_SpriteGetFrame(g_Battle.lpSummonSprite, g_Battle.iSummonFrame),
		//	g_Battle.lpSceneBuf, pos);

		// 新版数据读取形象帧
		PAL_RLEBlitToSurface(PAL_New_DoubleMKFGetFrame(g_Battle.lpSummonSprite, g_Battle.iSummonChinkNum, g_Battle.iSummonFrame, gpGlobals->f.fpFGOD),
			g_Battle.lpSceneBuf, pos);

	}
	else
	{
		//
		// Draw the players
		// 若我方未召唤了神时，重绘队员
		for (i = gpGlobals->wMaxPartyMemberIndex; i >= 0; i--)
		{
			pos = g_Battle.rgPlayer[i].pos;

			if (gpGlobals->rgPlayerStatus[gpGlobals->rgParty[i].wPlayerRole][kStatusConfused] != 0 &&
				gpGlobals->rgPlayerStatus[gpGlobals->rgParty[i].wPlayerRole][kStatusSleep] == 0 &&
				gpGlobals->rgPlayerStatus[gpGlobals->rgParty[i].wPlayerRole][kStatusParalyzed] == 0 &&
				gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[i].wPlayerRole] > 0)
			{
				//
				// Player is confused
				//
				continue;
			}

			pos = PAL_XY(PAL_X(pos) - PAL_RLEGetWidth(PAL_SpriteGetFrame(g_Battle.rgPlayer[i].lpSprite, g_Battle.rgPlayer[i].wCurrentFrame)) / 2,
				PAL_Y(pos) - PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle.rgPlayer[i].lpSprite, g_Battle.rgPlayer[i].wCurrentFrame)));

			if (g_Battle.rgPlayer[i].iColorShift != 0)
			{
				PAL_RLEBlitWithColorShift(PAL_SpriteGetFrame(g_Battle.rgPlayer[i].lpSprite, g_Battle.rgPlayer[i].wCurrentFrame),
					g_Battle.lpSceneBuf, pos, g_Battle.rgPlayer[i].iColorShift);
			}
			else if (g_Battle.iHidingTime == 0)
			{
				PAL_RLEBlitToSurface(PAL_SpriteGetFrame(g_Battle.rgPlayer[i].lpSprite, g_Battle.rgPlayer[i].wCurrentFrame),
					g_Battle.lpSceneBuf, pos);
			}
		}

		//
		// Confused players should be drawn on top of normal players
		// 混乱状态的玩家（图层）应该被画在正常玩家之上
		for (i = gpGlobals->wMaxPartyMemberIndex; i >= 0; i--)
		{
			if (gpGlobals->rgPlayerStatus[gpGlobals->rgParty[i].wPlayerRole][kStatusConfused] != 0 &&
				gpGlobals->rgPlayerStatus[gpGlobals->rgParty[i].wPlayerRole][kStatusSleep] == 0 &&
				gpGlobals->rgPlayerStatus[gpGlobals->rgParty[i].wPlayerRole][kStatusParalyzed] == 0 &&
				gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[i].wPlayerRole] > 0)
			{
				//
				// Player is confused
				//
				int xd = PAL_X(g_Battle.rgPlayer[i].pos), yd = PAL_Y(g_Battle.rgPlayer[i].pos);
				if (!PAL_IsPlayerDying(gpGlobals->rgParty[i].wPlayerRole))
					yd += RandomLong(-1, 1);
				pos = PAL_XY(xd, yd);
				pos = PAL_XY(PAL_X(pos) - PAL_RLEGetWidth(PAL_SpriteGetFrame(g_Battle.rgPlayer[i].lpSprite, g_Battle.rgPlayer[i].wCurrentFrame)) / 2,
					PAL_Y(pos) - PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle.rgPlayer[i].lpSprite, g_Battle.rgPlayer[i].wCurrentFrame)));

				if (g_Battle.rgPlayer[i].iColorShift != 0)
				{
					PAL_RLEBlitWithColorShift(PAL_SpriteGetFrame(g_Battle.rgPlayer[i].lpSprite, g_Battle.rgPlayer[i].wCurrentFrame),
						g_Battle.lpSceneBuf, pos, g_Battle.rgPlayer[i].iColorShift);
				}
				else if (g_Battle.iHidingTime == 0)
				{
					PAL_RLEBlitToSurface(PAL_SpriteGetFrame(g_Battle.rgPlayer[i].lpSprite, g_Battle.rgPlayer[i].wCurrentFrame),
						g_Battle.lpSceneBuf, pos);
				}
			}
		}
	}
}

VOID
PAL_BattleFadeScene(
	VOID
)
/*++
  Purpose:

	Fade in the scene of battle.
	淡出战斗场景。

  Parameters:

	None.

  Return value:

	None.

--*/
{
	int               i, j, k;
	DWORD             time;
	BYTE              a, b;
	const int         rgIndex[6] = { 0, 3, 1, 5, 2, 4 };

	time = SDL_GetTicks();

	for (i = 0; i < 12; i++)
	{
		for (j = 0; j < 6; j++)
		{
			PAL_DelayUntil(time);
			time = SDL_GetTicks() + 16;

			//
			// Blend the pixels in the 2 buffers, and put the result into the
			// backup buffer
			// 混合2个缓冲区中的像素，并将结果放入备用缓冲器
			for (k = rgIndex[j]; k < gpScreen->pitch * gpScreen->h; k += 6)
			{
				a = ((LPBYTE)(g_Battle.lpSceneBuf->pixels))[k];
				b = ((LPBYTE)(gpScreenBak->pixels))[k];

				if (i > 0)
				{
					if ((a & 0x0F) > (b & 0x0F))
					{
						b++;
					}
					else if ((a & 0x0F) < (b & 0x0F))
					{
						b--;
					}
				}

				((LPBYTE)(gpScreenBak->pixels))[k] = ((a & 0xF0) | (b & 0x0F));
			}

			//
			// Draw the backup buffer to the screen
			// 将备份缓冲区绘制到屏幕
			VIDEO_RestoreScreen(gpScreen);

			PAL_BattleUIUpdate();
			VIDEO_UpdateScreen(NULL);
		}
	}

	//
	// Draw the result buffer to the screen as the final step
	// 将结果缓冲区绘制到屏幕上作为最后一步
	VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);
	PAL_BattleUIUpdate();

	VIDEO_UpdateScreen(NULL);
}

static BATTLERESULT
PAL_BattleMain(
	VOID
)
/*++
  Purpose:

	The main battle routine.
	主要的战斗程序。

  Parameters:

	None.

  Return value:

	The result of the battle.

--*/
{
	int         i;
	DWORD       dwTime;

	VIDEO_BackupScreen(gpScreen);

	//
	// Generate the scene and draw the scene to the screen buffer
	// 生成场景并将场景绘制到屏幕缓冲区
	PAL_BattleMakeScene();
	VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);

	//
	// Fade out the music and delay for a while
	// 淡出音乐并延迟一段时间
	AUDIO_PlayMusic(0, FALSE, 1);
	UTIL_Delay(200);

	//
	// Switch the screen
	// 切换屏幕
	VIDEO_SwitchScreen(5);

	//
	// Play the battle music
	// 播放战斗音乐
	AUDIO_PlayMusic(gpGlobals->wNumBattleMusic, TRUE, 0);

	//
	// Fade in the screen when needed
	// 需要时淡出屏幕
	if (gpGlobals->fNeedToFadeIn)
	{
		PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
		gpGlobals->fNeedToFadeIn = FALSE;
	}

	//
	// Run the pre-battle scripts for each enemies
	// 为每个敌人运行战斗前脚本
	for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
	{
		g_Battle.rgEnemy[i].wScriptOnTurnStart =
			PAL_RunTriggerScript(g_Battle.rgEnemy[i].wScriptOnTurnStart, i, FALSE);

		if (g_Battle.BattleResult != kBattleResultPreBattle)
		{
			break;
		}
	}

	if (g_Battle.BattleResult == kBattleResultPreBattle)
	{
		g_Battle.BattleResult = kBattleResultOnGoing;
	}

#ifndef PAL_CLASSIC
	PAL_UpdateTimeChargingUnit();
#endif

	dwTime = SDL_GetTicks();

	PAL_ClearKeyState();

	//
	// Run the main battle loop.
	// 运行主战斗循环
	while (TRUE)
	{
		//
		// Break out if the battle ended.
		// 战斗结束后跳出
		if (g_Battle.BattleResult != kBattleResultOnGoing)
		{
			break;
		}

		//
		// Wait for the time of one frame. Accept input here.
		// 等待一帧的时间。在此接受输入。
		PAL_DelayUntil(dwTime);

		//
		// Set the time of the next frame.
		// 设置下一帧的时间。
		dwTime = SDL_GetTicks() + BATTLE_FRAME_TIME;

		//
		// Run the main frame routine.
		// 运行主机架例程。
		PAL_BattleStartFrame();

		//
		// Update the screen.
		// 更新屏幕
		VIDEO_UpdateScreen(NULL);
	}

	//
	// Return the battle result
	// 返回战斗结果
	return g_Battle.BattleResult;
}

static VOID
PAL_FreeBattleSprites(
	VOID
)
/*++
  Purpose:

	Free all the loaded sprites.
	释放所有加载的精灵。

  Parameters:

	None.

  Return value:

	None.

--*/
{
	int         i;

	//
	// Free all the loaded sprites
	//
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		if (g_Battle.rgPlayer[i].lpSprite != NULL)
		{
			free(g_Battle.rgPlayer[i].lpSprite);
		}
		g_Battle.rgPlayer[i].lpSprite = NULL;
	}

	for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
	{
		if (g_Battle.rgEnemy[i].lpSprite != NULL)
		{
			free(g_Battle.rgEnemy[i].lpSprite);
		}
		g_Battle.rgEnemy[i].lpSprite = NULL;
	}

	if (g_Battle.lpSummonSprite != NULL)
	{
		free(g_Battle.lpSummonSprite);
	}
	g_Battle.lpSummonSprite = NULL;
}

VOID
PAL_LoadBattleSprites(
	VOID
)
/*++
  Purpose:

	Load all the loaded sprites.
	加载所有加载的精灵。

  Parameters:

	None.

  Return value:

	None.

--*/
{
	int           i, l, x, y, s;
	FILE* fp;

	PAL_FreeBattleSprites();

	fp = UTIL_OpenRequiredFile("abc.mkf");

	//
	// Load battle sprites for players
	// 加载我方队员战斗形象
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		s = PAL_GetPlayerBattleSprite(gpGlobals->rgParty[i].wPlayerRole);

		l = PAL_MKFGetDecompressedSize(s, gpGlobals->f.fpF);

		if (l <= 0)
		{
			continue;
		}

		g_Battle.rgPlayer[i].lpSprite = UTIL_calloc(l, 1);

		PAL_MKFDecompressChunk(g_Battle.rgPlayer[i].lpSprite, l, s, gpGlobals->f.fpF);

		//
		// Set the default position for this player
		// 设置此队员的默认位置
		x = g_rgPlayerPos[gpGlobals->wMaxPartyMemberIndex][i][0];
		y = g_rgPlayerPos[gpGlobals->wMaxPartyMemberIndex][i][1];

		g_Battle.rgPlayer[i].posOriginal = PAL_XY(x, y);
		g_Battle.rgPlayer[i].pos = PAL_XY(x, y);
	}

	//
	// Load battle sprites for enemies
	// 加载敌方单位战斗形象
	for (i = 0; i < MAX_ENEMIES_IN_TEAM; i++)
	{
		if (g_Battle.rgEnemy[i].wObjectID == 0)
		{
			continue;
		}

		l = PAL_MKFGetDecompressedSize(
			gpGlobals->g.rgObject[g_Battle.rgEnemy[i].wObjectID].enemy.wEnemyID, fp);

		if (l <= 0)
		{
			continue;
		}

		g_Battle.rgEnemy[i].lpSprite = UTIL_calloc(l, 1);

		PAL_MKFDecompressChunk(g_Battle.rgEnemy[i].lpSprite, l,
			gpGlobals->g.rgObject[g_Battle.rgEnemy[i].wObjectID].enemy.wEnemyID, fp);

		//
		// Set the default position for this enemy
		// 设置此敌人的默认位置
		x = gpGlobals->g.EnemyPos.pos[i][g_Battle.wMaxEnemyIndex].x;
		y = gpGlobals->g.EnemyPos.pos[i][g_Battle.wMaxEnemyIndex].y;

		y += g_Battle.rgEnemy[i].e.wYPosOffset;

		g_Battle.rgEnemy[i].posOriginal = PAL_XY(x, y);
		g_Battle.rgEnemy[i].pos = PAL_XY(x, y);
	}

	fclose(fp);
}

static VOID
PAL_LoadBattleBackground(
	VOID
)
/*++
  Purpose:

	Load the screen background picture of the battle.
	加载战斗的屏幕背景图片。

  Parameters:

	None.

  Return value:

	None.

--*/
{
	PAL_LARGE BYTE           buf[320 * 200];

	//
	// Create the surface
	//
	g_Battle.lpBackground = VIDEO_CreateCompatibleSurface(gpScreen);

	if (g_Battle.lpBackground == NULL)
	{
		TerminateOnError("PAL_LoadBattleBackground(): failed to create surface!");
	}

	//
	// Load the picture
	//
	PAL_MKFDecompressChunk(buf, 320 * 200, gpGlobals->wNumBattleField, gpGlobals->f.fpFBP);

	//
	// Draw the picture to the surface.
	//
	PAL_FBPBlitToSurface(buf, g_Battle.lpBackground);
}

static VOID
PAL_BattleWon(
	VOID
)
/*++
  Purpose:

	Show the "you win" message and add the experience points for players.
	显示“你赢了”信息并为队员添加经验点。

  Parameters:

	None.

  Return value:

	None.

--*/
{
	const SDL_Rect   rect = { 0, 45, 320, 100 };
	SDL_Rect   rect1 = { 80, 0, 180, 200 };

	int              i, j, iTotalCount;
	DWORD            dwExp;
	WORD             w;
	BOOL             fLevelUp;
	PLAYERROLES      OrigPlayerRoles;

	//
	// Backup the initial player stats
	// 备份结算前玩家统计数据
	OrigPlayerRoles = gpGlobals->g.PlayerRoles;

	VIDEO_BackupScreen(gpScreen);

	if (g_Battle.iExpGained > 0)
	{
		int w1 = PAL_WordWidth(BATTLEWIN_GETEXP_LABEL) + 3;
		int ww1 = (w1 - 8) << 3;
		//
		// Play the "battle win" music
		// 播放战斗胜利音乐
		AUDIO_PlayMusic(g_Battle.fIsBoss ? 2 : 3, FALSE, 0);

		//
		// Show the message about the total number of exp. and cash gained
		// 显示获得的经验值、金钱数目
		PAL_CreateSingleLineBox(PAL_XY(83 - ww1, 60), w1, FALSE);
		PAL_CreateSingleLineBox(PAL_XY(65, 105), 10, FALSE);

		PAL_DrawText(PAL_GetWord(BATTLEWIN_GETEXP_LABEL), PAL_XY(95 - ww1, 70), 0X0F, FALSE, FALSE, FALSE);
		PAL_DrawText(PAL_GetWord(BATTLEWIN_BEATENEMY_LABEL), PAL_XY(77, 115), 0X0F, FALSE, FALSE, FALSE);
		PAL_DrawText(PAL_GetWord(BATTLEWIN_DOLLAR_LABEL), PAL_XY(197, 115), 0X0F, FALSE, FALSE, FALSE);

		PAL_DrawNumber(g_Battle.iExpGained, 5, PAL_XY(182 + ww1, 74), kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(g_Battle.iCashGained, 5, PAL_XY(162, 119), kNumColorGold, kNumAlignMid);

		VIDEO_UpdateScreen(&rect);
		PAL_WaitForKey(g_Battle.fIsBoss ? 5500 : 3000);
	}

	//
	// Add the cash value
	// 增加金钱
	gpGlobals->dwCash += g_Battle.iCashGained;

	if (g_Battle.iCollectValue > 0)
	{
		int w2 = PAL_WordWidth(LABEL_COLLECT_VALUE) + 3;
		int ww2 = (w2 - 8) << 3;

		//
		// Show the message about the total number of exp. and cash gained
		// 显示获得的灵葫值
		PAL_CreateSingleLineBox(PAL_XY(83 - ww2, 45), w2, FALSE);
		PAL_DrawText(PAL_GetWord(LABEL_COLLECT_VALUE), PAL_XY(95 - ww2, 55), 0X0F, FALSE, FALSE, FALSE);
		PAL_DrawNumber(g_Battle.iCollectValue, 5, PAL_XY(182 + ww2, 59), kNumColorCyan, kNumAlignRight);

		VIDEO_UpdateScreen(&rect);
		PAL_WaitForKey(g_Battle.fIsBoss ? 5500 : 3000);
	}

	//
	// 增加灵葫值
	//
	gpGlobals->wCollectValue += g_Battle.iCollectValue;


	const MENUITEM      rgFakeMenuItem[] =
	{
		// value  label                        enabled   pos
		{ 1,      gpGlobals->g.PlayerRoles.rgwName[0],    TRUE,     PAL_XY(0, 0) },
		{ 2,      gpGlobals->g.PlayerRoles.rgwName[1],    TRUE,     PAL_XY(0, 0) },
		{ 3,      gpGlobals->g.PlayerRoles.rgwName[2],    TRUE,     PAL_XY(0, 0) },
		{ 4,      gpGlobals->g.PlayerRoles.rgwName[3],    TRUE,     PAL_XY(0, 0) },
		{ 5,      gpGlobals->g.PlayerRoles.rgwName[4],    TRUE,     PAL_XY(0, 0) },
		{ 6,      gpGlobals->g.PlayerRoles.rgwName[5],    TRUE,     PAL_XY(0, 0) },
	};
	int maxNameWidth = PAL_MenuTextMaxWidth(rgFakeMenuItem, sizeof(rgFakeMenuItem) / sizeof(MENUITEM));
	const MENUITEM      rgFakeMenuItem2[] =
	{
		// value  label                        enabled   pos
		{ 1,      STATUS_LABEL_LEVEL,          TRUE,     PAL_XY(0, 0) },
		{ 2,      STATUS_LABEL_HP,             TRUE,     PAL_XY(0, 0) },
		{ 3,      STATUS_LABEL_MP,             TRUE,     PAL_XY(0, 0) },
		{ 4,      STATUS_LABEL_SP,             TRUE,     PAL_XY(0, 0) },
		{ 5,      STATUS_LABEL_ATTACKPOWER,    TRUE,     PAL_XY(0, 0) },
		{ 6,      STATUS_LABEL_MAGICPOWER,     TRUE,     PAL_XY(0, 0) },
		{ 7,      STATUS_LABEL_RESISTANCE,     TRUE,     PAL_XY(0, 0) },
		{ 8,      STATUS_LABEL_DEXTERITY,      TRUE,     PAL_XY(0, 0) },
		{ 9,      STATUS_LABEL_FLEERATE,       TRUE,     PAL_XY(0, 0) },
	};
	int maxPropertyWidth = PAL_MenuTextMaxWidth(rgFakeMenuItem2, sizeof(rgFakeMenuItem2) / sizeof(MENUITEM)) - 1;
	int propertyLength = maxPropertyWidth - 1;
	int offsetX = -8 * propertyLength;
	rect1.x += offsetX;
	rect1.w -= 2 * offsetX;

	//
	// Add the experience points for each players
	// 为每个队员添加经验点
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		fLevelUp = FALSE;

		w = gpGlobals->rgParty[i].wPlayerRole;
		if (gpGlobals->g.PlayerRoles.rgwHP[w] == 0)
		{
			continue; // don't care about dead players 不要为死亡队员增加经验值
		}

		dwExp = gpGlobals->Exp.rgPrimaryExp[w].wExp;
		dwExp += g_Battle.iExpGained;

		if (gpGlobals->g.PlayerRoles.rgwLevel[w] > MAX_LEVELS)
		{
			gpGlobals->g.PlayerRoles.rgwLevel[w] = MAX_LEVELS;
		}

		while (dwExp >= gpGlobals->g.rgLevelUpExp[gpGlobals->g.PlayerRoles.rgwLevel[w]])
		{
			dwExp -= gpGlobals->g.rgLevelUpExp[gpGlobals->g.PlayerRoles.rgwLevel[w]];

			if (gpGlobals->g.PlayerRoles.rgwLevel[w] < MAX_LEVELS)
			{
				fLevelUp = TRUE;
				PAL_PlayerLevelUp(w, 1);

				gpGlobals->g.PlayerRoles.rgwHP[w] = gpGlobals->g.PlayerRoles.rgwMaxHP[w];
				gpGlobals->g.PlayerRoles.rgwMP[w] = gpGlobals->g.PlayerRoles.rgwMaxMP[w];

				// 精力恢复满
				gpGlobals->g.PlayerRoles.rgwSP[w] = gpGlobals->g.PlayerRoles.rgwMaxSP[w];
			}
		}

		gpGlobals->Exp.rgPrimaryExp[w].wExp = (WORD)dwExp;

		if (fLevelUp)
		{
			VIDEO_RestoreScreen(gpScreen);
			//
			// Player has gained a level. Show the message
			// 队员修行晋，显示具体数据变化信息
			PAL_CreateSingleLineBox(PAL_XY(offsetX + 80, 0), propertyLength + 10, FALSE);
			PAL_CreateBox(PAL_XY(offsetX + 82, 32), 7, propertyLength + 8, 1, FALSE);

			WCHAR buffer[256] = L"";
			PAL_swprintf(buffer, sizeof(buffer) / sizeof(WCHAR), L"%ls%ls%ls", PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]), PAL_GetWord(STATUS_LABEL_LEVEL), PAL_GetWord(BATTLEWIN_LEVELUP_LABEL));
			PAL_DrawText(buffer, PAL_XY(110, 10), 0, FALSE, FALSE, FALSE);

			// 显示表示变化的右箭头
			for (j = 0; j < sizeof(rgFakeMenuItem2) / sizeof(MENUITEM); j++)
			{
				PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ARROW),
					gpScreen, PAL_XY(-offsetX + 181, 40 + 18 * j));
			}

			j = 0;

			PAL_DrawText(PAL_GetWord(STATUS_LABEL_LEVEL), PAL_XY(offsetX + 90, 35 + 18 * j++), BATTLEWIN_LEVELUP_LABEL_COLOR, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(STATUS_LABEL_HP), PAL_XY(offsetX + 90, 35 + 18 * j++), BATTLEWIN_LEVELUP_LABEL_COLOR, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(STATUS_LABEL_MP), PAL_XY(offsetX + 90, 35 + 18 * j++), BATTLEWIN_LEVELUP_LABEL_COLOR, TRUE, FALSE, FALSE);

			// 显示精力增长标签
			PAL_DrawText(PAL_GetWord(STATUS_LABEL_SP), PAL_XY(offsetX + 90, 35 + 18 * j++), BATTLEWIN_LEVELUP_LABEL_COLOR, TRUE, FALSE, FALSE);

			PAL_DrawText(PAL_GetWord(STATUS_LABEL_ATTACKPOWER), PAL_XY(offsetX + 90, 35 + 18 * j++), BATTLEWIN_LEVELUP_LABEL_COLOR, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(STATUS_LABEL_MAGICPOWER), PAL_XY(offsetX + 90, 35 + 18 * j++), BATTLEWIN_LEVELUP_LABEL_COLOR, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(STATUS_LABEL_RESISTANCE), PAL_XY(offsetX + 90, 35 + 18 * j++), BATTLEWIN_LEVELUP_LABEL_COLOR, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(STATUS_LABEL_DEXTERITY), PAL_XY(offsetX + 90, 35 + 18 * j++), BATTLEWIN_LEVELUP_LABEL_COLOR, TRUE, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(STATUS_LABEL_FLEERATE), PAL_XY(offsetX + 90, 35 + 18 * j++), BATTLEWIN_LEVELUP_LABEL_COLOR, TRUE, FALSE, FALSE);

			int sbsbs = 7;
			int sbsbsb = 8;

			j = 0;

			//
			// Draw the original stats and stats after level up
			// 绘制原始统计数据和升级后的统计数据
			PAL_DrawNumber(OrigPlayerRoles.rgwLevel[w], 4, PAL_XY(-offsetX + 126, 39 + 18 * j),
				kNumColorYellow, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwLevel[w], 4, PAL_XY(-offsetX + 203, 39 + 18 * j++),
				kNumColorYellow, kNumAlignRight);

			j = 0;

			PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen,
				PAL_XY(-offsetX + 149, 57 + 18 * j));
			PAL_DrawNumber(OrigPlayerRoles.rgwHP[w], 5, PAL_XY(-offsetX + 120, 55 + 18 * j),
				kNumColorYellow, kNumAlignRight);
			PAL_DrawNumber(OrigPlayerRoles.rgwMaxHP[w], 5, PAL_XY(-offsetX + 148, 59 + 18 * j),
				kNumColorBlue, kNumAlignRight);
			PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen,
				PAL_XY(-offsetX + 219, 57 + 18 * j));
			PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwHP[w], 5, PAL_XY(-offsetX + 190, 55 + 18 * j),
				kNumColorYellow, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxHP[w], 5, PAL_XY(-offsetX + 218, 59 + 18 * j++),
				kNumColorBlue, kNumAlignRight);

			PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen,
				PAL_XY(-offsetX + 149, 57 + 18 * j));
			PAL_DrawNumber(OrigPlayerRoles.rgwMP[w], 5, PAL_XY(-offsetX + 120, 55 + 18 * j),
				kNumColorYellow, kNumAlignRight);
			PAL_DrawNumber(OrigPlayerRoles.rgwMaxMP[w], 5, PAL_XY(-offsetX + 148, 59 + 18 * j),
				kNumColorBlue, kNumAlignRight);
			PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen,
				PAL_XY(-offsetX + 219, 57 + 18 * j));
			PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMP[w], 5, PAL_XY(-offsetX + 190, 55 + 18 * j),
				kNumColorYellow, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxMP[w], 5, PAL_XY(-offsetX + 218, 59 + 18 * j++),
				kNumColorBlue, kNumAlignRight);

			// 精力变化值
			PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen,
				PAL_XY(-offsetX + 149, 57 + 18 * j));
			PAL_DrawNumber(OrigPlayerRoles.rgwSP[w], 5, PAL_XY(-offsetX + 120, 55 + 18 * j),
				kNumColorYellow, kNumAlignRight);
			PAL_DrawNumber(OrigPlayerRoles.rgwMaxSP[w], 5, PAL_XY(-offsetX + 148, 59 + 18 * j),
				kNumColorBlue, kNumAlignRight);
			PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen,
				PAL_XY(-offsetX + 219, 57 + 18 * j));
			PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwSP[w], 5, PAL_XY(-offsetX + 190, 55 + 18 * j),
				kNumColorYellow, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxSP[w], 5, PAL_XY(-offsetX + 218, 59 + 18 * j++),
				kNumColorBlue, kNumAlignRight);

			j = 0;

			PAL_DrawNumber(OrigPlayerRoles.rgwAttackStrength[w] + PAL_GetPlayerAttackStrength(w) -
				gpGlobals->g.PlayerRoles.rgwAttackStrength[w],
				6, PAL_XY(-offsetX + 133, 111 + 18 * j), kNumColorYellow, kNumAlignLeft);
			PAL_DrawNumber(PAL_GetPlayerAttackStrength(w), 6, PAL_XY(-offsetX + 209, 111 + 18 * j++),
				kNumColorYellow, kNumAlignLeft);

			PAL_DrawNumber(OrigPlayerRoles.rgwMagicStrength[w] + PAL_GetPlayerMagicStrength(w) -
				gpGlobals->g.PlayerRoles.rgwMagicStrength[w],
				6, PAL_XY(-offsetX + 133, 111 + 18 * j), kNumColorYellow, kNumAlignLeft);
			PAL_DrawNumber(PAL_GetPlayerMagicStrength(w), 6, PAL_XY(-offsetX + 209, 111 + 18 * j++),
				kNumColorYellow, kNumAlignLeft);

			PAL_DrawNumber(OrigPlayerRoles.rgwDefense[w] + PAL_GetPlayerDefense(w) -
				gpGlobals->g.PlayerRoles.rgwDefense[w],
				6, PAL_XY(-offsetX + 133, 111 + 18 * j), kNumColorYellow, kNumAlignLeft);
			PAL_DrawNumber(PAL_GetPlayerDefense(w), 6, PAL_XY(-offsetX + 209, 111 + 18 * j++),
				kNumColorYellow, kNumAlignLeft);

			PAL_DrawNumber(OrigPlayerRoles.rgwDexterity[w] + PAL_GetPlayerDexterity(w) -
				gpGlobals->g.PlayerRoles.rgwDexterity[w],
				6, PAL_XY(-offsetX + 133, 111 + 18 * j), kNumColorYellow, kNumAlignLeft);
			PAL_DrawNumber(PAL_GetPlayerDexterity(w), 6, PAL_XY(-offsetX + 209, 111 + 18 * j++),
				kNumColorYellow, kNumAlignLeft);

			PAL_DrawNumber(OrigPlayerRoles.rgwFleeRate[w] + PAL_GetPlayerFleeRate(w) -
				gpGlobals->g.PlayerRoles.rgwFleeRate[w],
				6, PAL_XY(-offsetX + 133, 111 + 18 * j), kNumColorYellow, kNumAlignLeft);
			PAL_DrawNumber(PAL_GetPlayerFleeRate(w), 6, PAL_XY(-offsetX + 209, 111 + 18 * j++),
				kNumColorYellow, kNumAlignLeft);

			//
			// Update the screen and wait for key
			// 更新屏幕并等待按键
			VIDEO_UpdateScreen(&rect1);
			PAL_WaitForKey(3000);

			OrigPlayerRoles = gpGlobals->g.PlayerRoles;
		}

		//
		// Increasing of other hidden levels
		// 增加其他隐藏等级
		iTotalCount = 0;

		iTotalCount += gpGlobals->Exp.rgHealthExp[w].wCount;
		iTotalCount += gpGlobals->Exp.rgMagicExp[w].wCount;
		iTotalCount += gpGlobals->Exp.rgUniqueSkillExp[w].wCount;
		iTotalCount += gpGlobals->Exp.rgAttackExp[w].wCount;
		iTotalCount += gpGlobals->Exp.rgMagicPowerExp[w].wCount;
		iTotalCount += gpGlobals->Exp.rgDefenseExp[w].wCount;
		iTotalCount += gpGlobals->Exp.rgDexterityExp[w].wCount;
		iTotalCount += gpGlobals->Exp.rgFleeExp[w].wCount;

		if (iTotalCount > 0)
		{
#define CHECK_HIDDEN_EXP(expname, statname, label)          \
{                                                           \
   dwExp = g_Battle.iExpGained;                             \
   dwExp *= gpGlobals->Exp.expname[w].wCount;               \
   dwExp /= iTotalCount;                                    \
   dwExp *= 2;                                              \
                                                            \
   dwExp += gpGlobals->Exp.expname[w].wExp;                 \
                                                            \
   if (gpGlobals->Exp.expname[w].wLevel > MAX_LEVELS)       \
   {                                                        \
      gpGlobals->Exp.expname[w].wLevel = MAX_LEVELS;        \
   }                                                        \
                                                            \
   while (dwExp >= gpGlobals->g.rgLevelUpExp[gpGlobals->Exp.expname[w].wLevel]) \
   {                                                        \
      dwExp -= gpGlobals->g.rgLevelUpExp[gpGlobals->Exp.expname[w].wLevel]; \
      gpGlobals->g.PlayerRoles.statname[w] += RandomLong(1, 2); \
      if (gpGlobals->Exp.expname[w].wLevel < MAX_LEVELS)    \
      {                                                     \
         gpGlobals->Exp.expname[w].wLevel++;                \
      }                                                     \
   }                                                        \
                                                            \
   gpGlobals->Exp.expname[w].wExp = (WORD)dwExp;            \
                                                            \
   if (gpGlobals->g.PlayerRoles.statname[w] != OrigPlayerRoles.statname[w]) \
   {                                                        \
      WCHAR buffer[256] = L""; \
      PAL_swprintf(buffer, sizeof(buffer) / sizeof(WCHAR), L"%ls%ls%ls", PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]), PAL_GetWord(label), PAL_GetWord(BATTLEWIN_LEVELUP_LABEL)); \
      PAL_CreateSingleLineBox(PAL_XY(offsetX+78, 60), maxNameWidth+maxPropertyWidth+PAL_TextWidth(PAL_GetWord(BATTLEWIN_LEVELUP_LABEL))/32+4, FALSE);    \
      PAL_DrawText(buffer, PAL_XY(offsetX+90, 70),  0x0F, FALSE, FALSE, FALSE); \
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.statname[w] - OrigPlayerRoles.statname[w], 5, PAL_XY(183+(maxNameWidth+maxPropertyWidth-3)*8, 74), kNumColorYellow, kNumAlignRight); \
      VIDEO_UpdateScreen(&rect);                            \
      PAL_WaitForKey(3000);                                 \
   }                                                        \
}

			CHECK_HIDDEN_EXP(rgHealthExp, rgwMaxHP, STATUS_LABEL_HP);
			CHECK_HIDDEN_EXP(rgMagicExp, rgwMaxMP, STATUS_LABEL_MP);
			CHECK_HIDDEN_EXP(rgUniqueSkillExp, rgwMagicStrength, STATUS_LABEL_MAGICPOWER);
			CHECK_HIDDEN_EXP(rgAttackExp, rgwAttackStrength, STATUS_LABEL_ATTACKPOWER);
			CHECK_HIDDEN_EXP(rgMagicPowerExp, rgwMagicStrength, STATUS_LABEL_MAGICPOWER);
			CHECK_HIDDEN_EXP(rgDefenseExp, rgwDefense, STATUS_LABEL_RESISTANCE);
			CHECK_HIDDEN_EXP(rgDexterityExp, rgwDexterity, STATUS_LABEL_DEXTERITY);
			CHECK_HIDDEN_EXP(rgFleeExp, rgwFleeRate, STATUS_LABEL_FLEERATE);

#undef CHECK_HIDDEN_EXP

			//
			// Avoid HP/MP out of sync with upgraded maxHP/MP
			// 避免HP/MP与升级的maxHP/MP不同步，直接使体力真气回满
			if (fLevelUp) {
				// 避免超出数值限制
				PAL_PlayerLevelUp(w, 0);
				gpGlobals->g.PlayerRoles.rgwHP[w] = gpGlobals->g.PlayerRoles.rgwMaxHP[w];
				gpGlobals->g.PlayerRoles.rgwMP[w] = gpGlobals->g.PlayerRoles.rgwMaxMP[w];
				gpGlobals->g.PlayerRoles.rgwSP[w] = gpGlobals->g.PlayerRoles.rgwMaxSP[w];
			}
		}

#if defined(PAL_NEW_ROOLMAGIC)
		// 若随机仙术开关关闭
		// Learn all magics at the current level
		// 随机习得仙术，获取我方仙术个数

		INT iMagicNum = 0;

		for (j = 0; j < MAX_PLAYER_MAGICS; j++)
		{
			if (gpGlobals->g.PlayerRoles.rgwMagic[j][w] == 0)
				continue;

			iMagicNum++;
		}

		// HACK 随机习得仙术，每四级习得一个法术，限制学到32个仙术
		while (gpGlobals->g.PlayerRoles.rgwLevel[w] / 3 + 3 > iMagicNum && iMagicNum < 32)
		{
			// 生成随机数
			j = RandomLong(0, 106);

			// 仙术随机
			if (j == 0)
				j = 588;
			else if (j == 1)
				j = RandomLong(584, 585);
			else
				j = RandomLong(295, 397);

			// 禁止领悟的仙术：酒神、灵葫咒、梦蛇以及一气化九百等
			if (j == 379 || j == 381 || j == 295)
			{
				continue;
			}

			// 队员领悟仙术
			if (PAL_AddMagic(w, j))
			{
				// 领悟成功，所需仙术数量减一，若领悟失败则重置随机数
				iMagicNum++;

				int ww;
				int w1 = (ww = PAL_WordWidth(gpGlobals->g.PlayerRoles.rgwName[w])) > 3 ? ww : 3;
				int w2 = (ww = PAL_WordWidth(BATTLEWIN_ADDMAGIC_LABEL)) > 2 ? ww : 2;
				int w3 = (ww = PAL_WordWidth(j)) > 5 ? ww : 5;
				ww = (w1 + w2 + w3 - 10) << 3;

				PAL_CreateSingleLineBox(PAL_XY(65 - ww, 105), w1 + w2 + w3, FALSE);

				PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]), PAL_XY(75 - ww, 115), MENUITEM_COLOR_SELECTED_FIRST, FALSE, FALSE, FALSE);
				PAL_DrawText(PAL_GetWord(BATTLEWIN_ADDMAGIC_LABEL), PAL_XY(75 + 16 * w1 - ww, 115), MENUITEM_COLOR, FALSE, FALSE, FALSE);
				PAL_DrawText(PAL_GetWord(j), PAL_XY(75 + 16 * (w1 + w2) - ww, 115), 0x1B, FALSE, FALSE, FALSE);

				VIDEO_UpdateScreen(&rect);
				PAL_WaitForKey(3000);
			}

		}
#else
		// 若随机仙术开关关闭
		// Learn all magics at the current level
		// 学习当前级别的所有魔法
		j = 0;

		while (j < gpGlobals->g.nLevelUpMagic)
		{
			if (gpGlobals->g.lprgLevelUpMagic[j].m[w].wMagic == 0 ||
				gpGlobals->g.lprgLevelUpMagic[j].m[w].wLevel > gpGlobals->g.PlayerRoles.rgwLevel[w])
			{
				j++;
				continue;
			}

			if (PAL_AddMagic(w, gpGlobals->g.lprgLevelUpMagic[j].m[w].wMagic))
			{
				int ww;
				int w1 = (ww = PAL_WordWidth(gpGlobals->g.PlayerRoles.rgwName[w])) > 3 ? ww : 3;
				int w2 = (ww = PAL_WordWidth(BATTLEWIN_ADDMAGIC_LABEL)) > 2 ? ww : 2;
				int w3 = (ww = PAL_WordWidth(gpGlobals->g.lprgLevelUpMagic[j].m[w].wMagic)) > 5 ? ww : 5;
				ww = (w1 + w2 + w3 - 10) << 3;

				PAL_CreateSingleLineBox(PAL_XY(65 - ww, 105), w1 + w2 + w3, FALSE);

				PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]), PAL_XY(75 - ww, 115), MENUITEM_COLOR_SELECTED_FIRST, FALSE, FALSE, FALSE);
				PAL_DrawText(PAL_GetWord(BATTLEWIN_ADDMAGIC_LABEL), PAL_XY(75 + 16 * w1 - ww, 115), MENUITEM_COLOR, FALSE, FALSE, FALSE);
				PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[j].m[w].wMagic), PAL_XY(75 + 16 * (w1 + w2) - ww, 115), 0x1B, FALSE, FALSE, FALSE);

				VIDEO_UpdateScreen(&rect);
				PAL_WaitForKey(3000);
			}

			j++;
		}
#endif
	}

	//
	// Run the post-battle scripts
	// 运行战斗后脚本
	for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
	{
		PAL_RunTriggerScript(g_Battle.rgEnemy[i].wScriptOnBattleEnd, i, FALSE);
	}

	//
	// Recover automatically after each battle
	//
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		w = gpGlobals->rgParty[i].wPlayerRole;

#if 1//def PAL_CLASSIC
		gpGlobals->g.PlayerRoles.rgwHP[w] +=
			(gpGlobals->g.PlayerRoles.rgwMaxHP[w] - gpGlobals->g.PlayerRoles.rgwHP[w]) / 2;
		gpGlobals->g.PlayerRoles.rgwMP[w] +=
			(gpGlobals->g.PlayerRoles.rgwMaxMP[w] - gpGlobals->g.PlayerRoles.rgwMP[w]) / 2;
		gpGlobals->g.PlayerRoles.rgwSP[w] +=
			(gpGlobals->g.PlayerRoles.rgwMaxSP[w] - gpGlobals->g.PlayerRoles.rgwSP[w]) / 2;
#else
		if (gpGlobals->g.PlayerRoles.rgwHP[w] == 0)
		{
			gpGlobals->g.PlayerRoles.rgwHP[w] = 1;
		}
		else if (g_Battle.iExpGained > 0)
		{
			FLOAT f =
				(gpGlobals->g.rgLevelUpExp[gpGlobals->g.PlayerRoles.rgwLevel[w]] / 5.0f) / g_Battle.iExpGained;

			if (f < 2)
			{
				f = 2;
			}

			gpGlobals->g.PlayerRoles.rgwHP[w] +=
				(gpGlobals->g.PlayerRoles.rgwMaxHP[w] - gpGlobals->g.PlayerRoles.rgwHP[w]) / f;
			gpGlobals->g.PlayerRoles.rgwMP[w] +=
				(gpGlobals->g.PlayerRoles.rgwMaxMP[w] - gpGlobals->g.PlayerRoles.rgwMP[w]) / f / 1.2;
		}
#endif
	}
}

VOID
PAL_BattleEnemyEscape(
	VOID
)
/*++
  Purpose:

	Enemy flee the battle.
	敌人逃离了战斗。

  Parameters:

	None.

  Return value:

	None.

--*/
{
	int j, x, y, w;
	BOOL f = TRUE;

	AUDIO_PlaySound(45);

	//
	// Show the animation
	//
	while (f)
	{
		f = FALSE;

		for (j = 0; j <= g_Battle.wMaxEnemyIndex; j++)
		{
			if (g_Battle.rgEnemy[j].wObjectID == 0)
			{
				continue;
			}

			x = PAL_X(g_Battle.rgEnemy[j].pos) - 5;
			y = PAL_Y(g_Battle.rgEnemy[j].pos);

			g_Battle.rgEnemy[j].pos = PAL_XY(x, y);

			w = PAL_RLEGetWidth(PAL_SpriteGetFrame(g_Battle.rgEnemy[j].lpSprite, 0));

			if (x + w > 0)
			{
				f = TRUE;
			}
		}

		PAL_BattleMakeScene();
		VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);
		VIDEO_UpdateScreen(NULL);

		UTIL_Delay(10);
	}

	UTIL_Delay(500);
	g_Battle.BattleResult = kBattleResultTerminated;
}

VOID
PAL_BattlePlayerEscape(
	VOID
)
/*++
  Purpose:

	Player flee the battle.
	我方逃离战斗。

  Parameters:

	None.

  Return value:

	None.

--*/
{
	int         i, j;
	WORD        wPlayerRole;

	AUDIO_PlaySound(45);

	PAL_BattleUpdateFighters();

	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

		if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] > 0)
		{
			g_Battle.rgPlayer[i].wCurrentFrame = 0;
		}
	}

	for (i = 0; i < 16; i++)
	{
		for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
		{
			wPlayerRole = gpGlobals->rgParty[j].wPlayerRole;

			if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] > 0)
			{
				//
				// TODO: This is still not the same as the original game
				// 这仍然与最初的游戏不同
				switch (j)
				{
				case 0:
					if (gpGlobals->wMaxPartyMemberIndex > 0)
					{
						g_Battle.rgPlayer[j].pos =
							PAL_XY(PAL_X(g_Battle.rgPlayer[j].pos) + 4,
								PAL_Y(g_Battle.rgPlayer[j].pos) + 6);
						break;
					}

				case 1:
					g_Battle.rgPlayer[j].pos =
						PAL_XY(PAL_X(g_Battle.rgPlayer[j].pos) + 4,
							PAL_Y(g_Battle.rgPlayer[j].pos) + 4);
					break;

				case 2:
					g_Battle.rgPlayer[j].pos =
						PAL_XY(PAL_X(g_Battle.rgPlayer[j].pos) + 6,
							PAL_Y(g_Battle.rgPlayer[j].pos) + 3);
					break;

				case 3:
					g_Battle.rgPlayer[j].pos =
						PAL_XY(PAL_X(g_Battle.rgPlayer[j].pos) + 6,
							PAL_Y(g_Battle.rgPlayer[j].pos) + 1);
					break;

				default:
					assert(FALSE); // Not possible
					break;
				}
			}
		}

		PAL_BattleDelay(1, 0, FALSE);
	}

	//
	// Remove all players from the screen
	// 从屏幕中删除所有玩家
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		g_Battle.rgPlayer[i].pos = PAL_XY(9999, 9999);
	}

	PAL_BattleDelay(1, 0, FALSE);

	g_Battle.BattleResult = kBattleResultFleed;
}

BATTLERESULT
PAL_StartBattle(
	WORD        wEnemyTeam,
	BOOL        fIsBoss
)
/*++
  Purpose:

	Start a battle.
	开始战斗。

  Parameters:

	[IN]  wEnemyTeam - the number of the enemy team.
	敌方队伍的编号。

	[IN]  fIsBoss - TRUE for boss fight (not allowed to flee).
	适用于boss战斗（不允许逃跑）。

  Return value:

	The result of the battle.
	战斗的结果。

--*/
{
	int            i;
	WORD           w, x, wPrevWaveLevel;
	SHORT          sPrevWaveProgression;

	// Debug 重新计数已过回合数，并清理上局残留的累计回合、伤害累计，将其置0，防止把上局的累计回合被此对局继承，
	g_Battle.rcmMagicType.wCurrentAllRrounds = 0;
	g_Battle.rcmMagicType.wMaxCumulativeRrounds = 0;
	g_Battle.rcmMagicType.wCurrentCumulativeRrounds = 0;
	g_Battle.rcmMagicType.wCumulativeDamageValue = 0;

	// 备份战斗前的我方数据
	gpGlobals->rgPreCombatPlayerRoles = gpGlobals->g.PlayerRoles;

	// 备份战斗前的库存道具
	for (i = 0; i < MAX_INVENTORY; i++)
	{
		gpGlobals->rgPreCombatInventory[i] = gpGlobals->rgInventory[i];
	}

	// 备份战斗前的所有游戏数据
	gpGlobals->gPreCombat = gpGlobals->g;

	//
	// Set the screen waving effects
	// 设置屏幕波动效果
	wPrevWaveLevel = gpGlobals->wScreenWave;
	sPrevWaveProgression = gpGlobals->sWaveProgression;

	gpGlobals->sWaveProgression = 0;
	gpGlobals->wScreenWave = gpGlobals->g.lprgBattleField[gpGlobals->wNumBattleField].wScreenWave;

	//
	// Make sure everyone in the party is alive, also clear all hidden
	// EXP count records
	// 确保队伍中的每个人都活着（复活所有阵亡队员1点体力），并清除所有隐藏的EXP计数记录
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		w = gpGlobals->rgParty[i].wPlayerRole;

		if (gpGlobals->g.PlayerRoles.rgwHP[w] == 0)
		{
			gpGlobals->g.PlayerRoles.rgwHP[w] = 1;
			gpGlobals->rgPlayerStatus[w][kStatusPuppet] = 0;
		}

		gpGlobals->Exp.rgHealthExp[w].wCount = 0;
		gpGlobals->Exp.rgMagicExp[w].wCount = 0;
		gpGlobals->Exp.rgUniqueSkillExp[w].wCount = 0;
		gpGlobals->Exp.rgAttackExp[w].wCount = 0;
		gpGlobals->Exp.rgMagicPowerExp[w].wCount = 0;
		gpGlobals->Exp.rgDefenseExp[w].wCount = 0;
		gpGlobals->Exp.rgDexterityExp[w].wCount = 0;
		gpGlobals->Exp.rgFleeExp[w].wCount = 0;
	}

	//
	// Clear all item-using records
	// 使用记录清除所有项目
	for (i = 0; i < MAX_INVENTORY; i++)
	{
		gpGlobals->rgInventory[i].nAmountInUse = 0;
	}

	//
	// Store all enemies
	// 使用记录清除所有项目
	for (i = 0; i < MAX_ENEMIES_IN_TEAM; i++)
	{
		memset(&(g_Battle.rgEnemy[i]), 0, sizeof(BATTLEENEMY));
		w = gpGlobals->g.lprgEnemyTeam[wEnemyTeam].rgwEnemy[i];

		if (w == 0xFFFF)
		{
			break;
		}

		if (w != 0)
		{
			g_Battle.rgEnemy[i].e = gpGlobals->g.lprgEnemy[gpGlobals->g.rgObject[w].enemy.wEnemyID];
			g_Battle.rgEnemy[i].wObjectID = w;
			g_Battle.rgEnemy[i].state = kFighterWait;
			g_Battle.rgEnemy[i].wScriptOnTurnStart = gpGlobals->g.rgObject[w].enemy.wScriptOnTurnStart;
			g_Battle.rgEnemy[i].wScriptOnBattleEnd = gpGlobals->g.rgObject[w].enemy.wScriptOnBattleEnd;
			g_Battle.rgEnemy[i].wScriptOnReady = gpGlobals->g.rgObject[w].enemy.wScriptOnReady;
			g_Battle.rgEnemy[i].iColorShift = 0;

			// 若这组怪被偷窃过了，则清空怪物身上的随身物品
			if ((&gpGlobals->g.lprgEventObject[g_Battle.wEventsThatCauseFighting])->fIsHasBeenStolen)
			{
				g_Battle.rgEnemy[i].e.wStealItem = 0;
			}

			// 从这里开始划分难度
			// 根据游戏难度为敌方单位加强 / 削弱属性
			//
			if (gpGlobals->wGameDifficulty == 1)
			{
				// 金钱收益削减
				g_Battle.rgEnemy[i].e.wCash /= 1.25;

				// 灵葫值收益削减
				g_Battle.rgEnemy[i].e.wCollectValue /= 1.25;

				// 可偷道具 / 金钱 收益削减
				g_Battle.rgEnemy[i].e.nStealItem /= 1.25;

				// 经验值收益削减
				g_Battle.rgEnemy[i].e.wExp /= 1.25;

				// 修行提升
				g_Battle.rgEnemy[i].e.wLevel *= 1.25;

				// 体力提升
				g_Battle.rgEnemy[i].e.wHealth *= 1.25;

				// 普攻附带概率提高
				g_Battle.rgEnemy[i].e.wAttackEquivItemRate *= 1.25;
				g_Battle.rgEnemy[i].e.wAttackEquivItemRate = min(g_Battle.rgEnemy[i].e.wAttackEquivItemRate, 100);

				// 五维提高
				if (g_Battle.rgEnemy[i].e.wAttackStrength > 0)
				{
					g_Battle.rgEnemy[i].e.wAttackStrength *= 1.25;
				}
				else
				{
					g_Battle.rgEnemy[i].e.wAttackStrength = 10 * (gpGlobals->wGameDifficulty + 1);
					g_Battle.rgEnemy[i].e.wAttackStrength *= 1.25;
				}

				if (g_Battle.rgEnemy[i].e.wMagicStrength > 0)
				{
					g_Battle.rgEnemy[i].e.wMagicStrength *= 1.25;
				}
				else
				{
					g_Battle.rgEnemy[i].e.wMagicStrength = 10 * (gpGlobals->wGameDifficulty + 1);
					g_Battle.rgEnemy[i].e.wMagicStrength *= 1.25;
				}

				if (g_Battle.rgEnemy[i].e.wDefense > 0)
				{
					g_Battle.rgEnemy[i].e.wDefense *= 1.25;
				}
				else
				{
					g_Battle.rgEnemy[i].e.wDefense = 10 * (gpGlobals->wGameDifficulty + 1);
					g_Battle.rgEnemy[i].e.wDefense *= 1.5;
				}

				if (g_Battle.rgEnemy[i].e.wDexterity > 0)
				{
					g_Battle.rgEnemy[i].e.wDexterity *= 1.25;
				}
				else
				{
					g_Battle.rgEnemy[i].e.wDexterity = 10 * (gpGlobals->wGameDifficulty + 1);
					g_Battle.rgEnemy[i].e.wDexterity *= 1.25;
				}

				if (g_Battle.rgEnemy[i].e.wFleeRate > 0)
				{
					g_Battle.rgEnemy[i].e.wFleeRate *= 1.25;
				}
				else
				{
					g_Battle.rgEnemy[i].e.wFleeRate = 10 * (gpGlobals->wGameDifficulty + 1);
					g_Battle.rgEnemy[i].e.wFleeRate *= 1.25;
				}

				// 行动次数增多
				if (g_Battle.rgEnemy[i].e.wDexterity / 3 > g_Battle.rgEnemy[i].e.wLevel)
				{
					g_Battle.rgEnemy[i].e.wDualMove += g_Battle.rgEnemy[i].e.wLevel / 12.5 / (gpGlobals->wGameDifficulty + 1);
					g_Battle.rgEnemy[i].e.wDualMove = min(g_Battle.rgEnemy[i].e.wDualMove, 100);
				}

				// 五灵抗性提高
				for (x = 0; x < NUM_MAGIC_ELEMENTAL; x++)
				{
					g_Battle.rgEnemy[i].e.wElemResistance[x] *= 1.25;
					g_Battle.rgEnemy[i].e.wElemResistance[x] = min(g_Battle.rgEnemy[i].e.wElemResistance[x], 100);
				}

				// 施法概率提高
				g_Battle.rgEnemy[i].e.wMagicRate *= 1.25;
				g_Battle.rgEnemy[i].e.wMagicRate = min(g_Battle.rgEnemy[i].e.wMagicRate, 100);
			}
			else if (gpGlobals->wGameDifficulty > 1)
			{
				// 金钱收益削减
				g_Battle.rgEnemy[i].e.wCash /= 1.55;

				// 灵葫收益削减
				g_Battle.rgEnemy[i].e.wCollectValue /= 1.55;

				// 可偷道具 / 金钱 收益削减
				g_Battle.rgEnemy[i].e.nStealItem /= 1.55;

				// 经验值收益削减
				g_Battle.rgEnemy[i].e.wExp /= 1.55;

				// 修行提高
				g_Battle.rgEnemy[i].e.wLevel *= 1.55;

				// 体力提高
				g_Battle.rgEnemy[i].e.wHealth *= 1.55;

				// 普攻附带概率提高
				g_Battle.rgEnemy[i].e.wAttackEquivItemRate *= 1.55;
				g_Battle.rgEnemy[i].e.wAttackEquivItemRate = min(g_Battle.rgEnemy[i].e.wAttackEquivItemRate, 100);

				// 五维提高
				if (g_Battle.rgEnemy[i].e.wAttackStrength > 0)
				{
					g_Battle.rgEnemy[i].e.wAttackStrength *= 1.55;
				}
				else
				{
					g_Battle.rgEnemy[i].e.wAttackStrength = 10 * (gpGlobals->wGameDifficulty + 1);
					g_Battle.rgEnemy[i].e.wAttackStrength *= 1.55;
				}

				if (g_Battle.rgEnemy[i].e.wMagicStrength > 0)
				{
					g_Battle.rgEnemy[i].e.wMagicStrength *= 1.55;
				}
				else
				{
					g_Battle.rgEnemy[i].e.wMagicStrength = 10 * (gpGlobals->wGameDifficulty + 1);
					g_Battle.rgEnemy[i].e.wMagicStrength *= 1.55;
				}

				if (g_Battle.rgEnemy[i].e.wDefense > 0)
				{
					g_Battle.rgEnemy[i].e.wDefense *= 1.55;
				}
				else
				{
					g_Battle.rgEnemy[i].e.wDefense = 10 * (gpGlobals->wGameDifficulty + 1);
					g_Battle.rgEnemy[i].e.wDefense *= 1.55;
				}

				if (g_Battle.rgEnemy[i].e.wDexterity > 0)
				{
					g_Battle.rgEnemy[i].e.wDexterity *= 1.55;
				}
				else
				{
					g_Battle.rgEnemy[i].e.wDexterity = 10 * (gpGlobals->wGameDifficulty + 1);
					g_Battle.rgEnemy[i].e.wDexterity *= 1.55;
				}

				if (g_Battle.rgEnemy[i].e.wFleeRate > 0)
				{
					g_Battle.rgEnemy[i].e.wFleeRate *= 1.55;
				}
				else
				{
					g_Battle.rgEnemy[i].e.wFleeRate = 10 * (gpGlobals->wGameDifficulty + 1);
					g_Battle.rgEnemy[i].e.wFleeRate *= 1.55;
				}

				// 行动次数增多
				if (g_Battle.rgEnemy[i].e.wDexterity / 3 > g_Battle.rgEnemy[i].e.wLevel)
				{
					g_Battle.rgEnemy[i].e.wDualMove += g_Battle.rgEnemy[i].e.wLevel / 12.5 / (gpGlobals->wGameDifficulty + 1);
					g_Battle.rgEnemy[i].e.wDualMove = min(g_Battle.rgEnemy[i].e.wDualMove, 100);
				}

				// 五灵抗性提高
				for (x = 0; x < NUM_MAGIC_ELEMENTAL; x++)
				{
					g_Battle.rgEnemy[i].e.wElemResistance[x] *= 1.55;
					g_Battle.rgEnemy[i].e.wElemResistance[x] = min(g_Battle.rgEnemy[i].e.wElemResistance[x], 100);
				}

				// 施法概率提高
				g_Battle.rgEnemy[i].e.wMagicRate *= 1.55;
				g_Battle.rgEnemy[i].e.wMagicRate = min(g_Battle.rgEnemy[i].e.wMagicRate, 100);
			}

#ifndef PAL_CLASSIC
			g_Battle.rgEnemy[i].flTimeMeter = 50;

			//
			// HACK: Otherwise the black thief lady will be too hard to beat
			//
			if (g_Battle.rgEnemy[i].e.wDexterity == 164)
			{
				g_Battle.rgEnemy[i].e.wDexterity /= ((gpGlobals->wMaxPartyMemberIndex == 0) ? 6 : 3);
			}

			//
			// HACK: Heal up automatically for final boss
			//
			if (g_Battle.rgEnemy[i].e.wHealth == 32760)
			{
				for (w = 0; w < MAX_PLAYER_ROLES; w++)
				{
					gpGlobals->g.PlayerRoles.rgwHP[w] = gpGlobals->g.PlayerRoles.rgwMaxHP[w];
					gpGlobals->g.PlayerRoles.rgwMP[w] = gpGlobals->g.PlayerRoles.rgwMaxMP[w];
				}
			}

			//
			// Yet another HACKs
			//
			if ((SHORT)g_Battle.rgEnemy[i].e.wDexterity == -32)
			{
				g_Battle.rgEnemy[i].e.wDexterity = 0; // for Grandma Knife
			}
			else if (g_Battle.rgEnemy[i].e.wDexterity == 20)
			{
				//
				// for Fox Demon
				//
				if (gpGlobals->g.PlayerRoles.rgwLevel[0] < 15)
				{
					g_Battle.rgEnemy[i].e.wDexterity = 8;
				}
				else if (gpGlobals->g.PlayerRoles.rgwLevel[4] > 28 ||
					gpGlobals->Exp.rgPrimaryExp[4].wExp > 0)
				{
					g_Battle.rgEnemy[i].e.wDexterity = 60;
				}
			}
			else if (g_Battle.rgEnemy[i].e.wExp == 250 &&
				g_Battle.rgEnemy[i].e.wCash == 1100)
			{
				g_Battle.rgEnemy[i].e.wDexterity += 12; // for Snake Demon
			}
			else if ((SHORT)g_Battle.rgEnemy[i].e.wDexterity == -60)
			{
				g_Battle.rgEnemy[i].e.wDexterity = 15; // for Spider
			}
			else if ((SHORT)g_Battle.rgEnemy[i].e.wDexterity == -30)
			{
				g_Battle.rgEnemy[i].e.wDexterity = (WORD)-10; // for Stone Head
			}
			else if ((SHORT)g_Battle.rgEnemy[i].e.wDexterity == -16)
			{
				g_Battle.rgEnemy[i].e.wDexterity = 0; // for Zombie
			}
			else if ((SHORT)g_Battle.rgEnemy[i].e.wDexterity == -20)
			{
				g_Battle.rgEnemy[i].e.wDexterity = -8; // for Flower Demon
			}
			else if (g_Battle.rgEnemy[i].e.wLevel < 20 &&
				gpGlobals->wNumScene >= 0xD8 && gpGlobals->wNumScene <= 0xE2)
			{
				//
				// for low-level monsters in the Cave of Trial
				//
				g_Battle.rgEnemy[i].e.wLevel += 15;
				g_Battle.rgEnemy[i].e.wDexterity += 25;
			}
			else if (gpGlobals->wNumScene == 0x90)
			{
				g_Battle.rgEnemy[i].e.wDexterity += 25; // for Tower Dragons
			}
			else if (g_Battle.rgEnemy[i].e.wLevel == 2 &&
				g_Battle.rgEnemy[i].e.wCash == 48)
			{
				g_Battle.rgEnemy[i].e.wDexterity += 8; // for Miao Fists
			}
			else if (g_Battle.rgEnemy[i].e.wLevel == 4 &&
				g_Battle.rgEnemy[i].e.wCash == 240)
			{
				g_Battle.rgEnemy[i].e.wDexterity += 18; // for Fat Miao
			}
			else if (g_Battle.rgEnemy[i].e.wLevel == 16 &&
				g_Battle.rgEnemy[i].e.wMagicRate == 4 &&
				g_Battle.rgEnemy[i].e.wAttackEquivItemRate == 4)
			{
				g_Battle.rgEnemy[i].e.wDexterity += 50; // for Black Spider
			}
#endif
		}
	}

	g_Battle.wMaxEnemyIndex = i - 1;

	//
	// Store all players
	// 存储所有玩家
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		g_Battle.rgPlayer[i].flTimeMeter = 15.0f;
#ifndef PAL_CLASSIC
		g_Battle.rgPlayer[i].flTimeSpeedModifier = 2.0f;
		g_Battle.rgPlayer[i].sTurnOrder = -1;
#endif
		g_Battle.rgPlayer[i].wHidingTime = 0;
		g_Battle.rgPlayer[i].state = kFighterWait;
		g_Battle.rgPlayer[i].fDefending = FALSE;
		g_Battle.rgPlayer[i].wCurrentFrame = 0;
		g_Battle.rgPlayer[i].iColorShift = FALSE;
	}

	//
	// Load sprites and background
	// 加载子画面和背景
	PAL_LoadBattleSprites();
	PAL_LoadBattleBackground();

	//
	// Create the surface for scene buffer
	// 为场景缓冲区创建曲面
	g_Battle.lpSceneBuf = VIDEO_CreateCompatibleSurface(gpScreen);

	if (g_Battle.lpSceneBuf == NULL)
	{
		TerminateOnError("PAL_StartBattle(): creating surface for scene buffer failed!");
	}

	PAL_UpdateEquipments();

	g_Battle.iExpGained = 0;
	g_Battle.iCashGained = 0;

	g_Battle.fIsBoss = fIsBoss;
	g_Battle.fEnemyCleared = FALSE;
	g_Battle.fEnemyMoving = FALSE;
	g_Battle.iHidingTime = 0;
	g_Battle.wMovingPlayerIndex = 0;

	g_Battle.UI.szMsg[0] = '\0';
	g_Battle.UI.szNextMsg[0] = '\0';
	g_Battle.UI.dwMsgShowTime = 0;
	g_Battle.UI.state = kBattleUIWait;
	g_Battle.UI.fAutoAttack = FALSE;
	g_Battle.UI.iSelectedIndex = 0;
	g_Battle.UI.iPrevEnemyTarget = -1;

	memset(g_Battle.UI.rgShowNum, 0, sizeof(g_Battle.UI.rgShowNum));

	g_Battle.lpSummonSprite = NULL;
	g_Battle.sBackgroundColorShift = 0;

	gpGlobals->fInBattle = TRUE;
	g_Battle.BattleResult = kBattleResultPreBattle;

	PAL_BattleUpdateFighters();

	//
	// Load the battle effect sprite.
	// 加载战斗效果特效，我方施法手的特效？
	i = PAL_MKFGetChunkSize(10, gpGlobals->f.fpDATA);
	g_Battle.lpEffectSprite = UTIL_malloc(i);

	PAL_MKFReadChunk(g_Battle.lpEffectSprite, i, 10, gpGlobals->f.fpDATA);

#ifdef PAL_CLASSIC
	g_Battle.Phase = kBattlePhaseSelectAction;
	g_Battle.fRepeat = FALSE;
	g_Battle.fForce = FALSE;
	g_Battle.fFlee = FALSE;
	g_Battle.fPrevAutoAtk = FALSE;
	g_Battle.fThisTurnCoop = FALSE;
#endif

	//
	// Run the main battle routine.
	// 执行主战斗程序
	i = PAL_BattleMain();

	if (i == kBattleResultWon)
	{
		//
		// Player won the battle. Add the Experience points.
		// 战斗胜利结算
		PAL_BattleWon();
	}

	//
	// Clear all item-using records
	// 使用记录清除所有项目
	for (w = 0; w < MAX_INVENTORY; w++)
	{
		gpGlobals->rgInventory[w].nAmountInUse = 0;
	}

	//
	// Clear all player status, poisons and temporary effects
	// 清除所有玩家状态、毒药和临时效果
	PAL_ClearAllPlayerStatus();
	for (w = 0; w < MAX_PLAYER_ROLES; w++)
	{
		PAL_CurePoisonByLevel(w, 3);
		PAL_RemoveEquipmentEffect(w, kBodyPartExtra);
	}

	// 关闭所有战场卦性转换开关
	PAL_RemoveDiagramsEffect();

	//
	// Free all the battle sprites
	// 释放所有战斗精灵
	PAL_FreeBattleSprites();
	free(g_Battle.lpEffectSprite);

	//
	// Free the surfaces for the background picture and scene buffer
	// 释放背景图片和场景缓冲区的曲面
	VIDEO_FreeSurface(g_Battle.lpBackground);
	VIDEO_FreeSurface(g_Battle.lpSceneBuf);

	g_Battle.lpBackground = NULL;
	g_Battle.lpSceneBuf = NULL;

	gpGlobals->fInBattle = FALSE;

	// 播放战斗音乐
	AUDIO_PlayMusic(gpGlobals->wNumMusic, TRUE, 1);

	//
	// Restore the screen waving effects
	// 恢复屏幕波动效果
	gpGlobals->sWaveProgression = sPrevWaveProgression;
	gpGlobals->wScreenWave = wPrevWaveLevel;

	return i;
}

VOID
PAL_BattleBackupScene(
	VOID
)
/*++
  目的：    Backup the scene buffer.
  备份场景缓存。
  参数：    无。
  返回值：    无。
  --*/
{	//其意义是将一个平面的一部分或全部图像整块从这个平面复制到另一个平面。
	SDL_BlitSurface(g_Battle.lpSceneBuf, NULL, gpScreenBak, NULL);
}

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

static struct MAGICITEM
{
	WORD         wMagic;
	WORD         wMP;
	WORD         wSP;
	WORD         wMagicElem;
	BOOL         fEnabled;
} rgMagicItem[MAX_PLAYER_MAGICS];

#if defined(PAL_NEW_MAGICCLASSIFY)
// 新增，用于仙术切换类型页
static struct tagCURRENTMAGICPAGE
{
	WORD         wDefaultMagic;
	WORD         wCurrentMagicElem;
} cmpCurrentMagicPage[MAX_PLAYER_ROLES];

static WORD     wCPMCurrentPlayerID = 0;
#endif

static int     g_iNumMagic = 0;
static int     g_iCurrentItem = 0;
static WORD    g_wPlayerMP = 0;
static WORD    g_wPlayerSP = 0;

WORD
PAL_MagicSelectionMenuUpdate(
	VOID
)
/*++
  Purpose:

	Update the magic selection menu.
	更新仙术选择菜单

  Parameters:

	None.

  Return value:

	The selected magic. 0 if cancelled, 0xFFFF if not confirmed.

--*/
{
	int         i, j, k, item_delta;
	BYTE        bColor;
	const int   iItemsPerLine = 32 / gConfig.dwWordLength;
	const int   iItemTextWidth = 8 * gConfig.dwWordLength + 7;
	// 仙术框的仙术显示的行数为5
	const int   iLinesPerPage = 5 - gConfig.ScreenLayout.ExtraMagicDescLines;
	// 仙术框的Y坐标为
	const int   iBoxYOffset = gConfig.ScreenLayout.ExtraMagicDescLines * 16;
	const int   iCursorXOffset = gConfig.dwWordLength * 5 / 2;
	const int   iPageLineOffset = iLinesPerPage / 2;

#if defined(PAL_NEW_MAGICCLASSIFY)
	WORD        wPlayerRole = wCPMCurrentPlayerID;

	//
	// Check for inputs
	// 分类选项按键控制
	if (g_InputState.dwKeyPress & kKeyMagicPageLeft)
	{
		// 若当前仙术类为 剑系 类则按 A 键切换至 绝学 系
		// 否则回到上一类
		if (cmpCurrentMagicPage[wPlayerRole].wCurrentMagicElem == kmsSwordAttribute)
			cmpCurrentMagicPage[wPlayerRole].wCurrentMagicElem = kmsUniqueSkillAttribute;
		else
			cmpCurrentMagicPage[wPlayerRole].wCurrentMagicElem -= 1;

		PAL_MagicSelectionMenuInit(-1);
	}
	if (g_InputState.dwKeyPress & kKeyMagicPageRight)
	{
		// 若当前仙术类为 绝学 类则按 D 键切换至 剑系
		// 否则切换到下一类
		if (cmpCurrentMagicPage[wPlayerRole].wCurrentMagicElem == kmsUniqueSkillAttribute)
			cmpCurrentMagicPage[wPlayerRole].wCurrentMagicElem = kmsSwordAttribute;
		else
			cmpCurrentMagicPage[wPlayerRole].wCurrentMagicElem += 1;

		PAL_MagicSelectionMenuInit(-1);
	}
#endif

	//
	// Check for inputs
	// 选项按键控制
	if (g_InputState.dwKeyPress & kKeyUp)
	{
		item_delta = -iItemsPerLine;
	}
	else if (g_InputState.dwKeyPress & kKeyDown)
	{
		item_delta = iItemsPerLine;
	}
	else if (g_InputState.dwKeyPress & kKeyLeft)
	{
		item_delta = -1;
	}
	else if (g_InputState.dwKeyPress & kKeyRight)
	{
		item_delta = 1;
	}
	else if (g_InputState.dwKeyPress & kKeyPgUp)
	{
		item_delta = -(iItemsPerLine * iLinesPerPage);
	}
	else if (g_InputState.dwKeyPress & kKeyPgDn)
	{
		item_delta = iItemsPerLine * iLinesPerPage;
	}
	else if (g_InputState.dwKeyPress & kKeyHome)
	{
		item_delta = -g_iCurrentItem;
	}
	else if (g_InputState.dwKeyPress & kKeyEnd)
	{
		item_delta = g_iNumMagic - g_iCurrentItem - 1;
	}
	else if (g_InputState.dwKeyPress & kKeyMenu)
	{
		return 0;
	}
	else
	{
		item_delta = 0;
	}

	//
	// Make sure the current menu item index is in bound
	// 确保当前菜单项索引已绑定
	if (g_iCurrentItem + item_delta < 0)
		g_iCurrentItem = 0;
	else if (g_iCurrentItem + item_delta >= g_iNumMagic)
		g_iCurrentItem = g_iNumMagic - 1;
	else
		g_iCurrentItem += item_delta;

	// 如果仙术栏为空则将一切仙术信息显示为0
	if (g_iNumMagic <= 0)
	{
		rgMagicItem[g_iCurrentItem].wMagic = 0;
		rgMagicItem[g_iCurrentItem].wMP = 0;
		rgMagicItem[g_iCurrentItem].wSP = 0;
	}

	//
	// Create the box.
	// 绘制仙术框
	PAL_CreateBoxWithShadow(PAL_XY(10, 47 + iBoxYOffset), iLinesPerPage - 1, 16, 1, FALSE, 0);
	//
	// Draw the MP of the selected magic.
	// 绘制高光闪烁的仙术需要消耗的MP框,取消框阴影的绘制
	//PAL_CreateSingleLineBox(PAL_XY(0, 0), 3, FALSE);
	PAL_CreateSingleLineBoxWithShadow(PAL_XY(0, 0), 3, FALSE, 0);
	PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, PAL_XY(31, 4));
	PAL_DrawNumber(rgMagicItem[g_iCurrentItem].wMP, 4, PAL_XY(4, 4), kNumColorYellow, kNumAlignRight);
	PAL_DrawNumber(g_wPlayerMP, 4, PAL_XY(36, 4), kNumColorCyan, kNumAlignRight);

	// 绘制仙术所需SP
	PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, PAL_XY(31, 13));
	PAL_DrawNumber(rgMagicItem[g_iCurrentItem].wSP, 4, PAL_XY(4, 13), kNumColorYellow, kNumAlignRight);
	PAL_DrawNumber(g_wPlayerSP, 4, PAL_XY(36, 13), kNumColorGold, kNumAlignRight);

	//
	// Draw the cash amount.
	// 绘制总钱数
	PAL_DrawNumber(gpGlobals->dwCash, 6, PAL_XY(23, 22), kNumColorYellow, kNumAlignRight);

#if defined(PAL_NEW_MAGICCLASSIFY)
	// 初始化仙术系型
	WCHAR wMagicAttribute[128];
	LPCWSTR wMagicAttributeText;
	WORD wCurrentMagicElem = cmpCurrentMagicPage[wCPMCurrentPlayerID].wCurrentMagicElem;

	//
	// 绘制仙术系型选项
	//
	for (i = 0; i <= kmsUniqueSkillAttribute; i++)
	{
		switch (i)
		{
		case kmsSwordAttribute:
			wMagicAttributeText = L"剑";
			break;

		case kmsWindAttribute:
			wMagicAttributeText = L"风";
			break;

		case kmsThunderAttribute:
			wMagicAttributeText = L"雷";
			break;

		case kmsWaterAttribute:
			wMagicAttributeText = L"水";
			break;

		case kmsFireAttribute:
			wMagicAttributeText = L"火";
			break;

		case kmsEarthAttribute:
			wMagicAttributeText = L"土";
			break;

		case kmsPoisonAttribute:
			wMagicAttributeText = L"毒";
			break;

		case kmsSootheAttribute:
			wMagicAttributeText = L"疗";
			break;

		case kmsSorceryAttribute:
			wMagicAttributeText = L"巫";
			break;

		case kmsUniqueSkillAttribute:
			wMagicAttributeText = L"绝";
			break;
		}
		wcscpy(wMagicAttribute, wMagicAttributeText);
		PAL_DrawText(wMagicAttribute, PAL_XY(0 + 32 * i, 144), MENUITEM_COLOR, TRUE, FALSE, FALSE);
	}

	//
	// 绘制仙术系型
	//
	switch (wCurrentMagicElem)
	{
	case kmsSwordAttribute:
		wMagicAttributeText = L"剑系";
		break;

	case kmsWindAttribute:
		wMagicAttributeText = L"风系";
		break;

	case kmsThunderAttribute:
		wMagicAttributeText = L"雷系";
		break;

	case kmsWaterAttribute:
		wMagicAttributeText = L"水系";
		break;

	case kmsFireAttribute:
		wMagicAttributeText = L"火系";
		break;

	case kmsEarthAttribute:
		wMagicAttributeText = L"土系";
		break;

	case kmsPoisonAttribute:
		wMagicAttributeText = L"毒系";
		break;

	case kmsSootheAttribute:
		wMagicAttributeText = L"疗系";
		break;

	case kmsSorceryAttribute:
		wMagicAttributeText = L"巫系";
		break;

	case kmsUniqueSkillAttribute:
		wMagicAttributeText = L"绝技";
		break;
	}
	wcscpy(wMagicAttribute, wMagicAttributeText);
	PAL_DrawText(wMagicAttribute, PAL_XY(0 + 32 * wCurrentMagicElem, 144), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
#endif

	WCHAR szDesc[512], * next;
	const WCHAR* d = PAL_GetObjectDesc(gpGlobals->lpObjectDesc, rgMagicItem[g_iCurrentItem].wMagic);

	//
	// Draw the magic description.
	// 绘制仙术描述
	if (d != NULL)
	{
		k = 1;
		wcscpy(szDesc, d);
		d = szDesc;

		while (TRUE)
		{
			next = wcschr(d, '*');
			if (next != NULL)
			{
				*next++ = '\0';
			}

			//仙术描述，X坐标为64
			PAL_DrawText(d, PAL_XY(64, k), DESCTEXT_COLOR, TRUE, FALSE, FALSE);
			k += 16;

			if (next == NULL)
			{
				break;
			}

			d = next;
		}
	}
	else
	{
		// 请选择仙术
		PAL_DrawText(L"暂未选择仙术。", PAL_XY(64, 1), DESCTEXT_COLOR, TRUE, FALSE, FALSE);
	}

	//
	// Draw the texts of the current page
	// 绘制当前页面的仙术名
	i = g_iCurrentItem / iItemsPerLine * iItemsPerLine - iItemsPerLine * iPageLineOffset;
	if (i < 0)
	{
		i = 0;
	}

	for (j = 0; j < iLinesPerPage; j++)
	{
		for (k = 0; k < iItemsPerLine; k++)
		{
			bColor = MENUITEM_COLOR;

			if (i >= g_iNumMagic)
			{
				//
				// End of the list reached
				// 已到达列表末尾
				j = iLinesPerPage;
				break;
			}

			if (i == g_iCurrentItem)
			{
				if (rgMagicItem[i].fEnabled)
				{
					bColor = MENUITEM_COLOR_SELECTED;
				}
				else
				{
					bColor = MENUITEM_COLOR_SELECTED_INACTIVE;
				}
			}
			else if (!rgMagicItem[i].fEnabled)
			{
				bColor = MENUITEM_COLOR_INACTIVE;
			}
			else if (rgMagicItem[i].wMagicElem == kmsUniqueSkillAttribute)
			{
				bColor = MENUITEM_COLOR_UNIQUESKILL;
			}

			//
			// Draw the text
			// 绘制当前页面仙术名
			PAL_DrawText(PAL_GetWord(rgMagicItem[i].wMagic), PAL_XY(35 + k * iItemTextWidth, 59 + j * 18 + iBoxYOffset), bColor, TRUE, FALSE, FALSE);

			//
			// Draw the cursor on the current selected item
			// 绘制选择的光标
			if (i == g_iCurrentItem)
			{
				PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR), gpScreen, PAL_XY(35 + iCursorXOffset + k * iItemTextWidth, 69 + j * 18 + iBoxYOffset));
			}

			i++;
		}
	}

	if (g_InputState.dwKeyPress & kKeySearch)
	{
		if (rgMagicItem[g_iCurrentItem].fEnabled)
		{
			j = g_iCurrentItem % iItemsPerLine;
			k = (g_iCurrentItem < iItemsPerLine* iPageLineOffset) ? (g_iCurrentItem / iItemsPerLine) : iPageLineOffset;

			j = 35 + j * iItemTextWidth;
			k = 59 + k * 18 + iBoxYOffset;

			PAL_DrawText(PAL_GetWord(rgMagicItem[g_iCurrentItem].wMagic), PAL_XY(j, k), MENUITEM_COLOR_CONFIRMED, FALSE, TRUE, FALSE);

			return rgMagicItem[g_iCurrentItem].wMagic;
		}
	}

	return 0xFFFF;
}

VOID
PAL_MagicSelectionMenuInit(
#if defined(PAL_NEW_MAGICCLASSIFY)
	SHORT         wPlayerRole
#else
	WORD         wPlayerRole,
	BOOL         fInBattle,
	WORD         wDefaultMagic
#endif
)
/*++
  Purpose:

	Initialize the magic selection menu.
	初始化仙术选择菜单。

  Parameters:

	[IN]  wPlayerRole - the player ID.
	队员ID

	[IN]  fInBattle - TRUE if in battle, FALSE if not.
	如果在战斗中，则为TRUE，否则为FALSE。

	[IN]  wDefaultMagic - the default magic item.
	默认仙术项目。

	// 新增
	[IN]  wElem - 要显示的仙术的类型

  Return value:

	None.
	无

--*/
{
	WORD       w;
	int        i, j;

	g_iCurrentItem = 0;
	g_iNumMagic = 0;

#if defined(PAL_NEW_MAGICCLASSIFY)
	BOOL       fInBattle = gpGlobals->fInBattle;

	if (wPlayerRole == -1)
		wPlayerRole = wCPMCurrentPlayerID;
	else
		wCPMCurrentPlayerID = wPlayerRole;

	WORD       wDefaultMagic = cmpCurrentMagicPage[wPlayerRole].wDefaultMagic;
	SHORT      wElem = cmpCurrentMagicPage[wPlayerRole].wCurrentMagicElem;
#endif

	g_wPlayerMP = gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole];
	g_wPlayerSP = gpGlobals->g.PlayerRoles.rgwSP[wPlayerRole];

	//
	// Put all magics of this player to the array
	// 将该玩家的所有魔法放入阵列
	for (i = 0; i < MAX_PLAYER_MAGICS; i++)
	{
		w = gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole];

		if (w != 0)
		{
			rgMagicItem[g_iNumMagic].wMagic = w;

			w = gpGlobals->g.rgObject[w].magic.wMagicNumber;

#if defined(PAL_NEW_MAGICCLASSIFY)
			// 跳过不符合系性的仙术
			if (gpGlobals->g.lprgMagic[w].wElemental != wElem)
				continue;
#endif

			rgMagicItem[g_iNumMagic].wMP = gpGlobals->g.lprgMagic[w].wCostMP;
			rgMagicItem[g_iNumMagic].wSP = gpGlobals->g.lprgMagic[w].wCostSP;

			rgMagicItem[g_iNumMagic].wMagicElem = gpGlobals->g.lprgMagic[w].wElemental;

			// 先将所有习得的仙术设为允许释放
			rgMagicItem[g_iNumMagic].fEnabled = TRUE;

			// MP或SP不足时不允许释放该仙术
			if (rgMagicItem[g_iNumMagic].wMP > g_wPlayerMP || rgMagicItem[g_iNumMagic].wSP > g_wPlayerSP)
			{
				rgMagicItem[g_iNumMagic].fEnabled = FALSE;
			}

			w = gpGlobals->g.rgObject[rgMagicItem[g_iNumMagic].wMagic].magic.wFlags;
			if (fInBattle)
			{
				if (!(w & kMagicFlagUsableInBattle))
				{
					rgMagicItem[g_iNumMagic].fEnabled = FALSE;
				}
			}
			else
			{
				if (!(w & kMagicFlagUsableOutsideBattle))
				{
					rgMagicItem[g_iNumMagic].fEnabled = FALSE;
				}
			}

			g_iNumMagic++;
		}
	}

	//
	// Sort the array
	// 对阵列排序
	for (i = 0; i < g_iNumMagic - 1; i++)
	{
		BOOL fCompleted = TRUE;

		for (j = 0; j < g_iNumMagic - 1 - i; j++)
		{
			if (rgMagicItem[j].wMagic > rgMagicItem[j + 1].wMagic)
			{
				struct MAGICITEM t = rgMagicItem[j];
				rgMagicItem[j] = rgMagicItem[j + 1];
				rgMagicItem[j + 1] = t;

				fCompleted = FALSE;
			}
		}

		if (fCompleted)
		{
			break;
		}
	}

	//
	// Place the cursor to the default item
	// 将光标放置到默认项目
	for (i = 0; i < g_iNumMagic; i++)
	{
		if (rgMagicItem[i].wMagic == wDefaultMagic)
		{
			g_iCurrentItem = i;
			break;
		}
	}
}

WORD
PAL_MagicSelectionMenu(
	WORD         wPlayerRole,
	BOOL         fInBattle,
	WORD         wDefaultMagic
)
/*++
  Purpose:

	Show the magic selection menu.

  Parameters:

	[IN]  wPlayerRole - the player ID.

	[IN]  fInBattle - TRUE if in battle, FALSE if not.

	[IN]  wDefaultMagic - the default magic item.

  Return value:

	The selected magic. 0 if cancelled.

--*/
{
	WORD            w;
	int             i;
	DWORD           dwTime;

#if defined(PAL_NEW_MAGICCLASSIFY)
	wCPMCurrentPlayerID = wPlayerRole;
	cmpCurrentMagicPage[wCPMCurrentPlayerID].wDefaultMagic = wDefaultMagic;
	cmpCurrentMagicPage[wCPMCurrentPlayerID].wCurrentMagicElem = max(cmpCurrentMagicPage[wPlayerRole].wCurrentMagicElem, 0);

	PAL_MagicSelectionMenuInit(-1);
#else
	PAL_MagicSelectionMenuInit(wPlayerRole, fInBattle, wDefaultMagic);
#endif

	PAL_ClearKeyState();

	dwTime = SDL_GetTicks();

	while (TRUE)
	{
		PAL_MakeScene();

		w = 7;

		// 绘制队员HMP框
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			PAL_PlayerInfoBox(PAL_XY(w, 165), gpGlobals->rgParty[i].wPlayerRole, 100,
				TIMEMETER_COLOR_DEFAULT, FALSE);

			w += 77;
		}

		w = PAL_MagicSelectionMenuUpdate();
		VIDEO_UpdateScreen(NULL);

		PAL_ClearKeyState();

		if (w != 0xFFFF)
		{
			return w;
		}

		PAL_ProcessEvent();
		while (!SDL_TICKS_PASSED(SDL_GetTicks(), dwTime))
		{
			PAL_ProcessEvent();
			if (g_InputState.dwKeyPress != 0)
			{
				break;
			}
			SDL_Delay(5);
		}

		dwTime = SDL_GetTicks() + FRAME_TIME;
	}

	return 0; // should not really reach here
}

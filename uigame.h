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

#ifndef UIGAME_H
#define UIGAME_H

#include "common.h"
#include "ui.h"

PAL_C_LINKAGE_BEGIN

// 最大存档个数
#define MAX_SAVE_NUMBER              8
#define SAVE_NUMBEROFFAST            9
#define SAVE_NUMBEROFAUTO            10
#define SAVE_COMPLETIONPROMPT        16
#define MAX_DIFFICULTY_MAX           3

static LPCWSTR
menuTextContent[SAVE_NUMBEROFAUTO] = {
	L"梦境一", L"梦境二", L"梦境三", L"梦境四", L"梦境五",
	L"梦境六", L"梦境七", L"梦境八", L"自动记忆", L"便捷记忆"
};

// 定义灵抗颜色
static NUMCOLOR magicsResistanceColor[NUM_MAGIC_ELEMENTAL] = {
	kNumColorCyan,
	kNumColorBlue,
	kNumColorYellow,
	kNumColorRed,
	kNumColorPink
};

VOID
PAL_DrawOpeningMenuBackground(
	VOID
);

// 开始菜单
INT
PAL_OpeningMenu(
	VOID
);

INT
PAL_SaveSlotMenu(
	WORD        wDefaultSlot
);

WORD
PAL_TripleMenu(
	WORD  wThirdWord
);

BOOL
PAL_ConfirmMenu(
	VOID
);

WORD
PAL_New_ConfirmMenu(
	VOID
);

BOOL
PAL_SwitchMenu(
	BOOL      fEnabled
);

VOID
PAL_InGameMagicMenu(
	VOID
);

VOID
PAL_InGameMenu(
	VOID
);

// 队员状态页面
VOID
PAL_PlayerStatus(
	VOID
);

WORD
PAL_ItemUseMenu(
	WORD           wItemToUse
);

VOID
PAL_BuyMenu(
	WORD           wStoreNum
);

VOID
PAL_SellMenu(
	VOID
);

VOID
PAL_EquipItemMenu(
	WORD           wItem
);

VOID
PAL_QuitGame(
	VOID
);

VOID
PAL_FastSaveGame(
	BOOL           IsAutoSave
);

WORD
PAL_New_GameDifficultyMenu(
	VOID
);

PAL_C_LINKAGE_END

#endif

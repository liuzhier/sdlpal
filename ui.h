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

#ifndef UI_H
#define UI_H

#include "common.h"

#define CHUNKNUM_SPRITEUI                  9

//#define MENUITEM_COLOR                     0x4F
#define MENUITEM_COLOR                     0x0F
#define MENUITEM_COLOR_INACTIVE            0x1C
#define MENUITEM_COLOR_CONFIRMED           0x2C
#define MENUITEM_COLOR_SELECTED_INACTIVE   0x1F
#define MENUITEM_COLOR_SELECTED_FIRST      0xF9
#define MENUITEM_COLOR_SELECTED_TOTALNUM   6
#define MENUITEM_COLOR_UNIQUESKILL         0xD8

#define MENUITEM_COLOR_SELECTED                                    \
   (MENUITEM_COLOR_SELECTED_FIRST +                                \
      SDL_GetTicks() / (600 / MENUITEM_COLOR_SELECTED_TOTALNUM)    \
      % MENUITEM_COLOR_SELECTED_TOTALNUM)

#define MENUITEM_COLOR_EQUIPPEDITEM        0xC8

//#define DESCTEXT_COLOR                     0x2E
#define DESCTEXT_COLOR                     0x3C

#define MAINMENU_BACKGROUND_FBPNUM         (gConfig.fIsWIN95 ? 2 :60)

#define RIX_NUM_OPENINGMENU                4
#define MAINMENU_LABEL_NEWGAME             7
#define MAINMENU_LABEL_LOADGAME            8

#define LOADMENU_LABEL_SLOT_FIRST          43

#define CONFIRMMENU_LABEL_NO               19
#define CONFIRMMENU_LABEL_YES              20

#define CASH_LABEL                         21

#define SWITCHMENU_LABEL_DISABLE           17
#define SWITCHMENU_LABEL_ENABLE            18

#define GAMEMENU_LABEL_STATUS              3
#define GAMEMENU_LABEL_MAGIC               4
#define GAMEMENU_LABEL_INVENTORY           5
#define GAMEMENU_LABEL_SYSTEM              6

#define SYSMENU_LABEL_SAVE                 11
#define SYSMENU_LABEL_LOAD                 12
#define SYSMENU_LABEL_MUSIC                13
#define SYSMENU_LABEL_SOUND                14
#define SYSMENU_LABEL_QUIT                 15
#define SYSMENU_LABEL_SAVEFINISH           16
//#define SYSMENU_LABEL_BATTLEMODE           606
//#define SYSMENU_LABEL_LAUNCHSETTING        612
#define SYSMENU_LABEL_BATTLEMODE           998
#define SYSMENU_LABEL_LAUNCHSETTING        999

#define BATTLESPEEDMENU_LABEL_1            (SYSMENU_LABEL_BATTLEMODE + 1)
#define BATTLESPEEDMENU_LABEL_2            (SYSMENU_LABEL_BATTLEMODE + 2)
#define BATTLESPEEDMENU_LABEL_3            (SYSMENU_LABEL_BATTLEMODE + 3)
#define BATTLESPEEDMENU_LABEL_4            (SYSMENU_LABEL_BATTLEMODE + 4)
#define BATTLESPEEDMENU_LABEL_5            (SYSMENU_LABEL_BATTLEMODE + 5)

#define BATTLECANMELTCORPSES_COLOR         0x0F
#define BATTLECANTMELTCORPSES_COLOR        0x18

#define PAL_ADDITIONAL_WORD_FIRST           10000
#define PAL_ADDITIONAL_WORD_SECOND			20001
#define PAL_ADDITIONAL_MSG_SECOND           20001

#define LABEL_NOTHING						PAL_ADDITIONAL_WORD_SECOND + 8

#define INVMENU_LABEL_USE                  23
#define INVMENU_LABEL_EQUIP                22

#define STATUS_BACKGROUND_FBPNUM           0
#define STATUS_LABEL_EXP                   2
#define STATUS_LABEL_LEVEL                 48
#define STATUS_LABEL_HP                    49
#define STATUS_LABEL_MP                    50
#define STATUS_LABEL_EXP_LAYOUT            29
#define STATUS_LABEL_LEVEL_LAYOUT          30
#define STATUS_LABEL_HP_LAYOUT             31
#define STATUS_LABEL_MP_LAYOUT             32
#define STATUS_LABEL_ATTACKPOWER           51
#define STATUS_LABEL_MAGICPOWER            52
#define STATUS_LABEL_RESISTANCE            53
#define STATUS_LABEL_DEXTERITY             54
#define STATUS_LABEL_FLEERATE              55
//#define STATUS_COLOR_EQUIPMENT             0xBE
#define STATUS_COLOR_EQUIPMENT             0x0F

#define EQUIP_LABEL_HEAD                   600
#define EQUIP_LABEL_SHOULDER               601
#define EQUIP_LABEL_BODY                   602
#define EQUIP_LABEL_HAND                   603
#define EQUIP_LABEL_FOOT                   604
#define EQUIP_LABEL_NECK                   605

#define BUYMENU_LABEL_CURRENT              35
#define SELLMENU_LABEL_PRICE               25

#define SPRITENUM_SLASH                    39
#define SPRITENUM_ITEMBOX                  70
#define SPRITENUM_CURSOR_YELLOW            68
#define SPRITENUM_CURSOR                   69
#define SPRITENUM_PLAYERINFOBOX            18
#define SPRITENUM_PLAYERFACE_FIRST         48

//#define EQUIPMENU_BACKGROUND_FBPNUM        1
//#define EQUIPMENU_BACKGROUND_FBPNUM        65

//#define ITEMUSEMENU_COLOR_STATLABEL        0xBB
#define ITEMUSEMENU_COLOR_STATLABEL        0x0F

#define BATTLEWIN_GETEXP_LABEL             30
#define BATTLEWIN_BEATENEMY_LABEL          9
#define BATTLEWIN_DOLLAR_LABEL             10
#define BATTLEWIN_LEVELUP_LABEL            32
#define BATTLEWIN_ADDMAGIC_LABEL           33
//#define BATTLEWIN_LEVELUP_LABEL_COLOR      0xBB
#define BATTLEWIN_LEVELUP_LABEL_COLOR      0x0F
#define SPRITENUM_ARROW                    47

#define BATTLE_LABEL_ESCAPEFAIL            31

// 新增
// 
// 标签：进度-进度六
#define LOADMENU_LABEL_SLOT_SIXTH          565

// 标签：难度-休闲
#define LABEL_DIFFICULTY_ARDER             570

// 标签：队员属性-精力
#define STATUS_LABEL_SP                    573
#define STATUS_LABEL_SP_LAYOUT             33

// 标签：队员属性提升-最大体力值
#define LABEL_HP_MAX                       574

// 标签：队员属性提升-最大真气值
#define LABEL_MP_MAX                       575

// 标签：队员属性提升-最大精力值
#define LABEL_SP_MAX                       576

// 标签：敌方状态-可    (偷窃)
#define LABEL_PILFERABLE                   577

// 标签：敌方状态-已    (偷窃)
#define LABEL_ALREADY                      580

// 标签：敌我通用属性-巫抗
#define LABEL_SORCERY_RESISTANCE           581

// 标签：敌我通用属性-毒抗
#define LABEL_POISON_RESISTANCE            582

// 标签：敌我通用属性-物抗
#define LABEL_PHYSICAL_RESISTANCE          583

// 标签：敌我通用属性-灵抗
#define LABEL_MAGICS_RESISTANCE_ALL        584

// 标签：敌方属性-攻击附带
#define LABEL_ATTACK_COLLATERAL            585

// 标签：敌方属性-默认仙术
#define LABEL_DEFAULT_MAGIC                586

// 标签：敌方属性-偷窃可得
#define LABEL_CAN_STEAL_SOMETHING          587

// 标签：敌方属性-战后掉落
#define LABEL_CAPTURED_EQUIPMENT           588

// 标签：敌方属性值-无
#define LABEL_NONE                         589

// 标签：敌方属性-灵葫
#define LABEL_COLLECT_ENEMY                590

// 标签：敌方属性-行动数
#define LABEL_NUM_OF_ACTIONS               591

// 标签：敌方属性-目标号
#define LABEL_TARGET_INDEX                 592

// 标签：现有系数-灵葫值
#define COLLECTVALUE_LABEL                 593

// 标签：购买个数-5个
#define CONFIRMMENU_LABEL_FIVE             594

// 标签：购买个数-10个
#define CONFIRMMENU_LABEL_TEN              595

// 标签：战后结算：获得灵葫值
#define LABEL_COLLECT_VALUE                596

// 标签：装备位置名称
#define LABEL_EQUEMENT_POSNAME             597

// 标签：抗性
#define LABEL_RESISTANCE_ALL               606


typedef struct tagBOX
{
	PAL_POS        pos;
	WORD           wWidth, wHeight;
	SDL_Surface* lpSavedArea;
} BOX, * LPBOX;

typedef struct tagMENUITEM
{
	WORD          wValue;
	WORD          wNumWord;
	BOOL          fEnabled;
	PAL_POS       pos;
} MENUITEM, * LPMENUITEM;
typedef const MENUITEM* LPCMENUITEM;

typedef struct tagOBJECTDESC
{
	WORD                        wObjectID;
	LPWSTR                      lpDesc;
	struct tagOBJECTDESC* next;
} OBJECTDESC, * LPOBJECTDESC;

typedef VOID(*LPITEMCHANGED_CALLBACK)(WORD);

#define MENUITEM_VALUE_CANCELLED      0xFFFF

typedef enum tagNUMCOLOR
{
	kNumColorYellow,
	kNumColorBlue,
	kNumColorCyan,
	kNumColorPink,
	kNumColorRed,
	kNumColorGold
} NUMCOLOR;

typedef enum tagNUMALIGN
{
	kNumAlignLeft,
	kNumAlignMid,
	kNumAlignRight
} NUMALIGN;

PAL_C_LINKAGE_BEGIN

INT
PAL_InitUI(
	VOID
);

VOID
PAL_FreeUI(
	VOID
);

LPBOX
PAL_CreateBox(
	PAL_POS        pos,
	INT            nRows,
	INT            nColumns,
	INT            iStyle,
	BOOL           fSaveScreen
);

LPBOX
PAL_CreateBoxWithShadow(
	PAL_POS        pos,
	INT            nRows,
	INT            nColumns,
	INT            iStyle,
	BOOL           fSaveScreen,
	INT            nShadowOffset
);

LPBOX
PAL_CreateSingleLineBox(
	PAL_POS        pos,
	INT            nLen,
	BOOL           fSaveScreen
);

LPBOX
PAL_CreateSingleLineBoxWithShadow(
	PAL_POS        pos,
	INT            nLen,
	BOOL           fSaveScreen,
	INT            nShadowOffset
);

VOID
PAL_DeleteBox(
	LPBOX          lpBox
);

WORD
PAL_ReadMenu(
	LPITEMCHANGED_CALLBACK    lpfnMenuItemChanged,
	LPCMENUITEM               rgMenuItem,
	INT                       nMenuItem,
	WORD                      wDefaultItem,
	BYTE                      bLabelColor
);

WORD
PAL_New_ReadMenu(
	LPITEMCHANGED_CALLBACK    lpfnMenuItemChanged,
	LPCMENUITEM               rgMenuItem,
	INT                       nMenuItem,
	WORD                      wDefaultItem,
	BYTE                      bLabelColor
);

VOID
PAL_DrawNumber(
	UINT            iNum,
	UINT            nLength,
	PAL_POS         pos,
	NUMCOLOR        color,
	NUMALIGN        align
);

INT
PAL_TextWidth(
	LPCWSTR        lpszItemText
);

INT
PAL_MenuTextMaxWidth(
	LPCMENUITEM    rgMenuItem,
	INT            nMenuItem
);

INT
PAL_WordMaxWidth(
	INT            nFirstWord,
	INT            nWordNum
);

INT
PAL_WordWidth(
	INT            nWordIndex
);

LPOBJECTDESC
PAL_LoadObjectDesc(
	LPCSTR          lpszFileName
);

VOID
PAL_FreeObjectDesc(
	LPOBJECTDESC    lpObjectDesc
);

LPCWSTR
PAL_GetObjectDesc(
	LPOBJECTDESC   lpObjectDesc,
	WORD           wObjectID
);

extern LPSPRITE gpSpriteUI;

PAL_C_LINKAGE_END

#endif

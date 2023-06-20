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

#ifndef FIGHT_H
#define FIGHT_H

#include "common.h"

PAL_C_LINKAGE_BEGIN

// 从中选择自动目标
INT
PAL_BattleSelectAutoTargetFrom(
	INT
);

// 玩家已死亡
BOOL
PAL_IsPlayerDying(
	WORD
);

// 玩家是否健康
BOOL
PAL_IsPlayerHealthy(
	WORD     wPlayerRole
);

// 战斗选择自动目标
INT
PAL_BattleSelectAutoTarget(
	VOID
);

#ifndef PAL_CLASSIC

// 更新时间计费单元
VOID
PAL_UpdateTimeChargingUnit(
	VOID
);

// 获取战斗速度
FLOAT
PAL_GetTimeChargingSpeed(
	WORD           wDexterity
);

#endif

// 战场更新打架
VOID
PAL_BattleUpdateFighters(
	VOID
);

// 战斗玩家检查就绪
VOID
PAL_BattlePlayerCheckReady(
	VOID
);

// 战斗开始帧
VOID
PAL_BattleStartFrame(
	VOID
);

// 作战承诺行动
VOID
PAL_BattleCommitAction(
	BOOL         fRepeat
);

// 战斗时队员执行行动
VOID
PAL_BattlePlayerPerformAction(
	WORD         wPlayerIndex
);

// 战斗时敌人执行行动
VOID
PAL_BattleEnemyPerformAction(
	WORD         wEnemyIndex
);

// 战斗时展示玩家施法前对玩家的影响
VOID
PAL_BattleShowPlayerPreMagicAnim(
	WORD         wPlayerIndex,
	BOOL         fSummon
);

// 作战延迟,也可显示文本
VOID
PAL_BattleDelay(
	WORD       wDuration,
	WORD       wObjectID,
	BOOL       fUpdateGesture
);

// 战斗时窃取敌方物资
VOID
PAL_BattleStealFromEnemy(
	WORD           wTarget,
	WORD           wStealRate
);

// 战斗时模拟魔法
VOID
PAL_BattleSimulateMagic(
	SHORT      sTarget,
	WORD       wMagicObjectID,
	INT        wBaseDamage
);

// 战斗时模拟魔法（直接的伤害，无伤害公式）
VOID
PAL_New_BattleSimulateMagicRealInjury(
	SHORT      sTarget,
	WORD       wMagicObjectID,
	INT        iBaseDamage
);

PAL_C_LINKAGE_END

#endif

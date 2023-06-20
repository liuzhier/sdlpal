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
#include <math.h>

//#define INVINCIBLE 1

BOOL
PAL_IsPlayerDying(
	WORD        wPlayerRole
)
/*++
  Purpose:

	Check if the player is dying.
	检查队员是否濒临死亡（虚弱）。

  Parameters:

	[IN]  wPlayerRole - the player role ID.

  Return value:

	TRUE if the player is dying, FALSE if not.

--*/
{
	return gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] <
		min(100, gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole] / 5);
}

BOOL
PAL_IsPlayerHealthy(
	WORD     wPlayerRole
)
/*++
 Purpose:

	Check if the player is healthy.
	 检查玩家是否健康。

 Parameters:

	[IN]  wPlayerRole - the player role ID.

 Return value:

	 TRUE if the player is healthy, FALSE if not.

 --*/
{
	return !PAL_IsPlayerDying(wPlayerRole) &&
		gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] == 0 &&
		gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] == 0 &&
		gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSilence] == 0 &&
		gpGlobals->rgPlayerStatus[wPlayerRole][kStatusParalyzed] == 0 &&
		gpGlobals->rgPlayerStatus[wPlayerRole][kStatusPuppet] == 0;
}

// 战斗选择自动目标
INT
PAL_BattleSelectAutoTarget(
	VOID
)
{
	return PAL_BattleSelectAutoTargetFrom(0);
}

INT
PAL_BattleSelectAutoTargetFrom(
	INT begin
)
/*++
  Purpose:

	Pick an enemy target automatically.
	自动选择敌方目标。

  Parameters:

	[IN]  begin - the beginning target ID.

  Return value:

	The index of enemy. -1 if failed.

--*/
{
	int          i;
	int          count;

	i = g_Battle.UI.iPrevEnemyTarget;

	if (i >= 0 && i <= g_Battle.wMaxEnemyIndex &&
		g_Battle.rgEnemy[i].wObjectID != 0 &&
		g_Battle.rgEnemy[i].e.wHealth > 0)
	{
		return i;
	}

	for (count = 0, i = (begin >= 0 ? begin : 0); count < MAX_ENEMIES_IN_TEAM; count++)
	{
		if (g_Battle.rgEnemy[i].wObjectID != 0 &&
			g_Battle.rgEnemy[i].e.wHealth > 0)
		{
			return i;
		}
		i = (i + 1) % MAX_ENEMIES_IN_TEAM;
	}

	return -1;
}

static INT
PAL_CalcBaseDamage(
	INT         wAttackStrength,
	INT         wDefense
)
/*++
  Purpose:

	Calculate the base damage value of attacking.
	计算攻击的基础伤害值。

  Parameters:

	[IN]  wAttackStrength - attack strength of attacker.

	[IN]  wDefense - defense value of inflictor.

  Return value:

	The base damage value of the attacking.

--*/
{
	INT            sDamage;

	//
	// Formula courtesy of palxex and shenyanduxing
	// 公式来自 palxex 与 慎言笃行
	if (wAttackStrength > wDefense)
	{
		// 施展攻击者的灵力大于受击者防御时
		//sDamage = (SHORT)(wAttackStrength * 2 - wDefense * 1.6 + 0.5);
		sDamage = wAttackStrength * 2 - wDefense * 1.6 + 0.5;
	}
	else if (wAttackStrength > wDefense * 0.6)
	{
		// 施展攻击者的灵力只大于受击者防御的6成时
		//sDamage = (SHORT)(wAttackStrength - wDefense * 0.6 + 0.5);
		sDamage = wAttackStrength - wDefense * 0.6 + 0.5;
	}
	else
	{
		// 施展攻击者的灵力小于受击者防御的6成时，基础伤害为0
		sDamage = 0;
	}

	return sDamage;
}

static INT
PAL_CalcMagicDamage(
	INT             wMagicStrength,
	INT             wDefense,
	const WORD      rgwElementalResistance[NUM_MAGIC_ELEMENTAL],
	INT             wPoisonResistance,
	INT             wResistanceMultiplier,
	WORD            wMagicID
)
/*++
   Purpose:

	 Calculate the damage of magic.
	 绘制仙术所造成的伤害

   Parameters:

	 [IN]  wMagicStrength - magic strength of attacker.
	 施展攻击者的灵力

	 [IN]  wDefense - defense value of victim.
	 受击者的防御值

	 [IN]  rgwElementalResistance - victim's resistance to the elemental magics.
	 受击者的灵抗

	 [IN]  wPoisonResistance - victim's resistance to poison.
	 受击者的毒抗

	 [IN]  wResistanceMultiplier - multiplier of resistance value.
	 受击者的物抗？

	 [IN]  wMagicID - object ID of the magic.
	 施展的仙术的ID

   Return value:

	 The damage value of the magic attack.

--*/
{
	//SHORT           sDamage;
	INT             iDamage;
	WORD            wElem;

	wMagicID = gpGlobals->g.rgObject[wMagicID].magic.wMagicNumber;

	//
	// Formula courtesy of palxex and shenyanduxing
	// 公式来自 palxex 与 慎言笃行

	// 一半概率增强0.1倍伤害
	wMagicStrength *= RandomFloat(10, 11);
	wMagicStrength /= 10;

	// DEBUG 最低按20算
	wResistanceMultiplier = (wResistanceMultiplier <= 20 && wResistanceMultiplier != 1) ? 20 : wResistanceMultiplier;

	iDamage = PAL_CalcBaseDamage(wMagicStrength, wDefense);
	iDamage /= 4;

	iDamage += gpGlobals->g.lprgMagic[wMagicID].wBaseDamage;

	if (gpGlobals->g.lprgMagic[wMagicID].wElemental != 0)
	{
		wElem = gpGlobals->g.lprgMagic[wMagicID].wElemental;

		if (wElem > NUM_MAGIC_ELEMENTAL)
		{
			iDamage *= 10 - ((FLOAT)wPoisonResistance / wResistanceMultiplier);
		}
		else if (wElem == 0)
		{
			iDamage *= 5;
		}
		else
		{
			iDamage *= 10 - ((FLOAT)rgwElementalResistance[wElem - 1] / wResistanceMultiplier);
		}

		iDamage /= 5;

		if (wElem <= NUM_MAGIC_ELEMENTAL)
		{
			iDamage *= 10 + gpGlobals->g.lprgBattleField[gpGlobals->wNumBattleField].rgsMagicEffect[wElem - 1];
			iDamage /= 10;
		}
	}

	return iDamage;
}

static INT
PAL_New_CalcMagicDamage(
	SHORT        wPlayerRoleID,
	WORD         wEnemyID,
	WORD         wMagicID
)
/*++
   Purpose:

	 Calculate the damage of magic.

   Parameters:

	 [IN]  wEnnmyID - Index of enemy.
	 施展攻击者的 灵力

	 [IN]  wMagicID - object ID of the magic.
	 施展攻击者使用的 仙术对象ID

   Return value:

	 The damage value of the magic attack.
	 仙术攻击伤害

--*/
{
	//
	// Formula courtesy of palxex and shenyanduxing
	// 原公式来自 palxex 与 慎言笃行，后续为 HACK
	INT           iDamage;
	SHORT         sElem;
	INT           iBaseDamage;
	WORD          wMagicNumber;
	INT           i;

	// 施展攻击者属性
	WORD          wAttacker;
	INT           iAttackerPoisonResistance;
	INT           iAttackerMagicStrength = 0;
	INT           iAttackerPhysicalResistance;
	INT           iAttackerElemResistance;
	INT           iAttackerSorceryResistance;
	INT           iAttackerUniqueSkill;

	// 受击者属性
	WORD          wVictim;
	INT           iVictimDef;
	INT           iVictimPhysicalResistance;
	INT           iVictimElementalResistance;
	INT           iVictimPoisonResistance;
	INT           iVictimSorceryResistance;

	// 获取仙术的基础伤害
	wMagicNumber = gpGlobals->g.rgObject[wMagicID].magic.wMagicNumber;
	iBaseDamage = gpGlobals->g.lprgMagic[wMagicNumber].wBaseDamage;

	// 获取仙术的隶属属性
	sElem = gpGlobals->g.lprgMagic[wMagicNumber].wElemental;

	// 若该仙术基础伤害为 0 则不进行后续计算直接返回 0
	if ((INT)iBaseDamage == 0)
	{
		return 0;
	}

	// 判断是哪方攻击
	if (wPlayerRoleID < 0)
	{
		// 我方攻击者
		wAttacker = PAL_New_GetMovingPlayerIndex();

		// 我方攻击者灵力
		if (wPlayerRoleID == -1) {
			//
			// 我方合体法术
			//
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				//
				// 将每位队员的武术和灵力加起来
				//
				//if (g_Battle.coopContributors[i] == FALSE)
				//	continue;

				iAttackerMagicStrength += PAL_GetPlayerAttackStrength(gpGlobals->rgParty[i].wPlayerRole) * 2.0;
				iAttackerMagicStrength += PAL_GetPlayerMagicStrength(gpGlobals->rgParty[i].wPlayerRole);
			}

			// 全队总灵攻 / 4 得到均值
			//iAttackerMagicStrength /= gpGlobals->wMaxPartyMemberIndex + 1;
		}
		else
		{
			iAttackerMagicStrength = PAL_GetPlayerMagicStrength(wAttacker);
		}

		// 我方攻击者物抗
		iAttackerPhysicalResistance = PAL_New_GetPlayerPhysicalResistance(wAttacker);

		// 我方攻击者灵抗
		iAttackerElemResistance = PAL_GetPlayerElementalResistance(wAttacker, sElem - 1);

		// 我方攻击者毒抗
		iAttackerPoisonResistance = PAL_GetPlayerPoisonResistance(wAttacker);

		// 我方攻击者巫抗
		iAttackerSorceryResistance = PAL_New_GetPlayerSorceryResistance(wAttacker);

		// 我方攻击者绝招加成
		iAttackerUniqueSkill = PAL_New_GetPlayerUniqueSkillResistance(wAttacker);


		// 敌方受击者
		wVictim = wEnemyID;

		// 敌方受击者防御
		iVictimDef = PAL_New_GetEnemyDefense(wVictim);
		iVictimDef += (PAL_New_GetEnemyLevel(wVictim) + 6) * 4;

		// 敌方受击者物抗
		iVictimPhysicalResistance = PAL_New_GetEnemyPhysicalResistance(wVictim);

		// 敌方受击者灵抗
		iVictimElementalResistance = PAL_New_GetEnemyElementalResistance(wVictim, sElem - 1);

		// 敌方受击者毒抗
		iVictimPoisonResistance = PAL_New_GetEnemyPoisonResistance(wVictim);

		// 敌方受击者巫抗
		iVictimSorceryResistance = PAL_New_GetEnemySorceryResistance(wVictim);
	}
	else
	{
		// 敌方攻击者
		wAttacker = wEnemyID;

		// 敌方攻击者毒抗
		iAttackerPoisonResistance = PAL_New_GetEnemyPoisonResistance(wAttacker);

		// 敌方攻击者灵力
		iAttackerMagicStrength = PAL_New_GetEnemyMagicStrength(wAttacker);

		// 敌方攻击者物抗
		iAttackerPhysicalResistance = PAL_New_GetEnemyPhysicalResistance(wAttacker);

		// 敌方攻击者灵抗
		iAttackerElemResistance = PAL_New_GetEnemyElementalResistance(wAttacker, sElem - 1);

		// 敌方攻击者巫抗
		iAttackerSorceryResistance = PAL_New_GetEnemySorceryResistance(wAttacker);


		// 我方受击者
		wVictim = wPlayerRoleID;

		// 我方受击者防御
		iVictimDef = PAL_GetPlayerDefense(wVictim);
		iVictimDef += (PAL_New_GetPlayerLevel(wVictim) + 6) * 4;

		// 我方受击者物抗
		iVictimPhysicalResistance = PAL_New_GetPlayerPhysicalResistance(wVictim);

		// 我方受击者灵抗
		iVictimElementalResistance = PAL_GetPlayerElementalResistance(wVictim, sElem - 1);

		// 我方受击者毒抗
		iVictimPoisonResistance = PAL_GetPlayerPoisonResistance(wVictim);

		// 我方受击者巫抗
		iVictimSorceryResistance = PAL_New_GetPlayerSorceryResistance(wVictim);

		// 我方受击者绝技加成
		iAttackerUniqueSkill = 20 * RandomFloat(1, 2.2);
	}

	// 一半概率增强 10% 灵力攻击
	iAttackerMagicStrength *= RandomFloat(10, 11);
	iAttackerMagicStrength /= 10.0;

	// 计算攻击的基础伤害值
	iDamage = PAL_CalcBaseDamage(iAttackerMagicStrength, iVictimDef) / 2 + iBaseDamage;

	// 根据 仙术的 隶属属性 完成伤害的后续计算
	switch (sElem)
	{
	case kmsSwordAttribute:
		//
		// 剑系（根据 受击者的 物抗 百分比 加强/削弱 伤害）
		//
		iDamage *= (100 + iAttackerPhysicalResistance - iVictimPhysicalResistance) / 100.0;
		break;

	case kmsWindAttribute:
		//
		// 风系，这里不写break就直接会跳到 case kmsEarthAttribute
		//

	case kmsThunderAttribute:
		//
		// 雷系，这里不写break就直接会跳到 case kmsEarthAttribute
		//

	case kmsWaterAttribute:
		//
		// 水系，这里不写break就直接会跳到 case kmsEarthAttribute
		//

	case kmsFireAttribute:
		//
		// 火系，这里不写break就直接会跳到 case kmsEarthAttribute
		//

	case kmsEarthAttribute:
		//
		// 土系（根据 受击者的 灵抗 百分比 加强/削弱 伤害，再根据 战场环境 对五灵的影响 十分比 加强/削弱 伤害）
		//
		iDamage *= (100 + iAttackerElemResistance - iVictimElementalResistance) / 100.0;
		iDamage *= (10 + PAL_New_GetBattleFieldComplete(sElem - 1)) / 10.0;
		break;

	case kmsPoisonAttribute:
		//
		// 毒系（根据 受击者的 毒抗 百分比 加强/削弱 伤害）
		//
		iDamage *= (100 + iAttackerPoisonResistance - iVictimPoisonResistance) / 100.0;
		break;

	case kmsSorceryAttribute:
		//
		// 巫术（预：敌方巫抗越高伤害越高）
		//
		iDamage *= (100 + iAttackerSorceryResistance) / 100.0;
		break;

	case kmsUniqueSkillAttribute:
		//
		// 绝招（绝对性的削弱 10% 伤害）
		//
		iDamage *= (100 + iAttackerUniqueSkill) / 100.0;
		break;

	case kmsDevilSkillAttribute:
		//
		// 魔功（根据物抗防御 &&%……*&*（￥%……￥……& 削弱 伤害）物抗最低只能按 1 算
		//
		iDamage -= iVictimDef * max(iVictimPhysicalResistance, 1) / 100.0;
		break;

	}

	// 防止伤害溢出，若溢出则伤害为 1
	iDamage = max(iDamage, 1);

	// 防止伤害溢出，伤害不得高于敌方当前体力值
	if (wPlayerRoleID < 0)
	{
		// 加上难度分配动态伤害+++++++++++++++++++++++++++
		iDamage *= gpGlobals->wGameDifficulty + 1;

		if (wPlayerRoleID == -2)
		{
			//
			// 投掷
			// 有天罡状态时投掷伤害翻倍
			if (gpGlobals->rgPlayerStatus[wAttacker][kStatusBravery] > 0)
			{
				iDamage *= 2;
			}
		}

		iDamage = min(iDamage, PAL_New_GetEnemyHealth(wVictim));
	}

	return iDamage;
}

INT
PAL_CalcPhysicalAttackDamage(
	INT           wAttackStrength,
	INT           wDefense,
	INT           wAttackResistance
)
/*++
  Purpose:

	Calculate the damage value of physical attacking.
	计算物理攻击的伤害值。

  Parameters:

	[IN]  wAttackStrength - attack strength of attacker.

	[IN]  wDefense - defense value of inflictor.

	[IN]  wAttackResistance - inflictor's resistance to physical attack.

  Return value:

	The damage value of the physical attacking.

--*/
{
	INT             iDamage;

	iDamage = PAL_CalcBaseDamage(wAttackStrength, wDefense);

	if (wAttackResistance != 0)
	{
		iDamage *= (100 + wAttackResistance) / 100.0;
	}

	return iDamage;
}

static SHORT
PAL_GetEnemyDexterity(
	WORD          wEnemyIndex
)
/*++
  Purpose:

	Get the dexterity value of the enemy.
	获取敌人的身法值。

  Parameters:

	[IN]  wEnemyIndex - the index of the enemy.

  Return value:

	The dexterity value of the enemy.

--*/
{
	SHORT      s;

	assert(g_Battle.rgEnemy[wEnemyIndex].wObjectID != 0);

	s = (g_Battle.rgEnemy[wEnemyIndex].e.wLevel + 6) * 3;
	s += (SHORT)g_Battle.rgEnemy[wEnemyIndex].e.wDexterity;

#ifndef PAL_CLASSIC
	if (s < 20)
	{
		s = 20;
	}

	if (g_Battle.rgEnemy[wEnemyIndex].rgwStatus[kStatusHaste] != 0)
	{
		s *= 6;
		s /= 5;
	}
	else if (g_Battle.rgEnemy[wEnemyIndex].rgwStatus[kStatusSlow] != 0)
	{
		s *= 2;
		s /= 3;
	}
#endif

	return s;
}

static WORD
PAL_GetPlayerActualDexterity(
	WORD            wPlayerRole
)
/*++
  Purpose:

	Get player's actual dexterity value in battle.
	在战斗中获得玩家的实际身法值。

  Parameters:

	[IN]  wPlayerRole - the player role ID.

  Return value:

	The player's actual dexterity value.

--*/
{
	WORD wDexterity = PAL_GetPlayerDexterity(wPlayerRole);

	if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusHaste] != 0)
	{
#ifdef PAL_CLASSIC
		wDexterity *= 3;
#else
		wDexterity *= 6;
		wDexterity /= 5;
#endif
	}
#ifndef PAL_CLASSIC
	else if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSlow] != 0)
	{
		wDexterity *= 2;
		wDexterity /= 3;
	}

	if (PAL_IsPlayerDying(wPlayerRole))
	{
		//
		// player who is low of HP should be slower
		// 生命值低的玩家应该慢一点
		wDexterity *= 4;
		wDexterity /= 5;
	}
#endif

#ifdef PAL_CLASSIC
	if (wDexterity > 999)
	{
		wDexterity = 999;
	}
#endif

	return wDexterity;
}

#ifndef PAL_CLASSIC

VOID
PAL_UpdateTimeChargingUnit(
	VOID
)
/*++
  Purpose:

	Update the base time unit of time-charging.
	更新时间计费的基本时间单位。

  Parameters:

	None.

  Return value:

	None.

--*/
{
	g_Battle.flTimeChargingUnit = (FLOAT)(pow(PAL_GetPlayerDexterity(0) + 5, 0.3));
	g_Battle.flTimeChargingUnit /= PAL_GetPlayerDexterity(0);

	if (gpGlobals->bBattleSpeed > 1)
	{
		g_Battle.flTimeChargingUnit /= 1 + (gpGlobals->bBattleSpeed - 1) * 0.5;
	}
	else
	{
		g_Battle.flTimeChargingUnit /= 1.2f;
	}
}

FLOAT
PAL_GetTimeChargingSpeed(
	WORD           wDexterity
)
/*++
  Purpose:

	Calculate the time charging speed.
	计算时间恢复速度。

  Parameters:

	[IN]  wDexterity - the dexterity value of player or enemy.

  Return value:

	The time-charging speed of the player or enemy.

--*/
{
	if ((g_Battle.UI.state == kBattleUISelectMove &&
		g_Battle.UI.MenuState != kBattleMenuMain) ||
		!SDL_TICKS_PASSED(SDL_GetTicks(), g_Battle.UI.dwMsgShowTime))
	{
		//
		// Pause the time when there are submenus or text messages
		//
		return 0;
	}

	//
	// The battle should be faster when using Auto-Battle
	//
	if (gpGlobals->fAutoBattle)
	{
		wDexterity *= 3;
	}

	return g_Battle.flTimeChargingUnit * wDexterity;
}

#endif

VOID
PAL_BattleDelay(
	WORD       wDuration,
	WORD       wObjectID,
	BOOL       fUpdateGesture
)
/*++
  Purpose:

	Delay a while during battle.
	在战斗中延迟一段时间，也可以输出一些文本

  Parameters:

	[IN]  wDuration - Number of frames of the delay.

	[IN]  wObjectID - The object ID to be displayed during the delay.

	[IN]  fUpdateGesture - TRUE if update the gesture for enemies, FALSE if not.

  Return value:

	None.

--*/
{
	int    i, j;
	DWORD  dwTime = SDL_GetTicks() + BATTLE_FRAME_TIME;

	for (i = 0; i < wDuration; i++)
	{
		if (fUpdateGesture)
		{
			//
			// Update the gesture of enemies.
			//
			for (j = 0; j <= g_Battle.wMaxEnemyIndex; j++)
			{
				if (g_Battle.rgEnemy[j].wObjectID == 0 ||
					g_Battle.rgEnemy[j].rgwStatus[kStatusSleep] != 0 ||
					g_Battle.rgEnemy[j].rgwStatus[kStatusParalyzed] != 0)
				{
					continue;
				}

				if (--g_Battle.rgEnemy[j].e.wIdleAnimSpeed == 0)
				{
					g_Battle.rgEnemy[j].wCurrentFrame++;
					g_Battle.rgEnemy[j].e.wIdleAnimSpeed =
						gpGlobals->g.lprgEnemy[gpGlobals->g.rgObject[g_Battle.rgEnemy[j].wObjectID].enemy.wEnemyID].wIdleAnimSpeed;
				}

				if (g_Battle.rgEnemy[j].wCurrentFrame >= g_Battle.rgEnemy[j].e.wIdleFrames)
				{
					g_Battle.rgEnemy[j].wCurrentFrame = 0;
				}
			}
		}

		//
		// Wait for the time of one frame. Accept input here.
		//
		PAL_DelayUntil(dwTime);

		//
		// Set the time of the next frame.
		//
		dwTime = SDL_GetTicks() + BATTLE_FRAME_TIME;

		PAL_BattleMakeScene();
		VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);
		PAL_BattleUIUpdate();

		if (wObjectID != 0)
		{
			if (wObjectID == BATTLE_LABEL_ESCAPEFAIL) // HACKHACK
			{
				PAL_DrawText(PAL_GetWord(wObjectID), PAL_XY(130, 75), 15, TRUE, FALSE, FALSE);
			}
			else if ((SHORT)wObjectID < 0)
			{
				//PAL_DrawText(PAL_GetWord(-((SHORT)wObjectID)), PAL_XY(170, 45), DESCTEXT_COLOR, TRUE, FALSE, FALSE);
				PAL_DrawText(PAL_GetWord(-((SHORT)wObjectID)), PAL_XY((320 - wcslen(PAL_GetWord(-(SHORT)wObjectID)) * 16) / 2, 10), DESCTEXT_COLOR, TRUE, FALSE, FALSE);
			}
			else
			{
				PAL_DrawText(PAL_GetWord(wObjectID), PAL_XY(210, 50), 15, TRUE, FALSE, FALSE);
			}
		}

		VIDEO_UpdateScreen(NULL);
	}
}

static VOID
PAL_BattleBackupStat(
	VOID
)
/*++
  Purpose:

	Backup HP and MP values of all players and enemies.
	备份所有玩家和敌人的HP和MP值。

  Parameters:

	None.

  Return value:

	None.

--*/
{
	int          i;
	WORD         wPlayerRole;

	for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
	{
		if (g_Battle.rgEnemy[i].wObjectID == 0)
		{
			continue;
		}
		g_Battle.rgEnemy[i].wPrevHP = g_Battle.rgEnemy[i].e.wHealth;
	}

	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

		g_Battle.rgPlayer[i].wPrevHP = gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole];
		g_Battle.rgPlayer[i].wPrevMP = gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole];
		g_Battle.rgPlayer[i].wPrevSP = gpGlobals->g.PlayerRoles.rgwSP[wPlayerRole];
	}
}

static BOOL
PAL_BattleDisplayStatChange(
	VOID
)
/*++
  Purpose:

	Display the HP and MP changes of all players and enemies.
	显示所有玩家和敌人的HP和MP变化。

  Parameters:

	None.

  Return value:

	TRUE if there are any number displayed, FALSE if not.

--*/
{
	int      i, x, y;
	//SHORT    sDamage;
	INT      sDamage;
	WORD     wPlayerRole;
	BOOL     f = FALSE;

	for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
	{
		if (g_Battle.rgEnemy[i].wObjectID == 0)
		{
			continue;
		}

		if (g_Battle.rgEnemy[i].wPrevHP != g_Battle.rgEnemy[i].e.wHealth)
		{
			//
			// Show the number of damage
			//显示敌方减血
			sDamage = g_Battle.rgEnemy[i].e.wHealth - g_Battle.rgEnemy[i].wPrevHP;

			x = PAL_X(g_Battle.rgEnemy[i].pos) - 9;
			y = PAL_Y(g_Battle.rgEnemy[i].pos) - 115;

			if (y < 10)
			{
				y = 10;
			}

			//if (sDamage < 0)
			//{
			//	PAL_BattleUIShowNum((WORD)(-sDamage), PAL_XY(x, y), kNumColorBlue);
			//}
			//else
			//{
			//	PAL_BattleUIShowNum((WORD)(sDamage), PAL_XY(x, y), kNumColorYellow);
			//}

			if (sDamage < 0)
			{
				PAL_BattleUIShowNum((-sDamage), PAL_XY(x, y), kNumColorRed);
			}
			else
			{
				PAL_BattleUIShowNum((sDamage), PAL_XY(x, y), kNumColorYellow);
			}

			f = TRUE;
		}
	}

	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

		if (g_Battle.rgPlayer[i].wPrevHP != gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole])
		{
			sDamage = gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] - g_Battle.rgPlayer[i].wPrevHP;

			// 使用仙术"一气化九百"时将累计本回合伤害值
			if (sDamage < 0 && g_Battle.rcmMagicType.wCurrentCumulativeRrounds < g_Battle.rcmMagicType.wMaxCumulativeRrounds)
				g_Battle.rcmMagicType.wCumulativeDamageValue -= sDamage;

			x = PAL_X(g_Battle.rgPlayer[i].pos) - 9;
			y = PAL_Y(g_Battle.rgPlayer[i].pos) - 75;

			if (y < 10)
			{
				y = 10;
			}

			//if (sDamage < 0)
			//{
			//	PAL_BattleUIShowNum((WORD)(-sDamage), PAL_XY(x, y), kNumColorBlue);
			//}
			//else
			//{
			//	PAL_BattleUIShowNum((WORD)(sDamage), PAL_XY(x, y), kNumColorYellow);
			//}

			if (sDamage < 0)
			{
				PAL_BattleUIShowNum(-sDamage, PAL_XY(x, y), kNumColorRed);
			}
			else
			{
				PAL_BattleUIShowNum(sDamage, PAL_XY(x, y), kNumColorYellow);
			}

			f = TRUE;
		}

		if (g_Battle.rgPlayer[i].wPrevMP != gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole])
		{
			sDamage =
				gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] - g_Battle.rgPlayer[i].wPrevMP;

			// 使用仙术"一气化九百"前记录谁使用的法术和每次使用的仙术所消耗的真气
			g_Battle.rcmMagicType.wThisUseMagicPlayerRoles = wPlayerRole;
			g_Battle.rcmMagicType.wThisUseMagicConsumedMP = 0 - sDamage;

			x = PAL_X(g_Battle.rgPlayer[i].pos) - 9;
			y = PAL_Y(g_Battle.rgPlayer[i].pos) - 67;

			if (y < 10)
			{
				y = 10;
			}

			//
			// Only show MP increasing
			//显示MP增加
			//if (sDamage > 0)
			//{
			//	PAL_BattleUIShowNum((WORD)(sDamage), PAL_XY(x, y), kNumColorCyan);
			//}
			if (sDamage < 0)
			{
				PAL_BattleUIShowNum(sDamage, PAL_XY(x, y), kNumColorBlue);
			}
			else
			{
				PAL_BattleUIShowNum(sDamage, PAL_XY(x, y), kNumColorCyan);
			}

			f = TRUE;
		}

		if (g_Battle.rgPlayer[i].wPrevSP != gpGlobals->g.PlayerRoles.rgwSP[wPlayerRole])
		{
			sDamage =
				gpGlobals->g.PlayerRoles.rgwSP[wPlayerRole] - g_Battle.rgPlayer[i].wPrevSP;

			x = PAL_X(g_Battle.rgPlayer[i].pos) - 9;
			y = PAL_Y(g_Battle.rgPlayer[i].pos) - 29;

			if (y < 10)
			{
				y = 10;
			}

			//
			// Only show MP increasing
			//显示SP增加
			//if (sDamage > 0)
			//{
			//	PAL_BattleUIShowNum((WORD)(sDamage), PAL_XY(x, y), kNumColorCyan);
			//}
			if (sDamage < 0)
			{
				PAL_BattleUIShowNum(sDamage, PAL_XY(x, y), kNumColorPink);
			}
			else
			{
				PAL_BattleUIShowNum(sDamage, PAL_XY(x, y), kNumColorGold);
			}

			f = TRUE;
		}
	}

	return f;
}

static VOID
PAL_BattlePostActionCheck(
	BOOL      fCheckPlayers
)
/*++
  Purpose:

	Essential checks after an action is executed.
	执行操作后的基本检查。

  Parameters:

	[IN]  fCheckPlayers - TRUE if check for players, FALSE if not.

  Return value:

	None.

--*/
{
	int      i, j;
	BOOL     fFade = FALSE;
	BOOL     fEnemyRemaining = FALSE;

	for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
	{
		if (g_Battle.rgEnemy[i].wObjectID == 0)
		{
			continue;
		}

		//if ((SHORT)(g_Battle.rgEnemy[i].e.wHealth) <= 0)
		//{
		//	//
		//	// This enemy is KO'ed
		//	//
		//	g_Battle.iExpGained += g_Battle.rgEnemy[i].e.wExp;
		//	g_Battle.iCashGained += g_Battle.rgEnemy[i].e.wCash;
		//
		//	AUDIO_PlaySound(g_Battle.rgEnemy[i].e.wDeathSound);
		//	g_Battle.rgEnemy[i].wObjectID = 0;
		//	fFade = TRUE;
		//
		//	continue;
		//}

		if ((g_Battle.rgEnemy[i].e.wHealth) <= 0)
		{
			//
			// This enemy is KO'ed
			//
			g_Battle.iExpGained += g_Battle.rgEnemy[i].e.wExp;
			g_Battle.iCashGained += g_Battle.rgEnemy[i].e.wCash;
			g_Battle.iCollectValue += g_Battle.rgEnemy[i].e.wCollectValue;

			AUDIO_PlaySound(g_Battle.rgEnemy[i].e.wDeathSound);
			g_Battle.rgEnemy[i].wObjectID = 0;
			fFade = TRUE;

			continue;
		}

		fEnemyRemaining = TRUE;
	}

	if (!fEnemyRemaining)
	{
		g_Battle.fEnemyCleared = TRUE;
		g_Battle.UI.state = kBattleUIWait;
	}

	if (fCheckPlayers && !gpGlobals->fAutoBattle)
	{
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			WORD w = gpGlobals->rgParty[i].wPlayerRole, wName;

			if (gpGlobals->g.PlayerRoles.rgwHP[w] < g_Battle.rgPlayer[i].wPrevHP &&
				gpGlobals->g.PlayerRoles.rgwHP[w] == 0)
			{
				w = gpGlobals->g.PlayerRoles.rgwCoveredBy[w];

				for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
				{
					if (gpGlobals->rgParty[j].wPlayerRole == w)
					{
						break;
					}
				}

				if (gpGlobals->g.PlayerRoles.rgwHP[w] > 0 &&
					gpGlobals->rgPlayerStatus[w][kStatusSleep] == 0 &&
					gpGlobals->rgPlayerStatus[w][kStatusParalyzed] == 0 &&
					gpGlobals->rgPlayerStatus[w][kStatusConfused] == 0 &&
					j <= gpGlobals->wMaxPartyMemberIndex)
				{
					wName = gpGlobals->g.PlayerRoles.rgwName[w];

					if (gpGlobals->g.rgObject[wName].player.wScriptOnFriendDeath != 0)
					{
						PAL_BattleDelay(10, 0, TRUE);

						PAL_BattleMakeScene();
						VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);
						VIDEO_UpdateScreen(NULL);

						g_Battle.BattleResult = kBattleResultPause;

						gpGlobals->g.rgObject[wName].player.wScriptOnFriendDeath =
							PAL_RunTriggerScript(gpGlobals->g.rgObject[wName].player.wScriptOnFriendDeath, w, FALSE);

						g_Battle.BattleResult = kBattleResultOnGoing;

						PAL_ClearKeyState();
						goto end;
					}
				}
			}
		}

		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			WORD w = gpGlobals->rgParty[i].wPlayerRole, wName;

			if (gpGlobals->rgPlayerStatus[w][kStatusSleep] != 0 ||
				gpGlobals->rgPlayerStatus[w][kStatusConfused] != 0)
			{
				continue;
			}

			if (gpGlobals->g.PlayerRoles.rgwHP[w] < g_Battle.rgPlayer[i].wPrevHP)
			{
				if (gpGlobals->g.PlayerRoles.rgwHP[w] > 0 && PAL_IsPlayerDying(w) &&
					g_Battle.rgPlayer[i].wPrevHP >= gpGlobals->g.PlayerRoles.rgwMaxHP[w] / 5)
				{
					WORD wCover = gpGlobals->g.PlayerRoles.rgwCoveredBy[w];

					if (gpGlobals->rgPlayerStatus[wCover][kStatusSleep] != 0 ||
						gpGlobals->rgPlayerStatus[wCover][kStatusParalyzed] != 0 ||
						gpGlobals->rgPlayerStatus[wCover][kStatusConfused] != 0)
					{
						continue;
					}

					wName = gpGlobals->g.PlayerRoles.rgwName[w];

					AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwDyingSound[w]);

					for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
					{
						if (gpGlobals->rgParty[j].wPlayerRole == wCover)
						{
							break;
						}
					}

					if (j > gpGlobals->wMaxPartyMemberIndex || gpGlobals->g.PlayerRoles.rgwHP[wCover] == 0)
					{
						continue;
					}

					if (gpGlobals->g.rgObject[wName].player.wScriptOnDying != 0)
					{
						PAL_BattleDelay(10, 0, TRUE);

						PAL_BattleMakeScene();
						VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);
						VIDEO_UpdateScreen(NULL);

						g_Battle.BattleResult = kBattleResultPause;

						gpGlobals->g.rgObject[wName].player.wScriptOnDying =
							PAL_RunTriggerScript(gpGlobals->g.rgObject[wName].player.wScriptOnDying, w, FALSE);

						g_Battle.BattleResult = kBattleResultOnGoing;
						PAL_ClearKeyState();
					}

					goto end;
				}
			}
		}
	}

end:
	if (fFade)
	{
		VIDEO_BackupScreen(g_Battle.lpSceneBuf);
		PAL_BattleMakeScene();
		PAL_BattleFadeScene();
	}

	//
	// Fade out the summoned god
	// 淡出召唤神
	if (g_Battle.lpSummonSprite != NULL)
	{
		PAL_BattleUpdateFighters();
		PAL_BattleDelay(1, 0, FALSE);

		free(g_Battle.lpSummonSprite);
		g_Battle.lpSummonSprite = NULL;

		g_Battle.sBackgroundColorShift = 0;

		VIDEO_BackupScreen(g_Battle.lpSceneBuf);
		PAL_BattleMakeScene();
		PAL_BattleFadeScene();
	}
}

VOID
PAL_BattleUpdateFighters(
	VOID
)
/*++
  Purpose:

	Update players' and enemies' gestures and locations in battle.
	更新玩家和敌人在战斗中的姿势和位置。

  Parameters:

	None.

  Return value:

	None.

--*/
{
	int        i;
	WORD       wPlayerRole;

	//
	// Update the gesture for all players
	//
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

		if (!g_Battle.rgPlayer[i].fDefending)
			g_Battle.rgPlayer[i].pos = g_Battle.rgPlayer[i].posOriginal;
		g_Battle.rgPlayer[i].iColorShift = 0;

		if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0)
		{
			if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusPuppet] == 0)
			{
				g_Battle.rgPlayer[i].wCurrentFrame = 2; // dead
			}
			else
			{
				g_Battle.rgPlayer[i].wCurrentFrame = 0; // puppet
			}
		}
		else
		{
			if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] != 0 ||
				PAL_IsPlayerDying(wPlayerRole))
			{
				g_Battle.rgPlayer[i].wCurrentFrame = 1;
			}
#ifndef PAL_CLASSIC
			else if (g_Battle.rgPlayer[i].state == kFighterAct &&
				g_Battle.rgPlayer[i].action.ActionType == kBattleActionMagic &&
				!g_Battle.fEnemyCleared)
			{
				//
				// Player is using a magic
				//
				g_Battle.rgPlayer[i].wCurrentFrame = 5;
			}
#endif
			else if (g_Battle.rgPlayer[i].fDefending && !g_Battle.fEnemyCleared)
			{
				g_Battle.rgPlayer[i].wCurrentFrame = 3;
			}
			else
			{
				g_Battle.rgPlayer[i].wCurrentFrame = 0;
			}
		}
	}

	//
	// Update the gesture for all enemies
	// 更新所有敌人的动作帧
	for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
	{
		if (g_Battle.rgEnemy[i].wObjectID == 0)
		{
			continue;
		}

		g_Battle.rgEnemy[i].pos = g_Battle.rgEnemy[i].posOriginal;
		g_Battle.rgEnemy[i].iColorShift = 0;

		if (g_Battle.rgEnemy[i].rgwStatus[kStatusSleep] > 0 ||
			g_Battle.rgEnemy[i].rgwStatus[kStatusParalyzed] > 0)
		{
			g_Battle.rgEnemy[i].wCurrentFrame = 0;
			continue;
		}

		if (--g_Battle.rgEnemy[i].e.wIdleAnimSpeed == 0)
		{
			g_Battle.rgEnemy[i].wCurrentFrame++;
			g_Battle.rgEnemy[i].e.wIdleAnimSpeed =
				gpGlobals->g.lprgEnemy[gpGlobals->g.rgObject[g_Battle.rgEnemy[i].wObjectID].enemy.wEnemyID].wIdleAnimSpeed;
		}

		if (g_Battle.rgEnemy[i].wCurrentFrame >= g_Battle.rgEnemy[i].e.wIdleFrames)
		{
			g_Battle.rgEnemy[i].wCurrentFrame = 0;
		}
	}
}

VOID
PAL_BattlePlayerCheckReady(
	VOID
)
/*++
  Purpose:

	Check if there are player who is ready.

  Parameters:

	None.

  Return value:

	None.

--*/
{
	float   flMax = 0;
	int     iMax = 0, i;

	//
	// Start the UI for the fastest and ready player
	// 为速度最快、准备就绪的玩家启动UI
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		if (g_Battle.rgPlayer[i].state == kFighterCom ||
			(g_Battle.rgPlayer[i].state == kFighterAct && g_Battle.rgPlayer[i].action.ActionType == kBattleActionCoopMagic))
		{
			flMax = 0;
			break;
		}
		else if (g_Battle.rgPlayer[i].state == kFighterWait)
		{
			if (g_Battle.rgPlayer[i].flTimeMeter > flMax)
			{
				iMax = i;
				flMax = g_Battle.rgPlayer[i].flTimeMeter;
			}
		}
	}

	if (flMax >= 100.0f)
	{
		g_Battle.rgPlayer[iMax].state = kFighterCom;
		g_Battle.rgPlayer[iMax].fDefending = FALSE;
	}
}

VOID
PAL_BattleStartFrame(
	VOID
)
/*++
  Purpose:

	Called once per video frame in battle.
	战斗中每视频帧调用一次。

  Parameters:

	None.

  Return value:

	None.

--*/
{
	int                      i, j, l;
	WORD                     wPlayerRole;
	WORD                     wDexterity;
	BOOL                     fOnlyPuppet = TRUE;

#ifndef PAL_CLASSIC
	FLOAT                    flMax;
	BOOL                     fMoved = FALSE;
	SHORT                    sMax, sMaxIndex;
#endif

	if (!g_Battle.fEnemyCleared)
	{
		PAL_BattleUpdateFighters();
	}

	//
	// Update the scene
	// 更新场景
	PAL_BattleMakeScene();
	VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);

	//
	// Check if the battle is over
	// 检查战斗是否结束
	if (g_Battle.fEnemyCleared)
	{
		//
		// All enemies are cleared. Won the battle.
		// 所有敌人都被清除。赢得了战斗
		g_Battle.BattleResult = kBattleResultWon;
		AUDIO_PlaySound(0);
		return;
	}
	else
	{
		BOOL fEnded = TRUE;

		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

			if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] != 0)
			{
				fOnlyPuppet = FALSE;
				fEnded = FALSE;
				break;
			}
			else if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusPuppet] != 0)
			{
				fOnlyPuppet = FALSE;
			}
		}

		if (fEnded)
		{
			//
			// All players are dead. Lost the battle.
			// 所有玩家都死了。输掉了战斗。
			g_Battle.BattleResult = kBattleResultLost;
			return;
		}
	}

#ifndef PAL_CLASSIC
	//
	// Check for hiding status
	// 检查隐藏状态
	if (g_Battle.iHidingTime > 0)
	{
		if (PAL_GetTimeChargingSpeed(9999) > 0)
		{
			g_Battle.iHidingTime--;
		}

		if (g_Battle.iHidingTime == 0)
		{
			VIDEO_BackupScreen(g_Battle.lpSceneBuf);
			PAL_BattleMakeScene();
			PAL_BattleFadeScene();
		}
	}

	//
	// Run the logic for all enemies
	//
	for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
	{
		if (g_Battle.rgEnemy[i].wObjectID == 0)
		{
			continue;
		}

		if (g_Battle.rgEnemy[i].fTurnStart)
		{
			g_Battle.rgEnemy[i].wScriptOnTurnStart =
				PAL_RunTriggerScript(g_Battle.rgEnemy[i].wScriptOnTurnStart, i, FALSE);

			g_Battle.rgEnemy[i].fTurnStart = FALSE;
			fMoved = TRUE;
		}
	}

	for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
	{
		if (g_Battle.rgEnemy[i].wObjectID == 0)
		{
			continue;
		}

		switch (g_Battle.rgEnemy[i].state)
		{
		case kFighterWait:
			flMax = PAL_GetTimeChargingSpeed(PAL_GetEnemyDexterity(i));
			flMax /= (gpGlobals->fAutoBattle ? 2 : 1);

			if (flMax != 0)
			{
				g_Battle.rgEnemy[i].flTimeMeter += flMax;

				if (g_Battle.rgEnemy[i].flTimeMeter > 100 && flMax > 0)
				{
					if (g_Battle.iHidingTime == 0)
					{
						g_Battle.rgEnemy[i].state = kFighterCom;
					}
					else
					{
						g_Battle.rgEnemy[i].flTimeMeter = 0;
					}
				}
			}
			break;

		case kFighterCom:
			g_Battle.rgEnemy[i].wScriptOnReady =
				PAL_RunTriggerScript(g_Battle.rgEnemy[i].wScriptOnReady, i, FALSE);
			g_Battle.rgEnemy[i].state = kFighterAct;
			fMoved = TRUE;
			break;

		case kFighterAct:
			if (!fMoved && (PAL_GetTimeChargingSpeed(9999) > 0 || g_Battle.rgEnemy[i].fDualMove) && !fOnlyPuppet)
			{
				fMoved = TRUE;

				g_Battle.fEnemyMoving = TRUE;

				g_Battle.rgEnemy[i].fDualMove =
					(!g_Battle.rgEnemy[i].fFirstMoveDone &&
						(g_Battle.rgEnemy[i].e.wDualMove >= 2 ||
							(g_Battle.rgEnemy[i].e.wDualMove != 0 && RandomLong(0, 1))));

				PAL_BattleEnemyPerformAction(i);

				g_Battle.rgEnemy[i].flTimeMeter = 0;
				g_Battle.rgEnemy[i].state = kFighterWait;
				g_Battle.fEnemyMoving = FALSE;

				if (g_Battle.rgEnemy[i].fDualMove)
				{
					g_Battle.rgEnemy[i].flTimeMeter = 100;
					g_Battle.rgEnemy[i].state = kFighterCom;
					g_Battle.rgEnemy[i].fFirstMoveDone = TRUE;
				}
				else
				{
					g_Battle.rgEnemy[i].fFirstMoveDone = FALSE;
					g_Battle.rgEnemy[i].fTurnStart = TRUE;
				}
			}
			break;
		}
	}

	//
	// Update the battle UI
	//
	PAL_BattleUIUpdate();

	//
	// Run the logic for all players
	//
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

		//
		// Skip dead players
		//
		if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0 &&
			gpGlobals->rgPlayerStatus[wPlayerRole][kStatusPuppet] == 0)
		{
			g_Battle.rgPlayer[i].state = kFighterWait;
			g_Battle.rgPlayer[i].flTimeMeter = 0;
			g_Battle.rgPlayer[i].flTimeSpeedModifier = 1.0f;
			g_Battle.rgPlayer[i].sTurnOrder = -1;
			continue;
		}

		switch (g_Battle.rgPlayer[i].state)
		{
		case kFighterWait:
			wDexterity = PAL_GetPlayerActualDexterity(wPlayerRole);
			g_Battle.rgPlayer[i].flTimeMeter +=
				PAL_GetTimeChargingSpeed(wDexterity) * g_Battle.rgPlayer[i].flTimeSpeedModifier;
			break;

		case kFighterCom:
			break;

		case kFighterAct:
			if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] > 0)
			{
				g_Battle.rgPlayer[i].action.ActionType = kBattleActionPass;
				g_Battle.rgPlayer[i].action.flRemainingTime = 0;
			}
			else if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] > 0)
			{
				g_Battle.rgPlayer[i].action.ActionType =
					(PAL_IsPlayerDying(wPlayerRole) ? kBattleActionPass : kBattleActionAttackMate);
				g_Battle.rgPlayer[i].action.flRemainingTime = 0;
			}
			else if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSilence] > 0 &&
				g_Battle.rgPlayer[i].action.ActionType == kBattleActionMagic)
			{
				g_Battle.rgPlayer[i].action.flRemainingTime = 0;
			}

			wDexterity = PAL_GetPlayerActualDexterity(wPlayerRole);
			g_Battle.rgPlayer[i].action.flRemainingTime -= PAL_GetTimeChargingSpeed(wDexterity);

			if (g_Battle.rgPlayer[i].action.flRemainingTime <= 0 &&
				g_Battle.rgPlayer[i].sTurnOrder == -1)
			{
				sMax = -1;

				for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
				{
					if (g_Battle.rgPlayer[j].sTurnOrder > sMax)
					{
						sMax = g_Battle.rgPlayer[j].sTurnOrder;
					}
				}

				g_Battle.rgPlayer[i].sTurnOrder = sMax + 1;
			}

			break;
		}
	}

	//
	// Preform action for player
	//
	if (!fMoved)
	{
		sMax = 9999;
		sMaxIndex = -1;

		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

			//
			// Skip dead players
			//
			if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0 &&
				gpGlobals->rgPlayerStatus[wPlayerRole][kStatusPuppet] == 0)
			{
				continue;
			}

			if (g_Battle.rgPlayer[i].state == kFighterAct &&
				g_Battle.rgPlayer[i].sTurnOrder != -1 &&
				g_Battle.rgPlayer[i].sTurnOrder < sMax)
			{
				sMax = g_Battle.rgPlayer[i].sTurnOrder;
				sMaxIndex = i;
			}
		}

		if (sMaxIndex != -1)
		{
			//
			// Perform the action for this player.
			//
			PAL_BattlePlayerPerformAction(sMaxIndex);

			g_Battle.rgPlayer[sMaxIndex].flTimeMeter = 0;
			g_Battle.rgPlayer[sMaxIndex].flTimeSpeedModifier = 1.0f;
			g_Battle.rgPlayer[sMaxIndex].sTurnOrder = -1;
		}
	}
#else
	if (g_Battle.Phase == kBattlePhaseSelectAction)
	{
		if (g_Battle.UI.state == kBattleUIWait)
		{
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

				//
				// Don't select action for this player if player is KO'ed,
				// sleeped, confused or paralyzed
				// 如果玩家状态为乱定封亡，则不要为该玩家选择动作，
				if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0 ||
					gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] ||
					gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] ||
					gpGlobals->rgPlayerStatus[wPlayerRole][kStatusParalyzed])
				{
					continue;
				}

				//
				// Start the menu for the first player whose action is not
				// yet selected
				// 为第一个尚未选择动作的队员启动菜单
				if (g_Battle.rgPlayer[i].state == kFighterWait)
				{
					g_Battle.wMovingPlayerIndex = i;
					g_Battle.rgPlayer[i].state = kFighterCom;
					PAL_BattleUIPlayerReady(i);
					break;
				}
				else if (g_Battle.rgPlayer[i].action.ActionType == kBattleActionCoopMagic)
				{
					//
					// Skip other players if someone selected coopmagic
					// 如果有队员选择了合体仙术，则跳过其他玩家
					i = gpGlobals->wMaxPartyMemberIndex + 1;
					break;
				}
			}

			if (i > gpGlobals->wMaxPartyMemberIndex)
			{
				//
				// Backup all actions once not repeating.
				// 备份所有操作一次，不再重复。
				if (!g_Battle.fRepeat)
				{
					for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
					{
						g_Battle.rgPlayer[i].prevAction = g_Battle.rgPlayer[i].action;
					}
				}

				//
				// actions for all players are decided. fill in the action queue.
				// 决定所有队员的行动。填写操作队列。
				g_Battle.fRepeat = FALSE;
				g_Battle.fForce = FALSE;
				g_Battle.fFlee = FALSE;
				g_Battle.fPrevAutoAtk = g_Battle.UI.fAutoAttack;
				g_Battle.fPrevPlayerAutoAtk = FALSE;

				g_Battle.iCurAction = 0;

				for (i = 0; i < MAX_ACTIONQUEUE_ITEMS; i++)
				{
					g_Battle.ActionQueue[i].wIndex = 0xFFFF;
					g_Battle.ActionQueue[i].fIsSecond = FALSE;
					g_Battle.ActionQueue[i].wDexterity = 0xFFFF;
				}

				j = 0;

				//
				// Put all enemies into action queue
				// 将所有敌人放入行动队列
				for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
				{
					if (g_Battle.rgEnemy[i].wObjectID == 0)
					{
						continue;
					}

					//g_Battle.ActionQueue[j].fIsEnemy = TRUE;
					//g_Battle.ActionQueue[j].wIndex = i;
					//g_Battle.ActionQueue[j].fIsSecond = FALSE;
					//g_Battle.ActionQueue[j].wDexterity = PAL_GetEnemyDexterity(i);
					//g_Battle.ActionQueue[j].wDexterity *= RandomFloat(0.9f, 1.1f);

					//j++;

					//if (g_Battle.rgEnemy[i].e.wDualMove)
					for (l = 1; l <= g_Battle.rgEnemy[i].e.wDualMove; l++)
					{
						g_Battle.ActionQueue[j].fIsEnemy = TRUE;
						g_Battle.ActionQueue[j].wIndex = i;
						g_Battle.ActionQueue[j].fIsSecond = FALSE;
						g_Battle.ActionQueue[j].wDexterity = PAL_GetEnemyDexterity(i);
						g_Battle.ActionQueue[j].wDexterity *= RandomFloat(0.9f, 1.1f);

						if (g_Battle.ActionQueue[j].wDexterity <= g_Battle.ActionQueue[j - l - 1].wDexterity)
							g_Battle.ActionQueue[j].fIsSecond = TRUE;
						else
							g_Battle.ActionQueue[j - 1].fIsSecond = TRUE;

						j++;
					}
				}

				//
				// Put all players into action queue
				// 将所有玩家放入行动队列
				for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
				{
					wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

					g_Battle.ActionQueue[j].fIsEnemy = FALSE;
					g_Battle.ActionQueue[j].wIndex = i;

					if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0 ||
						gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] > 0 ||
						gpGlobals->rgPlayerStatus[wPlayerRole][kStatusParalyzed] > 0)
					{
						//
						// players who are unable to move should attack physically if recovered
						// in the same turn
						// 无法移动的队员如果恢复，应进行身体攻击
						// 同一回合
						g_Battle.ActionQueue[j].wDexterity = 0;
						g_Battle.rgPlayer[i].action.ActionType = kBattleActionAttack;
						g_Battle.rgPlayer[i].action.wActionID = 0;
						g_Battle.rgPlayer[i].state = kFighterAct;
					}
					else
					{
						wDexterity = PAL_GetPlayerActualDexterity(wPlayerRole);

						if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] > 0)
						{
							g_Battle.rgPlayer[i].action.ActionType = kBattleActionAttack;
							g_Battle.rgPlayer[i].action.wActionID = 0; //avoid be deduced to autoattack 避免被推断为自动攻击
							g_Battle.rgPlayer[i].state = kFighterAct;
						}

						switch (g_Battle.rgPlayer[i].action.ActionType)
						{
						case kBattleActionCoopMagic:
							wDexterity *= 10;
							break;

						case kBattleActionDefend:
							wDexterity *= 5;
							break;

						case kBattleActionMagic:
							if ((gpGlobals->g.rgObject[g_Battle.rgPlayer[i].action.wActionID].magic.wFlags & kMagicFlagUsableToEnemy) == 0)
							{
								wDexterity *= 3;
							}
							break;

						case kBattleActionFlee:
							wDexterity /= 2;
							break;

						case kBattleActionUseItem:
							wDexterity *= 3;
							break;

						default:
							break;
						}

						if (PAL_IsPlayerDying(wPlayerRole))
						{
							wDexterity /= 2;
						}

						wDexterity *= RandomFloat(0.9f, 1.1f);

						g_Battle.ActionQueue[j].wDexterity = wDexterity;
					}

					j++;
				}

				//
				// Sort the action queue by dexterity value
				// 按身法值排序动作队列
				for (i = 0; i < MAX_ACTIONQUEUE_ITEMS; i++)
				{
					for (j = i; j < MAX_ACTIONQUEUE_ITEMS; j++)
					{
						if ((SHORT)g_Battle.ActionQueue[i].wDexterity < (SHORT)g_Battle.ActionQueue[j].wDexterity)
						{
							ACTIONQUEUE t = g_Battle.ActionQueue[i];
							g_Battle.ActionQueue[i] = g_Battle.ActionQueue[j];
							g_Battle.ActionQueue[j] = t;
						}
					}
				}

				//
				// Perform the actions
				// 执行操作
				g_Battle.Phase = kBattlePhasePerformAction;
			}
		}
	}
	else
	{
		//
		// Are all actions finished?
		// 所有操作都完成了吗？
		if (g_Battle.iCurAction >= MAX_ACTIONQUEUE_ITEMS ||
			g_Battle.ActionQueue[g_Battle.iCurAction].wDexterity == 0xFFFF)
		{
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				g_Battle.rgPlayer[i].fDefending = FALSE;
				//
				// Restore player pos from MANUAL defending
				// 从手动防守恢复球员位置
				g_Battle.rgPlayer[i].pos = g_Battle.rgPlayer[i].posOriginal;
			}

			//
			// Run poison scripts
			// 运行毒物脚本
			PAL_BattleBackupStat();

			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

				for (j = 0; j < MAX_POISONS; j++)
				{
					if (gpGlobals->rgPoisonStatus[j][i].wPoisonID != 0)
					{
						gpGlobals->rgPoisonStatus[j][i].wPoisonScript = PAL_RunTriggerScript(gpGlobals->rgPoisonStatus[j][i].wPoisonScript, wPlayerRole, FALSE);
					}
				}

				//
				// Update statuses
				// 更新队员状态
				for (j = 0; j < kStatusAll; j++)
				{
					if (gpGlobals->rgPlayerStatus[wPlayerRole][j] > 0)
					{
						gpGlobals->rgPlayerStatus[wPlayerRole][j]--;
					}
				}
			}

			for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
			{
				for (j = 0; j < MAX_POISONS; j++)
				{
					if (g_Battle.rgEnemy[i].rgPoisons[j].wPoisonID != 0)
					{
						g_Battle.rgEnemy[i].rgPoisons[j].wPoisonScript = PAL_RunTriggerScript(g_Battle.rgEnemy[i].rgPoisons[j].wPoisonScript, (WORD)i, FALSE);
					}
				}

				//
				// Update statuses
				// 更新敌方状态
				for (j = 0; j < kStatusAll; j++)
				{
					if (g_Battle.rgEnemy[i].rgwStatus[j] > 0)
					{
						g_Battle.rgEnemy[i].rgwStatus[j]--;
					}
				}
			}

			PAL_BattlePostActionCheck(FALSE);
			if (PAL_BattleDisplayStatChange())
			{
				PAL_BattleDelay(8, 0, TRUE);
			}

			// 增加回合数，若达到规定回合数则对敌造成指定累计伤害的二倍，该值将不大于两万
			if (g_Battle.rcmMagicType.wMaxCumulativeRrounds != 0)
			{
				if (++g_Battle.rcmMagicType.wCurrentCumulativeRrounds >= g_Battle.rcmMagicType.wMaxCumulativeRrounds)
				{

					// 显示系统出招
					g_Battle.wMagicMoving = g_Battle.rcmMagicType.wCumulativeRroundsObjectID;

					// 计算伤害
					i = g_Battle.rcmMagicType.wCumulativeDamageValue * g_Battle.rcmMagicType.wCumulativeDamageValueMultiple;

					// 使用后续指定法术（这里伤害计算用到了防御值等数据的介入，因此不适用于直接性伤害）
					//PAL_BattleSimulateMagic(0xFFFF, g_Battle.rcmMagicType.wCumulativeRroundsObjectID, min(20000, i));

					// 对敌造成真实伤害，并将伤害控制在20000内
					PAL_New_BattleSimulateMagicRealInjury(0xFFFF, g_Battle.rcmMagicType.wCumulativeRroundsObjectID, min(99999999, i));

					// 关闭系统出招
					g_Battle.wMagicMoving = 0;

					// 回合、伤害累计置0
					g_Battle.rcmMagicType.wMaxCumulativeRrounds = 0;
					g_Battle.rcmMagicType.wCurrentCumulativeRrounds = 0;
					g_Battle.rcmMagicType.wCumulativeDamageValue = 0;
				}
			}

			if (g_Battle.iHidingTime > 0)
			{
				if (--g_Battle.iHidingTime == 0)
				{
					VIDEO_BackupScreen(g_Battle.lpSceneBuf);
					PAL_BattleMakeScene();
					PAL_BattleFadeScene();
				}
			}

			if (g_Battle.iHidingTime == 0)
			{
				for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
				{
					if (g_Battle.rgEnemy[i].wObjectID == 0)
					{
						continue;
					}

					g_Battle.rgEnemy[i].wScriptOnTurnStart =
						PAL_RunTriggerScript(g_Battle.rgEnemy[i].wScriptOnTurnStart, i, FALSE);
				}
			}

			//
			// Clear all item-using records
			// 使用记录清除所有项目
			for (i = 0; i < MAX_INVENTORY; i++)
			{
				gpGlobals->rgInventory[i].nAmountInUse = 0;
			}

			//
			// Proceed to next turn...
			// 回合数自增并继续下一轮。。。
			g_Battle.rcmMagicType.wCurrentAllRrounds++;
			g_Battle.Phase = kBattlePhaseSelectAction;
#ifdef PAL_CLASSIC
			g_Battle.fThisTurnCoop = FALSE;
#endif
		}
		else
		{
			i = g_Battle.ActionQueue[g_Battle.iCurAction].wIndex;

			if (g_Battle.ActionQueue[g_Battle.iCurAction].fIsEnemy)
			{
				if (g_Battle.iHidingTime == 0 && !fOnlyPuppet &&
					g_Battle.rgEnemy[i].wObjectID != 0)
				{
					g_Battle.rgEnemy[i].wScriptOnReady =
						PAL_RunTriggerScript(g_Battle.rgEnemy[i].wScriptOnReady, i, FALSE);

					g_Battle.fEnemyMoving = TRUE;
					PAL_BattleEnemyPerformAction(i);
					g_Battle.fEnemyMoving = FALSE;
				}
			}
			else if (g_Battle.rgPlayer[i].state == kFighterAct)
			{
				wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

				if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0)
				{
					if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusPuppet] == 0)
					{
						g_Battle.rgPlayer[i].action.ActionType = kBattleActionPass;
					}
				}
				else if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] > 0 ||
					gpGlobals->rgPlayerStatus[wPlayerRole][kStatusParalyzed] > 0)
				{
					g_Battle.rgPlayer[i].action.ActionType = kBattleActionPass;
				}
				else if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] > 0)
				{
					g_Battle.rgPlayer[i].action.ActionType =
						(PAL_IsPlayerDying(wPlayerRole) ? kBattleActionPass : kBattleActionAttackMate);
				}
				else if (g_Battle.rgPlayer[i].action.ActionType == kBattleActionAttack &&
					g_Battle.rgPlayer[i].action.wActionID != 0)
				{
					g_Battle.fPrevPlayerAutoAtk = TRUE;
				}
				else if (g_Battle.fPrevPlayerAutoAtk)
				{
					g_Battle.UI.wCurPlayerIndex = i;
					g_Battle.UI.iSelectedIndex = g_Battle.rgPlayer[i].action.sTarget;
					g_Battle.UI.wActionType = kBattleActionAttack;
					PAL_BattleCommitAction(FALSE);
				}

				//
				// Perform the action for this player.
				// 执行此玩家的操作。
				g_Battle.wMovingPlayerIndex = i;
				PAL_BattlePlayerPerformAction(i);
			}

			g_Battle.iCurAction++;
		}
	}

	//
	// The R and F keys and Fleeing should affect all players
	// R键和F键以及逃跑应该会影响所有玩家
	if (g_Battle.UI.MenuState == kBattleMenuMain &&
		g_Battle.UI.state == kBattleUISelectMove)
	{
		if (g_InputState.dwKeyPress & kKeyRepeat)
		{
			g_Battle.fRepeat = TRUE;
			g_Battle.UI.fAutoAttack = g_Battle.fPrevAutoAtk;
		}
		else if (g_InputState.dwKeyPress & kKeyForce)
		{
			g_Battle.fForce = TRUE;
		}
	}

	if (g_Battle.fRepeat)
	{
		g_InputState.dwKeyPress = kKeyRepeat;
	}
	else if (g_Battle.fForce)
	{
		g_InputState.dwKeyPress = kKeyForce;
	}
	else if (g_Battle.fFlee)
	{
		g_InputState.dwKeyPress = kKeyFlee;
	}

	//
	// Update the battle UI
	// 更新战场UI
	PAL_BattleUIUpdate();

#endif
}

VOID
PAL_BattleCommitAction(
	BOOL           fRepeat
)
/*++
  Purpose:

	Commit the action which the player decided.
	执行玩家决定的动作。

  Parameters:

	[IN]  fRepeat - TRUE if repeat the last action.

  Return value:

	None.

--*/
{
	WORD      wIndex, wActionID, wType, wMPNotEnough, wSPNotEnough;

	if (!fRepeat)
	{
		// clear action cache first; avoid cache pollution
		// 首先清除动作缓存；避免缓存污染
		memset(&g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action, 0, sizeof(g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action));
		g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.ActionType =
			g_Battle.UI.wActionType;
		g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.sTarget =
			(SHORT)g_Battle.UI.iSelectedIndex;

		if (g_Battle.UI.wActionType == kBattleActionAttack)
		{
			g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.wActionID =
				(g_Battle.UI.fAutoAttack ? 1 : 0);
		}
		else
		{
			g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.wActionID =
				g_Battle.UI.wObjectID;
		}
#ifndef PAL_CLASSIC
		g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].prevAction =
			g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action;
#endif
	}
	else
	{
		SHORT target = g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.sTarget;
		g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action =
			g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].prevAction;
		g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.sTarget = target;

		if (g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.ActionType == kBattleActionPass)
		{
			g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.ActionType = kBattleActionAttack;
			g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.wActionID = 0;
			g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.sTarget = -1;
		}
	}

	//
	// Check if the action is valid
	// 检查操作是否有效
	switch (g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.ActionType)
	{
	case kBattleActionMagic:
		wActionID = g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.wActionID;
		wMPNotEnough = gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[wActionID].magic.wMagicNumber].wCostMP;
		wSPNotEnough = gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[wActionID].magic.wMagicNumber].wCostSP;

		if (gpGlobals->g.PlayerRoles.rgwMP[gpGlobals->rgParty[g_Battle.UI.wCurPlayerIndex].wPlayerRole] < wMPNotEnough ||
			gpGlobals->g.PlayerRoles.rgwSP[gpGlobals->rgParty[g_Battle.UI.wCurPlayerIndex].wPlayerRole] < wSPNotEnough)
		{
			wType = gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[wActionID].magic.wMagicNumber].wType;

			if (wType == kMagicTypeApplyToPlayer || wType == kMagicTypeApplyToParty ||
				wType == kMagicTypeApplyToCasterHimself || wType == kMagicTypeTrance)
			{
				g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.ActionType = kBattleActionDefend;
			}
			else
			{
				g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.ActionType = kBattleActionAttack;
				if (g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.sTarget == -1)
				{
					g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.sTarget = 0;
				}
				g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.wActionID = 0;
			}
		}
		break;

#ifdef PAL_CLASSIC
	case kBattleActionUseItem:
		if ((gpGlobals->g.rgObject[g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.wActionID].item.wFlags & kItemFlagConsuming) == 0)
		{
			break;
		}

	case kBattleActionThrowItem:
		for (wIndex = 0; wIndex < MAX_INVENTORY; wIndex++)
		{
			if (gpGlobals->rgInventory[wIndex].wItem == g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.wActionID)
			{
				gpGlobals->rgInventory[wIndex].nAmountInUse++;
				break;
			}
		}
		break;
#endif

	default:
		break;
	}

#ifndef PAL_CLASSIC
	//
	// Calculate the waiting time for the action
	// 计算操作的等待时间
	switch (g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.ActionType)
	{
	case kBattleActionMagic:
	{
		LPMAGIC      p;
		WORD         wCostMP;

		//
		// The base casting time of magic is set to the MP costed
		// 魔法的基础施法时间设置为MP costed
		w = g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.wActionID;
		p = &(gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[w].magic.wMagicNumber]);
		wCostMP = p->wCostMP;

		if (wCostMP == 1)
		{
			if (p->wType == kMagicTypeSummon)
			{
				//
				// The Wine God is an ultimate move which should take long
				// 酒神是一个最终的动作，需要很长时间
				wCostMP = 175;
			}
		}
		else if (p->wType == kMagicTypeApplyToPlayer || p->wType == kMagicTypeApplyToParty ||
			p->wType == kMagicTypeTrance)
		{
			//
			// Healing magics should take shorter
			// 治疗魔法需要更短的时间
			wCostMP /= 3;
		}

		g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.flRemainingTime = wCostMP + 5;
	}
	break;

	case kBattleActionAttack:
	case kBattleActionFlee:
	case kBattleActionUseItem:
	case kBattleActionThrowItem:
	default:
		//
		// Other actions take no time
		// 其他操作不需要时间
		g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.flRemainingTime = 0;
		break;
	}
#else
	if (g_Battle.UI.wActionType == kBattleActionFlee)
	{
		g_Battle.fFlee = TRUE;
	}
#endif

	g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].state = kFighterAct;
	g_Battle.UI.state = kBattleUIWait;

#ifndef PAL_CLASSIC
	if (g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.flRemainingTime <= 0)
	{
		SHORT sMax = -1;

		for (w = 0; w <= gpGlobals->wMaxPartyMemberIndex; w++)
		{
			if (g_Battle.rgPlayer[w].sTurnOrder > sMax)
			{
				sMax = g_Battle.rgPlayer[w].sTurnOrder;
			}
		}

		g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].sTurnOrder = sMax + 1;
	}
	else
	{
		g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].sTurnOrder = -1;
	}
#endif
}

static VOID
PAL_BattleShowPlayerAttackAnim(
	WORD        wPlayerIndex,
	BOOL        fCritical
)
/*++
  Purpose:

	Show the physical attack effect for player.
	显示玩家的物理攻击效果。

  Parameters:

	[IN]  wPlayerIndex - the index of the player.

	[IN]  fCritical - TRUE if this is a critical hit.

  Return value:

	None.

--*/
{
	WORD wPlayerRole = gpGlobals->rgParty[wPlayerIndex].wPlayerRole;
	SHORT sTarget = g_Battle.rgPlayer[wPlayerIndex].action.sTarget;

	int index, i, j;
	int enemy_x = 0, enemy_y = 0, enemy_h = 0, x, y, dist = 0;

	DWORD dwTime;

	if (sTarget != -1)
	{
		enemy_x = PAL_X(g_Battle.rgEnemy[sTarget].pos);
		enemy_y = PAL_Y(g_Battle.rgEnemy[sTarget].pos);

		enemy_h = PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle.rgEnemy[sTarget].lpSprite, g_Battle.rgEnemy[sTarget].wCurrentFrame));

		if (sTarget >= 3)
		{
			dist = (sTarget - wPlayerIndex) * 8;
		}
	}
	else
	{
		enemy_x = 150;
		enemy_y = 100;
	}

	index = gpGlobals->g.rgwBattleEffectIndex[PAL_GetPlayerBattleSprite(wPlayerRole)][1];
	index *= 3;

	//
	// Play the attack voice
	// 播放攻击语音
	if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] > 0)
	{
		if (!fCritical)
		{
			AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwAttackSound[wPlayerRole]);
		}
		else
		{
			AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwCriticalSound[wPlayerRole]);
		}
	}

	//
	// Show the animation
	// 显示动画
	x = enemy_x - dist + 64;
	y = enemy_y + dist + 20;

	g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 8;
	g_Battle.rgPlayer[wPlayerIndex].pos = PAL_XY(x, y);

	PAL_BattleDelay(2, 0, TRUE);

	x -= 10;
	y -= 2;
	g_Battle.rgPlayer[wPlayerIndex].pos = PAL_XY(x, y);

	PAL_BattleDelay(1, 0, TRUE);

	g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 9;
	x -= 16;
	y -= 4;

	AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwWeaponSound[wPlayerRole]);

	x = enemy_x;
	y = enemy_y - enemy_h / 3 + 10;

	dwTime = SDL_GetTicks() + BATTLE_FRAME_TIME;

	for (i = 0; i < 3; i++)
	{
		LPCBITMAPRLE b = PAL_SpriteGetFrame(g_Battle.lpEffectSprite, index++);

		//
		// Wait for the time of one frame. Accept input here.
		// 等待一帧的时间。在此接受输入。
		PAL_DelayUntil(dwTime);

		//
		// Set the time of the next frame.
		// 设置下一帧的时间。
		dwTime = SDL_GetTicks() + BATTLE_FRAME_TIME;

		//
		// Update the gesture of enemies.
		// 更新敌人的姿势。
		for (j = 0; j <= g_Battle.wMaxEnemyIndex; j++)
		{
			if (g_Battle.rgEnemy[j].wObjectID == 0 ||
				g_Battle.rgEnemy[j].rgwStatus[kStatusSleep] > 0 ||
				g_Battle.rgEnemy[j].rgwStatus[kStatusParalyzed] > 0)
			{
				continue;
			}

			if (--g_Battle.rgEnemy[j].e.wIdleAnimSpeed == 0)
			{
				g_Battle.rgEnemy[j].wCurrentFrame++;
				g_Battle.rgEnemy[j].e.wIdleAnimSpeed =
					gpGlobals->g.lprgEnemy[gpGlobals->g.rgObject[g_Battle.rgEnemy[j].wObjectID].enemy.wEnemyID].wIdleAnimSpeed;
			}

			if (g_Battle.rgEnemy[j].wCurrentFrame >= g_Battle.rgEnemy[j].e.wIdleFrames)
			{
				g_Battle.rgEnemy[j].wCurrentFrame = 0;
			}
		}

		PAL_BattleMakeScene();
		VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);

		PAL_RLEBlitToSurface(b, gpScreen, PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));
		x -= 16;
		y += 16;

		PAL_BattleUIUpdate();

		if (i == 0)
		{
			if (sTarget == -1)
			{
				for (j = 0; j <= g_Battle.wMaxEnemyIndex; j++)
				{
					g_Battle.rgEnemy[j].iColorShift = 6;
				}
			}
			else
			{
				g_Battle.rgEnemy[sTarget].iColorShift = 6;
			}

			PAL_BattleDisplayStatChange();
			PAL_BattleBackupStat();
		}

		VIDEO_UpdateScreen(NULL);

		if (i == 1)
		{
			g_Battle.rgPlayer[wPlayerIndex].pos =
				PAL_XY(PAL_X(g_Battle.rgPlayer[wPlayerIndex].pos) + 2,
					PAL_Y(g_Battle.rgPlayer[wPlayerIndex].pos) + 1);
		}
	}

	dist = 8;

	for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
	{
		g_Battle.rgEnemy[i].iColorShift = 0;
	}

	if (sTarget == -1)
	{
		for (i = 0; i < 3; i++)
		{
			for (j = 0; j <= g_Battle.wMaxEnemyIndex; j++)
			{
				x = PAL_X(g_Battle.rgEnemy[j].pos);
				y = PAL_Y(g_Battle.rgEnemy[j].pos);

				x -= dist;
				//            y -= dist / 2;
				g_Battle.rgEnemy[j].pos = PAL_XY(x, y);
			}

			PAL_BattleDelay(1, 0, TRUE);
			dist /= -2;
		}
	}
	else
	{
		x = PAL_X(g_Battle.rgEnemy[sTarget].pos);
		y = PAL_Y(g_Battle.rgEnemy[sTarget].pos);

		for (i = 0; i < 3; i++)
		{
			x -= dist;
			dist /= -2;
			y += dist;
			g_Battle.rgEnemy[sTarget].pos = PAL_XY(x, y);

			PAL_BattleDelay(1, 0, TRUE);
		}
	}
}

static VOID
PAL_BattleShowPlayerUseItemAnim(
	WORD         wPlayerIndex,
	WORD         wObjectID,
	SHORT        sTarget
)
/*++
  Purpose:

	Show the "use item" effect for player.
	为玩家显示“使用物品”效果。

  Parameters:

	[IN]  wPlayerIndex - the index of the player.

	[IN]  wObjectID - the object ID of the item to be used.

	[IN]  sTarget - the target player of the action.

  Return value:

	None.

--*/
{
	int i, j;

	PAL_BattleDelay(4, 0, TRUE);

	g_Battle.rgPlayer[wPlayerIndex].pos =
		PAL_XY(PAL_X(g_Battle.rgPlayer[wPlayerIndex].pos) - 15,
			PAL_Y(g_Battle.rgPlayer[wPlayerIndex].pos) - 7);

	g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 5;

	AUDIO_PlaySound(28);

	for (i = 0; i <= 6; i++)
	{
		if (sTarget == -1)
		{
			for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
			{
				g_Battle.rgPlayer[j].iColorShift = i;
			}
		}
		else
		{
			g_Battle.rgPlayer[sTarget].iColorShift = i;
		}

		PAL_BattleDelay(1, wObjectID, TRUE);
	}

	for (i = 5; i >= 0; i--)
	{
		if (sTarget == -1)
		{
			for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
			{
				g_Battle.rgPlayer[j].iColorShift = i;
			}
		}
		else
		{
			g_Battle.rgPlayer[sTarget].iColorShift = i;
		}

		PAL_BattleDelay(1, wObjectID, TRUE);
	}
}

VOID
PAL_BattleShowPlayerPreMagicAnim(
	WORD         wPlayerIndex,
	BOOL         fSummon
)
/*++
  Purpose:

	Show the effect for player before using a magic.
	在使用魔法之前为玩家展示效果。

  Parameters:

	[IN]  wPlayerIndex - the index of the player.

	[IN]  fSummon - TRUE if player is using a summon magic.

  Return value:

	None.

--*/
{
	int   i, j;
	DWORD dwTime = SDL_GetTicks();
	WORD  wPlayerRole = gpGlobals->rgParty[wPlayerIndex].wPlayerRole;

	for (i = 0; i < 4; i++)
	{
		g_Battle.rgPlayer[wPlayerIndex].pos =
			PAL_XY(PAL_X(g_Battle.rgPlayer[wPlayerIndex].pos) - (4 - i),
				PAL_Y(g_Battle.rgPlayer[wPlayerIndex].pos) - (4 - i) / 2);

		PAL_BattleDelay(1, 0, TRUE);
	}

	PAL_BattleDelay(2, 0, TRUE);

	g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 5;
	if (!gConfig.fIsWIN95)
	{
		AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwMagicSound[wPlayerRole]);
	}

	if (!fSummon)
	{
		int x, y, index;

		x = PAL_X(g_Battle.rgPlayer[wPlayerIndex].pos);
		y = PAL_Y(g_Battle.rgPlayer[wPlayerIndex].pos);

		index = gpGlobals->g.rgwBattleEffectIndex[PAL_GetPlayerBattleSprite(wPlayerRole)][0];
		index *= 10;
		index += 15;
		if (gConfig.fIsWIN95)
		{
			AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwMagicSound[wPlayerRole]);
		}
		for (i = 0; i < 10; i++)
		{
			LPCBITMAPRLE b = PAL_SpriteGetFrame(g_Battle.lpEffectSprite, index++);

			//
			// Wait for the time of one frame. Accept input here.
			// 等待一帧的时间。在此接受输入。
			PAL_DelayUntil(dwTime);

			//
			// Set the time of the next frame.
			// 设置下一帧的时间。
			dwTime = SDL_GetTicks() + BATTLE_FRAME_TIME;

			//
			// Update the gesture of enemies.
			// 更新敌人的姿势。
			for (j = 0; j <= g_Battle.wMaxEnemyIndex; j++)
			{
				if (g_Battle.rgEnemy[j].wObjectID == 0 ||
					g_Battle.rgEnemy[j].rgwStatus[kStatusSleep] != 0 ||
					g_Battle.rgEnemy[j].rgwStatus[kStatusParalyzed] != 0)
				{
					continue;
				}

				if (--g_Battle.rgEnemy[j].e.wIdleAnimSpeed == 0)
				{
					g_Battle.rgEnemy[j].wCurrentFrame++;
					g_Battle.rgEnemy[j].e.wIdleAnimSpeed =
						gpGlobals->g.lprgEnemy[gpGlobals->g.rgObject[g_Battle.rgEnemy[j].wObjectID].enemy.wEnemyID].wIdleAnimSpeed;
				}

				if (g_Battle.rgEnemy[j].wCurrentFrame >= g_Battle.rgEnemy[j].e.wIdleFrames)
				{
					g_Battle.rgEnemy[j].wCurrentFrame = 0;
				}
			}

			PAL_BattleMakeScene();
			VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);

			PAL_RLEBlitToSurface(b, gpScreen, PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));

			PAL_BattleUIUpdate();

			VIDEO_UpdateScreen(NULL);
		}
	}

	PAL_BattleDelay(1, 0, TRUE);
}

static VOID
PAL_BattleShowPlayerDefMagicAnim(
	WORD         wPlayerIndex,
	WORD         wObjectID,
	SHORT        sTarget
)
/*++
  Purpose:

	Show the defensive magic effect for player.
	展示队员的防御魔法效果。

  Parameters:

	[IN]  wPlayerIndex - the index of the player.

	[IN]  wObjectID - the object ID of the magic to be used.

	[IN]  sTarget - the target player of the action.

  Return value:

	None.

--*/
{
	int        l, iMagicNum, iEffectNum, n, i, j, x, y;
	DWORD      dwTime = SDL_GetTicks();

	iMagicNum = gpGlobals->g.rgObject[wObjectID].magic.wMagicNumber;
	iEffectNum = gpGlobals->g.lprgMagic[iMagicNum].wEffect;

	// 获取MKF解压缩大小（子文件解压缩后的大小）
	l = PAL_MKFGetChunkSize(iEffectNum, gpGlobals->f.fpFIRE);
	if (l <= 0)
	{
		return;
	}

	// 开辟内存空间，申请内存65535字节，使 lpSpriteEffect 指向这段内存
	LPSPRITE   lpSpriteEffect = (LPSPRITE)UTIL_malloc(65535), lpNullBreak = (LPSPRITE)UTIL_malloc(65535);

	// 获取该仙术的特效总帧数
	n = 0;
	while (PAL_New_DoubleMKFGetFrame(lpNullBreak, iEffectNum, n, gpGlobals->f.fpFIRE) != NULL)
		n++;

	g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 6;
	PAL_BattleDelay(1, 0, TRUE);

	for (i = 0; i < n; i++)
	{
		LPCBITMAPRLE b = PAL_New_DoubleMKFGetFrame(lpSpriteEffect, iEffectNum, i, gpGlobals->f.fpFIRE);

		if (i == (gConfig.fIsWIN95 ? 0 : gpGlobals->g.lprgMagic[iMagicNum].wFireDelay))
		{
			AUDIO_PlaySound(gpGlobals->g.lprgMagic[iMagicNum].wSound);
		}

		//
		// Wait for the time of one frame. Accept input here.
		// 等待一帧的时间。在此接受输入。
		PAL_DelayUntil(dwTime);

		//
		// Set the time of the next frame.
		// 设置下一帧的时间。
		//dwTime = SDL_GetTicks() + (gpGlobals->g.lprgMagic[iMagicNum].wSpeed + 5) * 10;
		dwTime = SDL_GetTicks() + (gpGlobals->g.lprgMagic[iMagicNum].wSpeed + 5) * 4.5;

		PAL_BattleMakeScene();
		VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);

		if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeApplyToParty)
		{
			assert(sTarget == -1);

			for (l = 0; l <= gpGlobals->wMaxPartyMemberIndex; l++)
			{
				x = PAL_X(g_Battle.rgPlayer[l].pos);
				y = PAL_Y(g_Battle.rgPlayer[l].pos);

				x += (SHORT)gpGlobals->g.lprgMagic[iMagicNum].wXOffset;
				y += (SHORT)gpGlobals->g.lprgMagic[iMagicNum].wYOffset;

				PAL_RLEBlitToSurface(b, gpScreen,
					PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));
			}
		}
		else if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeApplyToPlayer
			|| gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeApplyToCasterHimself)
		{
			if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeApplyToCasterHimself)
				sTarget = wPlayerIndex;

			assert(sTarget != -1);

			x = PAL_X(g_Battle.rgPlayer[sTarget].pos);
			y = PAL_Y(g_Battle.rgPlayer[sTarget].pos);

			x += (SHORT)gpGlobals->g.lprgMagic[iMagicNum].wXOffset;
			y += (SHORT)gpGlobals->g.lprgMagic[iMagicNum].wYOffset;

			PAL_RLEBlitToSurface(b, gpScreen,
				PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));

			//
			// Repaint the previous player
			// 重新喷漆前一个玩家
			if (sTarget > 0 && g_Battle.iHidingTime == 0)
			{
				if (gpGlobals->rgPlayerStatus[gpGlobals->rgParty[sTarget - 1].wPlayerRole][kStatusConfused] == 0)
				{
					LPCBITMAPRLE p = PAL_SpriteGetFrame(g_Battle.rgPlayer[sTarget - 1].lpSprite, g_Battle.rgPlayer[sTarget - 1].wCurrentFrame);

					x = PAL_X(g_Battle.rgPlayer[sTarget - 1].pos);
					y = PAL_Y(g_Battle.rgPlayer[sTarget - 1].pos);

					x -= PAL_RLEGetWidth(p) / 2;
					y -= PAL_RLEGetHeight(p);

					PAL_RLEBlitToSurface(p, gpScreen, PAL_XY(x, y));
				}
			}
		}
		else
		{
			assert(FALSE);
		}

		PAL_BattleUIUpdate();

		VIDEO_UpdateScreen(NULL);
	}

	free(lpSpriteEffect);
	free(lpNullBreak);

	for (i = 0; i < 6; i++)
	{
		if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeApplyToParty)
		{
			for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
			{
				g_Battle.rgPlayer[j].iColorShift = i;
			}
		}
		else
		{
			g_Battle.rgPlayer[sTarget].iColorShift = i;
		}

		PAL_BattleDelay(1, 0, TRUE);
	}

	for (i = 6; i >= 0; i--)
	{
		if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeApplyToParty)
		{
			for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
			{
				g_Battle.rgPlayer[j].iColorShift = i;
			}
		}
		else
		{
			g_Battle.rgPlayer[sTarget].iColorShift = i;
		}

		PAL_BattleDelay(1, 0, TRUE);
	}
}

static VOID
PAL_BattleShowPlayerOffMagicAnim(
	WORD         wPlayerIndex,
	WORD         wObjectID,
	SHORT        sTarget,
	BOOL         fSummon
)
/*++
  Purpose:

	Show the offensive magic animation for player.
	为玩家展示进攻仙术动画。

  Parameters:

	[IN]  wPlayerIndex - the index of the player.
	玩家的索引。

	[IN]  wObjectID - the object ID of the magic to be used.
	要使用的法术的对象ID。

	[IN]  sTarget - the target enemy of the action.
	行动的目标敌人。

  Return value:

	None.

--*/
{
	int        l, iMagicNum, iEffectNum, n, i, k, x, y, wave, blow;
	DWORD      dwTime = SDL_GetTicks();

	iMagicNum = gpGlobals->g.rgObject[wObjectID].magic.wMagicNumber;
	iEffectNum = gpGlobals->g.lprgMagic[iMagicNum].wEffect;

	// 获取MKF中指定的子区块的压缩块解压后的大小
	l = PAL_MKFGetChunkSize(iEffectNum, gpGlobals->f.fpFIRE);
	if (l <= 0)
	{
		return;
	}

	// 开辟内存空间，申请内存65535字节，使 lpSpriteEffect 指向这段内存
	LPSPRITE   lpSpriteEffect = (LPSPRITE)UTIL_malloc(65535), lpNullBreak = (LPSPRITE)UTIL_malloc(65535);

	// 获取该仙术的特效总帧数
	n = 0;
	while (PAL_New_DoubleMKFGetFrame(lpNullBreak, iEffectNum, n, gpGlobals->f.fpFIRE) != NULL)
		n++;

	if (gConfig.fIsWIN95 && wPlayerIndex != (WORD)-1)
	{
		g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 6;
	}

	PAL_BattleDelay(1, 0, TRUE);

	l = n - gpGlobals->g.lprgMagic[iMagicNum].wFireDelay;
	l *= (SHORT)gpGlobals->g.lprgMagic[iMagicNum].wEffectTimes;
	l += n;
	l += gpGlobals->g.lprgMagic[iMagicNum].wShake;

	wave = gpGlobals->wScreenWave;
	gpGlobals->wScreenWave += gpGlobals->g.lprgMagic[iMagicNum].wWave;

	if (gConfig.fIsWIN95 && !fSummon && gpGlobals->g.lprgMagic[iMagicNum].wSound != 0)
	{
		AUDIO_PlaySound(gpGlobals->g.lprgMagic[iMagicNum].wSound);
	}

	for (i = 0; i < l; i++)
	{
		LPCBITMAPRLE b;

		if (!gConfig.fIsWIN95 && i == gpGlobals->g.lprgMagic[iMagicNum].wFireDelay && wPlayerIndex != (WORD)-1)
		{
			g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 6;
		}
		blow = ((g_Battle.iBlow > 0) ? RandomLong(0, g_Battle.iBlow) : RandomLong(g_Battle.iBlow, 0));

		for (k = 0; k <= g_Battle.wMaxEnemyIndex; k++)
		{
			if (g_Battle.rgEnemy[k].wObjectID == 0)
			{
				continue;
			}

			x = PAL_X(g_Battle.rgEnemy[k].pos) + blow;
			y = PAL_Y(g_Battle.rgEnemy[k].pos) + blow / 2;

			g_Battle.rgEnemy[k].pos = PAL_XY(x, y);
		}

		if (l - i > gpGlobals->g.lprgMagic[iMagicNum].wShake)
		{
			if (i < n)
			{
				k = i;
			}
			else
			{
				k = i - gpGlobals->g.lprgMagic[iMagicNum].wFireDelay;
				k %= n - gpGlobals->g.lprgMagic[iMagicNum].wFireDelay;
				k += gpGlobals->g.lprgMagic[iMagicNum].wFireDelay;
			}

			b = PAL_New_DoubleMKFGetFrame(lpSpriteEffect, iEffectNum, k, gpGlobals->f.fpFIRE);

			if (!gConfig.fIsWIN95 && (i - gpGlobals->g.lprgMagic[iMagicNum].wFireDelay) % n == 0)
			{
				AUDIO_PlaySound(gpGlobals->g.lprgMagic[iMagicNum].wSound);
			}
		}
		else
		{
			VIDEO_ShakeScreen(i, 3);
			b = PAL_New_DoubleMKFGetFrame(lpSpriteEffect, iEffectNum, (l - gpGlobals->g.lprgMagic[iMagicNum].wShake - 1) % n, gpGlobals->f.fpFIRE);
		}

		//
		// Wait for the time of one frame. Accept input here.
		// 等待一帧的时间。在此接受输入。
		PAL_DelayUntil(dwTime);

		//
		// Set the time of the next frame.
		// 设置下一帧的时间。
		//dwTime = SDL_GetTicks() + (gpGlobals->g.lprgMagic[iMagicNum].wSpeed + 5) * 10;
		dwTime = SDL_GetTicks() + (gpGlobals->g.lprgMagic[iMagicNum].wSpeed + 5) * 4.5;

		PAL_BattleMakeScene();
		VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);

		if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeNormal)
		{
			assert(sTarget != -1);

			x = PAL_X(g_Battle.rgEnemy[sTarget].pos);
			y = PAL_Y(g_Battle.rgEnemy[sTarget].pos);

			x += (SHORT)gpGlobals->g.lprgMagic[iMagicNum].wXOffset;
			y += (SHORT)gpGlobals->g.lprgMagic[iMagicNum].wYOffset;

			PAL_RLEBlitToSurface(b, gpScreen, PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));

			if (i == l - 1 && gpGlobals->wScreenWave < 9 &&
				gpGlobals->g.lprgMagic[iMagicNum].wKeepEffect == 0xFFFF)
			{
				PAL_RLEBlitToSurface(b, g_Battle.lpBackground,
					PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));
			}
		}
		else if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeAttackAll)
		{
			assert(sTarget == -1);

			for (k = 0; k < 7; k++)
			{
				x = 230 - 30 * k;
				y = 70 + 10 * k;

				x += (SHORT)gpGlobals->g.lprgMagic[iMagicNum].wXOffset;
				y += (SHORT)gpGlobals->g.lprgMagic[iMagicNum].wYOffset;

				PAL_RLEBlitToSurface(b, gpScreen, PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));

				if (i == l - 1 && gpGlobals->wScreenWave < 9
					&& gpGlobals->g.lprgMagic[iMagicNum].wKeepEffect == 0xFFFF)
				{
					PAL_RLEBlitToSurface(b, g_Battle.lpBackground,
						PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));
				}
			}
		}
		else if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeAttackWhole ||
			gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeAttackField)
		{
			//
			// 提示外码出错
			assert(sTarget == -1);

			if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeAttackWhole)
			{
				x = 120;
				y = 100;
			}
			else
			{
				x = 160;
				y = 200;
			}

			x += (SHORT)gpGlobals->g.lprgMagic[iMagicNum].wXOffset;
			y += (SHORT)gpGlobals->g.lprgMagic[iMagicNum].wYOffset;

			PAL_RLEBlitToSurface(b, gpScreen, PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));

			if (i == l - 1 && gpGlobals->wScreenWave < 9 &&
				gpGlobals->g.lprgMagic[iMagicNum].wKeepEffect == 0xFFFF)
			{
				PAL_RLEBlitToSurface(b, g_Battle.lpBackground,
					PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));
			}
		}
		else
		{
			assert(FALSE);
		}

		PAL_BattleUIUpdate();
		VIDEO_UpdateScreen(NULL);
	}

	gpGlobals->wScreenWave = wave;
	VIDEO_ShakeScreen(0, 0);

	free(lpSpriteEffect);
	free(lpNullBreak);

	for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
	{
		g_Battle.rgEnemy[i].pos = g_Battle.rgEnemy[i].posOriginal;
	}
}

static VOID
PAL_BattleShowEnemyMagicAnim(
	WORD         wEnemyIndex,
	WORD         wObjectID,
	SHORT        sTarget
)
/*++
  Purpose:

	Show the offensive magic animation for enemy.
	为敌人展示进攻仙术动画。

  Parameters:

	[IN]  wObjectID - the object ID of the magic to be used.

	[IN]  sTarget - the target player index of the action.

  Return value:

	None.

--*/
{
	int        l, iMagicNum, iEffectNum, n, i, k, x, y, wave, blow;
	DWORD      dwTime = SDL_GetTicks();

	iMagicNum = gpGlobals->g.rgObject[wObjectID].magic.wMagicNumber;
	iEffectNum = gpGlobals->g.lprgMagic[iMagicNum].wEffect;

	l = PAL_MKFGetChunkSize(iEffectNum, gpGlobals->f.fpFIRE);
	if (l <= 0)
	{
		return;
	}

	// 开辟内存空间，申请内存65535字节，使 lpSpriteEffect 指向这段内存
	LPSPRITE   lpSpriteEffect = (LPSPRITE)UTIL_malloc(65535), lpNullBreak = (LPSPRITE)UTIL_malloc(65535);

	// 获取该仙术的特效总帧数
	n = 0;
	while (PAL_New_DoubleMKFGetFrame(lpNullBreak, iEffectNum, n, gpGlobals->f.fpFIRE) != NULL)
		n++;

	l = n - gpGlobals->g.lprgMagic[iMagicNum].wFireDelay;
	l *= (SHORT)gpGlobals->g.lprgMagic[iMagicNum].wEffectTimes;
	l += n;
	l += gpGlobals->g.lprgMagic[iMagicNum].wShake;

	wave = gpGlobals->wScreenWave;
	gpGlobals->wScreenWave += gpGlobals->g.lprgMagic[iMagicNum].wWave;

	for (i = 0; i < l; i++)
	{
		LPCBITMAPRLE b;

		blow = ((g_Battle.iBlow > 0) ? RandomLong(0, g_Battle.iBlow) : RandomLong(g_Battle.iBlow, 0));

		for (k = 0; k <= gpGlobals->wMaxPartyMemberIndex; k++)
		{
			x = PAL_X(g_Battle.rgPlayer[k].pos) + blow;
			y = PAL_Y(g_Battle.rgPlayer[k].pos) + blow / 2;

			g_Battle.rgPlayer[k].pos = PAL_XY(x, y);
		}

		if (l - i > gpGlobals->g.lprgMagic[iMagicNum].wShake)
		{
			if (i < n)
			{
				k = i;
			}
			else
			{
				k = i - gpGlobals->g.lprgMagic[iMagicNum].wFireDelay;
				k %= n - gpGlobals->g.lprgMagic[iMagicNum].wFireDelay;
				k += gpGlobals->g.lprgMagic[iMagicNum].wFireDelay;
			}

			b = PAL_New_DoubleMKFGetFrame(lpSpriteEffect, iEffectNum, k, gpGlobals->f.fpFIRE);

			if (i == (gConfig.fIsWIN95 ? 0 : gpGlobals->g.lprgMagic[iMagicNum].wFireDelay))
			{
				if (!gConfig.fIsWIN95 || g_Battle.rgEnemy[wEnemyIndex].e.wMagicSound >= 0)
					AUDIO_PlaySound(gpGlobals->g.lprgMagic[iMagicNum].wSound);
			}

			if (gpGlobals->g.lprgMagic[iMagicNum].wFireDelay > 0 &&
				i >= gpGlobals->g.lprgMagic[iMagicNum].wFireDelay &&
				i < gpGlobals->g.lprgMagic[iMagicNum].wFireDelay + g_Battle.rgEnemy[wEnemyIndex].e.wAttackFrames)
			{
				g_Battle.rgEnemy[wEnemyIndex].wCurrentFrame =
					i - gpGlobals->g.lprgMagic[iMagicNum].wFireDelay + g_Battle.rgEnemy[wEnemyIndex].e.wIdleFrames + g_Battle.rgEnemy[wEnemyIndex].e.wMagicFrames;
			}
		}
		else
		{
			VIDEO_ShakeScreen(i, 3);
			b = PAL_New_DoubleMKFGetFrame(lpSpriteEffect, iEffectNum, (l - gpGlobals->g.lprgMagic[iMagicNum].wShake - 1) % n, gpGlobals->f.fpFIRE);
		}

		//
		// Wait for the time of one frame. Accept input here.
		//
		PAL_DelayUntil(dwTime);

		//
		// Set the time of the next frame.
		//
		//dwTime = SDL_GetTicks() + (gpGlobals->g.lprgMagic[iMagicNum].wSpeed + 5) * 10;
		dwTime = SDL_GetTicks() + (gpGlobals->g.lprgMagic[iMagicNum].wSpeed + 5) * 4.5;

		PAL_BattleMakeScene();
		VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);

		if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeNormal)
		{
			assert(sTarget != -1);

			x = PAL_X(g_Battle.rgPlayer[sTarget].pos);
			y = PAL_Y(g_Battle.rgPlayer[sTarget].pos);

			x += (SHORT)gpGlobals->g.lprgMagic[iMagicNum].wXOffset;
			y += (SHORT)gpGlobals->g.lprgMagic[iMagicNum].wYOffset;

			PAL_RLEBlitToSurface(b, gpScreen, PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));

			if (i == l - 1 && gpGlobals->wScreenWave < 9 &&
				gpGlobals->g.lprgMagic[iMagicNum].wKeepEffect == 0xFFFF)
			{
				PAL_RLEBlitToSurface(b, g_Battle.lpBackground,
					PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));
			}
		}
		else if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeAttackAll)
		{
			//const int effectpos[3][2] = { {180, 180}, {234, 170}, {270, 146} };

			//assert(sTarget == -1);

			//for (k = 0; k < 3; k++)
			//{
			//	x = effectpos[k][0];
			//	y = effectpos[k][1];

			//	x += (SHORT)gpGlobals->g.lprgMagic[iMagicNum].wXOffset;
			//	y += (SHORT)gpGlobals->g.lprgMagic[iMagicNum].wYOffset;

			//	PAL_RLEBlitToSurface(b, gpScreen, PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));

			//	if (i == l - 1 && gpGlobals->wScreenWave < 9 &&
			//		gpGlobals->g.lprgMagic[iMagicNum].wKeepEffect == 0xFFFF)
			//	{
			//		PAL_RLEBlitToSurface(b, g_Battle.lpBackground,
			//			PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));
			//	}
			//}

			assert(sTarget == -1);

			for (k = 0; k < 7; k++)
			{
				x = 230 / 3 * 4 - 30 * k;
				y = 100 / 2 * 3 + 10 * k;

				x += (SHORT)gpGlobals->g.lprgMagic[iMagicNum].wXOffset;
				y += (SHORT)gpGlobals->g.lprgMagic[iMagicNum].wYOffset;

				PAL_RLEBlitToSurface(b, gpScreen, PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));

				if (i == l - 1 && gpGlobals->wScreenWave < 9 &&
					gpGlobals->g.lprgMagic[iMagicNum].wKeepEffect == 0xFFFF)
				{
					PAL_RLEBlitToSurface(b, g_Battle.lpBackground,
						PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));
				}
			}
		}
		else if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeAttackWhole ||
			gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeAttackField)
		{
			assert(sTarget == -1);

			if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeAttackWhole)
			{
				x = 240;
				y = 150;
			}
			else
			{
				x = 160;
				y = 200;
			}

			x += (SHORT)gpGlobals->g.lprgMagic[iMagicNum].wXOffset;
			y += (SHORT)gpGlobals->g.lprgMagic[iMagicNum].wYOffset;

			PAL_RLEBlitToSurface(b, gpScreen, PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));

			if (i == l - 1 && gpGlobals->wScreenWave < 9 &&
				gpGlobals->g.lprgMagic[iMagicNum].wKeepEffect == 0xFFFF)
			{
				PAL_RLEBlitToSurface(b, g_Battle.lpBackground,
					PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));
			}
		}
		else
		{
			assert(FALSE);
		}

		PAL_BattleUIUpdate();

		VIDEO_UpdateScreen(NULL);
	}

	gpGlobals->wScreenWave = wave;
	VIDEO_ShakeScreen(0, 0);

	free(lpSpriteEffect);
	free(lpNullBreak);

	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		g_Battle.rgPlayer[i].pos = g_Battle.rgPlayer[i].posOriginal;
	}
}

static VOID
PAL_BattleShowPlayerSummonMagicAnim(
	WORD         wPlayerIndex,
	WORD         wObjectID
)
/*++
  Purpose:

	Show the summon magic animation for player.
	显示我方召唤神仙术

  Parameters:

	[IN]  wPlayerIndex - the index of the player.
	施法队员

	[IN]  wObjectID - the object ID of the magic to be used.
	仙术ID

  Return value:

	None.

--*/
{
	int           i, j;
	WORD          wMagicNum = gpGlobals->g.rgObject[wObjectID].magic.wMagicNumber;
	WORD          wEffectMagicID = 0;
	DWORD         dwTime = SDL_GetTicks();

	for (wEffectMagicID = 0; wEffectMagicID < MAX_OBJECTS; wEffectMagicID++)
	{
		if (gpGlobals->g.rgObject[wEffectMagicID].magic.wMagicNumber ==
			gpGlobals->g.lprgMagic[wMagicNum].wEffect)
		{
			break;
		}
	}

	assert(wEffectMagicID < MAX_OBJECTS);

	//
	// Sound should be played before magic begins
	// 魔法开始前应播放声音
	if (gConfig.fIsWIN95)
	{
		AUDIO_PlaySound(gpGlobals->g.lprgMagic[wMagicNum].wSound);
	}

	//
	// Brighten the players
	// 使全体队员变亮
	for (i = 1; i <= 10; i++)
	{
		for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
		{
			g_Battle.rgPlayer[j].iColorShift = i;
		}

		PAL_BattleDelay(1, wObjectID, TRUE);
	}

	VIDEO_BackupScreen(g_Battle.lpSceneBuf);

	//
	// Load the sprite of the summoned god
	// 加载新版被召唤的神的形象
	j = gpGlobals->g.lprgMagic[wMagicNum].wSummonEffect;

	// 申请内存空间
	LPBYTE        lpNullBreak = UTIL_malloc(65535);
	g_Battle.lpSummonSprite = UTIL_malloc(65535);

	g_Battle.iSummonChinkNum = j;
	g_Battle.iSummonFrame = 0;

	g_Battle.posSummon = PAL_XY(230 + (SHORT)(gpGlobals->g.lprgMagic[wMagicNum].wXOffset),
		155 + (SHORT)(gpGlobals->g.lprgMagic[wMagicNum].wYOffset));
	g_Battle.sBackgroundColorShift = (SHORT)(gpGlobals->g.lprgMagic[wMagicNum].wEffectTimes);

	//
	// Fade in the summoned god
	// 淡入召唤的神
	PAL_BattleMakeScene();
	PAL_BattleFadeScene();

	//
	// Show the animation of the summoned god
	// TODO: There is still something missing here compared to the original game.
	// 显示被召唤的神的动画
	// TODO:与最初的游戏相比，这里仍然缺少一些东西。
	while (PAL_New_DoubleMKFGetFrame(lpNullBreak, g_Battle.iSummonChinkNum, g_Battle.iSummonFrame + 1, gpGlobals->f.fpFGOD) != NULL)
	{
		//
		// Wait for the time of one frame. Accept input here.
		// 等待一帧的时间。在此接受输入。
		PAL_DelayUntil(dwTime);

		//
		// Set the time of the next frame.
		// 设置下一帧的时间。（这里对战斗进行了适当的加速）
		//dwTime = SDL_GetTicks() + (gpGlobals->g.lprgMagic[wMagicNum].wSpeed + 5) * 10;
		dwTime = SDL_GetTicks() + (gpGlobals->g.lprgMagic[wMagicNum].wSpeed + 5) * 4.5;

		PAL_BattleMakeScene();
		VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);

		PAL_BattleUIUpdate();

		VIDEO_UpdateScreen(NULL);

		g_Battle.iSummonFrame++;
	}

	// 这个东西在这里只作为判断使用，，，不释放会浪费内存
	free(lpNullBreak);

	//
	// Show the actual magic effect
	// 显示实际的魔法效果
	PAL_BattleShowPlayerOffMagicAnim((WORD)-1, wEffectMagicID, -1, TRUE);
}

static VOID
PAL_BattleShowPostMagicAnim(
	VOID
)
/*++
  Purpose:

	Show the post-magic animation.

  Parameters:

	None

  Return value:

	None.

--*/
{
	int         i, j, x, y, dist = 8;
	PAL_POS     rgEnemyPosBak[MAX_ENEMIES_IN_TEAM];

	for (i = 0; i < MAX_ENEMIES_IN_TEAM; i++)
	{
		rgEnemyPosBak[i] = g_Battle.rgEnemy[i].pos;
	}

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j <= g_Battle.wMaxEnemyIndex; j++)
		{
			if (g_Battle.rgEnemy[j].e.wHealth == g_Battle.rgEnemy[j].wPrevHP)
			{
				continue;
			}

			x = PAL_X(g_Battle.rgEnemy[j].pos);
			y = PAL_Y(g_Battle.rgEnemy[j].pos);

			x -= dist;
			//         y -= dist / 2;

			g_Battle.rgEnemy[j].pos = PAL_XY(x, y);

			g_Battle.rgEnemy[j].iColorShift = ((i == 1) ? 6 : 0);
		}

		PAL_BattleDelay(1, 0, TRUE);
		dist /= -2;
	}

	for (i = 0; i < MAX_ENEMIES_IN_TEAM; i++)
	{
		g_Battle.rgEnemy[i].pos = rgEnemyPosBak[i];
	}

	PAL_BattleDelay(1, 0, TRUE);
}

static VOID
PAL_BattlePlayerValidateAction(
	WORD         wPlayerIndex
)
/*++
  Purpose:

	Validate player's action, fallback to other action when needed.
	验证玩家的动作，需要时回退到其他动作。

  Parameters:

	[IN]  wPlayerIndex - the index of the player.

  Return value:

	None.

--*/
{
	const WORD   wPlayerRole = gpGlobals->rgParty[wPlayerIndex].wPlayerRole;
	const WORD   wObjectID = g_Battle.rgPlayer[wPlayerIndex].action.wActionID;
	const SHORT  sTarget = g_Battle.rgPlayer[wPlayerIndex].action.sTarget;
	BOOL         fValid = TRUE, fToEnemy = FALSE;
	WORD         w;
	int          i;

	switch (g_Battle.rgPlayer[wPlayerIndex].action.ActionType)
	{
	case kBattleActionAttack:
		fToEnemy = TRUE;
		break;

	case kBattleActionPass:
		break;

	case kBattleActionDefend:
		break;

	case kBattleActionMagic:
		//
		// Make sure player actually has the magic to be used
		// 确保队员确实拥有使用的魔法
		for (i = 0; i < MAX_PLAYER_MAGICS; i++)
		{
			if (gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole] == wObjectID)
			{
				break; // player has this magic 队员确实有这种仙术，跳出并进一步验证
			}
		}

		if (i >= MAX_PLAYER_MAGICS)
		{
			fValid = FALSE;
		}

		w = gpGlobals->g.rgObject[wObjectID].magic.wMagicNumber;

		if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSilence] > 0)
		{
			//
			// Player is silenced
			// 队员被咒封
			fValid = FALSE;
		}

		if (gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] <
			gpGlobals->g.lprgMagic[w].wCostMP)
		{
			//
			// No enough MP
			// 没有足够的MP
			fValid = FALSE;
		}

		if (gpGlobals->g.PlayerRoles.rgwSP[wPlayerRole] <
			gpGlobals->g.lprgMagic[w].wCostSP)
		{
			//
			// No enough SP
			// 没有足够的SP
			fValid = FALSE;
		}

		//
		// Fallback to physical attack if player is using an offensive magic,
		// defend if player is using a defensive or healing magic
		// 如果玩家使用进攻魔法，则退回到物理攻击，如果玩家使用防守魔法或治疗魔法，则进行防守
		if (gpGlobals->g.rgObject[wObjectID].magic.wFlags & kMagicFlagUsableToEnemy)
		{
			if (!fValid)
			{
				g_Battle.rgPlayer[wPlayerIndex].action.ActionType = kBattleActionAttack;
				g_Battle.rgPlayer[wPlayerIndex].action.wActionID = 0;
			}
			else if (gpGlobals->g.rgObject[wObjectID].magic.wFlags & kMagicFlagApplyToAll)
			{
				g_Battle.rgPlayer[wPlayerIndex].action.sTarget = -1;
			}
			else if (sTarget == -1)
			{
				g_Battle.rgPlayer[wPlayerIndex].action.sTarget = PAL_BattleSelectAutoTargetFrom(g_Battle.rgPlayer[wPlayerIndex].action.sTarget);
			}

			fToEnemy = TRUE;
		}
		else
		{
			if (!fValid)
			{
				g_Battle.rgPlayer[wPlayerIndex].action.ActionType = kBattleActionDefend;
			}
			else if (gpGlobals->g.rgObject[wObjectID].magic.wFlags & kMagicFlagApplyToAll)
			{
				g_Battle.rgPlayer[wPlayerIndex].action.sTarget = -1;
			}
			else if (g_Battle.rgPlayer[wPlayerIndex].action.sTarget == -1)
			{
				g_Battle.rgPlayer[wPlayerIndex].action.sTarget = wPlayerIndex;
			}
		}
		break;

	case kBattleActionCoopMagic:
		fToEnemy = TRUE;

#ifdef PAL_CLASSIC
		{
			int iTotalHealthy = 0;
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				w = gpGlobals->rgParty[i].wPlayerRole;
				g_Battle.coopContributors[i] = PAL_IsPlayerHealthy(w);
				if (g_Battle.coopContributors[i])
					iTotalHealthy++;
			}
			if (iTotalHealthy <= 1)
			{
				g_Battle.rgPlayer[wPlayerIndex].action.ActionType = kBattleActionAttack;
				g_Battle.rgPlayer[wPlayerIndex].action.wActionID = 0;
			}
		}
#else
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			w = gpGlobals->rgParty[i].wPlayerRole;

			if (PAL_IsPlayerDying(w) ||
				gpGlobals->rgPlayerStatus[w][kStatusSilence] > 0 ||
				gpGlobals->rgPlayerStatus[w][kStatusSleep] > 0 ||
				gpGlobals->rgPlayerStatus[w][kStatusConfused] > 0 ||
				g_Battle.rgPlayer[i].flTimeMeter < 100 ||
				(g_Battle.rgPlayer[i].state == kFighterAct && i != wPlayerIndex))
			{
				g_Battle.rgPlayer[wPlayerIndex].action.ActionType = kBattleActionAttack;
				g_Battle.rgPlayer[wPlayerIndex].action.wActionID = 0;
				break;
			}
		}
#endif

		if (g_Battle.rgPlayer[wPlayerIndex].action.ActionType == kBattleActionCoopMagic)
		{
			if (gpGlobals->g.rgObject[wObjectID].magic.wFlags & kMagicFlagApplyToAll)
			{
				g_Battle.rgPlayer[wPlayerIndex].action.sTarget = -1;
			}
			else if (sTarget == -1)
			{
				g_Battle.rgPlayer[wPlayerIndex].action.sTarget = PAL_BattleSelectAutoTargetFrom(g_Battle.rgPlayer[wPlayerIndex].action.sTarget);
			}
		}
		break;

	case kBattleActionFlee:
		break;

	case kBattleActionThrowItem:
		fToEnemy = TRUE;

		if (PAL_GetItemAmount(wObjectID) == 0)
		{
			g_Battle.rgPlayer[wPlayerIndex].action.ActionType = kBattleActionAttack;
			g_Battle.rgPlayer[wPlayerIndex].action.wActionID = 0;
		}
		//else if (gpGlobals->g.rgObject[wObjectID].item.wFlags & kItemFlagApplyToAll)
		else if (gpGlobals->g.rgObject[wObjectID].item.wFlags & kItemFlagApplyToEnemyAll)
		{
			//
			// 投掷时作用于敌方全体
			//
			g_Battle.rgPlayer[wPlayerIndex].action.sTarget = -1;
		}
		else if (g_Battle.rgPlayer[wPlayerIndex].action.sTarget == -1)
		{
			g_Battle.rgPlayer[wPlayerIndex].action.sTarget = PAL_BattleSelectAutoTargetFrom(g_Battle.rgPlayer[wPlayerIndex].action.sTarget);
		}
		break;

	case kBattleActionUseItem:
		if (PAL_GetItemAmount(wObjectID) == 0)
		{
			g_Battle.rgPlayer[wPlayerIndex].action.ActionType = kBattleActionDefend;
		}
		//else if (gpGlobals->g.rgObject[wObjectID].item.wFlags & kItemFlagApplyToAll)
		else if (gpGlobals->g.rgObject[wObjectID].item.wFlags & kItemFlagApplyToPlayerAll)
		{
			g_Battle.rgPlayer[wPlayerIndex].action.sTarget = -1;
		}
		else if (g_Battle.rgPlayer[wPlayerIndex].action.sTarget == -1)
		{
			g_Battle.rgPlayer[wPlayerIndex].action.sTarget = wPlayerIndex;
		}
		break;

	case kBattleActionAttackMate:
		if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] == 0)
		{
			//
			// Attack enemies instead if player is not confused
			// 如果队员状态不为疯魔，则攻击敌人
			fToEnemy = TRUE;
			g_Battle.rgPlayer[wPlayerIndex].action.ActionType = kBattleActionAttack;
			g_Battle.rgPlayer[wPlayerIndex].action.wActionID = 0; //avoid be deduced to autoattack 避免被推断为自动攻击
		}
		else
		{
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				if (i != wPlayerIndex &&
					gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[i].wPlayerRole] != 0)
				{
					break;
				}
			}

			if (i > gpGlobals->wMaxPartyMemberIndex)
			{
				//
				// DISABLE Attack enemies if no one else is alive; since original version behaviour is not same
				// 如果没有其他队员活着，禁用攻击敌人；因为原始版本的行为不同
	//            fToEnemy = TRUE;
				g_Battle.rgPlayer[wPlayerIndex].action.ActionType = kBattleActionPass;
				g_Battle.rgPlayer[wPlayerIndex].action.wActionID = 0;
			}
		}
		break;
	}

	//
	// Check if player can attack all enemies at once, or attack one enemy
	// 检查玩家是否可以同时攻击所有敌人，还是只能攻击一个敌人
	if (g_Battle.rgPlayer[wPlayerIndex].action.ActionType == kBattleActionAttack)
	{
		if (sTarget == -1)
		{
			if (!PAL_PlayerCanAttackAll(wPlayerRole))
			{
				g_Battle.rgPlayer[wPlayerIndex].action.sTarget = PAL_BattleSelectAutoTargetFrom(g_Battle.rgPlayer[wPlayerIndex].action.sTarget);
			}
		}
		else if (PAL_PlayerCanAttackAll(wPlayerRole))
		{
			g_Battle.rgPlayer[wPlayerIndex].action.sTarget = -1;
		}
	}

	if (fToEnemy && g_Battle.rgPlayer[wPlayerIndex].action.sTarget >= 0)
	{
		if (g_Battle.rgEnemy[g_Battle.rgPlayer[wPlayerIndex].action.sTarget].wObjectID == 0)
		{
			g_Battle.rgPlayer[wPlayerIndex].action.sTarget = PAL_BattleSelectAutoTargetFrom(g_Battle.rgPlayer[wPlayerIndex].action.sTarget);
			assert(g_Battle.rgPlayer[wPlayerIndex].action.sTarget >= 0);
		}
	}
}

static VOID
PAL_BattleCheckHidingEffect(
	VOID
)
/*++
  Purpose:

	Check if we should enter hiding state after using items or magics.
	检查使用物品或魔法后是否应进入隐藏状态。

  Parameters:

	None.

  Return value:

	None.

--*/
{
	if (g_Battle.iHidingTime < 0)
	{
#ifdef PAL_CLASSIC
		g_Battle.iHidingTime = -g_Battle.iHidingTime;
#else
		g_Battle.iHidingTime = -g_Battle.iHidingTime * 20;

		if (gpGlobals->bBattleSpeed > 1)
		{
			g_Battle.iHidingTime *= 1 + (gpGlobals->bBattleSpeed - 1) * 0.5;
		}
		else
		{
			g_Battle.iHidingTime *= 1.2;
		}
#endif
		VIDEO_BackupScreen(g_Battle.lpSceneBuf);
		PAL_BattleMakeScene();
		PAL_BattleFadeScene();
	}
}

INT
FIGHT_DetectMagicTargetChange(
	WORD wMagicNum,
	INT sTarget
)
{
	if (sTarget == -1 && (
		gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeNormal
		|| gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeApplyToPlayer
		|| gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeTrance
		))
		sTarget = 0;
	else if (sTarget == -1 && (gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeApplyToCasterHimself))
		sTarget = -2;


	if (sTarget != -1 && (
		gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeAttackAll
		|| gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeAttackWhole
		|| gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeAttackField
		|| gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeApplyToParty
		|| gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeSummon
		))
		sTarget = -1;

	return sTarget;
}

VOID
PAL_BattlePlayerPerformAction(
	WORD         wPlayerIndex
)
/*++
  Purpose:

	Perform the selected action for a player.
	为玩家执行选定的操作

  Parameters:

	[IN]  wPlayerIndex - the index of the player.

  Return value:

	None.

--*/
{
	//SHORT    sDamage;
	UINT      iDamage;
	WORD     wPlayerRole = gpGlobals->rgParty[wPlayerIndex].wPlayerRole;
	SHORT    sTarget;
	int      x, y;
	int      i, j, t;
	INT      str, def, res, wObject, wMagicNum;
	BOOL     fCritical;
	//WORD     rgwCoopPos[3][2] = { {208, 157}, {234, 170}, {260, 183} };
	WORD     rgwCoopPos[5][2] = { {208, 157}, {234, 170}, {260, 183},{ 286,196 },{ 312,200 } };
#ifndef PAL_CLASSIC
	BOOL     fPoisoned, fCheckPoison;
#endif
	INT iPoisonResistance, iPhysicalResistance;

	g_Battle.wMovingPlayerIndex = wPlayerIndex;
	g_Battle.iBlow = 0;

	SHORT origTarget = g_Battle.rgPlayer[wPlayerIndex].action.sTarget;
	PAL_BattlePlayerValidateAction(wPlayerIndex);
	PAL_BattleBackupStat();

	sTarget = g_Battle.rgPlayer[wPlayerIndex].action.sTarget;

	switch (g_Battle.rgPlayer[wPlayerIndex].action.ActionType)
	{
	case kBattleActionAttack:
		//
		// 我方行动：普攻
		//
#ifdef PAL_CLASSIC
		if (g_Battle.fThisTurnCoop)
			break;
#endif
		// 暴击倍率
		WORD wTimes = 1;
		if (sTarget != -1)
		{
			//
			// Attack one enemy
			// 普攻攻击敌方单体
			for (t = 0; t < (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusDualAttack] ? 2 : 1); t++)
			{
				// 获取队员武术值  敌方防御  敌方等级 * 4 + 24  敌方物抗
				str = PAL_GetPlayerAttackStrength(wPlayerRole);
				def = g_Battle.rgEnemy[sTarget].e.wDefense;
				def += (g_Battle.rgEnemy[sTarget].e.wLevel + 6) * 4;
				res = PAL_New_GetPlayerPhysicalResistance(wPlayerRole) - g_Battle.rgEnemy[sTarget].e.wPhysicalResistance;

				// 初始化暴击状态为未暴击
				fCritical = FALSE;

				// 计算伤害
				iDamage = PAL_CalcPhysicalAttackDamage(str, def, res) + RandomLong(1, 2);

				if (PAL_New_GetTrueByPercentage(25) == 0 ||
					gpGlobals->rgPlayerStatus[wPlayerRole][kStatusBravery] > 0)
				{
					//
					// Critical Hit
					// 随机暴击 或 天罡战气的必暴击
					iDamage *= 2;
					fCritical = TRUE;

					// 成功暴击后体力、武术的隐藏经验值随机增加0~2点
					gpGlobals->Exp.rgHealthExp[wPlayerRole].wCount += RandomLong(0, 2);
					gpGlobals->Exp.rgAttackExp[wPlayerRole].wCount += RandomLong(0, 2);
				}

				//if (wPlayerRole == 0 && RandomLong(0, 11) == 0)
				if (wPlayerRole == RoleID_LiXiaoYao && PAL_New_GetTrueByPercentage(20))
				{
					//
					// Bonus hit for Li Xiaoyao
					// 李逍遥，会心一击
					iDamage *= 2;
					fCritical = TRUE;

					// 成功暴击后体力、武术的隐藏经验值随机增加0~2点
					gpGlobals->Exp.rgHealthExp[wPlayerRole].wCount += RandomLong(0, 2);
					gpGlobals->Exp.rgAttackExp[wPlayerRole].wCount += RandomLong(0, 2);
				}

				if (wPlayerRole == RoleID_LinYueRu && PAL_New_GetTrueByPercentage(20))
				{
					//
					// Bonus hit for Lin YueRu
					// 林月如，会心一击
					iDamage *= 2;
					fCritical = TRUE;

					// 成功暴击后体力、武术的隐藏经验值随机增加0~2点
					gpGlobals->Exp.rgHealthExp[wPlayerRole].wCount += RandomLong(0, 2);
					gpGlobals->Exp.rgAttackExp[wPlayerRole].wCount += RandomLong(0, 2);
				}

				// 一半概率增加12.5%暴击
				iDamage = (UINT)(iDamage * RandomFloat(1, 1.125));

				// 加上难度分配动态伤害+++++++++++++++++++++++++++
				iDamage *= (gpGlobals->wGameDifficulty + 1);

				// 防止伤害溢出，溢出后伤害只有1
				iDamage = max(iDamage, 1);

				// 防止伤害溢出，伤害不得高于敌方当前体力值
				iDamage = min(iDamage, g_Battle.rgEnemy[sTarget].e.wHealth);

				// 对敌方造成伤害
				g_Battle.rgEnemy[sTarget].e.wHealth -= iDamage;

				// 我方施展攻击者置帧为预备攻击帧
				if (t == 0)
				{
					g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 7;
					PAL_BattleDelay(4, 0, TRUE);
				}

				// 显示队员普攻特效
				PAL_BattleShowPlayerAttackAnim(wPlayerIndex, fCritical);
			}
		}
		else
		{
			//
			// Attack all enemies
			// 普攻攻击敌方全体
			for (t = 0; t < (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusDualAttack] ? 2 : 1); t++)
			{
				// 初始化敌方当前受击目标号，初始化敌方受击顺序
				int division = 1;
				const int index[MAX_ENEMIES_IN_TEAM] = { 2, 1, 0, 4, 3 };

				// 初始化暴击状态为未暴击
				//fCritical = (RandomLong(0, 5) == 0 || gpGlobals->rgPlayerStatus[wPlayerRole][kStatusBravery] > 0);
				fCritical = FALSE;

				if (PAL_New_GetTrueByPercentage(25) ||
					gpGlobals->rgPlayerStatus[wPlayerRole][kStatusBravery] > 0)
				{
					//
					// Critical Hit
					// 随机暴击 或 天罡战气的必暴击
					wTimes *= 2;
					fCritical = TRUE;

					// 成功暴击后体力、武术的隐藏经验值随机增加0~2点
					gpGlobals->Exp.rgHealthExp[wPlayerRole].wCount += RandomLong(0, 2);
					gpGlobals->Exp.rgAttackExp[wPlayerRole].wCount += RandomLong(0, 2);
				}

				if (wPlayerRole == RoleID_LiXiaoYao && PAL_New_GetTrueByPercentage(20))
				{
					//
					// Bonus hit for Li Xiaoyao
					// 李逍遥，会心一击
					wTimes *= 2;
					fCritical = TRUE;

					// 成功暴击后体力、武术的隐藏经验值随机增加0~2点
					gpGlobals->Exp.rgHealthExp[wPlayerRole].wCount += RandomLong(0, 2);
					gpGlobals->Exp.rgAttackExp[wPlayerRole].wCount += RandomLong(0, 2);
				}

				if (wPlayerRole == RoleID_LinYueRu && PAL_New_GetTrueByPercentage(20))
				{
					//
					// Bonus hit for Lin YueRu
					// 林月如，会心一击
					wTimes *= 2;
					fCritical = TRUE;

					// 成功暴击后体力、武术的隐藏经验值随机增加0~2点
					gpGlobals->Exp.rgHealthExp[wPlayerRole].wCount += RandomLong(0, 2);
					gpGlobals->Exp.rgAttackExp[wPlayerRole].wCount += RandomLong(0, 2);
				}

				// 我方施展攻击者置帧为预备攻击帧
				if (t == 0)
				{
					g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 7;
					PAL_BattleDelay(4, 0, TRUE);
				}

				for (i = 0; i < MAX_ENEMIES_IN_TEAM; i++)
				{
					// 若当前当前敌方单位已阵亡，则跳过该敌人
					if (g_Battle.rgEnemy[index[i]].wObjectID == 0 ||
						index[i] > g_Battle.wMaxEnemyIndex)
					{
						continue;
					}

					// 获取队员武术值  敌方防御  敌方等级 * 4 + 24  敌方物抗
					str = PAL_GetPlayerAttackStrength(wPlayerRole);
					def = g_Battle.rgEnemy[index[i]].e.wDefense;
					def += (g_Battle.rgEnemy[index[i]].e.wLevel + 6) * 4;
					res = PAL_New_GetPlayerPhysicalResistance(wPlayerRole) - g_Battle.rgEnemy[index[i]].e.wPhysicalResistance;

					// 一半概率2倍伤害
					iDamage = PAL_CalcPhysicalAttackDamage(str, def, res) + RandomLong(1, 2);

					// 一半概率增加12.5%暴击
					iDamage = (UINT)(iDamage * RandomFloat(1, 1.125));

					// 暴击加成
					iDamage *= wTimes;

					// 除以这个是为了剩下的敌人受到的伤害依次降低
					iDamage /= division;

					// 防止伤害溢出，溢出后伤害只有1
					iDamage = max(iDamage, 1);

					// 加上难度分配动态伤害+++++++++++++++++++++++++++
					iDamage *= gpGlobals->wGameDifficulty + 1;

					// 防止伤害溢出，伤害不得高于敌方当前体力值
					iDamage = min(iDamage, g_Battle.rgEnemy[index[i]].e.wHealth);

					// 若伤害不为0，则当前受击的敌人有33%概率使我方解除昏睡状态
					if (iDamage > 0)
					{
						if (RandomLong(0, 100) <= 30)
						{
							g_Battle.rgEnemy[index[i]].rgwStatus[kStatusSleep] = 0;
						}
					}

					// 对敌方造成伤害
					g_Battle.rgEnemy[index[i]].e.wHealth -= iDamage;

					// 使下一个敌方当前受击目标号受击
					division++;
					if (division > 3)
					{
						division = 3;
					}
				}

				// 显示队员普攻特效
				PAL_BattleShowPlayerAttackAnim(wPlayerIndex, fCritical);
			}
		}

		// 更新战场中的全部战斗者，延迟三帧
		PAL_BattleUpdateFighters();
		PAL_BattleMakeScene();
		PAL_BattleDelay(3, 0, TRUE);

		// 体力的隐藏经验值增加1点、武术的隐藏经验值随机增加2~3点
		gpGlobals->Exp.rgAttackExp[wPlayerRole].wCount++;
		gpGlobals->Exp.rgHealthExp[wPlayerRole].wCount += RandomLong(2, 3);
		break;

	case kBattleActionAttackMate:
		//
		// 我方行动：疯魔状态时随机攻击队友
		//
#ifdef PAL_CLASSIC
		if (g_Battle.fThisTurnCoop)
			break;
#endif
		//
		// Check if there is someone else who is alive
		// 检查是否还有其他人活着
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			if (i == wPlayerIndex)
			{
				continue;
			}

			if (gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[i].wPlayerRole] > 0)
			{
				break;
			}
		}

		if (i <= gpGlobals->wMaxPartyMemberIndex)
		{
			//
			// Pick a target randomly
			// 随机选取目标
			do
			{
				sTarget = RandomLong(0, gpGlobals->wMaxPartyMemberIndex);
			} while (sTarget == wPlayerIndex || gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[sTarget].wPlayerRole] == 0);

			for (j = 0; j < 2; j++)
			{
				g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 8;
				PAL_BattleDelay(1, 0, TRUE);

				g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 0;
				PAL_BattleDelay(1, 0, TRUE);
			}

			PAL_BattleDelay(2, 0, TRUE);

			x = PAL_X(g_Battle.rgPlayer[sTarget].pos) + 30;
			y = PAL_Y(g_Battle.rgPlayer[sTarget].pos) + 12;

			g_Battle.rgPlayer[wPlayerIndex].pos = PAL_XY(x, y);
			g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 8;
			PAL_BattleDelay(5, 0, TRUE);

			g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 9;
			AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwWeaponSound[wPlayerRole]);

			str = PAL_GetPlayerAttackStrength(wPlayerRole);
			def = PAL_GetPlayerDefense(gpGlobals->rgParty[sTarget].wPlayerRole);
			if (g_Battle.rgPlayer[sTarget].fDefending)
			{
				def *= 2;
			}
			res = g_Battle.rgEnemy[sTarget].e.wPhysicalResistance;

			iDamage = PAL_CalcPhysicalAttackDamage(str, def, res);
			if (gpGlobals->rgPlayerStatus[gpGlobals->rgParty[sTarget].wPlayerRole][kStatusProtect] > 0)
			{
				iDamage /= 2;
			}

			//if (iDamage <= 0)
			//{
			//	iDamage = 1;
			//}

			// 防止伤害溢出，若溢出则伤害为 1
			iDamage = max(iDamage, 1);

			// 加上难度分配动态伤害+++++++++++++++++++++++++++
			iDamage *= gpGlobals->wGameDifficulty + 1;

			// 防止伤害溢出，伤害不得高于敌方当前体力值
			//iDamage = min(iDamage, gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[sTarget].wPlayerRole]);

			//if (iDamage > (SHORT)gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[sTarget].wPlayerRole])
			if (iDamage > gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[sTarget].wPlayerRole])
			{
				iDamage = gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[sTarget].wPlayerRole];
			}

			gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[sTarget].wPlayerRole] -= iDamage;

			g_Battle.rgPlayer[sTarget].pos =
				PAL_XY(PAL_X(g_Battle.rgPlayer[sTarget].pos) - 12,
					PAL_Y(g_Battle.rgPlayer[sTarget].pos) - 6);
			PAL_BattleDelay(1, 0, TRUE);

			g_Battle.rgPlayer[sTarget].iColorShift = 6;
			PAL_BattleDelay(1, 0, TRUE);

			PAL_BattleDisplayStatChange();

			g_Battle.rgPlayer[sTarget].iColorShift = 0;
			PAL_BattleDelay(4, 0, TRUE);

			PAL_BattleUpdateFighters();
			PAL_BattleDelay(4, 0, TRUE);
		}
		break;

	case kBattleActionCoopMagic:
		//
		// 我方行动：合体法术
		//
#ifdef PAL_CLASSIC
		g_Battle.fThisTurnCoop = TRUE;
#endif
		wObject = PAL_GetPlayerCooperativeMagic(gpGlobals->rgParty[wPlayerIndex].wPlayerRole);
		wMagicNum = gpGlobals->g.rgObject[wObject].magic.wMagicNumber;

		sTarget = FIGHT_DetectMagicTargetChange(wMagicNum, sTarget);

		// DEBUG 目标为发起者本人
		if (sTarget == -2)
			sTarget = wPlayerIndex;

		if (gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeSummon)
		{
			//
			// 合体法术为召唤神时，展示施法前对队员的影响，之后展示召唤神
			//
			PAL_BattleShowPlayerPreMagicAnim(wPlayerIndex, TRUE);
			PAL_BattleShowPlayerSummonMagicAnim((WORD)-1, wObject);
		}
		else
		{
			//
			// Sound should be played before action begins
			// 行动开始前应播放声音
			AUDIO_PlaySound(29);

			for (i = 1; i <= 6; i++)
			{
				//
				// Update the position for the player who invoked the action
				// 更新调用动作的玩家的位置
				x = PAL_X(g_Battle.rgPlayer[wPlayerIndex].posOriginal) * (6 - i);
				y = PAL_Y(g_Battle.rgPlayer[wPlayerIndex].posOriginal) * (6 - i);

				x += rgwCoopPos[0][0] * i;
				y += rgwCoopPos[0][1] * i;

				x /= 6;
				y /= 6;

				g_Battle.rgPlayer[wPlayerIndex].pos = PAL_XY(x, y);

				//
				// Update the position for other players
				// 更新其他玩家的位置
				t = 0;

				for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
				{
					if ((WORD)j == wPlayerIndex)
					{
						continue;
					}

					t++;

#ifdef PAL_CLASSIC
					if (g_Battle.coopContributors[j] == FALSE)
						continue;
#endif

					x = PAL_X(g_Battle.rgPlayer[j].posOriginal) * (6 - i);
					y = PAL_Y(g_Battle.rgPlayer[j].posOriginal) * (6 - i);

					x += rgwCoopPos[t][0] * i;
					y += rgwCoopPos[t][1] * i;

					x /= 6;
					y /= 6;

					g_Battle.rgPlayer[j].pos = PAL_XY(x, y);
				}

				PAL_BattleDelay(1, 0, TRUE);
			}

			for (i = gpGlobals->wMaxPartyMemberIndex; i >= 0; i--)
			{
				if ((WORD)i == wPlayerIndex)
				{
					continue;
				}
#ifdef PAL_CLASSIC
				if (g_Battle.coopContributors[i] == FALSE)
					continue;
#endif

				g_Battle.rgPlayer[i].wCurrentFrame = 5;

				PAL_BattleDelay(3, 0, TRUE);
			}

			g_Battle.rgPlayer[wPlayerIndex].iColorShift = 6;
			g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 5;
			PAL_BattleDelay(5, 0, TRUE);

			g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 6;
			g_Battle.rgPlayer[wPlayerIndex].iColorShift = 0;
			PAL_BattleDelay(3, 0, TRUE);

			PAL_BattleShowPlayerOffMagicAnim((WORD)-1, wObject, sTarget, FALSE);
		}

		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
#ifdef PAL_CLASSIC
			if (g_Battle.coopContributors[i] == FALSE)
				continue;
#endif

			// 按 合体法术 需要消耗的 真气的 1.45倍 与精力的0.75倍 消耗 发起合体者的 精力值
			gpGlobals->g.PlayerRoles.rgwSP[gpGlobals->rgParty[i].wPlayerRole] -= gpGlobals->g.lprgMagic[wMagicNum].wCostMP * 1.45;
			gpGlobals->g.PlayerRoles.rgwSP[gpGlobals->rgParty[i].wPlayerRole] -= gpGlobals->g.lprgMagic[wMagicNum].wCostSP * 0.75;

			// emmmm，合体仙术的 发起者 是不可以被 消耗到负数的！至少 0 点精力值
			if (gpGlobals->g.PlayerRoles.rgwSP[gpGlobals->rgParty[i].wPlayerRole] <= 0)
			{
				gpGlobals->g.PlayerRoles.rgwSP[gpGlobals->rgParty[i].wPlayerRole] = 0;
			}

			//
			// Reset the time meter for everyone when using coopmagic
			// 使用合体法术时重置每个人的计时器
#ifdef PAL_CLASSIC
			g_Battle.rgPlayer[i].state = kFighterWait;
#else
			g_Battle.rgPlayer[i].flTimeMeter = 0;
			g_Battle.rgPlayer[i].flTimeSpeedModifier = 2;
#endif
		}

		// 备份所有玩家和敌人的HP和MP值
		PAL_BattleBackupStat(); // so that "damages" to players won't be shown 这样就不会显示对玩家的“伤害”

		// 初始化全队总灵攻
		//str = 0;

//		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
//		{
//			//
//			// 将每位队员的武术和灵力加起来
//			//
//#ifdef PAL_CLASSIC
//			if (g_Battle.coopContributors[i] == FALSE)
//				continue;
//#endif
//
//			str += PAL_GetPlayerAttackStrength(gpGlobals->rgParty[i].wPlayerRole);
//			str += PAL_GetPlayerMagicStrength(gpGlobals->rgParty[i].wPlayerRole);
//		}
//
//		// 全队总灵攻 / 4 得到均值，赵灵儿发起则只削弱一半，林月如则削弱三分之二
//		//str /= 4;
//		if (gpGlobals->wMaxPartyMemberIndex == 1)
//		{
//			str /= 2;
//		}
//		else if (gpGlobals->wMaxPartyMemberIndex == 2)
//		{
//			str /= 3;
//		}
//		else
//		{
//			str /= 4;
//		}

		//
		// Inflict damage to enemies
		// 对敌人造成伤害
		if (sTarget == -1)
		{
			//
			// Attack all enemies
			// 攻击敌方全体
			for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
			{
				if (g_Battle.rgEnemy[i].wObjectID == 0)
				{
					continue;
				}

				//def = g_Battle.rgEnemy[i].e.wDefense;
				//def += (g_Battle.rgEnemy[i].e.wLevel + 6) * 4;

				// 这里的伤害代码因为溢出，做了调整
				//iDamage = PAL_CalcMagicDamage(str, def, g_Battle.rgEnemy[i].e.wElemResistance, g_Battle.rgEnemy[i].e.wPoisonResistance, 1, wObject);
				//iPoisonResistance = PAL_New_GetEnemyPoisonResistance(i);
				//iPhysicalResistance = PAL_New_GetEnemyPhysicalResistance(i);

				//iDamage = PAL_New_CalcMagicDamage(str, def, g_Battle.rgEnemy[i].e.wElemResistance, iPoisonResistance, iPhysicalResistance, wObject);
				iDamage = PAL_New_CalcMagicDamage(-1, i, wObject);

				g_Battle.rgEnemy[i].e.wHealth -= iDamage;
			}
		}
		else
		{
			//
			// Attack one enemy
			// 攻击敌方单体
			//def = g_Battle.rgEnemy[sTarget].e.wDefense;
			//def += (g_Battle.rgEnemy[sTarget].e.wLevel + 6) * 4;

			// 这里的伤害代码因为溢出，做了调整
			//iPoisonResistance = PAL_New_GetEnemyPoisonResistance(sTarget);
			//iPhysicalResistance = PAL_New_GetEnemyPhysicalResistance(sTarget);

			//iDamage = PAL_New_CalcMagicDamage(str, def, g_Battle.rgEnemy[sTarget].e.wElemResistance, iPoisonResistance, iPhysicalResistance, wObject);
			iDamage = PAL_New_CalcMagicDamage(-2, sTarget, wObject);

			// 防止伤害溢出，若溢出则伤害为 1
			//iDamage = max(iDamage, 1);

			// 防止伤害溢出，伤害不得高于敌方当前体力值
			//iDamage = min(iDamage, g_Battle.rgEnemy[sTarget].e.wHealth);

			// 当伤害大于0时有33%概率将敌方击醒（取消敌方昏眠状态）
			if (iDamage > 0)
			{
				if (RandomLong(0, 100) <= 30)
				{
					g_Battle.rgEnemy[sTarget].rgwStatus[kStatusSleep] = 0;
				}
			}

			g_Battle.rgEnemy[sTarget].e.wHealth -= iDamage;
		}

		PAL_BattleDisplayStatChange();
		PAL_BattleShowPostMagicAnim();
		PAL_BattleDelay(5, 0, TRUE);

		if (gpGlobals->g.lprgMagic[wMagicNum].wType != kMagicTypeSummon)
		{
			PAL_BattlePostActionCheck(FALSE);

			//
			// Move all players back to the original position
			// 将所有玩家移回初始位置
			for (i = 1; i <= 6; i++)
			{
				//
				// Update the position for the player who invoked the action
				// 更新调用动作的玩家的位置
				x = PAL_X(g_Battle.rgPlayer[wPlayerIndex].posOriginal) * i;
				y = PAL_Y(g_Battle.rgPlayer[wPlayerIndex].posOriginal) * i;

				x += rgwCoopPos[0][0] * (6 - i);
				y += rgwCoopPos[0][1] * (6 - i);

				x /= 6;
				y /= 6;

				g_Battle.rgPlayer[wPlayerIndex].pos = PAL_XY(x, y);

				//
				// Update the position for other players
				// 更新其他玩家的位置
				t = 0;

				for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
				{
#ifdef PAL_CLASSIC
					if (g_Battle.coopContributors[j] == FALSE)
						continue;
#endif

					g_Battle.rgPlayer[j].wCurrentFrame = 0;

					if ((WORD)j == wPlayerIndex)
					{
						continue;
					}

					t++;

					x = PAL_X(g_Battle.rgPlayer[j].posOriginal) * i;
					y = PAL_Y(g_Battle.rgPlayer[j].posOriginal) * i;

					x += rgwCoopPos[t][0] * (6 - i);
					y += rgwCoopPos[t][1] * (6 - i);

					x /= 6;
					y /= 6;

					g_Battle.rgPlayer[j].pos = PAL_XY(x, y);
				}

				PAL_BattleDelay(1, 0, TRUE);
			}
		}
		break;

	case kBattleActionDefend:
		//
		// 我方行动：防御
		//
#ifdef PAL_CLASSIC
		if (g_Battle.fThisTurnCoop)
			break;
#endif
		g_Battle.rgPlayer[wPlayerIndex].fDefending = TRUE;
		gpGlobals->Exp.rgDefenseExp[wPlayerRole].wCount += 2;
		break;

	case kBattleActionFlee:
		//
		// 我方行动：逃跑
		//
#ifdef PAL_CLASSIC
		if (g_Battle.fThisTurnCoop)
			break;
#endif
		str = PAL_GetPlayerFleeRate(wPlayerRole);
		def = 0;

		for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
		{
			if (g_Battle.rgEnemy[i].wObjectID == 0)
			{
				continue;
			}

			def += (SHORT)(g_Battle.rgEnemy[i].e.wDexterity);
			def += (g_Battle.rgEnemy[i].e.wLevel + 6) * 4;
		}

		if ((SHORT)def < 0)
		{
			def = 0;
		}

		if (str >= RandomLong(0, def) && !g_Battle.fIsBoss)
		{
			//
			// Successful escape
			// 我方逃跑成功
			PAL_BattlePlayerEscape();
		}
		else
		{
			//
			// Failed escape
			// 我方逃跑失败
			g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 0;

			for (i = 0; i < 3; i++)
			{
				x = PAL_X(g_Battle.rgPlayer[wPlayerIndex].pos) + 4;
				y = PAL_Y(g_Battle.rgPlayer[wPlayerIndex].pos) + 2;

				g_Battle.rgPlayer[wPlayerIndex].pos = PAL_XY(x, y);

				PAL_BattleDelay(1, 0, TRUE);
			}

			g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 1;
			PAL_BattleDelay(8, BATTLE_LABEL_ESCAPEFAIL, TRUE);

			gpGlobals->Exp.rgFleeExp[wPlayerRole].wCount += 2;
		}
		break;

	case kBattleActionMagic:
		//
		// 我方行动：仙术
		//
#ifdef PAL_CLASSIC
		if (g_Battle.fThisTurnCoop)
			break;
#endif
		wObject = g_Battle.rgPlayer[wPlayerIndex].action.wActionID;
		wMagicNum = gpGlobals->g.rgObject[wObject].magic.wMagicNumber;

		sTarget = FIGHT_DetectMagicTargetChange(wMagicNum, sTarget);

		PAL_BattleShowPlayerPreMagicAnim(wPlayerIndex,
			(gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeSummon));

		if (!gpGlobals->fAutoBattle)
		{
			gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] -= gpGlobals->g.lprgMagic[wMagicNum].wCostMP;
			gpGlobals->g.PlayerRoles.rgwSP[wPlayerRole] -= gpGlobals->g.lprgMagic[wMagicNum].wCostSP;

			if (gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] < 0)
			{
				gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] = 0;
			}

			if (gpGlobals->g.PlayerRoles.rgwSP[wPlayerRole] < 0)
			{
				gpGlobals->g.PlayerRoles.rgwSP[wPlayerRole] = 0;
			}
		}

		if (gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeApplyToPlayer ||
			gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeApplyToParty ||
			gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeApplyToCasterHimself ||
			gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeTrance)
		{
			//
			// Using a defensive magic
			// 使用防御仙术
			WORD w = 0;

			if (g_Battle.rgPlayer[wPlayerIndex].action.sTarget != -1)
			{
				w = gpGlobals->rgParty[g_Battle.rgPlayer[wPlayerIndex].action.sTarget].wPlayerRole;
			}
			else if (gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeApplyToCasterHimself)
				w = wPlayerIndex;
			else if (gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeTrance)
			{
				w = wPlayerRole;
			}

			gpGlobals->g.rgObject[wObject].magic.wScriptOnUse =
				PAL_RunTriggerScript(gpGlobals->g.rgObject[wObject].magic.wScriptOnUse, wPlayerRole, FALSE);

			if (g_fScriptSuccess)
			{
				PAL_BattleShowPlayerDefMagicAnim(wPlayerIndex, wObject, sTarget);

				gpGlobals->g.rgObject[wObject].magic.wScriptOnSuccess =
					PAL_RunTriggerScript(gpGlobals->g.rgObject[wObject].magic.wScriptOnSuccess, w, FALSE);

				if (g_fScriptSuccess)
				{
					if (gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeTrance)
					{
						for (i = 0; i < 6; i++)
						{
							g_Battle.rgPlayer[wPlayerIndex].iColorShift = i * 2;
							PAL_BattleDelay(1, 0, TRUE);
						}

						VIDEO_BackupScreen(g_Battle.lpSceneBuf);
						PAL_LoadBattleSprites();

						g_Battle.rgPlayer[wPlayerIndex].iColorShift = 0;

						PAL_BattleMakeScene();
						PAL_BattleFadeScene();
					}
				}
			}
		}
		else
		{
			//
			// Using an offensive magic
			// 使用攻击性仙术
			gpGlobals->g.rgObject[wObject].magic.wScriptOnUse =
				PAL_RunTriggerScript(gpGlobals->g.rgObject[wObject].magic.wScriptOnUse, wPlayerRole, FALSE);

			if (g_fScriptSuccess)
			{
				if (gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeSummon)
				{
					PAL_BattleShowPlayerSummonMagicAnim(wPlayerIndex, wObject);
				}
				else
				{
					PAL_BattleShowPlayerOffMagicAnim(wPlayerIndex, wObject, sTarget, FALSE);
				}

				gpGlobals->g.rgObject[wObject].magic.wScriptOnSuccess =
					PAL_RunTriggerScript(gpGlobals->g.rgObject[wObject].magic.wScriptOnSuccess, (WORD)sTarget, FALSE);

				//
				// Inflict damage to enemies
				// 对敌人造成伤害
				//if ((SHORT)(gpGlobals->g.lprgMagic[wMagicNum].wBaseDamage) > 0)
				if ((INT)(gpGlobals->g.lprgMagic[wMagicNum].wBaseDamage) > 0)
				{
					if (sTarget == -1)
					{
						//
						// Attack all enemies
						// 攻击敌方全体
						for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
						{
							if (g_Battle.rgEnemy[i].wObjectID == 0)
							{
								continue;
							}

							//str = PAL_GetPlayerMagicStrength(wPlayerRole);
							//def = g_Battle.rgEnemy[i].e.wDefense;
							//def += (g_Battle.rgEnemy[i].e.wLevel + 6) * 4;

							//iDamage = PAL_CalcMagicDamage(str, def, g_Battle.rgEnemy[i].e.wElemResistance, g_Battle.rgEnemy[i].e.wPoisonResistance, 1, wObject);
							// 这里的伤害代码因为溢出，做了调整
							//iPoisonResistance = PAL_New_GetEnemyPoisonResistance(i);
							//iPhysicalResistance = PAL_New_GetEnemyPhysicalResistance(i);

							//iDamage = PAL_New_CalcMagicDamage(str, def, g_Battle.rgEnemy[i].e.wElemResistance, iPoisonResistance, iPhysicalResistance, wObject);
							iDamage = PAL_New_CalcMagicDamage(-1, i, wObject);

							// 防止伤害溢出，若溢出则伤害为 1
							//iDamage = max(iDamage, 1);

							// 防止伤害溢出，伤害不得高于敌方当前体力值
							//iDamage = min(iDamage, g_Battle.rgEnemy[i].e.wHealth);

							g_Battle.rgEnemy[i].e.wHealth -= iDamage;
						}
					}
					else
					{
						//
						// Attack one enemy
						// 攻击敌方单体
						//str = PAL_GetPlayerMagicStrength(wPlayerRole);
						//def = g_Battle.rgEnemy[sTarget].e.wDefense;
						//def += (g_Battle.rgEnemy[sTarget].e.wLevel + 6) * 4;

						//iDamage = PAL_CalcMagicDamage(str, def, g_Battle.rgEnemy[sTarget].e.wElemResistance, g_Battle.rgEnemy[sTarget].e.wPoisonResistance, 1, wObject);
						// 这里的伤害代码因为溢出，做了调整
						//iPoisonResistance = PAL_New_GetEnemyPoisonResistance(sTarget);
						//iPhysicalResistance = PAL_New_GetEnemyPhysicalResistance(sTarget);

						//iDamage = PAL_New_CalcMagicDamage(str, def, g_Battle.rgEnemy[sTarget].e.wElemResistance, iPoisonResistance, iPhysicalResistance, wObject);
						iDamage = PAL_New_CalcMagicDamage(-2, sTarget, wObject);

						// 防止伤害溢出，若溢出则伤害为 1
						//iDamage = max(iDamage, 1);

						// 防止伤害溢出，伤害不得高于敌方当前体力值
						//iDamage = min(iDamage, g_Battle.rgEnemy[sTarget].e.wHealth);

						// 当伤害大于0时有33%概率将敌方击醒（取消敌方昏眠状态）
						if (iDamage > 0)
						{
							if (RandomLong(0, 100) <= 30)
							{
								g_Battle.rgEnemy[sTarget].rgwStatus[kStatusSleep] = 0;
							}
						}

						g_Battle.rgEnemy[sTarget].e.wHealth -= iDamage;
					}
				}
			}
		}

		PAL_BattleDisplayStatChange();
		PAL_BattleShowPostMagicAnim();
		PAL_BattleDelay(5, 0, TRUE);

		PAL_BattleCheckHidingEffect();

		gpGlobals->Exp.rgMagicExp[wPlayerRole].wCount += RandomLong(2, 3);
		gpGlobals->Exp.rgMagicPowerExp[wPlayerRole].wCount++;
		break;

	case kBattleActionThrowItem:
		//
		// 我方行动：投掷道具
		//
#ifdef PAL_CLASSIC
		if (g_Battle.fThisTurnCoop)
			break;
#endif
		wObject = g_Battle.rgPlayer[wPlayerIndex].action.wActionID;

		for (i = 0; i < 4; i++)
		{
			g_Battle.rgPlayer[wPlayerIndex].pos =
				PAL_XY(PAL_X(g_Battle.rgPlayer[wPlayerIndex].pos) - (4 - i),
					PAL_Y(g_Battle.rgPlayer[wPlayerIndex].pos) - (4 - i) / 2);

			PAL_BattleDelay(1, 0, TRUE);
		}

		PAL_BattleDelay(2, wObject, TRUE);

		g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 5;
		AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwMagicSound[wPlayerRole]);

		PAL_BattleDelay(8, wObject, TRUE);

		g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 6;
		PAL_BattleDelay(2, wObject, TRUE);

		//
		// Run the script
		// 执行脚本
		gpGlobals->g.rgObject[wObject].item.wScriptOnThrow =
			PAL_RunTriggerScript(gpGlobals->g.rgObject[wObject].item.wScriptOnThrow, (WORD)sTarget, FALSE);

		//
		// Remove the thrown item from inventory
		// 从库存中删除丢弃的道具
		PAL_AddItemToInventory(wObject, -1);

		PAL_BattleDisplayStatChange();
		PAL_BattleDelay(4, 0, TRUE);
		PAL_BattleUpdateFighters();
		PAL_BattleDelay(4, 0, TRUE);

		PAL_BattleCheckHidingEffect();

		break;

	case kBattleActionUseItem:
		//
		// 我方行动：使用道具
		//
#ifdef PAL_CLASSIC
		if (g_Battle.fThisTurnCoop)
			break;
#endif
		wObject = g_Battle.rgPlayer[wPlayerIndex].action.wActionID;

		PAL_BattleShowPlayerUseItemAnim(wPlayerIndex, wObject, sTarget);

		//
		// Run the script
		//
		gpGlobals->g.rgObject[wObject].item.wScriptOnUse =
			PAL_RunTriggerScript(gpGlobals->g.rgObject[wObject].item.wScriptOnUse,
				(sTarget == -1) ? 0xFFFF : gpGlobals->rgParty[sTarget].wPlayerRole, FALSE);

		//
		// Remove the item if the item is consuming
		//
		if (gpGlobals->g.rgObject[wObject].item.wFlags & kItemFlagConsuming)
		{
			PAL_AddItemToInventory(wObject, -1);
		}

		PAL_BattleCheckHidingEffect();

		PAL_BattleUpdateFighters();
		PAL_BattleDisplayStatChange();
		PAL_BattleDelay(8, 0, TRUE);
		break;

	case kBattleActionPass:
		//
		// 我方行动：Pass（跳过，我方定身/昏眠）
		//
		break;
	}

	//
	// Revert this player back to waiting state.
	// 将此播放器恢复到等待状态
	g_Battle.rgPlayer[wPlayerIndex].state = kFighterWait;
	g_Battle.rgPlayer[wPlayerIndex].flTimeMeter = 0;

	PAL_BattlePostActionCheck(FALSE);

	//
	// Revert target slot of this player 
	// 恢复此玩家的目标插槽
	g_Battle.rgPlayer[wPlayerIndex].action.sTarget = origTarget;

#ifndef PAL_CLASSIC
	//
	// Only check for poisons when the battle is not ended
	//
	fCheckPoison = FALSE;

	if (g_Battle.BattleResult == kBattleResultOnGoing)
	{
		for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
		{
			if (g_Battle.rgEnemy[i].wObjectID != 0)
			{
				fCheckPoison = TRUE;
				break;
			}
		}
	}

	//
	// Check for poisons
	//
	if (fCheckPoison)
	{
		fPoisoned = FALSE;
		PAL_BattleBackupStat();

		for (i = 0; i < MAX_POISONS; i++)
		{
			wObject = gpGlobals->rgPoisonStatus[i][wPlayerIndex].wPoisonID;

			if (wObject != 0)
			{
				fPoisoned = TRUE;
				gpGlobals->rgPoisonStatus[i][wPlayerIndex].wPoisonScript =
					PAL_RunTriggerScript(gpGlobals->rgPoisonStatus[i][wPlayerIndex].wPoisonScript, wPlayerRole, FALSE);
			}
		}

		if (fPoisoned)
		{
			PAL_BattleDelay(3, 0, TRUE);
			PAL_BattleUpdateFighters();
			if (PAL_BattleDisplayStatChange())
			{
				PAL_BattleDelay(6, 0, TRUE);
			}
		}
	}

	//
	// Update statuses
	//
	for (i = 0; i < kStatusAll; i++)
	{
		if (gpGlobals->rgPlayerStatus[wPlayerRole][i] > 0)
		{
			gpGlobals->rgPlayerStatus[wPlayerRole][i]--;
		}
	}
#endif
}

static INT
PAL_BattleEnemySelectEnemyTargetIndex(
	VOID
)
/*++
 Purpose:

 Select a attackable enemy randomly.

 Parameters:

 None.

 Return value:

 None.

 --*/
{
	int i;

	i = RandomLong(0, g_Battle.wMaxEnemyIndex);

	while (g_Battle.rgEnemy[i].wObjectID == 0 || g_Battle.rgEnemy[i].e.wHealth == 0)
	{
		i = RandomLong(0, g_Battle.wMaxEnemyIndex);
	}

	return i;
}

static INT
PAL_BattleEnemySelectTargetIndex(
	VOID
)
/*++
  Purpose:

	Select a attackable player randomly.

  Parameters:

	None.

  Return value:

	None.

--*/
{
	int i;

	i = RandomLong(0, gpGlobals->wMaxPartyMemberIndex);

	while (gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[i].wPlayerRole] == 0)
	{
		i = RandomLong(0, gpGlobals->wMaxPartyMemberIndex);
	}

	return i;
}

VOID
PAL_BattleEnemyPerformAction(
	WORD         wEnemyIndex
)
/*++
  Purpose:

	Perform the selected action for a player.
	为玩家执行选定的操作。

  Parameters:

	[IN]  wEnemyIndex - the index of the player.

  Return value:

	None.

--*/
{
	int        str, def, iCoverIndex, i, x, y, ex, ey, iSound;
	//WORD       rgwElementalResistance[NUM_MAGIC_ELEMENTAL];
	INT       rgwElementalResistance[NUM_MAGIC_ELEMENTAL];
	// 毒抗
	INT        iPoisonResistance;
	// 物抗
	INT        iPhysicalResistance;
	WORD       wPlayerRole, w, wMagic, wMagicNum;
	SHORT      sTarget;
	//SHORT      sDamage;
	INT        iDamage;
	BOOL       fAutoDefend = FALSE, rgfMagAutoDefend[MAX_PLAYERS_IN_PARTY];

	PAL_BattleBackupStat();
	g_Battle.iBlow = 0;

	sTarget = PAL_BattleEnemySelectTargetIndex();
	wPlayerRole = gpGlobals->rgParty[sTarget].wPlayerRole;
	wMagic = g_Battle.rgEnemy[wEnemyIndex].e.wMagic;

	if (g_Battle.rgEnemy[wEnemyIndex].rgwStatus[kStatusSleep] > 0 ||
		g_Battle.rgEnemy[wEnemyIndex].rgwStatus[kStatusParalyzed] > 0 ||
		g_Battle.iHidingTime > 0)
	{
		//
		// Do nothing
		// 敌方状态为 昏眠 或 定身 则不进行行动
		goto end;
	}
	else if (g_Battle.rgEnemy[wEnemyIndex].rgwStatus[kStatusConfused] > 0)
	{
		INT  iTarget = PAL_BattleEnemySelectEnemyTargetIndex();
		if (iTarget == wEnemyIndex)
			goto end;
		INT  iX = PAL_X(g_Battle.rgEnemy[iTarget].pos);
		INT  iY = PAL_Y(g_Battle.rgEnemy[iTarget].pos);
		for (i = 0; i < 3; i++)
		{
			x = PAL_X(g_Battle.rgEnemy[wEnemyIndex].pos);
			y = PAL_Y(g_Battle.rgEnemy[wEnemyIndex].pos);

			x += iX;
			y += iY;

			x /= 2;
			y /= 2;

			g_Battle.rgEnemy[wEnemyIndex].pos = PAL_XY(x, y);

			PAL_BattleDelay(1, 0, TRUE);
		}

		DWORD dwTime = SDL_GetTicks() + BATTLE_FRAME_TIME;
		x = (PAL_X(g_Battle.rgEnemy[wEnemyIndex].pos) + PAL_X(g_Battle.rgEnemy[iTarget].pos)) / 2;
		y = PAL_Y(g_Battle.rgEnemy[iTarget].pos) - PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle.rgEnemy[iTarget].lpSprite, 0)) / 3 + 10;
		for (i = 9; i < 12; i++)
		{
			LPCBITMAPRLE b = PAL_SpriteGetFrame(g_Battle.lpEffectSprite, i);

			PAL_DelayUntil(dwTime);
			dwTime = SDL_GetTicks() + BATTLE_FRAME_TIME;

			PAL_BattleMakeScene();
			VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);

			PAL_RLEBlitToSurface(b, gpScreen, PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));

			PAL_BattleUIUpdate();

			VIDEO_UpdateScreen(NULL);
		}

		//int str = (SHORT)g_Battle.rgEnemy[wEnemyIndex].e.wAttackStrength;
		int str = g_Battle.rgEnemy[wEnemyIndex].e.wAttackStrength;
		str += (g_Battle.rgEnemy[wEnemyIndex].e.wLevel + 6) * 6;
		//int def = (SHORT)g_Battle.rgEnemy[iTarget].e.wDefense;
		int def = g_Battle.rgEnemy[iTarget].e.wDefense;
		def += (g_Battle.rgEnemy[iTarget].e.wLevel + 6) * 4;

		if (g_Battle.rgEnemy[iTarget].e.wPhysicalResistance > 0)
			iDamage = PAL_CalcBaseDamage(str, def) * 2 / g_Battle.rgEnemy[iTarget].e.wPhysicalResistance / 10.0;
		else
			iDamage = PAL_CalcBaseDamage(str, def) * 2;

		// 防止伤害溢出，若溢出则伤害为 1
		iDamage = max(iDamage, 1);

		// 防止伤害溢出，伤害不得高于敌方当前体力值
		iDamage = min(iDamage, g_Battle.rgEnemy[iTarget].e.wHealth);

		g_Battle.rgEnemy[iTarget].e.wHealth -= iDamage;

		PAL_BattleDisplayStatChange();
		PAL_BattleShowPostMagicAnim();
		PAL_BattleDelay(5, 0, TRUE);

		g_Battle.rgEnemy[wEnemyIndex].pos = g_Battle.rgEnemy[wEnemyIndex].posOriginal;
		PAL_BattleDelay(2, 0, TRUE);

		PAL_BattlePostActionCheck(FALSE);
	}
	else if (wMagic != 0 &&
		RandomLong(0, 99) < g_Battle.rgEnemy[wEnemyIndex].e.wMagicRate &&
		g_Battle.rgEnemy[wEnemyIndex].rgwStatus[kStatusSilence] == 0)
	{
		//
		// Magical attack
		// 敌方使用仙术
		if (wMagic == 0xFFFF)
		{
			//
			// Do nothing
			//
			goto end;
		}

		// 备份敌方本回合使用的法术，并在uibattle.c自动展示在屏幕正上方
		g_Battle.wMagicMoving = wMagic;

		wMagicNum = gpGlobals->g.rgObject[wMagic].magic.wMagicNumber;

		//str = g_Battle.rgEnemy[wEnemyIndex].e.wMagicStrength;
		//str += (g_Battle.rgEnemy[wEnemyIndex].e.wLevel + 6) * 6;
		//if (str < 0)
		//{
		//	str = 0;
		//}

		ex = PAL_X(g_Battle.rgEnemy[wEnemyIndex].pos);
		ey = PAL_Y(g_Battle.rgEnemy[wEnemyIndex].pos);

		ex += 12;
		ey += 6;

		g_Battle.rgEnemy[wEnemyIndex].pos = PAL_XY(ex, ey);
		PAL_BattleDelay(1, 0, FALSE);

		ex += 4;
		ey += 2;

		g_Battle.rgEnemy[wEnemyIndex].pos = PAL_XY(ex, ey);
		PAL_BattleDelay(1, 0, FALSE);

		AUDIO_PlaySound(g_Battle.rgEnemy[wEnemyIndex].e.wMagicSound);

		for (i = 0; i < g_Battle.rgEnemy[wEnemyIndex].e.wMagicFrames; i++)
		{
			g_Battle.rgEnemy[wEnemyIndex].wCurrentFrame =
				g_Battle.rgEnemy[wEnemyIndex].e.wIdleFrames + i;
			PAL_BattleDelay(g_Battle.rgEnemy[wEnemyIndex].e.wActWaitFrames, 0, FALSE);
		}

		if (g_Battle.rgEnemy[wEnemyIndex].e.wMagicFrames == 0)
		{
			PAL_BattleDelay(1, 0, FALSE);
		}

		if (gpGlobals->g.lprgMagic[wMagicNum].wFireDelay == 0)
		{
			for (i = 0; i <= g_Battle.rgEnemy[wEnemyIndex].e.wAttackFrames; i++)
			{
				g_Battle.rgEnemy[wEnemyIndex].wCurrentFrame =
					i - 1 + g_Battle.rgEnemy[wEnemyIndex].e.wIdleFrames + g_Battle.rgEnemy[wEnemyIndex].e.wMagicFrames;
				PAL_BattleDelay(g_Battle.rgEnemy[wEnemyIndex].e.wActWaitFrames, 0, FALSE);
			}
		}

		if (gpGlobals->g.lprgMagic[wMagicNum].wType != kMagicTypeNormal)
		{
			sTarget = -1;

			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				w = gpGlobals->rgParty[i].wPlayerRole;

				if (gpGlobals->rgPlayerStatus[w][kStatusSleep] == 0 &&
#ifdef PAL_CLASSIC
					gpGlobals->rgPlayerStatus[w][kStatusParalyzed] == 0 &&
#else
					gpGlobals->rgPlayerStatus[w][kStatusSlow] == 0 &&
#endif
					gpGlobals->rgPlayerStatus[w][kStatusConfused] == 0 &&
					RandomLong(0, 2) == 0 &&
					gpGlobals->g.PlayerRoles.rgwHP[w] != 0)
				{
					rgfMagAutoDefend[i] = TRUE;
					g_Battle.rgPlayer[i].wCurrentFrame = 3;
				}
				else
				{
					rgfMagAutoDefend[i] = FALSE;
				}
			}
		}
		else if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] == 0 &&
#ifdef PAL_CLASSIC
			gpGlobals->rgPlayerStatus[wPlayerRole][kStatusParalyzed] == 0 &&
#else
			gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSlow] == 0 &&
#endif
			gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] == 0 &&
			RandomLong(0, 2) == 0)
		{
			//选择的目标不能是HP等于0的，所以不用在此判断HP不等于0
			fAutoDefend = TRUE;
			g_Battle.rgPlayer[sTarget].wCurrentFrame = 3;
		}

		//PAL_BattleDelay(12, (WORD)(-((SHORT)wMagic)), FALSE);

		gpGlobals->g.rgObject[wMagic].magic.wScriptOnUse =
			PAL_RunTriggerScript(gpGlobals->g.rgObject[wMagic].magic.wScriptOnUse, wPlayerRole, FALSE);

		if (g_fScriptSuccess)
		{
			PAL_BattleShowEnemyMagicAnim(wEnemyIndex, wMagic, sTarget);

			gpGlobals->g.rgObject[wMagic].magic.wScriptOnSuccess =
				PAL_RunTriggerScript(gpGlobals->g.rgObject[wMagic].magic.wScriptOnSuccess, wPlayerRole, FALSE);
		}

		if ((SHORT)(gpGlobals->g.lprgMagic[wMagicNum].wBaseDamage) > 0)
		{
			if (sTarget == -1)
			{
				//
				// damage all players
				// 攻击我方全体
				for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
				{
					w = gpGlobals->rgParty[i].wPlayerRole;
					if (gpGlobals->g.PlayerRoles.rgwHP[w] == 0)
					{
						//
						// skip dead players
						// 我方阵亡的队员不会受到伤害
						continue;
					}

					//def = PAL_GetPlayerDefense(w);

					//for (x = 0; x < NUM_MAGIC_ELEMENTAL; x++)
					//{
					//	rgwElementalResistance[x] = PAL_GetPlayerElementalResistance(w, x);
					//}

					//iDamage = PAL_CalcMagicDamage(str, def, rgwElementalResistance, 100 + PAL_GetPlayerPoisonResistance(w), 20, wMagic);
					// 这里的伤害代码因为溢出，做了调整
					//iPoisonResistance = PAL_GetPlayerPoisonResistance(w);
					//iPhysicalResistance = PAL_New_GetPlayerPhysicalResistance(w);

					//iDamage = PAL_New_CalcMagicDamage(str, def, rgwElementalResistance, iPoisonResistance, iPhysicalResistance, wMagic);
					iDamage = PAL_New_CalcMagicDamage(i, wEnemyIndex, wMagic);

					iDamage /= ((g_Battle.rgPlayer[i].fDefending ? 2 : 1) *
						((gpGlobals->rgPlayerStatus[w][kStatusProtect] > 0) ? 2 : 1)) +
						(rgfMagAutoDefend[i] ? 1 : 0);

					// 防止伤害溢出，若溢出则攻击无伤害
					//iDamage = max(iDamage, 0);

					// 防止伤害溢出，伤害不得高于我方当前体力值
					iDamage = min(iDamage, gpGlobals->g.PlayerRoles.rgwHP[w]);

					// 我方有 33% 概率自动解昏眠
					if (gpGlobals->rgPlayerStatus[w][kStatusSleep] > 0)
					{
						if (RandomLong(0, 100) <= 33)
						{
							gpGlobals->rgPlayerStatus[w][kStatusSleep] = 0;
						}
					}
#ifndef INVINCIBLE
					gpGlobals->g.PlayerRoles.rgwHP[w] -= iDamage;
#endif

					if (gpGlobals->g.PlayerRoles.rgwHP[w] == 0)
					{
						AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwDeathSound[w]);
					}
				}
			}
			else
			{
				//
				// damage one player
				// 攻击我方单体
				//def = PAL_GetPlayerDefense(wPlayerRole);

				//for (x = 0; x < NUM_MAGIC_ELEMENTAL; x++)
				//{
				//	rgwElementalResistance[x] = PAL_GetPlayerElementalResistance(wPlayerRole, x);
				//}

				//iDamage = PAL_CalcMagicDamage(str, def, rgwElementalResistance, 100 + PAL_GetPlayerPoisonResistance(wPlayerRole), 20, wMagic);
				// 这里的伤害代码因为溢出，做了调整
				//iPoisonResistance = PAL_GetPlayerPoisonResistance(wPlayerRole);
				//iPhysicalResistance = PAL_New_GetPlayerPhysicalResistance(wPlayerRole);

				//iDamage = PAL_New_CalcMagicDamage(str, def, rgwElementalResistance, iPoisonResistance, iPhysicalResistance, wMagic);
				iDamage = PAL_New_CalcMagicDamage(wPlayerRole, wEnemyIndex, wMagic);

				iDamage /= ((g_Battle.rgPlayer[sTarget].fDefending ? 2 : 1) *
					((gpGlobals->rgPlayerStatus[wPlayerRole][kStatusProtect] > 0) ? 2 : 1)) +
					(fAutoDefend ? 1 : 0);

				// 防止伤害溢出，若溢出则攻击无伤害
				//iDamage = max(iDamage, 0);

				// 防止伤害溢出，伤害不得高于我方当前体力值
				iDamage = min(iDamage, gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole]);

				// 我方有 33% 概率自动解昏眠
				if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] > 0)
				{
					if (RandomLong(0, 100) <= 33)
					{
						gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] = 0;
					}
				}

				// 当血量小于或等于0时，血量置0，避免溢出
				iDamage = min(iDamage, gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole]);
#ifndef INVINCIBLE
				gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] -= iDamage;
#endif

				// 我方防御敌方的普攻的音效
				if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0)
				{
					AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwDeathSound[wPlayerRole]);
				}
			}
		}

		// 仅在非自动战斗场合显示HPMP增减变化（像盖罗娇对战石长老是不显示增减变化的）
		//if (!gpGlobals->fAutoBattle)
		{
			PAL_BattleDisplayStatChange();
		}

		for (i = 0; i < 5; i++)
		{
			if (sTarget == -1)
			{
				for (x = 0; x <= gpGlobals->wMaxPartyMemberIndex; x++)
				{
					if (g_Battle.rgPlayer[x].wPrevHP ==
						gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[x].wPlayerRole])
					{
						//
						// Skip unaffected players
						// 跳过未受影响的玩家
						continue;
					}

					g_Battle.rgPlayer[x].wCurrentFrame = 4;
					if (i > 0)
					{
						g_Battle.rgPlayer[x].pos =
							PAL_XY(PAL_X(g_Battle.rgPlayer[x].pos) + (8 >> i),
								PAL_Y(g_Battle.rgPlayer[x].pos) + (4 >> i));
					}
					g_Battle.rgPlayer[x].iColorShift = ((i < 3) ? 6 : 0);
				}
			}
			else
			{
				g_Battle.rgPlayer[sTarget].wCurrentFrame = 4;
				if (i > 0)
				{
					g_Battle.rgPlayer[sTarget].pos =
						PAL_XY(PAL_X(g_Battle.rgPlayer[sTarget].pos) + (8 >> i),
							PAL_Y(g_Battle.rgPlayer[sTarget].pos) + (4 >> i));
				}
				g_Battle.rgPlayer[sTarget].iColorShift = ((i < 3) ? 6 : 0);
			}

			PAL_BattleDelay(1, 0, FALSE);
		}

		// 敌方施法完毕清除刚开始执行行动时备份的法术
		g_Battle.wMagicMoving = 0;

		g_Battle.rgEnemy[wEnemyIndex].wCurrentFrame = 0;
		g_Battle.rgEnemy[wEnemyIndex].pos = g_Battle.rgEnemy[wEnemyIndex].posOriginal;

		PAL_BattleDelay(1, 0, FALSE);
		PAL_BattleUpdateFighters();

		PAL_BattlePostActionCheck(TRUE);
		PAL_BattleDelay(8, 0, TRUE);
	}
	else
	{
		//
		// Physical attack
		// 敌方物理攻击
		WORD wFrameBak = g_Battle.rgPlayer[sTarget].wCurrentFrame;

		//str = (SHORT)g_Battle.rgEnemy[wEnemyIndex].e.wAttackStrength;
		str = g_Battle.rgEnemy[wEnemyIndex].e.wAttackStrength;
		str += (g_Battle.rgEnemy[wEnemyIndex].e.wLevel + 6) * 6;

		if (str < 0)
		{
			str = 0;
		}

		def = PAL_GetPlayerDefense(wPlayerRole);
		//def = PAL_New_GetEnemyAttackStrength(wEnemyIndex);

		// 若我方为 手动防御 则防御值按 2 倍处理
		if (g_Battle.rgPlayer[sTarget].fDefending)
		{
			def *= 2;
		}

		if (gConfig.fIsWIN95 && g_Battle.ActionQueue[g_Battle.iCurAction].fIsSecond && g_Battle.rgEnemy[wEnemyIndex].e.wMagic == 0)
			AUDIO_PlaySound(g_Battle.rgEnemy[wEnemyIndex].e.wMagicSound);
		else
			AUDIO_PlaySound(g_Battle.rgEnemy[wEnemyIndex].e.wAttackSound);

		iCoverIndex = -1;

		// 我方有 58.8 % 的概率 自动防御
		fAutoDefend = (RandomLong(0, 16) >= 10);

		//
		// Check if the inflictor should be protected
		// 检查敌方要攻击的队员是否需要队友援护，若该队员本回合自动防御，且其状态为 亡 乱 眠 定，则不需要援护，两者有一条不成立则进入代码块
		if ((PAL_IsPlayerDying(wPlayerRole) ||
			gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] > 0 ||
			gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] > 0 ||
			gpGlobals->rgPlayerStatus[wPlayerRole][kStatusParalyzed] > 0) && fAutoDefend)
		{
			// 为本回合设置援护者
			w = gpGlobals->g.PlayerRoles.rgwCoveredBy[wPlayerRole];

			// 获取援护者编号
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				if (gpGlobals->rgParty[i].wPlayerRole == w)
				{
					iCoverIndex = i;
					break;
				}
			}

			// 若此队员确实存在援护者，则执行
			if (iCoverIndex != -1)
			{
				// 若援护者状态为 亡 乱 眠 定，则无队员可援护此虚弱者（取消对队员援护）
				if (PAL_IsPlayerDying(gpGlobals->rgParty[iCoverIndex].wPlayerRole) ||
					gpGlobals->rgPlayerStatus[gpGlobals->rgParty[iCoverIndex].wPlayerRole][kStatusConfused] > 0 ||
					gpGlobals->rgPlayerStatus[gpGlobals->rgParty[iCoverIndex].wPlayerRole][kStatusSleep] > 0 ||
					gpGlobals->rgPlayerStatus[gpGlobals->rgParty[iCoverIndex].wPlayerRole][kStatusParalyzed] > 0)
				{
					iCoverIndex = -1;
				}
			}
		}

		//
		// If no one can cover the inflictor and inflictor is in a
		// bad status, don't evade
		// -如果没有人能够援护队友，并且队友处于糟糕的状态，不要逃避-
		// 若其他队员全灭，且当前虚弱队员状态为 乱 眠 定，则此队员取消自动防御
		if (iCoverIndex == -1 &&
			(gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] > 0 ||
				gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] > 0 ||
#ifdef PAL_CLASSIC
				gpGlobals->rgPlayerStatus[wPlayerRole][kStatusParalyzed] > 0))
#else
			gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSlow] > 0))
#endif
		{
			fAutoDefend = FALSE;
		}

		// 敌方攻击我方前的预运动帧
		for (i = 0; i < g_Battle.rgEnemy[wEnemyIndex].e.wMagicFrames; i++)
		{
			g_Battle.rgEnemy[wEnemyIndex].wCurrentFrame =
				g_Battle.rgEnemy[wEnemyIndex].e.wIdleFrames + i;
			PAL_BattleDelay(2, 0, FALSE);
		}

		// 敌方攻击我方前后退蓄力？？
		for (i = 0; i < 3 - g_Battle.rgEnemy[wEnemyIndex].e.wMagicFrames; i++)
		{
			x = PAL_X(g_Battle.rgEnemy[wEnemyIndex].pos) - 2;
			y = PAL_Y(g_Battle.rgEnemy[wEnemyIndex].pos) - 1;
			g_Battle.rgEnemy[wEnemyIndex].pos = PAL_XY(x, y);
			PAL_BattleDelay(1, 0, FALSE);
		}

		// 播放敌方物攻音效
		if (!gConfig.fIsWIN95 || g_Battle.rgEnemy[wEnemyIndex].e.wActionSound != 0)
		{
			AUDIO_PlaySound(g_Battle.rgEnemy[wEnemyIndex].e.wActionSound);
		}
		PAL_BattleDelay(1, 0, FALSE);

		ex = PAL_X(g_Battle.rgPlayer[sTarget].pos) - 44;
		ey = PAL_Y(g_Battle.rgPlayer[sTarget].pos) - 16;

		iSound = g_Battle.rgEnemy[wEnemyIndex].e.wCallSound;

		if (iCoverIndex != -1)
		{
			iSound = gpGlobals->g.PlayerRoles.rgwCoverSound[gpGlobals->rgParty[iCoverIndex].wPlayerRole];

			g_Battle.rgPlayer[iCoverIndex].wCurrentFrame = 3;

			x = PAL_X(g_Battle.rgPlayer[sTarget].pos) - 24;
			y = PAL_Y(g_Battle.rgPlayer[sTarget].pos) - 12;

			g_Battle.rgPlayer[iCoverIndex].pos = PAL_XY(x, y);
		}
		else if (fAutoDefend)
		{
			// 准备我方受击音效及我方置帧到受击帧
			g_Battle.rgPlayer[sTarget].wCurrentFrame = 3;
			iSound = gpGlobals->g.PlayerRoles.rgwCoverSound[wPlayerRole];
		}

		if (g_Battle.rgEnemy[wEnemyIndex].e.wAttackFrames == 0)
		{
			g_Battle.rgEnemy[wEnemyIndex].wCurrentFrame =
				g_Battle.rgEnemy[wEnemyIndex].e.wIdleFrames - 1;

			g_Battle.rgEnemy[wEnemyIndex].pos = PAL_XY(ex, ey);

			PAL_BattleDelay(2, 0, FALSE);
		}
		else
		{
			for (i = 0; i <= g_Battle.rgEnemy[wEnemyIndex].e.wAttackFrames; i++)
			{
				// 敌方一步移动到我方面前
				g_Battle.rgEnemy[wEnemyIndex].wCurrentFrame =
					g_Battle.rgEnemy[wEnemyIndex].e.wIdleFrames +
					g_Battle.rgEnemy[wEnemyIndex].e.wMagicFrames + i - 1;

				g_Battle.rgEnemy[wEnemyIndex].pos = PAL_XY(ex, ey);

				// 播放敌方物攻延迟帧
				PAL_BattleDelay(g_Battle.rgEnemy[wEnemyIndex].e.wActWaitFrames, 0, FALSE);
			}
		}

		// 当我方非自动防御时计算对我方造成的伤害
		if (!fAutoDefend)
		{
			g_Battle.rgPlayer[sTarget].wCurrentFrame = 4;

			//iDamage = PAL_CalcPhysicalAttackDamage(str + RandomLong(0, 2), def, 2);
			iPhysicalResistance = PAL_New_GetPlayerPhysicalResistance(wPlayerRole);
			iDamage = PAL_CalcPhysicalAttackDamage(str + RandomLong(0, 2), def, PAL_New_GetEnemyPhysicalResistance(wEnemyIndex) - iPhysicalResistance);
			iDamage += RandomLong(0, 1);

			// 敌方会有 10 % 概率普攻暴击
			if (RandomLong(1, 100) <= 10)
			{
				iDamage *= 2;
			}

			if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusProtect])
			{
				iDamage /= 2;
			}

			// 防止伤害溢出，若溢出则伤害为 1
			iDamage = max(iDamage, 1);

			// 防止伤害溢出，伤害不得高于我方当前体力值
			iDamage = min(iDamage, gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole]);

#ifndef INVINCIBLE
			gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] -= iDamage;
#endif
			// 我方有 33% 概率自动解昏眠（被敌方打醒）
			if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] > 0)
			{
				if (RandomLong(0, 100) <= 33)
				{
					gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] = 0;
				}
			}

			// 战斗显示状态更改
			PAL_BattleDisplayStatChange();

			// 我方受击者战像颜色偏移至高光
			g_Battle.rgPlayer[sTarget].iColorShift = 6;
		}

		// 若敌人普攻附带物为 对象79 则根据 喂屎概率 回复敌方血量
		if (g_Battle.rgEnemy[wEnemyIndex].e.wAttackEquivItem == 79 &&
			g_Battle.rgEnemy[wEnemyIndex].e.wAttackEquivItemRate >= RandomLong(1, 100))
		{
			g_Battle.rgEnemy[wEnemyIndex].dwActualHealth += iDamage * 4;
			if (g_Battle.rgEnemy[wEnemyIndex].dwActualHealth > g_Battle.rgEnemy[wEnemyIndex].dwMaxHealth)
			{
				g_Battle.rgEnemy[wEnemyIndex].dwActualHealth = g_Battle.rgEnemy[wEnemyIndex].dwMaxHealth;
			}
		}

		if (!gConfig.fIsWIN95 || iSound != 0)
		{
			// 播放我方受击音效
			AUDIO_PlaySound(iSound);
		}

		// 延迟1帧后更新屏幕
		PAL_BattleDelay(1, 0, FALSE);

		// 我方受击者战像颜色偏移回正常
		g_Battle.rgPlayer[sTarget].iColorShift = 0;

		if (iCoverIndex != -1)
		{
			// 我方援护者受击
			g_Battle.rgEnemy[wEnemyIndex].pos =
				PAL_XY(PAL_X(g_Battle.rgEnemy[wEnemyIndex].pos) - 10,
					PAL_Y(g_Battle.rgEnemy[wEnemyIndex].pos) - 8);
			g_Battle.rgPlayer[iCoverIndex].pos =
				PAL_XY(PAL_X(g_Battle.rgPlayer[iCoverIndex].pos) + 4,
					PAL_Y(g_Battle.rgPlayer[iCoverIndex].pos) + 2);
		}
		else
		{
			// 我方本人受击
			g_Battle.rgPlayer[sTarget].pos =
				PAL_XY(PAL_X(g_Battle.rgPlayer[sTarget].pos) + 8,
					PAL_Y(g_Battle.rgPlayer[sTarget].pos) + 4);
		}

		// 延迟1帧后更新屏幕
		PAL_BattleDelay(1, 0, FALSE);

		if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0)
		{
			// 若敌方攻击被我方抵挡则播放防御音效，并将受击者继续置帧至帧二（防御帧）
			AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwDeathSound[wPlayerRole]);
			wFrameBak = 2;
		}
		else if (PAL_IsPlayerDying(wPlayerRole))
		{
			// 若我方未抵挡敌方攻击则置帧至帧一（受击帧）
			wFrameBak = 1;
		}

		if (iCoverIndex == -1)
		{
			// 我方受击身位后移
			g_Battle.rgPlayer[sTarget].pos =
				PAL_XY(PAL_X(g_Battle.rgPlayer[sTarget].pos) + 2,
					PAL_Y(g_Battle.rgPlayer[sTarget].pos) + 1);
		}

		// 延迟3帧后更新屏幕
		PAL_BattleDelay(3, 0, FALSE);

		// 预定敌方施展攻击者下次身位至攻击前身位，并将敌方施展攻击者置帧帧零（空闲帧）
		g_Battle.rgEnemy[wEnemyIndex].pos = g_Battle.rgEnemy[wEnemyIndex].posOriginal;
		g_Battle.rgEnemy[wEnemyIndex].wCurrentFrame = 0;

		// 延迟1帧后更新屏幕
		PAL_BattleDelay(1, 0, FALSE);

		// 预定我方受击者下次身位至受击前身位，并将我方受击者置帧帧零（空闲帧）
		g_Battle.rgPlayer[sTarget].wCurrentFrame = wFrameBak;
		PAL_BattleDelay(1, 0, TRUE);

		// 延迟1帧后更新屏幕
		//PAL_BattleDelay(4, 0, TRUE);
		PAL_BattleDelay(3, 0, TRUE);

		// 更新战斗者
		PAL_BattleUpdateFighters();

		// 若无援护者正在援护受击队员，且普攻喂屎未失手，则喂屎成功。这里我删掉了毒抗判断
		//if (iCoverIndex == -1 && !fAutoDefend &&
		//	g_Battle.rgEnemy[wEnemyIndex].e.wAttackEquivItemRate >= RandomLong(1, 10) &&
		//	PAL_GetPlayerPoisonResistance(wPlayerRole) < RandomLong(1, 100))
		if (iCoverIndex == -1 && !fAutoDefend &&
			g_Battle.rgEnemy[wEnemyIndex].e.wAttackEquivItemRate >= RandomLong(1, 100))
		{
			i = g_Battle.rgEnemy[wEnemyIndex].e.wAttackEquivItem;
			gpGlobals->g.rgObject[i].item.wScriptOnUse =
				PAL_RunTriggerScript(gpGlobals->g.rgObject[i].item.wScriptOnUse, wPlayerRole, FALSE);
		}

		// 战后行动检查，检查敌方全体是否血量为0，若是则敌方死亡，若敌方全部死亡则战斗胜利
		PAL_BattlePostActionCheck(TRUE);
	}

end:
#ifndef PAL_CLASSIC
	//
	// Check poisons
	//
	if (!g_Battle.rgEnemy[wEnemyIndex].fDualMove)
	{
		PAL_BattleBackupStat();

		for (i = 0; i < MAX_POISONS; i++)
		{
			if (g_Battle.rgEnemy[wEnemyIndex].rgPoisons[i].wPoisonID != 0)
			{
				g_Battle.rgEnemy[wEnemyIndex].rgPoisons[i].wPoisonScript =
					PAL_RunTriggerScript(g_Battle.rgEnemy[wEnemyIndex].rgPoisons[i].wPoisonScript, wEnemyIndex, FALSE);
			}
		}

		if (PAL_BattleDisplayStatChange())
		{
			PAL_BattleDelay(6, 0, FALSE);
		}
	}

	PAL_BattlePostActionCheck(FALSE);

	//
	// Update statuses
	//
	for (i = 0; i < kStatusAll; i++)
	{
		if (g_Battle.rgEnemy[wEnemyIndex].rgwStatus[i] > 0)
		{
			g_Battle.rgEnemy[wEnemyIndex].rgwStatus[i]--;
		}
	}
#else
	i = 0; // do nothing
#endif
}

VOID
PAL_BattleStealFromEnemy(
	WORD           wTarget,
	WORD           wStealRate
)
/*++
  Purpose:

	Steal from the enemy.

  Parameters:

	[IN]  wTarget - the target enemy index.

	[IN]  wStealRate - the rate of successful theft.

  Return value:

	None.

--*/
{
	int   iPlayerIndex = g_Battle.wMovingPlayerIndex;
	int   offset, x, y, i, xBase, yBase;
	WCHAR s[256] = L"";

	BYTE  h;
	BATTLEENEMY			be;

	static BYTE			bufImage[2048];
	const int           iPictureYOffset = (gConfig.ScreenLayout.ExtraItemDescLines > 1) ? (gConfig.ScreenLayout.ExtraItemDescLines - 1) * 16 : 0;
	int   iItemNum;

	LPCWSTR lpszItemText;
	LPCBITMAPRLE pBG;
	INT iBGWidth, iBGHeight, iBG_X, iBG_Y;

	// 写入当前事件状态，当前战斗已偷窃过了，下次战斗时不能再偷了
	(&gpGlobals->g.lprgEventObject[g_Battle.wEventsThatCauseFighting])->fIsHasBeenStolen = 1;

	//
	// FixBug 参与一场敌方有五人的战役时，我方全灭后去参与一场小于五人的战役时
	// 上一次战役的敌方信息并未被清理，导致战斗开始会有上一场的残留信息，把上
	// 一场残留的敌人的随身物品也偷走了。。。。。MAX_ENEMIES_IN_TEAM -> g_Battle.wMaxEnemyIndex
	// 
	for (h = 0; h <= g_Battle.wMaxEnemyIndex; h++)
	{
		// 我方战斗图像置帧10
		g_Battle.rgPlayer[iPlayerIndex].wCurrentFrame = 10;

		// 若当前敌人已阵亡则跳过敌人
		be = g_Battle.rgEnemy[h];
		if (be.wObjectID == 0 || be.e.wHealth == 0)
		{
			continue;
		}

		offset = ((INT)wTarget - iPlayerIndex) * 8;

		x = PAL_X(be.pos) + 64 - offset;
		y = PAL_Y(be.pos) + 20 - offset / 2;

		g_Battle.rgPlayer[iPlayerIndex].pos = PAL_XY(x, y);

		PAL_BattleDelay(1, 0, TRUE);
		AUDIO_PlaySound(174);

		for (i = 0; i < MAX_ENEMIES_IN_TEAM; i++)
		{
			x -= i + 8;
			y -= 4;

			g_Battle.rgPlayer[iPlayerIndex].pos = PAL_XY(x, y);

			if (i == 4)
			{
				be.iColorShift = 6;
			}

			PAL_BattleDelay(1, 0, TRUE);
		}

		be.iColorShift = 0;
		x--;
		g_Battle.rgPlayer[iPlayerIndex].pos = PAL_XY(x, y);
		PAL_BattleDelay(3, 0, TRUE);

		g_Battle.rgPlayer[iPlayerIndex].state = kFighterWait;
		g_Battle.rgPlayer[iPlayerIndex].flTimeMeter = 0;
		PAL_BattleUpdateFighters();
		PAL_BattleDelay(1, 0, TRUE);

		// 我方战斗图像置帧0
		g_Battle.rgPlayer[iPlayerIndex].wCurrentFrame = 0;
	}

	// 最后一次我方战斗图像置帧0 Debug
	g_Battle.rgPlayer[iPlayerIndex].wCurrentFrame = 0;

	for (h = 0; h <= g_Battle.wMaxEnemyIndex; h++)
	{
		// 若当前敌人已阵亡则跳过敌人
		be = g_Battle.rgEnemy[h];
		if (be.wObjectID == 0 || be.e.wHealth == 0)
		{
			continue;
		}

		if (be.e.nStealItem > 0)
		{
			// 设置战利品图像框坐标
			xBase = (320 - 64) / 2;
			yBase = 73;

			if (be.e.wStealItem == 0)
			{
				//
				// stolen coins
				// 若可偷物品为金钱

				// 将敌方可偷金钱添加到腰包，并删除敌方可偷金钱
				iItemNum = g_Battle.rgEnemy[h].e.nStealItem;
				gpGlobals->dwCash += iItemNum;
				g_Battle.rgEnemy[h].e.nStealItem = 0;

				if (iItemNum > 0)
				{
					// 组织语言“偷得-谁谁谁-的 多少 道具”
					PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"偷得-%ls- @%d @%ls@", PAL_GetWord(be.wObjectID), iItemNum, PAL_GetWord(BATTLEWIN_DOLLAR_LABEL));

					// 绘制偷得谁谁谁的多少金钱
					if (s[0] != '\0')
					{
						PAL_StartDialog(kDialogCenterWindow, 0, 0, FALSE);
						PAL_ShowDialogText(s);
					}
				}
			}
			else
			{
				//
				// stolen item
				// 若可偷物品为道具

				// 将敌方可偷物添加到腰包，并删除敌方可偷物
				iItemNum = g_Battle.rgEnemy[h].e.nStealItem;
				PAL_AddItemToInventory(be.e.wStealItem, iItemNum);
				g_Battle.rgEnemy[h].e.nStealItem = 0;

				pBG = PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX);
				iBGWidth = PAL_RLEGetWidth(pBG);
				iBGHeight = PAL_RLEGetHeight(pBG);
				iBG_X = (320 - iBGWidth) / 2;
				iBG_Y = (200 - iBGHeight) / 2;
				SDL_Rect rect = { iBG_X, iBG_Y, iBGWidth, iBGHeight };

				// 绘制物品图边框
				PAL_RLEBlitToSurface(pBG, gpScreen, PAL_XY(iBG_X, iBG_Y));

				// 绘制物品图
				PAL_MKFReadChunk(bufImage, 2048, gpGlobals->g.rgObject[be.e.wStealItem].item.wBitmap, gpGlobals->f.fpBALL);
				PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(xBase + 8, yBase + 7 - iPictureYOffset));

				VIDEO_UpdateScreen(&rect);

				// 组织语言“偷得-谁谁谁-的 多少 道具”
				lpszItemText = PAL_GetWord(be.e.wStealItem);
				PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"偷得-%ls- @%d @%ls@", PAL_GetWord(be.wObjectID), iItemNum, lpszItemText);

				//
				// Draw the picture of current selected item
				// 绘制偷得谁谁谁的多少道具
				if (s[0] != '\0')
				{
					PAL_StartDialog(kDialogCenterWindow, 0, 0, FALSE);
					PAL_ShowDialogText(s);
				}
			}

			// 更新屏幕，同步画面
			VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);
			VIDEO_UpdateScreen(NULL);
		}
	}
}

VOID
PAL_BattleSimulateMagic(
	SHORT      sTarget,
	WORD       wMagicObjectID,
	INT        wBaseDamage
)
/*++
  Purpose:

	Simulate a magic for players. Mostly used in item throwing script.
	为玩家模拟魔法。主要用于物品投掷脚本。

  Parameters:

	[IN]  sTarget - the target enemy index. -1 = all enemies.
	目标敌人索引-1=所有敌人。

	[IN]  wMagicObjectID - the object ID of the magic to be simulated.
	要模拟的魔术的对象ID。

	[IN]  wBaseDamage - the base damage of the simulation.
	模拟的基础损伤。

  Return value:

	None.
	无

--*/
{
	//SHORT   sDamage;
	INT		iDamage, iPoisonResistance, iPhysicalResistance;
	int     i, def;

	if (gpGlobals->g.rgObject[wMagicObjectID].magic.wFlags & kMagicFlagApplyToAll)
	{
		//
		// 该仙术本就可以攻击敌方全体，将目标改为敌方全体
		//
		sTarget = -1;
	}
	else if (sTarget == -1)
	{
		//
		// 敌方目标号参数为 -1 时（-1 就是随机攻击目标），将随机攻击敌方单人
		//
		sTarget = PAL_BattleSelectAutoTargetFrom(sTarget);
	}

	//
	// Show the magic animation
	// 显示仙术动画
	PAL_BattleShowPlayerOffMagicAnim(0xFFFF, wMagicObjectID, sTarget, FALSE);

	if (gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[wMagicObjectID].magic.wMagicNumber].wBaseDamage > 0 ||
		wBaseDamage > 0)
	{
		if (sTarget == -1)
		{
			//
			// Apply to all enemies
			// 攻击敌方全体
			for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
			{
				if (g_Battle.rgEnemy[i].wObjectID == 0)
				{
					continue;
				}

				//def = g_Battle.rgEnemy[i].e.wDefense;
				//def += (g_Battle.rgEnemy[i].e.wLevel + 6) * 4;

				//if (def < 0)
				//{
				//	def = 0;
				//}

				//iDamage = PAL_CalcMagicDamage(wBaseDamage, (WORD)def, g_Battle.rgEnemy[i].e.wElemResistance, g_Battle.rgEnemy[i].e.wPoisonResistance, 1, wMagicObjectID);
				// 这里的伤害代码因为溢出，做了调整
				//iPoisonResistance = PAL_New_GetEnemyPoisonResistance(i);
				//iPhysicalResistance = PAL_New_GetEnemyPhysicalResistance(i);

				//iDamage = PAL_New_CalcMagicDamage(wBaseDamage, def, g_Battle.rgEnemy[i].e.wElemResistance, iPoisonResistance, iPhysicalResistance, wMagicObjectID);
				iDamage = PAL_New_CalcMagicDamage(-2, i, wMagicObjectID);

				//iDamage = max(iDamage, 1);
				//iDamage = min(iDamage, g_Battle.rgEnemy[i].e.wHealth);

				g_Battle.rgEnemy[i].e.wHealth -= iDamage;
			}
		}
		else
		{
			//
			// Apply to one enemy
			// 攻击敌方单体
			//def = g_Battle.rgEnemy[sTarget].e.wDefense;
			//def += (g_Battle.rgEnemy[sTarget].e.wLevel + 6) * 4;

			//if (def < 0)
			//{
			//	def = 0;
			//}

			//iDamage = PAL_CalcMagicDamage(wBaseDamage, (WORD)def, g_Battle.rgEnemy[sTarget].e.wElemResistance, g_Battle.rgEnemy[sTarget].e.wPoisonResistance, 1, wMagicObjectID);
			// 这里的伤害代码因为溢出，做了调整
			//iPoisonResistance = PAL_New_GetEnemyPoisonResistance(sTarget);
			//iPhysicalResistance = PAL_New_GetEnemyPhysicalResistance(sTarget);

			//iDamage = PAL_New_CalcMagicDamage(wBaseDamage, def, g_Battle.rgEnemy[sTarget].e.wElemResistance, iPoisonResistance, iPhysicalResistance, wMagicObjectID);
			iDamage = PAL_New_CalcMagicDamage(-2, sTarget, wMagicObjectID);

			//iDamage = max(iDamage, 1);
			//iDamage = min(iDamage, g_Battle.rgEnemy[sTarget].e.wHealth);

			g_Battle.rgEnemy[sTarget].e.wHealth -= iDamage;
		}
	}
}

VOID
PAL_New_BattleSimulateMagicRealInjury(
	SHORT      sTarget,
	WORD       wMagicObjectID,
	INT        iBaseDamage
)
/*++
  Purpose:

	Simulate a magic for players. Mostly used in item throwing script.
	为玩家模拟魔法。主要用于物品投掷脚本。

  Parameters:

	[IN]  sTarget - the target enemy index. -1 = all enemies.
	目标敌人索引-1=所有敌人。

	[IN]  wMagicObjectID - the object ID of the magic to be simulated.
	要模拟的魔术的对象ID。

	[IN]  wBaseDamage - the base damage of the simulation.
	模拟的基础损伤。

  Return value:

	None.
	无

--*/
{
	int     i;

	if (sTarget == -1)
	{
		//
		// Apply to all enemies
		// 攻击敌方全体
		for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
		{
			if (g_Battle.rgEnemy[i].wObjectID == 0)
			{
				continue;
			}

			iBaseDamage = min(g_Battle.rgEnemy[i].e.wHealth, iBaseDamage);
			g_Battle.rgEnemy[i].e.wHealth -= iBaseDamage;
		}
	}
	else
	{
		//
		// Apply to one enemy
		// 攻击敌方单体
		iBaseDamage = min(g_Battle.rgEnemy[sTarget].e.wHealth, iBaseDamage);
		g_Battle.rgEnemy[sTarget].e.wHealth -= iBaseDamage;
	}

	if (gpGlobals->g.rgObject[wMagicObjectID].magic.wFlags & kMagicFlagApplyToAll)
	{
		sTarget = -1;
	}
	else if (sTarget == -1)
	{
		sTarget = PAL_BattleSelectAutoTargetFrom(sTarget);
	}

	//
	// Show the magic animation
	// 显示仙术动画
	PAL_BattleShowPlayerOffMagicAnim(0xFFFF, wMagicObjectID, sTarget, FALSE);

	// 显示全局体力真气耗损值向上飘动
	PAL_BattleDisplayStatChange();

	PAL_BattleDelay(8, 0, TRUE);

	// 检查敌方死未
	PAL_BattlePostActionCheck(FALSE);

	if (g_Battle.iHidingTime > 0)
	{
		if (--g_Battle.iHidingTime == 0)
		{
			VIDEO_BackupScreen(g_Battle.lpSceneBuf);
			PAL_BattleMakeScene();
			PAL_BattleFadeScene();
		}
	}
}

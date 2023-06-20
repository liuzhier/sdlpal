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

#ifndef BATTLE_H
#define BATTLE_H

#include "global.h"
#include "uibattle.h"

//#define       BATTLE_FPS               25
#define       BATTLE_FPS               36
#define       BATTLE_FRAME_TIME        (1000 / BATTLE_FPS)

typedef enum tagBATTLERESULT
{
	kBattleResultWon = 3,      // player won the battle 战斗胜利
	kBattleResultLost = 1,      // player lost the battle 我方全灭
	kBattleResultFleed = 0xFFFF, // player fleed from the battle 我方逃离战斗
	kBattleResultTerminated = 0,      // battle terminated with scripts 使用脚本终止战斗
	kBattleResultOnGoing = 1000,   // the battle is ongoing 战斗正在进行
	kBattleResultPreBattle = 1001,   // running pre-battle scripts 运行战前脚本
	kBattleResultPause = 1002,   // battle pause 战斗暂停
} BATTLERESULT;

typedef enum tagFIGHTERSTATE
{
	kFighterWait,  // waiting time 等待时间
	kFighterCom,   // accepting command 接受命令
	kFighterAct,   // doing the actual move	做实际动作
} FIGHTERSTATE;

typedef enum tagBATTLEACTIONTYPE
{
	kBattleActionPass,          // do nothing 什么都不做
	kBattleActionDefend,        // defend 防御
	kBattleActionAttack,        // physical attack 物理攻击
	kBattleActionMagic,         // use magic 使用仙术
	kBattleActionCoopMagic,     // use cooperative magic 使用合体法术
	kBattleActionFlee,          // flee from the battle 逃离战斗
	kBattleActionThrowItem,     // throw item onto enemy 向敌人投掷物品
	kBattleActionUseItem,       // use item 使用道具
	kBattleActionAttackMate,    // attack teammate (confused only) 攻击队友（仅限混乱状态）
} BATTLEACTIONTYPE;

typedef struct tagBATTLEACTION
{
	BATTLEACTIONTYPE   ActionType;
	WORD               wActionID;   // item/magic to use 要使用的物品/魔法
	SHORT              sTarget;     // -1 for everyone -1就是作用于全体
	FLOAT              flRemainingTime;  // remaining waiting time before the action start 动作开始前的剩余等待时间
} BATTLEACTION;

#pragma pack(1)
typedef struct tagBATTLEENEMY
{
	WORD               wObjectID;              // Object ID of this enemy 此敌人的对象ID
	ENEMY              e;                      // detailed data of this enemy 这个敌人的详细资料
	WORD               rgwStatus[kStatusAll];  // status effects 状态效应
	FLOAT              flTimeMeter;            // time-charging meter (0 = empty, 100 = full). 计时充电表（0=空，100=满）。
	POISONSTATUS       rgPoisons[MAX_POISONS]; // poisons 毒物状态 毒物[MAX_POISONS]
	LPSPRITE           lpSprite;
	PAL_POS            pos;                    // current position on the screen 屏幕上的当前位置
	PAL_POS            posOriginal;            // original position on the screen 屏幕上的原始位置
	WORD               wCurrentFrame;          // current frame number 当前帧号
	FIGHTERSTATE       state;                  // state of this enemy 这个敌人的状态

#ifndef PAL_CLASSIC
	BOOL               fTurnStart;             // 转弯启动
	BOOL               fFirstMoveDone;         // 首次行动完成
	BOOL               fDualMove;              // 两次行动
#endif

	WORD               wScriptOnTurnStart;     // 战斗脚本
	WORD               wScriptOnBattleEnd;     // 战后脚本
	WORD               wScriptOnReady;         // 战前脚本

	INT               wPrevHP;              // HP value prior to action 行动前的HP值

	INT                iColorShift;

	// 新增
	INT              dwActualHealth;         // 敌方单位实际血量
	INT              dwMaxHealth;            // 敌方单位最大血量
} BATTLEENEMY;
#pragma pack()

// We only put some data used in battle here; other data can be accessed in the global data.
// 我们这里只放了一些作战中使用的数据；可以在全局数据中访问其他数据。
typedef struct tagBATTLEPLAYER
{
	INT                iColorShift;
	FLOAT              flTimeMeter;          // time-charging meter (0 = empty, 100 = full). 浮动计时器；//计时充电表（0=空，100=满）。
	FLOAT              flTimeSpeedModifier;  // 时间速度修改器
	WORD               wHidingTime;          // remaining hiding time 剩余隐藏时间
	LPSPRITE           lpSprite;
	PAL_POS            pos;                  // current position on the screen 屏幕上的当前位置
	PAL_POS            posOriginal;          // original position on the screen 屏幕上的原始位置
	WORD               wCurrentFrame;        // current frame number 当前帧号
	FIGHTERSTATE       state;                // state of this player 此玩家的状态
	BATTLEACTION       action;               // action to perform 要执行的操作
	BATTLEACTION       prevAction;           // action of the previous turn 前一轮的动作
	BOOL               fDefending;           // TRUE if player is defending 如果玩家正在防守，则为TRUE
	//WORD               wPrevHP;              // HP value prior to action 行动前的HP值
	INT               wPrevHP;              // HP value prior to action 行动前的HP值
	//WORD               wPrevMP;              // MP value prior to action	 行动前MP值
	INT               wPrevMP;              // MP value prior to action	 行动前MP值
	INT               wPrevSP;              // MP value prior to action	 行动前MP值
#ifndef PAL_CLASSIC
	SHORT              sTurnOrder;           // turn order 转向命令???
#endif
} BATTLEPLAYER;

typedef struct tagSUMMON
{
	LPSPRITE           lpSprite;             // 召唤敌方单位
	WORD               wCurrentFrame;        // 当前帧
} SUMMON;

#define MAX_BATTLE_ACTIONS    256            // 最大战斗回合数？？？
#define MAX_KILLED_ENEMIES    256            // 最大可杀人员数？？？

#ifdef PAL_CLASSIC

typedef enum tabBATTLEPHASE
{
	kBattlePhaseSelectAction,                // 作战阶段选择行动
	kBattlePhasePerformAction                // 作战阶段性能动作
} BATTLEPHASE;

typedef struct tagACTIONQUEUE
{
	BOOL       fIsEnemy;                     // 是否是敌方单位
	WORD       wDexterity;                   // 身法值
	WORD       wIndex;                       // 位置索引
	BOOL       fIsSecond;                    // 是第二个
} ACTIONQUEUE;

//#define MAX_ACTIONQUEUE_ITEMS (MAX_PLAYERS_IN_PARTY + MAX_ENEMIES_IN_TEAM * 2)          // 最大操作队列项（我方最大队伍人数+敌方最大队伍人数 * 2）
#define MAX_ACTIONQUEUE_ITEMS (MAX_PLAYERS_IN_PARTY + MAX_ENEMIES_IN_TEAM * 18)          // 最大操作队列项（我方最大队伍人数+敌方最大队伍人数 * 2）

#endif

// 累计回合型法术
typedef struct tagROUNDCOUNTMAGIC
{
	WORD             wCurrentAllRrounds;                 // 当前累计回合数
	WORD             wThisUseMagicPlayerRoles;           // 本次施法队员编号
	WORD             wThisUseMagicConsumedMP;            // 本次施法消耗的真气
	WORD             wCurrentCumulativeRrounds;          // 当前累计回合数
	WORD             wMaxCumulativeRrounds;              // 最大累计回合数
	WORD             wCumulativeDamageValue;             // 我方累计受到的伤害值
	WORD             wCumulativeRroundsObjectID;         // 回合累计结束后自动使用的法术
	WORD             wCumulativeDamageValueMultiple;     // 回合累计结束后造成的累计伤害的倍数
} ROUNDCOUNTMAGIC;

typedef struct tagBATTLE
{
	BATTLEPLAYER     rgPlayer[MAX_PLAYERS_IN_PARTY];     // 我方队员[我方最大队伍人数]
	BATTLEENEMY      rgEnemy[MAX_ENEMIES_IN_TEAM];       // 敌方队员[敌方最大队伍人数]

	WORD             wMaxEnemyIndex;                     // 最大敌人索引

	SDL_Surface* lpSceneBuf;                             // 场景Buf
	SDL_Surface* lpBackground;                           // 战斗场景

	SHORT            sBackgroundColorShift;              // 背景色偏移

	LPSPRITE         lpSummonSprite;       // sprite of summoned god 被召唤的神的图像
	PAL_POS          posSummon;
	INT              iSummonFrame;         // current frame of the summoned god 被召唤的神的当前帧数

	INT              iExpGained;           // total experience value gained 获得的经验总值
	INT              iCashGained;          // total cash gained 获得的金钱总额

	BOOL             fIsBoss;              // TRUE if boss fight 是否为BOSS战
	BOOL             fEnemyCleared;        // TRUE if enemies are cleared 是否清除了敌人
	BATTLERESULT     BattleResult;         // 战斗结果

	FLOAT            flTimeChargingUnit;   // the base waiting time unit 基本等待时间单位

	BATTLEUI         UI;                   // 不明UI？？？

	LPBYTE           lpEffectSprite;       // 五灵影响

	BOOL             fEnemyMoving;         // TRUE if enemy is moving 敌方是否正在行动

	INT              iHidingTime;          // Time of hiding 隐藏时间

	WORD             wMovingPlayerIndex;   // current moving player index 当前行动中的队员索引（当前是哪个队员在行动）

	int              iBlow;                // 风吹？？？还是物理暴击？？？

#ifdef PAL_CLASSIC
	BATTLEPHASE      Phase;                // 阶段？？？下个回合开始？？？
	ACTIONQUEUE      ActionQueue[MAX_ACTIONQUEUE_ITEMS];               // 操作队列？？？？
	int              iCurAction;           // 当前行动
	BOOL             fRepeat;              // TRUE if player pressed Repeat 玩家是否按下重复上个动作键
	BOOL             fForce;               // TRUE if player pressed Force 玩家是否按下使用最强仙术键
	BOOL             fFlee;                // TRUE if player pressed Flee 玩家是否按下“逃跑”键
	BOOL             fPrevAutoAtk;         // TRUE if auto-attack was used in the previous turn 玩家是否在上一个回合使用了自动攻击（围攻）
	BOOL             fPrevPlayerAutoAtk;   // TRUE if auto-attack was used by previous player in the same turn 玩家是否让前一个队员在同一回合中使用了自动攻击

	WORD             coopContributors[MAX_PLAYERS_IN_PARTY];           // 谁使用了合体法术？？
	BOOL             fThisTurnCoop;        // 本回合为合体法术？？？

	// 新增
	BOOL	             bUiBattleShowDataKey; // 敌方单位本回合使用的法术
	WORD	             wMagicMoving;         // 敌方单位本回合使用的法术
	INT              iSummonChinkNum;      // 召唤神的ID
	ROUNDCOUNTMAGIC  rcmMagicType;         // 累计回合型法术
	INT              iCollectValue;        // 获得的灵葫值总额
	WORD             wEventsThatCauseFighting;                         // 引起该场战斗的事件对象编号
	//WORD	             wSuccessRescue[MAX_PLAYABLE_PLAYER_ROLES];             // 我方援护者成功为虚弱者抵挡敌方攻击次数（为修复我方援助BUG）
#endif
} BATTLE;

typedef enum tagMAGICSPIRITUALITY
{
	kmsSwordAttribute,       // 剑系
	kmsWindAttribute,        // 风系
	kmsThunderAttribute,     // 雷系
	kmsWaterAttribute,       // 水系
	kmsFireAttribute,        // 火系
	kmsEarthAttribute,       // 土系
	kmsPoisonAttribute,      // 毒系
	kmsSootheAttribute,      // 疗系
	kmsSorceryAttribute,     // 巫系
	kmsUniqueSkillAttribute, // 绝技
	kmsDevilSkillAttribute,  // 魔功
} MAGICSPIRITUALITY;

PAL_C_LINKAGE_BEGIN

extern BATTLE g_Battle;

// 加载所有敌方单位
VOID
PAL_LoadBattleSprites(
	VOID
);

// 将战斗场景生成到场景缓冲区中。
VOID
PAL_BattleMakeScene(
	VOID
);

// 淡出战斗场景。
VOID
PAL_BattleFadeScene(
	VOID
);

// 敌方单位逃离战斗。
VOID
PAL_BattleEnemyEscape(
	VOID
);

// 我方队员逃离战斗。
VOID
PAL_BattlePlayerEscape(
	VOID
);

// 开始战斗
BATTLERESULT
PAL_StartBattle(
	WORD        wEnemyTeam,
	BOOL        fIsBoss
);

// 备份场景缓存
VOID
PAL_BattleBackupScene(
	VOID
);

PAL_C_LINKAGE_END

#endif

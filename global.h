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

#ifndef GLOBAL_H
#define GLOBAL_H

#include "common.h"
#include "palcommon.h"
#include "map.h"
#include "ui.h"

//
// SOME NOTES ON "AUTO SCRIPT" AND "TRIGGER SCRIPT":
//
// Auto scripts are executed automatically in each frame.
//
// Trigger scripts are only executed when the event is triggered (player touched
// an event object, player triggered an event script by pressing Spacebar).
//

// status of characters
typedef enum tagSTATUS
{
	kStatusConfused = 0,  // attack friends randomly 疯魔
#ifdef PAL_CLASSIC
	kStatusParalyzed,     // paralyzed 定身
#else
	kStatusSlow,          // slower
#endif
	kStatusSleep,         // not allowed to move 昏眠
	kStatusSilence,       // cannot use magic 咒封
	kStatusPuppet,        // for dead players only, continue attacking 傀儡
	kStatusBravery,       // more power for physical attacks 天罡战气
	kStatusProtect,       // more defense value 真元/金刚
	kStatusHaste,         // faster 身法更快（敌我出招顺序按身法排序的）
	kStatusDualAttack,    // dual attack 醉仙望月步（两次普攻）
	kStatusAll
} STATUS;

#ifndef PAL_CLASSIC
#define kStatusParalyzed kStatusSleep
#endif

// body parts of equipments
typedef enum tagBODYPART
{
	kBodyPartHead = 0,
	kBodyPartBody,
	kBodyPartShoulder,
	kBodyPartHand,
	kBodyPartFeet,
	kBodyPartWear,
	kBodyPartExtra,
} BODYPART;

// state of event object, used by the sState field of the EVENTOBJECT struct
typedef enum tagOBJECTSTATE
{
	kObjStateHidden = 0,
	kObjStateNormal = 1,
	kObjStateBlocker = 2
} OBJECTSTATE, * LPOBJECTSTATE;

typedef enum tagTRIGGERMODE
{
	kTriggerNone = 0,
	kTriggerSearchNear = 1,
	kTriggerSearchNormal = 2,
	kTriggerSearchFar = 3,
	kTriggerTouchNear = 4,
	kTriggerTouchNormal = 5,
	kTriggerTouchFar = 6,
	kTriggerTouchFarther = 7,
	kTriggerTouchFarthest = 8
} TRIGGERMODE;

typedef struct tagEVENTOBJECT
{
	SHORT        sVanishTime;         // vanish time (?) 一般用于怪物这种触发战斗的事件对象，战后设置隐藏该事件对象的时间
	WORD         x;                   // X coordinate on the map 地图上的X坐标
	WORD         y;                   // Y coordinate on the map 地图上的Y坐标
	SHORT        sLayer;              // layer value 图层
	WORD         wTriggerScript;      // Trigger script entry 调查脚本
	WORD         wAutoScript;         // Auto script entry 自动脚本
	SHORT        sState;              // state of this object 事件状态（0=隐藏 1=飘浮 2=实体(碰撞检测) 3=特殊状态(一般用作某些对象脚本的开关)）
	WORD         wTriggerMode;        // trigger mode 触发方式（有自动触发还有调查触发）
	WORD         wSpriteNum;          // number of the sprite 形象号
	USHORT       nSpriteFrames;       // total number of frames of the sprite 每个方向的图像帧数
	WORD         wDirection;          // direction 事件面朝的方向
	WORD         wCurrentFrameNum;    // current frame number 当前帧数是当前方向图像序列的第几帧
	USHORT       nScriptIdleFrame;    // count of idle frames, used by trigger script
	WORD         wSpritePtrOffset;    // FIXME: ???
	USHORT       nSpriteFramesAuto;   // total number of frames of the sprite, used by auto script
	WORD         wScriptIdleFrameCountAuto;     // count of idle frames, used by auto script

	// 新增
	WORD         fIsHasBeenStolen;    // 已被玩家偷窃过了，即使逃跑也不可以再偷第二遍了
} EVENTOBJECT, * LPEVENTOBJECT;

typedef struct tagSCENE
{
	WORD         wMapNum;         // number of the map
	WORD         wScriptOnEnter;  // when entering this scene, execute script from here
	WORD         wScriptOnTeleport;  // when teleporting out of this scene, execute script from here
	WORD         wEventObjectIndex;  // event objects in this scene begins from number wEventObjectIndex + 1
} SCENE, * LPSCENE;

// object including system strings, players, items, magics, enemies and poison scripts.

// system strings and players
typedef struct tagOBJECT_PLAYER
{
	WORD         wReserved[2];    // always zero
	WORD         wScriptOnFriendDeath; // when friends in party dies, execute script from here
	WORD         wScriptOnDying;  // when dying, execute script from here
} OBJECT_PLAYER;

typedef enum tagITEMFLAG
{
	kItemFlagApplyToPlayerAll = (1 << 0),
	kItemFlagUsable = (1 << 1),
	kItemFlagEquipable = (1 << 2),
	kItemFlagThrowable = (1 << 3),
	kItemFlagConsuming = (1 << 4),
	kItemFlagApplyToEnemyAll = (1 << 5),
	kItemFlagSellable = (1 << 6),
	kItemFlagEquipableByPlayerRole_First = (1 << 7),
} ITEMFLAG;

// items
typedef struct tagOBJECT_ITEM_DOS
{
	WORD         wBitmap;         // bitmap number in BALL.MKF
	WORD         wPrice;          // price
	WORD         wScriptOnUse;    // script executed when using this item
	WORD         wScriptOnEquip;  // script executed when equipping this item
	WORD         wScriptOnThrow;  // script executed when throwing this item to enemy
	WORD         wFlags;          // flags
} OBJECT_ITEM_DOS;

// items
typedef struct tagOBJECT_ITEM
{
	WORD         wBitmap;         // bitmap number in BALL.MKF
	WORD         wPrice;          // price
	WORD         wScriptOnUse;    // script executed when using this item
	WORD         wScriptOnEquip;  // script executed when equipping this item
	WORD         wScriptOnThrow;  // script executed when throwing this item to enemy
	WORD         wScriptDesc;     // description script
	WORD         wFlags;          // flags
} OBJECT_ITEM;

typedef enum tagMAGICFLAG
{
	kMagicFlagUnknown1 = (1 << 0),
	kMagicFlagUsableOutsideBattle = (1 << 1),
	kMagicFlagUsableInBattle = (1 << 2),
	kMagicFlagUnknown2 = (1 << 3),
	kMagicFlagUsableToEnemy = (1 << 4),
	kMagicFlagApplyToAll = (1 << 5)
} MAGICFLAG;

// magics
typedef struct tagOBJECT_MAGIC_DOS
{
	WORD         wMagicNumber;      // magic number, according to DATA.MKF #3
	WORD         wReserved1;        // always zero
	WORD         wScriptOnSuccess;  // when magic succeed, execute script from here
	WORD         wScriptOnUse;      // when use this magic, execute script from here
	WORD         wReserved2;        // always zero
	WORD         wFlags;            // flags
} OBJECT_MAGIC_DOS;

// magics
typedef struct tagOBJECT_MAGIC
{
	WORD         wMagicNumber;      // magic number, according to DATA.MKF #3
	WORD         wReserved1;        // always zero
	WORD         wScriptOnSuccess;  // when magic succeed, execute script from here
	WORD         wScriptOnUse;      // when use this magic, execute script from here
	WORD         wScriptDesc;       // description script
	WORD         wReserved2;        // always zero
	WORD         wFlags;            // flags
} OBJECT_MAGIC;

// enemies
typedef struct tagOBJECT_ENEMY
{
	WORD         wEnemyID;        // ID of the enemy, according to DATA.MKF #1.
	// Also indicates the bitmap number in ABC.MKF.
	WORD         wResistanceToSorcery;  // resistance to sorcery and poison (0 min, 10 max)
	WORD         wScriptOnTurnStart;    // script executed when turn starts
	WORD         wScriptOnBattleEnd;    // script executed when battle ends
	WORD         wScriptOnReady;        // script executed when the enemy is ready
} OBJECT_ENEMY;

// poisons (scripts executed in each round)
typedef struct tagOBJECT_POISON
{
	WORD         wPoisonLevel;    // level of the poison
	WORD         wColor;          // color of avatars
	WORD         wPlayerScript;   // script executed when player has this poison (per round)
	WORD         wReserved;       // always zero
	WORD         wEnemyScript;    // script executed when enemy has this poison (per round)
} OBJECT_POISON;

typedef union tagOBJECT_DOS
{
	WORD              rgwData[6];
	OBJECT_PLAYER     player;
	OBJECT_ITEM_DOS   item;
	OBJECT_MAGIC_DOS  magic;
	OBJECT_ENEMY      enemy;
	OBJECT_POISON     poison;
} OBJECT_DOS, * LPOBJECT_DOS;

typedef union tagOBJECT
{
	WORD              rgwData[7];
	OBJECT_PLAYER     player;
	OBJECT_ITEM       item;
	OBJECT_MAGIC      magic;
	OBJECT_ENEMY      enemy;
	OBJECT_POISON     poison;
} OBJECT, * LPOBJECT;

typedef struct tagSCRIPTENTRY
{
	WORD          wOperation;     // operation code
	WORD          rgwOperand[3];  // operands
} SCRIPTENTRY, * LPSCRIPTENTRY;

typedef struct tagINVENTORY
{
	WORD          wItem;             // item object code
	USHORT        nAmount;           // amount of this item
	USHORT        nAmountInUse;      // in-use amount of this item
} INVENTORY, * LPINVENTORY;

typedef struct tagSTORE
{
	WORD          rgwItems[MAX_STORE_ITEM];
} STORE, * LPSTORE;

typedef struct tagENEMY
{
	INT        wIdleFrames;         // total number of frames when idle 原地蠕动帧数
	INT        wMagicFrames;        // total number of frames when using magics 施法帧数
	INT        wAttackFrames;       // total number of frames when doing normal attack 物攻帧数
	INT        wIdleAnimSpeed;      // speed of the animation when idle 原地蠕动速度/延迟
	INT        wActWaitFrames;      // FIXME: ??? 动作等待帧？？？
	INT        wYPosOffset;         // 战场位置的Y坐标偏移（有的图像高度多大需要额外调整）
	INT        wAttackSound;        // sound played when this enemy uses normal attack 物攻音效
	INT        wActionSound;        // FIXME: ??? 行动音效？？？
	INT        wMagicSound;         // sound played when this enemy uses magic 施法音效
	INT        wDeathSound;         // sound played when this enemy dies 去世音效
	INT        wCallSound;          // sound played when entering the battle 进场音效（天空一声巨响，Alex闪亮登场！！！！）
	//WORD        wHealth;             // total HP of the enemy 敌方总体力值（旧的）
	UINT       wHealth;             // total HP of the enemy 敌方总体力值
	INT        wExp;                // How many EXPs we'll get for beating this enemy 战利品：经验值
	INT        wCash;               // how many cashes we'll get for beating this enemy 战利品：金钱
	INT        wLevel;              // this enemy's level 敌方修行
	INT        wMagic;              // this enemy's magic number 敌方法术编号（word.dat地址 / 10）
	INT        wMagicRate;          // chance for this enemy to use magic 施法概率
	INT        wAttackEquivItem;    // equivalence item of this enemy's normal attack 攻击附带哪种道具的使用效果（物攻强行喂屎）道具编号（word.dat地址 / 10）
	INT        wAttackEquivItemRate;// chance for equivalence item 强行喂屎概率
	INT        wStealItem;          // which item we'll get when stealing from this enemy 可偷道具/金钱（word.dat地址 / 10）0 = 可偷金钱
	INT        nStealItem;          // total amount of the items which can be stolen 可偷道具/金钱数目
	INT        wAttackStrength;     // normal attack strength 武
	INT        wMagicStrength;      // magical attack strength 灵
	INT        wDefense;            // resistance to all kinds of attacking 防
	INT        wDexterity;          // dexterity 速（身法，决定敌我谁先出手）
	INT        wFleeRate;           // chance for successful fleeing 逃（吉运，决定我方逃逸率）
	INT        wPoisonResistance;   // resistance to poison 毒抗
	INT        wElemResistance[NUM_MAGIC_ELEMENTAL]; // resistance to elemental magics 灵抗（五灵）
	INT        wPhysicalResistance; // resistance to physical attack 物抗
	INT        wDualMove;           // whether this enemy can do dual move or not 两次行动
	INT        wCollectValue;       // value for collecting this enemy for items 灵葫值
} ENEMY, * LPENEMY;

typedef struct tagENEMYTEAM
{
	WORD        rgwEnemy[MAX_ENEMIES_IN_TEAM];
} ENEMYTEAM, * LPENEMYTEAM;

//typedef WORD PLAYERS[MAX_PLAYER_ROLES];
typedef INT PLAYERS[MAX_PLAYER_ROLES];

#pragma pack(1)
typedef struct tagPLAYERROLES
{
	PLAYERS            rgwAvatar;             // avatar (shown in status view)
	PLAYERS            rgwSpriteNumInBattle;  // sprite displayed in battle (in F.MKF)
	PLAYERS            rgwSpriteNum;          // sprite displayed in normal scene (in MGO.MKF)
	PLAYERS            rgwName;               // name of player class (in WORD.DAT)
	PLAYERS            rgwAttackAll;          // whether player can attack everyone in a bulk or not
	PLAYERS            rgwUnknown1;           // FIXME: ???
	PLAYERS            rgwLevel;              // level
	PLAYERS            rgwMaxHP;              // maximum HP
	PLAYERS            rgwMaxMP;              // maximum MP
	PLAYERS            rgwHP;                 // current HP
	PLAYERS            rgwMP;                 // current MP
	INT                rgwEquipment[MAX_PLAYER_EQUIPMENTS][MAX_PLAYER_ROLES]; // equipments
	PLAYERS            rgwAttackStrength;     // normal attack strength
	PLAYERS            rgwMagicStrength;      // magical attack strength
	PLAYERS            rgwDefense;            // resistance to all kinds of attacking
	PLAYERS            rgwDexterity;          // dexterity
	PLAYERS            rgwFleeRate;           // chance of successful fleeing
	PLAYERS            rgwPoisonResistance;   // resistance to poison
	INT                rgwElementalResistance[NUM_MAGIC_ELEMENTAL][MAX_PLAYER_ROLES]; // resistance to elemental magics
	PLAYERS            rgwSorceryResistance;  // 未知属性 //暂时用来作为巫抗
	PLAYERS            rgwPhysicalResistance; // 未知属性 //暂时用来作为物抗
	PLAYERS            rgwUniqueSkillResistance;    // 未知属性 //暂时用来作为绝技抗性，绝技加成
	PLAYERS            rgwCoveredBy;          // who will cover me when I am low of HP or not sane
	WORD               rgwMagic[MAX_PLAYER_MAGICS][MAX_PLAYER_ROLES]; // magics
	PLAYERS            rgwWalkFrames;         // walk frame (???)
	PLAYERS            rgwCooperativeMagic;   // cooperative magic
	PLAYERS            rgwMaxSP;              // maximum SP
	PLAYERS            rgwSP;                 // current SP
	PLAYERS            rgwDeathSound;         // sound played when player dies
	PLAYERS            rgwAttackSound;        // sound played when player attacks
	PLAYERS            rgwWeaponSound;        // weapon sound (???)
	PLAYERS            rgwCriticalSound;      // sound played when player make critical hits
	PLAYERS            rgwMagicSound;         // sound played when player is casting a magic
	PLAYERS            rgwCoverSound;         // sound played when player cover others
	PLAYERS            rgwDyingSound;         // sound played when player is dying
} PLAYERROLES, * LPPLAYERROLES;
#pragma pack()

typedef enum tagMAGIC_TYPE
{
	kMagicTypeNormal = 0,        // 攻击敌方单人，单特效
	kMagicTypeAttackAll = 1,     // draw the effect on each of the enemies  攻击敌方全体，多特效
	kMagicTypeAttackWhole = 2,   // draw the effect on the whole enemy team 攻击敌方全体，单特效
	kMagicTypeAttackField = 3,   // draw the effect on the battle field     攻击敌方全体，全屏单特效
	kMagicTypeApplyToPlayer = 4,  // the magic is used on one player        我方单人
	kMagicTypeApplyToParty = 5,  // the magic is used on the whole party    我方全体
	kMagicTypeApplyToCasterHimself = 7,  // Apply on the caster himself     施法者自己
	kMagicTypeTrance = 8,  // trance the player                             施法者自己觉醒（队员变身时的过渡效果）
	kMagicTypeSummon = 9,  // summon                                        召唤神
} MAGIC_TYPE;

typedef struct tagMAGIC
{
	WORD               wEffect;               // effect sprite
	WORD               wType;                 // type of this magic
	SHORT               wXOffset;
	SHORT               wYOffset;
	WORD               wSummonEffect;         // summon effect sprite (in F.MKF)
	SHORT              wSpeed;                // speed of the effect
	WORD               wKeepEffect;           // FIXME: ???
	WORD               wFireDelay;            // start frame of the magic fire stage  仙术的起始帧（仙术音效的延迟）
	WORD               wEffectTimes;          // total times of effect  总作用时间
	WORD               wShake;                // shake screen 震动屏幕
	WORD               wWave;                 // wave screen 使画面波浪化
	WORD               wCostSP;              // FIXME: ???
	WORD               wCostMP;               // MP cost
	//WORD               wBaseDamage;           // base damage
	SHORT              wBaseDamage;           // base damage
	WORD               wElemental;            // elemental (0 = No Elemental, last = poison)
	SHORT              wSound;                // sound played when using this magic
} MAGIC, * LPMAGIC;

typedef struct tagBATTLEFIELD
{
	WORD               wScreenWave;                      // level of screen waving 屏幕波动水平
	SHORT              rgsMagicEffect[NUM_MAGIC_ELEMENTAL]; // effect of attributed magics 五灵影响
} BATTLEFIELD, * LPBATTLEFIELD;

// magics learned when level up
typedef struct tagLEVELUPMAGIC
{
	WORD               wLevel;    // level reached
	WORD               wMagic;    // magic learned
} LEVELUPMAGIC, * LPLEVELUPMAGIC;

typedef struct tagLEVELUPMAGIC_ALL
{
	LEVELUPMAGIC       m[MAX_PLAYABLE_PLAYER_ROLES];
} LEVELUPMAGIC_ALL, * LPLEVELUPMAGIC_ALL;

typedef struct tagPALPOS
{
	WORD      x;
	WORD      y;
} PALPOS;

typedef struct tagENEMYPOS
{
	PALPOS pos[MAX_ENEMIES_IN_TEAM][MAX_ENEMIES_IN_TEAM];
} ENEMYPOS, * LPENEMYPOS;

// Exp. points needed for the next level
// 下一级所需的经验点
typedef WORD LEVELUPEXP, * LPLEVELUPEXP;

// game data which is available in data files.
// 数据文件中可用的游戏数据
typedef struct tagGAMEDATA
{
	LPEVENTOBJECT           lprgEventObject;
	int                     nEventObject;

	SCENE                   rgScene[MAX_SCENES];
	OBJECT                  rgObject[MAX_OBJECTS];

	LPSCRIPTENTRY           lprgScriptEntry;
	int                     nScriptEntry;

	LPSTORE                 lprgStore;
	int                     nStore;

	LPENEMY                 lprgEnemy;
	int                     nEnemy;

	LPENEMYTEAM             lprgEnemyTeam;
	int                     nEnemyTeam;

	PLAYERROLES             PlayerRoles;

	LPMAGIC                 lprgMagic;
	int                     nMagic;

	LPBATTLEFIELD           lprgBattleField;
	int                     nBattleField;

	LPLEVELUPMAGIC_ALL      lprgLevelUpMagic;
	int                     nLevelUpMagic;

	ENEMYPOS                EnemyPos;
	LEVELUPEXP              rgLevelUpExp[MAX_LEVELS + 1];

	WORD                    rgwBattleEffectIndex[10][2];
} GAMEDATA, * LPGAMEDATA;

typedef struct tagFILES
{
	FILE* fpFBP;      // battlefield background images
	FILE* fpMGO;      // sprites in scenes
	FILE* fpBALL;     // item bitmaps
	FILE* fpDATA;     // misc data
	FILE* fpF;        // player sprites during battle
	FILE* fpFIRE;     // fire effect sprites
	FILE* fpRGM;      // character face bitmaps
	FILE* fpSSS;      // script data
	FILE* fpFGOD;     // 所有召唤神图像
} FILES, * LPFILES;

// player party
typedef struct tagPARTY
{
	WORD             wPlayerRole;         // player role
	SHORT            x, y;                // position
	WORD             wFrame;              // current frame number
	WORD             wImageOffset;        // FIXME: ???
} PARTY, * LPPARTY;

// player trail, used for other party members to follow the main party member
typedef struct tagTRAIL
{
	WORD             x, y;          // position
	WORD             wDirection;    // direction
} TRAIL, * LPTRAIL;

typedef struct tagEXPERIENCE
{
	WORD         wExp;                // current experience points
	WORD         wReserved;
	WORD         wLevel;              // current level
	WORD         wCount;
} EXPERIENCE, * LPEXPERIENCE;

typedef struct tagALLEXPERIENCE
{
	EXPERIENCE        rgPrimaryExp[MAX_PLAYER_ROLES];
	EXPERIENCE        rgHealthExp[MAX_PLAYER_ROLES];
	EXPERIENCE        rgMagicExp[MAX_PLAYER_ROLES];
	EXPERIENCE        rgAttackExp[MAX_PLAYER_ROLES];
	EXPERIENCE        rgMagicPowerExp[MAX_PLAYER_ROLES];
	EXPERIENCE        rgDefenseExp[MAX_PLAYER_ROLES];
	EXPERIENCE        rgDexterityExp[MAX_PLAYER_ROLES];
	EXPERIENCE        rgFleeExp[MAX_PLAYER_ROLES];

	// 精力隐藏经验
	EXPERIENCE        rgUniqueSkillExp[MAX_PLAYER_ROLES];
} ALLEXPERIENCE, * LPALLEXPERIENCE;

typedef struct tagPOISONSTATUS
{
	WORD              wPoisonID;       // kind of the poison
	WORD              wPoisonScript;   // script entry
	WORD			      wPoisonIntensity; // 毒的烈度
} POISONSTATUS, * LPPOISONSTATUS;

typedef struct tagGLOBALVARS
{
	FILES            f;
	GAMEDATA         g;

	int              iCurMainMenuItem;    // current main menu item number
	int              iCurSystemMenuItem;  // current system menu item number
	int              iCurInvMenuItem;     // current inventory menu item number
	int              iCurPlayingRNG;      // current playing RNG animation
	BYTE             bCurrentSaveSlot;    // current save slot (1-5)
	BOOL             fInMainGame;         // TRUE if in main game
	BOOL             fEnteringScene;      // TRUE if entering a new scene
	BOOL             fNeedToFadeIn;       // TRUE if need to fade in when drawing scene
	BOOL             fInBattle;           // TRUE if in battle
	BOOL             fAutoBattle;         // TRUE if auto-battle
#ifndef PAL_CLASSIC
	BYTE             bBattleSpeed;        // Battle Speed (1 = Fastest, 5 = Slowest)
#endif
	WORD             wLastUnequippedItem; // last unequipped item

	PLAYERROLES      rgEquipmentEffect[MAX_PLAYER_EQUIPMENTS + 1]; // equipment effects
	WORD             rgPlayerStatus[MAX_PLAYER_ROLES][kStatusAll]; // player status

	PAL_POS          viewport;            // viewport coordination 视角坐标（注意与领队坐标不同！）
	PAL_POS          partyoffset;
	WORD             wLayer;
	WORD             wMaxPartyMemberIndex;// max index of members in party (0 to MAX_PLAYERS_IN_PARTY - 1)
	PARTY            rgParty[MAX_PLAYABLE_PLAYER_ROLES]; // player party
	TRAIL            rgTrail[MAX_PLAYABLE_PLAYER_ROLES]; // player trail
	WORD             wPartyDirection;     // direction of the party
	WORD             wNumScene;           // current scene number
	WORD             wNumPalette;         // current palette number
	BOOL             fNightPalette;       // TRUE if use the darker night palette
	WORD             wNumMusic;           // current music number
	WORD             wNumBattleMusic;     // current music number in battle
	WORD             wNumBattleField;     // current battle field number
	DWORD            wCollectValue;       // value of "collected" items
	WORD             wScreenWave;         // level of screen waving
	SHORT            sWaveProgression;
	WORD             wChaseRange;
	WORD             wChasespeedChangeCycles;
	USHORT           nFollower;

	DWORD            dwCash;              // amount of cash

	ALLEXPERIENCE    Exp;                 // experience status
	POISONSTATUS     rgPoisonStatus[MAX_POISONS][MAX_PLAYABLE_PLAYER_ROLES]; // poison status
	INVENTORY        rgInventory[MAX_INVENTORY];  // inventory status
	LPOBJECTDESC     lpObjectDesc;
	DWORD            dwFrameNum;

	//新增
	BOOL				 fShowDataInBattle;
	BOOL             fIsArchiveCompleted;  // 玩家进行了存档
	WORD             wGameDifficulty;      // 游戏难度
	GAMEDATA         gPreCombat;           // 战斗前全部 DATA 数据
	PLAYERROLES      rgPreCombatPlayerRoles;       // 战斗前我方全部数据
	INVENTORY        rgPreCombatInventory[MAX_INVENTORY];  // 战斗前全部库存的道具
	BOOL             fGameStart;          // TRUE 刚刚进入场景，同样也是使用道具的状态开关
	BOOL             rgsDiagramsEffect[NUM_MAGIC_ELEMENTAL]; // 灵珠转换的战场五灵卦性
} GLOBALVARS, * LPGLOBALVARS;

PAL_C_LINKAGE_BEGIN

extern GLOBALVARS* const gpGlobals;

BOOL
PAL_IsWINVersion(
	BOOL* pfIsWIN95
);

CODEPAGE
PAL_DetectCodePage(
	const char* filename
);

INT
PAL_InitGlobals(
	VOID
);

VOID
PAL_FreeGlobals(
	VOID
);

VOID
PAL_SaveGame(
	int           iSaveSlot,
	WORD          wSavedTimes
);

VOID
PAL_InitGameData(
	INT           iSaveSlot
);

VOID
PAL_ReloadInNextTick(
	INT           iSaveSlot
);

INT
PAL_CountItem(
	WORD          wObjectID
);

BOOL
PAL_AddItemToInventory(
	WORD          wObjectID,
	INT           iNum
);

BOOL
PAL_IncreaseHPMPSP(
	WORD          wPlayerRole,
	//SHORT         sHP,
	//SHORT         sMP
	INT         sHP,
	INT         sMP,
	INT         sSP
);

INT
PAL_GetItemAmount(
	WORD        wItem
);

VOID
PAL_UpdateEquipments(
	VOID
);

VOID
PAL_CompressInventory(
	VOID
);

VOID
PAL_RemoveEquipmentEffect(
	WORD         wPlayerRole,
	WORD         wEquipPart
);

VOID
PAL_AddPoisonForPlayer(
	WORD           wPlayerRole,
	WORD           wPoisonID
);

VOID
PAL_CurePoisonByKind(
	WORD           wPlayerRole,
	WORD           wPoisonID
);

VOID
PAL_CurePoisonByLevel(
	WORD           wPlayerRole,
	WORD           wMaxLevel
);

BOOL
PAL_IsPlayerPoisonedByLevel(
	WORD           wPlayerRole,
	WORD           wMinLevel
);

BOOL
PAL_IsPlayerPoisonedByKind(
	WORD           wPlayerRole,
	WORD           wPoisonID
);

INT
PAL_GetPlayerAttackStrength(
	WORD           wPlayerRole
);

INT
PAL_GetPlayerMagicStrength(
	WORD           wPlayerRole
);

INT
PAL_GetPlayerDefense(
	WORD           wPlayerRole
);

INT
PAL_GetPlayerDexterity(
	WORD           wPlayerRole
);

INT
PAL_GetPlayerFleeRate(
	WORD           wPlayerRole
);

INT
PAL_GetPlayerPoisonResistance(
	WORD           wPlayerRole
);

INT
PAL_GetPlayerElementalResistance(
	WORD           wPlayerRole,
	INT            iAttrib
);

WORD
PAL_GetPlayerBattleSprite(
	WORD           wPlayerRole
);

WORD
PAL_GetPlayerCooperativeMagic(
	WORD           wPlayerRole
);

BOOL
PAL_PlayerCanAttackAll(
	WORD           wPlayerRole
);

BOOL
PAL_AddMagic(
	WORD           wPlayerRole,
	WORD           wMagic
);

VOID
PAL_RemoveMagic(
	WORD           wPlayerRole,
	WORD           wMagic
);

VOID
PAL_SetPlayerStatus(
	WORD         wPlayerRole,
	WORD         wStatusID,
	WORD         wNumRound
);

VOID
PAL_RemovePlayerStatus(
	WORD         wPlayerRole,
	WORD         wStatusID
);

VOID
PAL_ClearAllPlayerStatus(
	VOID
);

VOID
PAL_PlayerLevelUp(
	WORD          wPlayerRole,
	WORD          wNumLevel
);

DWORD
PAL_GetLevelUpBaseExp(
	DWORD		wLevel
);

INT
PAL_GetPlayerMaxHP(
	WORD           wPlayerRole
);

INT
PAL_GetPlayerMaxMP(
	WORD           wPlayerRole
);

INT
PAL_GetPlayerMaxSP(
	WORD           wPlayerRole
);

INT
PAL_New_GetPlayerLevel(
	WORD           wPlayerRole
);

INT
PAL_New_GetPlayerHealth(
	WORD           wPlayerRole
);

WORD
PAL_New_GetPlayerSorceryResistance(
	WORD			wPlayerRole
);

WORD
PAL_New_GetPlayerUniqueSkillResistance(
	WORD			wPlayerRole
);

BOOL
PAL_FindEnemyBooty(
	WORD           wScriptEntry,
	WORD           wEventObjectID,
	WORD           wEnemyIndex,
	PAL_POS        pNumPos,
	PAL_POS        pTextPos,
	BOOL           bbJumpScript
);

INT
PAL_New_GetPlayerIndex(
	WORD		wPlayerRole
);

INT
PAL_New_GetPlayerIndexByParty(
	WORD		wPlayerRole
);

BOOL
PAL_New_GetTrueByPercentage(
	WORD		wPercentage
);

INT
PAL_New_GetPlayerPhysicalResistance(
	WORD			wPlayerRole
);

INT
PAL_New_GetPlayerUniqueSkill(
	WORD		wPlayerRole
);

VOID
PAL_New_IncreaseExp(
	INT			iExp
);

WORD
PAL_New_GetMovingPlayerIndex(
	VOID
);

SHORT
PAL_New_GetBattleField(
	SHORT			sElem
);

BOOL
PAL_New_GetDiagramsEffect(
	SHORT			sElem
);

VOID
PAL_RemoveDiagramsEffect(
	VOID
);


PAL_C_LINKAGE_END

#endif

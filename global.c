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
#include "resampler.h"
#include "palcfg.h"

static GLOBALVARS _gGlobals;
GLOBALVARS* const  gpGlobals = &_gGlobals;

CONFIGURATION gConfig;

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define DO_BYTESWAP(buf, size)
#else
#define DO_BYTESWAP(buf, size)                                   \
   do {                                                          \
      int i;                                                     \
      for (i = 0; i < (size) / 2; i++)                           \
      {                                                          \
         ((LPWORD)(buf))[i] = SDL_SwapLE16(((LPWORD)(buf))[i]);  \
      }                                                          \
   } while(0)
#endif

#define LOAD_DATA(buf, size, chunknum, fp)                       \
   do {                                                          \
      PAL_MKFReadChunk((LPBYTE)(buf), (size), (chunknum), (fp)); \
      DO_BYTESWAP(buf, size);                                    \
   } while(0)

BOOL
PAL_IsWINVersion(
	BOOL* pfIsWIN95
)
{
	// 待检查YJ签名的游戏
	//FILE* fps[] = { UTIL_OpenRequiredFile("abc.mkf"), UTIL_OpenRequiredFile("map.mkf"), gpGlobals->f.fpF, gpGlobals->f.fpFBP, gpGlobals->f.fpFIRE, gpGlobals->f.fpMGO };
	FILE* fps[] = { UTIL_OpenRequiredFile("abc.mkf"), UTIL_OpenRequiredFile("map.mkf"), gpGlobals->f.fpF, gpGlobals->f.fpFBP, gpGlobals->f.fpMGO };
	uint8_t* data = NULL;
	int data_size = 0, dos_score = 0, win_score = 0;
	BOOL result = FALSE;

	for (int i = 0; i < sizeof(fps) / sizeof(FILE*); i++)
	{
		//
		// Find the first non-empty sub-file
		// 查找第一个非空子文件
		int count = PAL_MKFGetChunkCount(fps[i]), j = 0, size;
		while (j < count && (size = PAL_MKFGetChunkSize(j, fps[i])) < 4) j++;
		if (j >= count) goto PAL_IsWINVersion_Exit;

		//
		// Read the content and check the compression signature
		// Note that this check is not 100% correct, however in incorrect situations,
		// the sub-file will be over 784MB if uncompressed, which is highly unlikely.
		// 读取内容并检查压缩签名
		// 注意，该检查并非100%正确，
		// 如果未压缩，子文件将超过784MB，这是极不可能的。
		if (data_size < size)
			data = (uint8_t*)realloc(data, data_size = size);

		PAL_MKFReadChunk(data, data_size, j, fps[i]);

		// 如果游戏文件签名不正确则强制退出程序
		if (data[0] == 'Y' && data[1] == 'J' && data[2] == '_' && data[3] == '1')
		{
			if (win_score > 0)
				goto PAL_IsWINVersion_Exit;
			else
				dos_score++;
		}
		else
		{
			if (dos_score > 0)
				goto PAL_IsWINVersion_Exit;
			else
				win_score++;
		}
	}

	//
	// Finally check the size of object definition
	//
	data_size = PAL_MKFGetChunkSize(2, gpGlobals->f.fpSSS);
	if (data_size % sizeof(OBJECT) == 0 && data_size % sizeof(OBJECT_DOS) != 0 && dos_score > 0) goto PAL_IsWINVersion_Exit;
	if (data_size % sizeof(OBJECT_DOS) == 0 && data_size % sizeof(OBJECT) != 0 && win_score > 0) goto PAL_IsWINVersion_Exit;

	if (pfIsWIN95) *pfIsWIN95 = (win_score == sizeof(fps) / sizeof(FILE*)) ? TRUE : FALSE;

	result = TRUE;

PAL_IsWINVersion_Exit:
	free(data);
	fclose(fps[1]);
	fclose(fps[0]);

	return result;
}

CODEPAGE
PAL_DetectCodePage(
	const char* filename
)
{
	FILE* fp;
	char* word_buf = NULL;
	size_t word_len;
	CODEPAGE cp = CP_BIG5;

	if (NULL != (fp = UTIL_OpenFile(filename)))
	{
		fseek(fp, 0, SEEK_END);
		word_len = ftell(fp);
		word_buf = (char*)malloc(word_len);
		fseek(fp, 0, SEEK_SET);
		word_len = fread(word_buf, 1, word_len, fp);
		UTIL_CloseFile(fp);
		// Eliminates null characters so that PAL_MultiByteToWideCharCP works properly
		for (char* ptr = word_buf; ptr < word_buf + word_len; ptr++)
		{
			if (!*ptr)
				*ptr = ' ';
		}
	}

	if (word_buf)
	{
		int probability;
		cp = PAL_DetectCodePageForString(word_buf, (int)word_len, cp, &probability);

		free(word_buf);

		if (probability == 100)
			UTIL_LogOutput(LOGLEVEL_INFO, "PAL_DetectCodePage detected code page '%s' for %s\n", cp ? "GBK" : "BIG5", filename);
		else
			UTIL_LogOutput(LOGLEVEL_WARNING, "PAL_DetectCodePage detected the most possible (%d) code page '%s' for %s\n", probability, cp ? "GBK" : "BIG5", filename);
	}

	return cp;
}

INT
PAL_InitGlobals(
	VOID
)
/*++
  Purpose:

	Initialize global data.
	初始化全局数据。

  Parameters:

	None.
	无

  Return value:

	0 = success, -1 = error.
	0成功  -1失败

--*/
{
	//
	// Open files
	// 打开文件
	gpGlobals->f.fpFBP = UTIL_OpenRequiredFile("fbp.mkf");
	gpGlobals->f.fpMGO = UTIL_OpenRequiredFile("mgo.mkf");
	gpGlobals->f.fpBALL = UTIL_OpenRequiredFile("ball.mkf");
	gpGlobals->f.fpDATA = UTIL_OpenRequiredFile("data.mkf");
	gpGlobals->f.fpF = UTIL_OpenRequiredFile("f.mkf");
	gpGlobals->f.fpFIRE = UTIL_OpenRequiredFile("fire.mkf");
	gpGlobals->f.fpRGM = UTIL_OpenRequiredFile("rgm.mkf");
	gpGlobals->f.fpSSS = UTIL_OpenRequiredFile("sss.mkf");
	gpGlobals->f.fpFGOD = UTIL_OpenRequiredFile("fgod.mkf");

	//
	// Retrieve game resource version
	// 检索游戏资源版本
	if (!PAL_IsWINVersion(&gConfig.fIsWIN95)) return -1;

	//
	// Enable AVI playing only when the resource is WIN95
	// 仅当资源为WIN95时启用AVI播放
	gConfig.fEnableAviPlay = gConfig.fEnableAviPlay && gConfig.fIsWIN95;

	//
	// Detect game language only when no message file specified
	// 仅当未指定消息文件时检测游戏语言
	if (!gConfig.pszMsgFile) PAL_SetCodePage(PAL_DetectCodePage("word.dat"));

	//
	// Set decompress function
	// 设置解压缩功能
	Decompress = gConfig.fIsWIN95 ? YJ2_Decompress : YJ1_Decompress;

	gpGlobals->lpObjectDesc = gConfig.fIsWIN95 ? NULL : PAL_LoadObjectDesc("desc.dat");
	gpGlobals->bCurrentSaveSlot = 1;

	return 0;
}

VOID
PAL_FreeGlobals(
	VOID
)
/*++
  Purpose:

	Free global data.
	释放全局数据

  Parameters:

	None.
	无

  Return value:

	None.
	无

--*/
{
	//
	// Close all opened files
	// 关闭所有打开的文件
	UTIL_CloseFile(gpGlobals->f.fpFBP);
	UTIL_CloseFile(gpGlobals->f.fpMGO);
	UTIL_CloseFile(gpGlobals->f.fpBALL);
	UTIL_CloseFile(gpGlobals->f.fpDATA);
	UTIL_CloseFile(gpGlobals->f.fpF);
	UTIL_CloseFile(gpGlobals->f.fpFIRE);
	UTIL_CloseFile(gpGlobals->f.fpRGM);
	UTIL_CloseFile(gpGlobals->f.fpSSS);
	UTIL_CloseFile(gpGlobals->f.fpFGOD);

	//
	// Free the game data
	// 释放游戏数据
	free(gpGlobals->g.lprgEventObject);
	free(gpGlobals->g.lprgScriptEntry);
	free(gpGlobals->g.lprgStore);
	free(gpGlobals->g.lprgEnemy);
	free(gpGlobals->g.lprgEnemyTeam);
	free(gpGlobals->g.lprgMagic);
	free(gpGlobals->g.lprgBattleField);
	free(gpGlobals->g.lprgLevelUpMagic);

	//
	// Free the object description data
	// 释放对象描述数据
	if (!gConfig.fIsWIN95)
		PAL_FreeObjectDesc(gpGlobals->lpObjectDesc);

	//
	// Clear the instance
	// 清除实例
	memset(gpGlobals, 0, sizeof(GLOBALVARS));

	PAL_FreeConfig();
}


static VOID
PAL_ReadGlobalGameData(
	VOID
)
/*++
  Purpose:

	Read global game data from data files.

  Parameters:

	None.

  Return value:

	None.

--*/
{
	const GAMEDATA* p = &gpGlobals->g;

	LOAD_DATA(p->lprgScriptEntry, p->nScriptEntry * sizeof(SCRIPTENTRY),
		4, gpGlobals->f.fpSSS);

	LOAD_DATA(p->lprgStore, p->nStore * sizeof(STORE), 0, gpGlobals->f.fpDATA);
	LOAD_DATA(p->lprgEnemy, p->nEnemy * sizeof(ENEMY), 1, gpGlobals->f.fpDATA);
	LOAD_DATA(p->lprgEnemyTeam, p->nEnemyTeam * sizeof(ENEMYTEAM),
		2, gpGlobals->f.fpDATA);
	LOAD_DATA(p->lprgMagic, p->nMagic * sizeof(MAGIC), 4, gpGlobals->f.fpDATA);
	LOAD_DATA(p->lprgBattleField, p->nBattleField * sizeof(BATTLEFIELD),
		5, gpGlobals->f.fpDATA);
	LOAD_DATA(p->lprgLevelUpMagic, p->nLevelUpMagic * sizeof(LEVELUPMAGIC_ALL),
		6, gpGlobals->f.fpDATA);
	LOAD_DATA(p->rgwBattleEffectIndex, sizeof(p->rgwBattleEffectIndex),
		11, gpGlobals->f.fpDATA);
	PAL_MKFReadChunk((LPBYTE) & (p->EnemyPos), sizeof(p->EnemyPos),
		13, gpGlobals->f.fpDATA);
	DO_BYTESWAP(&(p->EnemyPos), sizeof(p->EnemyPos));
	PAL_MKFReadChunk((LPBYTE)(p->rgLevelUpExp), sizeof(p->rgLevelUpExp),
		14, gpGlobals->f.fpDATA);
	DO_BYTESWAP(p->rgLevelUpExp, sizeof(p->rgLevelUpExp));
}

static VOID
PAL_InitGlobalGameData(
	VOID
)
/*++
  Purpose:

	Initialize global game data.

  Parameters:

	None.

  Return value:

	None.

--*/
{
	int        len;

#define PAL_DOALLOCATE(fp, num, type, lptype, ptr, n)                            \
   {                                                                             \
      len = PAL_MKFGetChunkSize(num, fp);                                        \
      ptr = (lptype)malloc(len);                                                 \
      n = len / sizeof(type);                                                    \
      if (ptr == NULL)                                                           \
      {                                                                          \
         TerminateOnError("PAL_InitGlobalGameData(): Memory allocation error!"); \
      }                                                                          \
   }

	//
	// If the memory has not been allocated, allocate first.
	//
	if (gpGlobals->g.lprgEventObject == NULL)
	{
		PAL_DOALLOCATE(gpGlobals->f.fpSSS, 0, EVENTOBJECT, LPEVENTOBJECT,
			gpGlobals->g.lprgEventObject, gpGlobals->g.nEventObject);

		PAL_DOALLOCATE(gpGlobals->f.fpSSS, 4, SCRIPTENTRY, LPSCRIPTENTRY,
			gpGlobals->g.lprgScriptEntry, gpGlobals->g.nScriptEntry);

		PAL_DOALLOCATE(gpGlobals->f.fpDATA, 0, STORE, LPSTORE,
			gpGlobals->g.lprgStore, gpGlobals->g.nStore);

		PAL_DOALLOCATE(gpGlobals->f.fpDATA, 1, ENEMY, LPENEMY,
			gpGlobals->g.lprgEnemy, gpGlobals->g.nEnemy);

		PAL_DOALLOCATE(gpGlobals->f.fpDATA, 2, ENEMYTEAM, LPENEMYTEAM,
			gpGlobals->g.lprgEnemyTeam, gpGlobals->g.nEnemyTeam);

		PAL_DOALLOCATE(gpGlobals->f.fpDATA, 4, MAGIC, LPMAGIC,
			gpGlobals->g.lprgMagic, gpGlobals->g.nMagic);

		PAL_DOALLOCATE(gpGlobals->f.fpDATA, 5, BATTLEFIELD, LPBATTLEFIELD,
			gpGlobals->g.lprgBattleField, gpGlobals->g.nBattleField);

		PAL_DOALLOCATE(gpGlobals->f.fpDATA, 6, LEVELUPMAGIC_ALL, LPLEVELUPMAGIC_ALL,
			gpGlobals->g.lprgLevelUpMagic, gpGlobals->g.nLevelUpMagic);

		PAL_ReadGlobalGameData();
	}
#undef PAL_DOALLOCATE
}

static VOID
PAL_LoadDefaultGame(
	VOID
)
/*++
  Purpose:

	Load the default game data.
	加载默认游戏数据。

  Parameters:

	None.

  Return value:

	None.

--*/
{
	GAMEDATA* p = &gpGlobals->g;
	UINT32       i;

	//
	// Load the default data from the game data files.
	// 从游戏数据文件加载默认数据。
	LOAD_DATA(p->lprgEventObject, p->nEventObject * sizeof(EVENTOBJECT),
		0, gpGlobals->f.fpSSS);
	PAL_MKFReadChunk((LPBYTE)(p->rgScene), sizeof(p->rgScene), 1, gpGlobals->f.fpSSS);
	DO_BYTESWAP(p->rgScene, sizeof(p->rgScene));
	if (gConfig.fIsWIN95)
	{
		PAL_MKFReadChunk((LPBYTE)(p->rgObject), sizeof(p->rgObject), 2, gpGlobals->f.fpSSS);
		DO_BYTESWAP(p->rgObject, sizeof(p->rgObject));
	}
	else
	{
		OBJECT_DOS objects[MAX_OBJECTS];
		PAL_MKFReadChunk((LPBYTE)(objects), sizeof(objects), 2, gpGlobals->f.fpSSS);
		DO_BYTESWAP(objects, sizeof(objects));
		//
		// Convert the DOS-style data structure to WIN-style data structure
		// 将DOS样式数据结构转换为WIN样式数据结构
		for (i = 0; i < MAX_OBJECTS; i++)
		{
			memcpy(&p->rgObject[i], &objects[i], sizeof(OBJECT_DOS));
			p->rgObject[i].rgwData[6] = objects[i].rgwData[5];     // wFlags 标志物
			p->rgObject[i].rgwData[5] = 0;                         // wScriptDesc or wReserved2 脚本描述或保留2
		}
	}

	PAL_MKFReadChunk((LPBYTE)(&(p->PlayerRoles)), sizeof(PLAYERROLES),
		3, gpGlobals->f.fpDATA);
	DO_BYTESWAP(&(p->PlayerRoles), sizeof(PLAYERROLES));

	//
	// Set some other default data.
	// 设置一些其他默认数据。
	gpGlobals->dwCash = 0;
	gpGlobals->wNumMusic = 0;
	gpGlobals->wNumPalette = 0;
	gpGlobals->wNumScene = 1;
	gpGlobals->wCollectValue = 0;
	gpGlobals->fNightPalette = FALSE;
	gpGlobals->wMaxPartyMemberIndex = 0;
	gpGlobals->viewport = PAL_XY(0, 0);
	gpGlobals->wLayer = 0;
	gpGlobals->wChaseRange = 1;
#ifndef PAL_CLASSIC
	gpGlobals->bBattleSpeed = 2;
#endif

	memset(gpGlobals->rgInventory, 0, sizeof(gpGlobals->rgInventory));
	memset(gpGlobals->rgPoisonStatus, 0, sizeof(gpGlobals->rgPoisonStatus));
	memset(gpGlobals->rgParty, 0, sizeof(gpGlobals->rgParty));
	memset(gpGlobals->rgTrail, 0, sizeof(gpGlobals->rgTrail));
	memset(&(gpGlobals->Exp), 0, sizeof(gpGlobals->Exp));

	for (i = 0; i < MAX_PLAYER_ROLES; i++)
	{
		gpGlobals->Exp.rgPrimaryExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
		gpGlobals->Exp.rgHealthExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
		gpGlobals->Exp.rgMagicExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
		gpGlobals->Exp.rgUniqueSkillExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
		gpGlobals->Exp.rgAttackExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
		gpGlobals->Exp.rgMagicPowerExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
		gpGlobals->Exp.rgDefenseExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
		gpGlobals->Exp.rgDexterityExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
		gpGlobals->Exp.rgFleeExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
	}

	gpGlobals->fEnteringScene = TRUE;
}

// 存档中的默认数据_通用
typedef struct tagSAVEDGAME_COMMON
{
	WORD             wSavedTimes;             // saved times
	WORD             wSavedGameDifficulty;    // 游戏难度
	WORD             wViewportX, wViewportY;  // viewport location
	WORD             nPartyMember;            // number of members in party
	WORD             wNumScene;               // scene number
	WORD             wNumPalette;             // current palette number
	WORD             wPaletteOffset;          // 调色板偏移
	WORD             wPartyDirection;         // party direction 队伍方向（领队）
	WORD             wNumMusic;               // music number 场景音乐
	WORD             wNumBattleMusic;         // battle music number 战场音乐
	WORD             wNumBattleField;         // battle field number 当前战斗场所
	WORD             wScreenWave;             // level of screen waving 屏幕流动等级
	WORD             wBattleSpeed;            // battle speed 战斗延迟
	DWORD            wCollectValue;           // value of "collected" items 现有灵葫值
	WORD             wLayer;
	WORD             wChaseRange;
	WORD             wChasespeedChangeCycles;
	WORD             nFollower;
	WORD             rgwReserved2[3];         // unused
	DWORD            dwCash;                  // amount of cash 现有金钱
	PARTY            rgParty[MAX_PLAYABLE_PLAYER_ROLES];       // player party
	TRAIL            rgTrail[MAX_PLAYABLE_PLAYER_ROLES];       // player trail
	ALLEXPERIENCE    Exp;                     // experience data 队员各属性隐藏经验
	PLAYERROLES      PlayerRoles;
	POISONSTATUS     rgPoisonStatus[MAX_POISONS][MAX_PLAYABLE_PLAYER_ROLES]; // poison status
	INVENTORY        rgInventory[MAX_INVENTORY];               // inventory status
	SCENE            rgScene[MAX_SCENES];
} SAVEDGAME_COMMON, * LPSAVEDGAME_COMMON;

// DOS版存档中的默认数据
typedef struct tagSAVEDGAME_DOS
{
	WORD             wSavedTimes;             // saved times
	WORD             wSavedGameDifficulty;    // 游戏难度
	WORD             wViewportX, wViewportY;  // viewport location
	WORD             nPartyMember;            // number of members in party
	WORD             wNumScene;               // scene number
	WORD             wNumPalette;             // current palette number
	WORD             wPaletteOffset;
	WORD             wPartyDirection;         // party direction
	WORD             wNumMusic;               // music number
	WORD             wNumBattleMusic;         // battle music number
	WORD             wNumBattleField;         // battle field number
	WORD             wScreenWave;             // level of screen waving
	WORD             wBattleSpeed;            // battle speed
	DWORD             wCollectValue;           // value of "collected" items
	WORD             wLayer;
	WORD             wChaseRange;
	WORD             wChasespeedChangeCycles;
	WORD             nFollower;
	WORD             rgwReserved2[3];         // unused
	DWORD            dwCash;                  // amount of cash
	PARTY            rgParty[MAX_PLAYABLE_PLAYER_ROLES];       // player party
	TRAIL            rgTrail[MAX_PLAYABLE_PLAYER_ROLES];       // player trail
	ALLEXPERIENCE    Exp;                     // experience data
	PLAYERROLES      PlayerRoles;
	POISONSTATUS     rgPoisonStatus[MAX_POISONS][MAX_PLAYABLE_PLAYER_ROLES]; // poison status
	INVENTORY        rgInventory[MAX_INVENTORY];               // inventory status
	SCENE            rgScene[MAX_SCENES];
	OBJECT_DOS       rgObject[MAX_OBJECTS];
	EVENTOBJECT      rgEventObject[MAX_EVENT_OBJECTS];
} SAVEDGAME_DOS, * LPSAVEDGAME_DOS;

// Win版存档中的默认数据
typedef struct tagSAVEDGAME_WIN
{
	WORD             wSavedTimes;             // saved times
	WORD             wSavedGameDifficulty;    // 游戏难度
	WORD             wViewportX, wViewportY;  // viewport location
	WORD             nPartyMember;            // number of members in party
	WORD             wNumScene;               // scene number
	WORD             wNumPalette;             // current palette number
	WORD             wPaletteOffset;
	WORD             wPartyDirection;         // party direction
	WORD             wNumMusic;               // music number
	WORD             wNumBattleMusic;         // battle music number
	WORD             wNumBattleField;         // battle field number
	WORD             wScreenWave;             // level of screen waving
	WORD             wBattleSpeed;            // battle speed
	DWORD             wCollectValue;           // value of "collected" items
	WORD             wLayer;
	WORD             wChaseRange;
	WORD             wChasespeedChangeCycles;
	WORD             nFollower;
	WORD             rgwReserved2[3];         // unused
	DWORD            dwCash;                  // amount of cash
	PARTY            rgParty[MAX_PLAYABLE_PLAYER_ROLES];       // player party
	TRAIL            rgTrail[MAX_PLAYABLE_PLAYER_ROLES];       // player trail
	ALLEXPERIENCE    Exp;                     // experience data
	PLAYERROLES      PlayerRoles;
	POISONSTATUS     rgPoisonStatus[MAX_POISONS][MAX_PLAYABLE_PLAYER_ROLES]; // poison status
	INVENTORY        rgInventory[MAX_INVENTORY];               // inventory status
	SCENE            rgScene[MAX_SCENES];
	OBJECT           rgObject[MAX_OBJECTS];
	EVENTOBJECT      rgEventObject[MAX_EVENT_OBJECTS];
} SAVEDGAME_WIN, * LPSAVEDGAME_WIN;

static BOOL
PAL_LoadGame_Common(
	int                 iSaveSlot,
	LPSAVEDGAME_COMMON  s,
	size_t              size
)
{
	//
	// Try to open the specified file
	// 尝试打开指定的文件
	FILE* fp = UTIL_OpenFileAtPath(gConfig.pszSavePath, PAL_va(1, "%d.rpg", iSaveSlot));
	//
	// Read all data from the file and close.
	// 读取文件中的所有数据并关闭。
	size_t n = fp ? fread(s, 1, size, fp) : 0;

	if (fp != NULL)
	{
		fclose(fp);
	}

	if (n < size - sizeof(EVENTOBJECT) * MAX_EVENT_OBJECTS)
	{
		return FALSE;
	}

	//
	// Adjust endianness
	// 调整尾数
	DO_BYTESWAP(s, size);

	//
	// Cash amount is in DWORD, so do a wordswap in Big-Endian.
	// 现金金额以DWORD表示，Big Endian中的单词交换也是如此。
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	s->dwCash = ((s->dwCash >> 16) | (s->dwCash << 16));
#endif

	//
	// Get common data from the saved game struct.
	// 从保存的游戏结构中获取公共数据。
	gpGlobals->viewport = PAL_XY(s->wViewportX, s->wViewportY);
	gpGlobals->wGameDifficulty = s->wSavedGameDifficulty;
	gpGlobals->wMaxPartyMemberIndex = s->nPartyMember;
	gpGlobals->wNumScene = s->wNumScene;
	gpGlobals->wNumPalette = s->wNumPalette;
	gpGlobals->fNightPalette = (s->wPaletteOffset != 0);
	gpGlobals->wPartyDirection = s->wPartyDirection;
	gpGlobals->wNumMusic = s->wNumMusic;
	gpGlobals->wNumBattleMusic = s->wNumBattleMusic;
	gpGlobals->wNumBattleField = s->wNumBattleField;
	gpGlobals->wScreenWave = s->wScreenWave;
	gpGlobals->sWaveProgression = 0;
	gpGlobals->wCollectValue = s->wCollectValue;
	gpGlobals->wLayer = s->wLayer;
	gpGlobals->wChaseRange = s->wChaseRange;
	gpGlobals->wChasespeedChangeCycles = s->wChasespeedChangeCycles;
	gpGlobals->nFollower = s->nFollower;
	gpGlobals->dwCash = s->dwCash;
#ifndef PAL_CLASSIC
	gpGlobals->bBattleSpeed = s->wBattleSpeed;
	if (gpGlobals->bBattleSpeed > 5 || gpGlobals->bBattleSpeed == 0)
	{
		gpGlobals->bBattleSpeed = 2;
	}
#endif

	memcpy(gpGlobals->rgParty, s->rgParty, sizeof(gpGlobals->rgParty));
	memcpy(gpGlobals->rgTrail, s->rgTrail, sizeof(gpGlobals->rgTrail));
	gpGlobals->Exp = s->Exp;
	gpGlobals->g.PlayerRoles = s->PlayerRoles;
	memset(gpGlobals->rgPoisonStatus, 0, sizeof(gpGlobals->rgPoisonStatus));
	memcpy(gpGlobals->rgInventory, s->rgInventory, sizeof(gpGlobals->rgInventory));
	memcpy(gpGlobals->g.rgScene, s->rgScene, sizeof(gpGlobals->g.rgScene));

	gpGlobals->fEnteringScene = FALSE;

	PAL_CompressInventory();

	return TRUE;
}

static INT
PAL_LoadGame_DOS(
	int            iSaveSlot
)
/*++
  Purpose:

	Load a saved game.
	DOS版读取存档

  Parameters:

	[IN]  szFileName - file name of saved game.

  Return value:

	0 if success, -1 if failed.

--*/
{
	SAVEDGAME_DOS* s = (SAVEDGAME_DOS*)malloc(sizeof(SAVEDGAME_DOS));
	int                       i;

	//
	// Get all the data from the saved game struct.
	// 从保存的游戏结构中获取所有数据。
	if (!PAL_LoadGame_Common(iSaveSlot, (LPSAVEDGAME_COMMON)s, sizeof(SAVEDGAME_DOS)))
		return -1;

	//
	// Convert the DOS-style data structure to WIN-style data structure
	// 将DOS样式数据结构转换为WIN样式数据结构
	for (i = 0; i < MAX_OBJECTS; i++)
	{
		memcpy(&gpGlobals->g.rgObject[i], &s->rgObject[i], sizeof(OBJECT_DOS));
		gpGlobals->g.rgObject[i].rgwData[6] = s->rgObject[i].rgwData[5];     // wFlags
		gpGlobals->g.rgObject[i].rgwData[5] = 0;                            // wScriptDesc or wReserved2
	}
	memcpy(gpGlobals->g.lprgEventObject, s->rgEventObject, sizeof(EVENTOBJECT) * gpGlobals->g.nEventObject);

	free(s);

	//
	// Success
	//
	return 0;
}

static INT
PAL_LoadGame_WIN(
	int            iSaveSlot
)
/*++
  Purpose:

	Load a saved game.
	WIN版读取存档

  Parameters:

	[IN]  szFileName - file name of saved game.

  Return value:

	0 if success, -1 if failed.

--*/
{
	SAVEDGAME_WIN* s = (SAVEDGAME_WIN*)malloc(sizeof(SAVEDGAME_WIN));

	//
	// Get all the data from the saved game struct.
	// 从保存的游戏结构中获取所有数据。
	if (!PAL_LoadGame_Common(iSaveSlot, (LPSAVEDGAME_COMMON)s, sizeof(SAVEDGAME_WIN)))
		return -1;

	memcpy(gpGlobals->g.rgObject, s->rgObject, sizeof(gpGlobals->g.rgObject));
	memcpy(gpGlobals->g.lprgEventObject, s->rgEventObject, sizeof(EVENTOBJECT) * gpGlobals->g.nEventObject);

	free(s);

	//
	// Success
	//
	return 0;
}

static INT
PAL_LoadGame(
	int            iSaveSlot
)
{
	return gConfig.fIsWIN95 ? PAL_LoadGame_WIN(iSaveSlot) : PAL_LoadGame_DOS(iSaveSlot);
}

static VOID
PAL_SaveGame_Common(
	int                iSaveSlot,
	WORD               wSavedTimes,
	LPSAVEDGAME_COMMON s,
	size_t             size
)
{
	FILE* fp;
	size_t i;

	s->wSavedTimes = wSavedTimes;
	s->wSavedGameDifficulty = gpGlobals->wGameDifficulty;
	s->wViewportX = PAL_X(gpGlobals->viewport);
	s->wViewportY = PAL_Y(gpGlobals->viewport);
	s->nPartyMember = gpGlobals->wMaxPartyMemberIndex;
	s->wNumScene = gpGlobals->wNumScene;
	s->wNumPalette = gpGlobals->wNumPalette;
	s->wPaletteOffset = (gpGlobals->fNightPalette ? 0x180 : 0);
	s->wPartyDirection = gpGlobals->wPartyDirection;
	s->wNumMusic = gpGlobals->wNumMusic;
	s->wNumBattleMusic = gpGlobals->wNumBattleMusic;
	s->wNumBattleField = gpGlobals->wNumBattleField;
	s->wScreenWave = gpGlobals->wScreenWave;
	s->wCollectValue = gpGlobals->wCollectValue;
	s->wLayer = gpGlobals->wLayer;
	s->wChaseRange = gpGlobals->wChaseRange;
	s->wChasespeedChangeCycles = gpGlobals->wChasespeedChangeCycles;
	s->nFollower = gpGlobals->nFollower;
	s->dwCash = gpGlobals->dwCash;
#ifndef PAL_CLASSIC
	s->wBattleSpeed = gpGlobals->bBattleSpeed;
#else
	s->wBattleSpeed = 2;
#endif

	memcpy(s->rgParty, gpGlobals->rgParty, sizeof(gpGlobals->rgParty));
	memcpy(s->rgTrail, gpGlobals->rgTrail, sizeof(gpGlobals->rgTrail));
	s->Exp = gpGlobals->Exp;
	s->PlayerRoles = gpGlobals->g.PlayerRoles;
	memcpy(s->rgPoisonStatus, gpGlobals->rgPoisonStatus, sizeof(gpGlobals->rgPoisonStatus));
	memcpy(s->rgInventory, gpGlobals->rgInventory, sizeof(gpGlobals->rgInventory));
	memcpy(s->rgScene, gpGlobals->g.rgScene, sizeof(gpGlobals->g.rgScene));

	//
	// Adjust endianness
	// 调整尾数
	DO_BYTESWAP(s, size);

	//
	// Cash amount is in DWORD, so do a wordswap in Big-Endian.
	// 现金金额以DWORD表示，Big Endian中的单词交换也是如此。
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	s->dwCash = ((s->dwCash >> 16) | (s->dwCash << 16));
#endif

	//
	// Try writing to file
	// 尝试写入文件
	fopen(gConfig.pszSavePath, "w");
	if ((fp = UTIL_OpenFileAtPathForMode(gConfig.pszSavePath, PAL_va(1, "%d.rpg", iSaveSlot), "wb")) == NULL)
	{
		return;
	}

	i = PAL_MKFGetChunkSize(0, gpGlobals->f.fpSSS);
	i += size - sizeof(EVENTOBJECT) * MAX_EVENT_OBJECTS;

	fwrite(s, i, 1, fp);
	fclose(fp);
}

static VOID
PAL_SaveGame_DOS(
	int            iSaveSlot,
	WORD           wSavedTimes
)
/*++
  Purpose:

	Save the current game state to file.

  Parameters:

	[IN]  szFileName - file name of saved game.

  Return value:

	None.

--*/
{
	SAVEDGAME_DOS* s = (SAVEDGAME_DOS*)malloc(sizeof(SAVEDGAME_DOS));
	UINT32                    i;

	//
	// Convert the WIN-style data structure to DOS-style data structure
	//
	for (i = 0; i < MAX_OBJECTS; i++)
	{
		memcpy(&s->rgObject[i], &gpGlobals->g.rgObject[i], sizeof(OBJECT_DOS));
		s->rgObject[i].rgwData[5] = gpGlobals->g.rgObject[i].rgwData[6];     // wFlags
	}
	memcpy(s->rgEventObject, gpGlobals->g.lprgEventObject, sizeof(EVENTOBJECT) * gpGlobals->g.nEventObject);

	//
	// Put all the data to the saved game struct.
	//
	PAL_SaveGame_Common(iSaveSlot, wSavedTimes, (LPSAVEDGAME_COMMON)s, sizeof(SAVEDGAME_DOS));
	free(s);
}

static VOID
PAL_SaveGame_WIN(
	int            iSaveSlot,
	WORD           wSavedTimes
)
/*++
  Purpose:

	Save the current game state to file.

  Parameters:

	[IN]  szFileName - file name of saved game.

  Return value:

	None.

--*/
{
	SAVEDGAME_WIN* s = (SAVEDGAME_WIN*)malloc(sizeof(SAVEDGAME_WIN));

	//
	// Put all the data to the saved game struct.
	//
	memcpy(&s->rgObject, gpGlobals->g.rgObject, sizeof(gpGlobals->g.rgObject));
	memcpy(&s->rgEventObject, gpGlobals->g.lprgEventObject, sizeof(EVENTOBJECT) * gpGlobals->g.nEventObject);

	PAL_SaveGame_Common(iSaveSlot, wSavedTimes, (LPSAVEDGAME_COMMON)s, sizeof(SAVEDGAME_WIN));

	free(s);
}

VOID
PAL_SaveGame(
	int            iSaveSlot,
	WORD           wSavedTimes
)
{
	// 若目录下无SAVE文件夹则创建一个
	mkdir(gConfig.pszSavePath, 0777);

	if (gConfig.fIsWIN95)
		PAL_SaveGame_WIN(iSaveSlot, wSavedTimes);
	else
		PAL_SaveGame_DOS(iSaveSlot, wSavedTimes);
}

VOID
PAL_ReloadInNextTick(
	INT           iSaveSlot
)
/*++
  Purpose:

	Reload the game IN NEXT TICK, avoid reentrant problems.
	在下一次点击时重新加载游戏，避免再次输入问题。

  Parameters:

	[IN]  iSaveSlot - Slot of saved game.

  Return value:

	None.

--*/
{
	gpGlobals->bCurrentSaveSlot = (BYTE)iSaveSlot;
	PAL_SetLoadFlags(kLoadGlobalData | kLoadScene | kLoadPlayerSprite);
	gpGlobals->fEnteringScene = TRUE;
	gpGlobals->fNeedToFadeIn = TRUE;
	gpGlobals->dwFrameNum = 0;
}

VOID
PAL_InitGameData(
	INT         iSaveSlot
)
/*++
  Purpose:

	Initialize the game data (used when starting a new game or loading a saved game).
	初始化游戏数据（在开始新游戏或加载已保存的游戏时使用）。

  Parameters:

	[IN]  iSaveSlot - Slot of saved game.

  Return value:

	None.

--*/
{
	PAL_InitGlobalGameData();

	gpGlobals->bCurrentSaveSlot = (BYTE)iSaveSlot;

	//
	// try loading from the saved game file.
	// 尝试从保存的游戏文件加载。
	if (iSaveSlot == 0 || PAL_LoadGame(iSaveSlot) != 0)
	{
		//
		// Cannot load the saved game file. Load the defaults.
		// 无法加载保存的游戏文件。加载默认项（新的故事）。
		PAL_LoadDefaultGame();
	}

	gpGlobals->iCurInvMenuItem = 0;
	gpGlobals->fInBattle = FALSE;

	memset(gpGlobals->rgPlayerStatus, 0, sizeof(gpGlobals->rgPlayerStatus));

	PAL_UpdateEquipments();
}

INT
PAL_CountItem(
	WORD          wObjectID
)
/*++
 Purpose:

 Count the specified kind of item in the inventory AND in players' equipments.
 清点库存和玩家装备中指定种类的物品。

 Parameters:

 [IN]  wObjectID - object number of the item.

 Return value:

 Counted value.

 --*/
{
	int          index;
	int          count;
	int          i, j, w;

	if (wObjectID == 0)
	{
		return FALSE;
	}

	index = 0;
	count = 0;

	//
	// Search for the specified item in the inventory
	// 搜索库存中的指定项目
	while (index < MAX_INVENTORY)
	{
		if (gpGlobals->rgInventory[index].wItem == wObjectID)
		{
			count = gpGlobals->rgInventory[index].nAmount;
			break;
		}
		else if (gpGlobals->rgInventory[index].wItem == 0)
		{
			break;
		}
		index++;
	}

	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		w = gpGlobals->rgParty[i].wPlayerRole;

		for (j = 0; j < MAX_PLAYER_EQUIPMENTS; j++)
		{
			if (gpGlobals->g.PlayerRoles.rgwEquipment[j][w] == wObjectID)
			{
				count++;
			}
		}
	}
	return count;
}

BOOL
PAL_AddItemToInventory(
	WORD          wObjectID,
	INT           iNum
)
/*++
  Purpose:

	Add or remove the specified kind of item in the inventory.
	在库存中添加或删除指定种类的项目。

  Parameters:

	[IN]  wObjectID - object number of the item.

	[IN]  iNum - number to be added (positive value) or removed (negative value).

  Return value:

	TRUE if succeeded, FALSE if failed.

--*/
{
	int          index;
	BOOL         fFound;

	if (wObjectID == 0)
	{
		return FALSE;
	}

	if (iNum == 0)
	{
		iNum = 1;
	}

	index = 0;
	fFound = FALSE;

	//
	// Search for the specified item in the inventory
	//
	while (index < MAX_INVENTORY)
	{
		if (gpGlobals->rgInventory[index].wItem == wObjectID)
		{
			fFound = TRUE;
			break;
		}
		else if (gpGlobals->rgInventory[index].wItem == 0)
		{
			break;
		}
		index++;
	}

	if (iNum > 0)
	{
		//
		// Add item
		// 添加道具
		if (index >= MAX_INVENTORY)
		{
			//
			// inventory is full. cannot add item
			// 库存已满。无法添加项目
			return FALSE;
		}

		if (fFound)
		{
			gpGlobals->rgInventory[index].nAmount += iNum;
			if (gpGlobals->rgInventory[index].nAmount > 999)
			{
				//
				// Maximum number is 999
				// 每项道具最大999个
				gpGlobals->rgInventory[index].nAmount = 999;
			}
		}
		else
		{
			gpGlobals->rgInventory[index].wItem = wObjectID;
			if (iNum > 999)
			{
				iNum = 999;
			}
			gpGlobals->rgInventory[index].nAmount = iNum;
		}

		return TRUE;
	}
	else
	{
		//
		// Remove item
		//
		if (fFound)
		{
			iNum *= -1;
			if (gpGlobals->rgInventory[index].nAmount < iNum)
			{
				//
				// This item has been run out
				//
				gpGlobals->rgInventory[index].nAmount = 0;
				return FALSE;
			}

			gpGlobals->rgInventory[index].nAmount -= iNum;
			//
			/// Need process last item
			//
			if (gpGlobals->rgInventory[index].nAmount == 0 && index == gpGlobals->iCurInvMenuItem && index + 1 < MAX_INVENTORY && gpGlobals->rgInventory[index + 1].nAmount <= 0)
				gpGlobals->iCurInvMenuItem--;
			return TRUE;
		}

		return FALSE;
	}
}

INT
PAL_GetItemAmount(
	WORD        wItem
)
/*++
  Purpose:

	Get the amount of the specified item in the inventory.

  Parameters:

	[IN]  wItem - the object ID of the item.

  Return value:

	The amount of the item in the inventory.

--*/
{
	int i;

	for (i = 0; i < MAX_INVENTORY; i++)
	{
		if (gpGlobals->rgInventory[i].wItem == 0)
		{
			break;
		}

		if (gpGlobals->rgInventory[i].wItem == wItem)
		{
			return gpGlobals->rgInventory[i].nAmount;
		}
	}

	return 0;
}

VOID
PAL_CompressInventory(
	VOID
)
/*++
  Purpose:

	Remove all the items in inventory which has a number of zero.
	删除库存中数量为零的所有项目。

  Parameters:

	None.

  Return value:

	None.

--*/
{
	int i, j;

	j = 0;

	for (i = 0; i < MAX_INVENTORY; i++)
	{
		//removed detect zero then break code, due to incompatible with save file hacked by palmod
		// 由于与仙剑的MOD HACK的保存文件不兼容，删除了检测零然后中断代码

		if (gpGlobals->rgInventory[i].nAmount > 0)
		{
			gpGlobals->rgInventory[j] = gpGlobals->rgInventory[i];
			j++;
		}
	}

	for (; j < MAX_INVENTORY; j++)
	{
		gpGlobals->rgInventory[j].nAmount = 0;
		gpGlobals->rgInventory[j].nAmountInUse = 0;
		gpGlobals->rgInventory[j].wItem = 0;
	}
}

BOOL
PAL_IncreaseHPMPSP(
	WORD          wPlayerRole,
	INT           sHP,
	INT           sMP,
	INT           sSP
)
/*++
  Purpose:

	Increase or decrease player's HP and/or MP.
	增加或减少队员的生命值和/或MP。

  Parameters:

	[IN]  wPlayerRole - the number of player role.

	[IN]  sHP - number of HP to be increased (positive value) or decrased
				(negative value).

	[IN]  sMP - number of MP to be increased (positive value) or decrased
				(negative value).

	[IN]  sSP - number of SP to be increased (positive value) or decrased
				(negative value).

  Return value:

	TRUE if the operation is succeeded, FALSE if not.

--*/
{
	BOOL           fSuccess = FALSE;
	//WORD           wOrigHP = gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole];
	//WORD           wOrigMP = gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole];
	INT           wOrigHP = gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole];
	INT           wOrigMP = gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole];

	// 备份精力
	INT           wOrigSP = gpGlobals->g.PlayerRoles.rgwSP[wPlayerRole];

	//
	// Only care about alive players
	// 仅作用于未阵亡队员
	if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] > 0)
	{
		//
		// change HP
		//
		gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] += sHP;

		//if ((SHORT)(gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole]) < 0)
		if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] < 0)
		{
			gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] = 0;
		}
		else if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] >
			gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole])
		{
			gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] =
				gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole];
		}

		//
		// Change MP
		//
		gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] += sMP;

		//if ((SHORT)(gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole]) < 0)
		if (gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] < 0)
		{
			gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] = 0;
		}
		else if (gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] >
			gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole])
		{
			gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] =
				gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole];
		}

		//
		// 改变精力
		//
		gpGlobals->g.PlayerRoles.rgwSP[wPlayerRole] += sSP;

		//if ((SHORT)(gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole]) < 0)
		if (gpGlobals->g.PlayerRoles.rgwSP[wPlayerRole] < 0)
		{
			gpGlobals->g.PlayerRoles.rgwSP[wPlayerRole] = 0;
		}
		else if (gpGlobals->g.PlayerRoles.rgwSP[wPlayerRole] >
			gpGlobals->g.PlayerRoles.rgwMaxSP[wPlayerRole])
		{
			gpGlobals->g.PlayerRoles.rgwSP[wPlayerRole] =
				gpGlobals->g.PlayerRoles.rgwMaxSP[wPlayerRole];
		}

		//
		// Avoid over treatment
		// 避免过度治疗
		if (wOrigHP != gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] ||
			wOrigMP != gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] ||
			wOrigSP != gpGlobals->g.PlayerRoles.rgwSP[wPlayerRole])
			fSuccess = TRUE;
	}

	return fSuccess;
}

VOID
PAL_UpdateEquipments(
	VOID
)
/*++
  Purpose:

	Update the effects of all equipped items for all players.
	更新所有玩家装备物品的效果。

  Parameters:

	None.

  Return value:

	None.

--*/
{
	int      i, j;
	WORD     w;

	memset(&(gpGlobals->rgEquipmentEffect), 0, sizeof(gpGlobals->rgEquipmentEffect));

	for (i = 0; i < MAX_PLAYER_ROLES; i++)
	{
		for (j = 0; j < MAX_PLAYER_EQUIPMENTS; j++)
		{
			w = gpGlobals->g.PlayerRoles.rgwEquipment[j][i];

			if (w != 0)
			{
				gpGlobals->g.rgObject[w].item.wScriptOnEquip =
					PAL_RunTriggerScript(gpGlobals->g.rgObject[w].item.wScriptOnEquip, (WORD)i, FALSE);
			}
		}
	}
}

VOID
PAL_RemoveEquipmentEffect(
	WORD         wPlayerRole,
	WORD         wEquipPart
)
/*++
  Purpose:

	Remove all the effects of the equipment for the player.
	移除（参数二）指定的装备对（参数一）指定的队员的所有属性影响。

  Parameters:

	[IN]  wPlayerRole - the player role.
	队员ID

	[IN]  wEquipPart - the part of the equipment.
	道具ID

  Return value:

	None.

--*/
{
	//WORD* p;
	INT* p;
	int         i, j;

	//p = (WORD*)(&gpGlobals->rgEquipmentEffect[wEquipPart]); // HACKHACK
	p = (INT*)(&gpGlobals->rgEquipmentEffect[wEquipPart]); // HACKHACK

	for (i = 0; i < sizeof(PLAYERROLES) / sizeof(PLAYERS); i++)
	{
		p[i * MAX_PLAYER_ROLES + wPlayerRole] = 0;
	}

	//
	// Reset some parameters to default when appropriate
	// 适当时将某些参数重置为默认值
	if (wEquipPart == kBodyPartHand)
	{
		//
		// reset the dual attack status
		// 重置双重攻击状态
		gpGlobals->rgPlayerStatus[wPlayerRole][kStatusDualAttack] = 0;
	}
	else if (wEquipPart == kBodyPartWear)
	{
		//
		// Remove all poisons leveled 99
		// 清除99级所有毒药
		for (i = 0; i <= (short)gpGlobals->wMaxPartyMemberIndex; i++)
		{
			if (gpGlobals->rgParty[i].wPlayerRole == wPlayerRole)
			{
				wPlayerRole = i;
				break;
			}
		}

		if (i <= (short)gpGlobals->wMaxPartyMemberIndex)
		{
			j = 0;

			for (i = 0; i < MAX_POISONS; i++)
			{
				WORD w = gpGlobals->rgPoisonStatus[i][wPlayerRole].wPoisonID;

				if (w == 0)
				{
					break;
				}

				if (gpGlobals->g.rgObject[w].poison.wPoisonLevel < 99)
				{
					gpGlobals->rgPoisonStatus[j][wPlayerRole] =
						gpGlobals->rgPoisonStatus[i][wPlayerRole];
					j++;
				}
			}

			while (j < MAX_POISONS)
			{
				gpGlobals->rgPoisonStatus[j][wPlayerRole].wPoisonID = 0;
				gpGlobals->rgPoisonStatus[j][wPlayerRole].wPoisonScript = 0;
				j++;
			}
		}
	}
}

VOID
PAL_AddPoisonForPlayer(
	WORD           wPlayerRole,
	WORD           wPoisonID
)
/*++
  Purpose:

	Add the specified poison to the player.

  Parameters:

	[IN]  wPlayerRole - the player role ID.

	[IN]  wPoisonID - the poison to be added.

  Return value:

	None.

--*/
{
	int         i, index;
	WORD        w;

	for (index = 0; index <= gpGlobals->wMaxPartyMemberIndex; index++)
	{
		if (gpGlobals->rgParty[index].wPlayerRole == wPlayerRole)
		{
			break;
		}
	}

	if (index > gpGlobals->wMaxPartyMemberIndex)
	{
		return; // don't go further
	}

	for (i = 0; i < MAX_POISONS; i++)
	{
		w = gpGlobals->rgPoisonStatus[i][index].wPoisonID;

		if (w == 0)
		{
			break;
		}

		if (w == wPoisonID)
		{
			return; // already poisoned
		}
	}

	if (i < MAX_POISONS)
	{
		gpGlobals->rgPoisonStatus[i][index].wPoisonID = wPoisonID;
		gpGlobals->rgPoisonStatus[i][index].wPoisonScript =
			PAL_RunTriggerScript(gpGlobals->g.rgObject[wPoisonID].poison.wPlayerScript, wPlayerRole, FALSE);
	}
}

VOID
PAL_CurePoisonByKind(
	WORD           wPlayerRole,
	WORD           wPoisonID
)
/*++
  Purpose:

	Remove the specified poison from the player.

  Parameters:

	[IN]  wPlayerRole - the player role ID.

	[IN]  wPoisonID - the poison to be removed.

  Return value:

	None.

--*/
{
	int i, index;

	for (index = 0; index <= gpGlobals->wMaxPartyMemberIndex; index++)
	{
		if (gpGlobals->rgParty[index].wPlayerRole == wPlayerRole)
		{
			break;
		}
	}

	if (index > gpGlobals->wMaxPartyMemberIndex)
	{
		return; // don't go further
	}

	for (i = 0; i < MAX_POISONS; i++)
	{
		if (gpGlobals->rgPoisonStatus[i][index].wPoisonID == wPoisonID)
		{
			gpGlobals->rgPoisonStatus[i][index].wPoisonID = 0;
			gpGlobals->rgPoisonStatus[i][index].wPoisonScript = 0;
		}
	}
}

VOID
PAL_CurePoisonByLevel(
	WORD           wPlayerRole,
	WORD           wMaxLevel
)
/*++
  Purpose:

	Remove the poisons which have a maximum level of wMaxLevel from the player.

  Parameters:

	[IN]  wPlayerRole - the player role ID.

	[IN]  wMaxLevel - the maximum level of poisons to be removed.

  Return value:

	None.

--*/
{
	int        i, index;
	WORD       w;

	for (index = 0; index <= gpGlobals->wMaxPartyMemberIndex; index++)
	{
		if (gpGlobals->rgParty[index].wPlayerRole == wPlayerRole)
		{
			break;
		}
	}

	if (index > gpGlobals->wMaxPartyMemberIndex)
	{
		return; // don't go further
	}

	for (i = 0; i < MAX_POISONS; i++)
	{
		w = gpGlobals->rgPoisonStatus[i][index].wPoisonID;

		if (gpGlobals->g.rgObject[w].poison.wPoisonLevel <= wMaxLevel)
		{
			gpGlobals->rgPoisonStatus[i][index].wPoisonID = 0;
			gpGlobals->rgPoisonStatus[i][index].wPoisonScript = 0;
		}
	}
}

BOOL
PAL_IsPlayerPoisonedByLevel(
	WORD           wPlayerRole,
	WORD           wMinLevel
)
/*++
  Purpose:

	Check if the player is poisoned by poisons at a minimum level of wMinLevel.

  Parameters:

	[IN]  wPlayerRole - the player role ID.

	[IN]  wMinLevel - the minimum level of poison.

  Return value:

	TRUE if the player is poisoned by poisons at a minimum level of wMinLevel;
	FALSE if not.

--*/
{
	int         i, index;
	WORD        w;

	for (index = 0; index <= gpGlobals->wMaxPartyMemberIndex; index++)
	{
		if (gpGlobals->rgParty[index].wPlayerRole == wPlayerRole)
		{
			break;
		}
	}

	if (index > gpGlobals->wMaxPartyMemberIndex)
	{
		return FALSE; // don't go further
	}

	for (i = 0; i < MAX_POISONS; i++)
	{
		w = gpGlobals->rgPoisonStatus[i][index].wPoisonID;
		w = gpGlobals->g.rgObject[w].poison.wPoisonLevel;

		if (w >= 99)
		{
			//
			// Ignore poisons which has a level of 99 (usually effect of equipment)
			// 忽略99级的毒药（通常是装备的效果）
			continue;
		}

		if (w >= wMinLevel)
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOL
PAL_IsPlayerPoisonedByKind(
	WORD           wPlayerRole,
	WORD           wPoisonID
)
/*++
  Purpose:

	Check if the player is poisoned by the specified poison.

  Parameters:

	[IN]  wPlayerRole - the player role ID.

	[IN]  wPoisonID - the poison to be checked.

  Return value:

	TRUE if player is poisoned by the specified poison;
	FALSE if not.

--*/
{
	int i, index;

	for (index = 0; index <= gpGlobals->wMaxPartyMemberIndex; index++)
	{
		if (gpGlobals->rgParty[index].wPlayerRole == wPlayerRole)
		{
			break;
		}
	}

	if (index > gpGlobals->wMaxPartyMemberIndex)
	{
		return FALSE; // don't go further
	}

	for (i = 0; i < MAX_POISONS; i++)
	{
		if (gpGlobals->rgPoisonStatus[i][index].wPoisonID == wPoisonID)
		{
			return TRUE;
		}
	}

	return FALSE;
}

INT
PAL_GetPlayerAttackStrength(
	WORD           wPlayerRole
)
/*++
  Purpose:

	Get the player's attack strength, count in the effect of equipments.

  Parameters:

	[IN]  wPlayerRole - the player role ID.

  Return value:

	The total attack strength of the player.

--*/
{
	INT        w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwAttackStrength[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwAttackStrength[wPlayerRole];
	}

	return w;
}

INT
PAL_GetPlayerMagicStrength(
	WORD           wPlayerRole
)
/*++
  Purpose:

	Get the player's magic strength, count in the effect of equipments.

  Parameters:

	[IN]  wPlayerRole - the player role ID.

  Return value:

	The total magic strength of the player.

--*/
{
	INT        w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwMagicStrength[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwMagicStrength[wPlayerRole];
	}

	return w;
}

INT
PAL_GetPlayerDefense(
	WORD           wPlayerRole
)
/*++
  Purpose:

	Get the player's defense value, count in the effect of equipments.

  Parameters:

	[IN]  wPlayerRole - the player role ID.

  Return value:

	The total defense value of the player.

--*/
{
	INT        w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwDefense[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwDefense[wPlayerRole];
	}

	return w;
}

INT
PAL_GetPlayerDexterity(
	WORD           wPlayerRole
)
/*++
  Purpose:

	Get the player's dexterity, count in the effect of equipments.

  Parameters:

	[IN]  wPlayerRole - the player role ID.

  Return value:

	The total dexterity of the player.

--*/
{
	INT        w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwDexterity[wPlayerRole];

#ifdef PAL_CLASSIC
	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
#else
	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS - 1; i++)
#endif
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwDexterity[wPlayerRole];
	}

	return w;
}

INT
PAL_GetPlayerFleeRate(
	WORD           wPlayerRole
)
/*++
  Purpose:

	Get the player's flee rate, count in the effect of equipments.
	获取玩家的逃跑率，计入装备效果。

  Parameters:

	[IN]  wPlayerRole - the player role ID.

  Return value:

	The total flee rate of the player.
	玩家的总逃跑率。

--*/
{
	INT        w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwFleeRate[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwFleeRate[wPlayerRole];
	}

	return w;
}

INT
PAL_GetPlayerPoisonResistance(
	WORD           wPlayerRole
)
/*++
  Purpose:

	Get the player's resistance to poisons, count in the effect of equipments.

  Parameters:

	[IN]  wPlayerRole - the player role ID.

  Return value:

	The total resistance to poisons of the player.

--*/
{
	INT        w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwPoisonResistance[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwPoisonResistance[wPlayerRole];
	}

	if (w > 100)
	{
		w = 100;
	}

	return w;
}

INT
PAL_GetPlayerElementalResistance(
	WORD           wPlayerRole,
	INT            iAttrib
)
/*++
  Purpose:

	Get the player's resistance to attributed magics, count in the effect
	of equipments.

  Parameters:

	[IN]  wPlayerRole - the player role ID.

	[IN]  iAttrib - the attribute of magics.

  Return value:

	The total resistance to the attributed magics of the player.

--*/
{
	INT        w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwElementalResistance[iAttrib][wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwElementalResistance[iAttrib][wPlayerRole];
	}

	if (w > 100)
	{
		w = 100;
	}

	return w;
}

WORD
PAL_GetPlayerBattleSprite(
	WORD             wPlayerRole
)
/*++
  Purpose:

	Get player's battle sprite.

  Parameters:

	[IN]  wPlayerRole - the player role ID.

  Return value:

	Number of the player's battle sprite.

--*/
{
	int       i;
	WORD      w;

	w = gpGlobals->g.PlayerRoles.rgwSpriteNumInBattle[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		if (gpGlobals->rgEquipmentEffect[i].rgwSpriteNumInBattle[wPlayerRole] != 0)
		{
			w = gpGlobals->rgEquipmentEffect[i].rgwSpriteNumInBattle[wPlayerRole];
		}
	}

	return w;
}

WORD
PAL_GetPlayerCooperativeMagic(
	WORD             wPlayerRole
)
/*++
  Purpose:

	Get player's cooperative magic.

  Parameters:

	[IN]  wPlayerRole - the player role ID.

  Return value:

	Object ID of the player's cooperative magic.

--*/
{
	int       i;
	WORD      w;

	w = gpGlobals->g.PlayerRoles.rgwCooperativeMagic[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		if (gpGlobals->rgEquipmentEffect[i].rgwCooperativeMagic[wPlayerRole] != 0)
		{
			w = gpGlobals->rgEquipmentEffect[i].rgwCooperativeMagic[wPlayerRole];
		}
	}

	return w;
}

BOOL
PAL_PlayerCanAttackAll(
	WORD        wPlayerRole
)
/*++
  Purpose:

	Check if the player can attack all of the enemies in one move.

  Parameters:

	[IN]  wPlayerRole - the player role ID.

  Return value:

	TRUE if player can attack all of the enemies in one move, FALSE if not.

--*/
{
	int       i;
	BOOL      f;

	f = FALSE;

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		if (gpGlobals->rgEquipmentEffect[i].rgwAttackAll[wPlayerRole] != 0)
		{
			f = TRUE;
			break;
		}
	}

	return f;
}

BOOL
PAL_AddMagic(
	WORD           wPlayerRole,
	WORD           wMagic
)
/*++
  Purpose:

	Add a magic to the player.
	队员领悟仙术

  Parameters:

	[IN]  wPlayerRole - the player role ID.
	队员编号

	[IN]  wMagic - the object ID of the magic.
	仙术的对象编号

  Return value:

	TRUE if succeeded, FALSE if failed.
	成功则返回TRUE，否则返回FALSE

--*/
{
	int            i;

	for (i = 0; i < MAX_PLAYER_MAGICS; i++)
	{
		if (gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole] == wMagic)
		{
			//
			// already have this magic
			// 我方已经领悟了这种仙术，无需再次领悟
			return FALSE;
		}
	}

	for (i = 0; i < MAX_PLAYER_MAGICS; i++)
	{
		if (gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole] == 0)
		{
			break;
		}
	}

	if (i >= MAX_PLAYER_MAGICS)
	{
		//
		// Not enough slots
		// 仙术空位不足，无法领悟更多仙术
		return FALSE;
	}

	// 以上条件均通过，允许进行领悟
	gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole] = wMagic;
	return TRUE;
}

VOID
PAL_RemoveMagic(
	WORD           wPlayerRole,
	WORD           wMagic
)
/*++
  Purpose:

	Remove a magic to the player.

  Parameters:

	[IN]  wPlayerRole - the player role ID.

	[IN]  wMagic - the object ID of the magic.

  Return value:

	None.

--*/
{
	int            i;

	for (i = 0; i < MAX_PLAYER_MAGICS; i++)
	{
		if (gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole] == wMagic)
		{
			gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole] = 0;
			break;
		}
	}
}

VOID
PAL_SetPlayerStatus(
	WORD         wPlayerRole,
	WORD         wStatusID,
	WORD         wNumRound
)
/*++
  Purpose:

	Set one of the statuses for the player.

  Parameters:

	[IN]  wPlayerRole - the player ID.

	[IN]  wStatusID - the status to be set.

	[IN]  wNumRound - the effective rounds of the status.

  Return value:

	None.

--*/
{
#ifndef PAL_CLASSIC
	if (wStatusID == kStatusSlow &&
		gpGlobals->rgPlayerStatus[wPlayerRole][kStatusHaste] > 0)
	{
		//
		// Remove the haste status
		//
		PAL_RemovePlayerStatus(wPlayerRole, kStatusHaste);
		return;
	}

	if (wStatusID == kStatusHaste &&
		gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSlow] > 0)
	{
		//
		// Remove the slow status
		//
		PAL_RemovePlayerStatus(wPlayerRole, kStatusSlow);
		return;
	}
#endif

	switch (wStatusID)
	{
	case kStatusConfused:
	case kStatusSleep:
	case kStatusSilence:
#ifdef PAL_CLASSIC
	case kStatusParalyzed:
#else
	case kStatusSlow:
#endif
		//
		// for "bad" statuses, don't set the status when we already have it
		//
		if (gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] == 0)
		{
			gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] = wNumRound;
		}
		break;

	case kStatusPuppet:
		//
		// only allow dead players for "puppet" status
		//
		if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0 &&
			gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] < wNumRound)
		{
			gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] = wNumRound;
		}
		break;

	case kStatusBravery:
	case kStatusProtect:
	case kStatusDualAttack:
	case kStatusHaste:
		//
		// for "good" statuses, reset the status if the status to be set lasts longer
		//
		if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] != 0 &&
			gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] < wNumRound)
		{
			gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] = wNumRound;
		}
		break;

	default:
		assert(FALSE);
		break;
	}
}

VOID
PAL_RemovePlayerStatus(
	WORD         wPlayerRole,
	WORD         wStatusID
)
/*++
  Purpose:

	Remove one of the status for player.
	清除所有玩家状态（不会移除装备效果）

  Parameters:

	[IN]  wPlayerRole - the player ID.

	[IN]  wStatusID - the status to be set.

  Return value:

	None.

--*/
{
	//
	// Don't remove effects of equipments
	// 不要移除设备的效果
	if (gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] <= 999)
	{
		gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] = 0;
	}
}

VOID
PAL_ClearAllPlayerStatus(
	VOID
)
/*++
  Purpose:

	Clear all player status.
	清除所有玩家状态（不会移除装备效果）

  Parameters:

	None.

  Return value:

	None.

--*/
{
	int      i, j;

	for (i = 0; i < MAX_PLAYER_ROLES; i++)
	{
		for (j = 0; j < kStatusAll; j++)
		{
			//
			// Don't remove effects of equipments
			// 不要移除设备的效果
			if (gpGlobals->rgPlayerStatus[i][j] <= 999)
			{
				gpGlobals->rgPlayerStatus[i][j] = 0;
			}
		}
	}
}

VOID
PAL_PlayerLevelUp(
	WORD          wPlayerRole,
	WORD          wNumLevel
)
/*++
  Purpose:

	Increase the player's level by wLevels.
	提升队员的修行

  Parameters:

	[IN]  wPlayerRole - player role ID.

	[IN]  wNumLevel - number of levels to be increased.

  Return value:

	None.

--*/
{
	WORD          i;

	if (wNumLevel != 0)
	{
		//
		// Add the level
		// 提升修行
		gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole] += wNumLevel;
		if (gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole] > MAX_LEVELS)
		{
			gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole] = MAX_LEVELS;
		}

		for (i = 0; i < wNumLevel; i++)
		{
			//
			// Increase player's stats
			// 增加玩家的统计数据
			gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole] += 10 + RandomLong(0, 8);
			gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole] += 8 + RandomLong(0, 6);
			gpGlobals->g.PlayerRoles.rgwAttackStrength[wPlayerRole] += 4 + RandomLong(0, 1);
			gpGlobals->g.PlayerRoles.rgwMagicStrength[wPlayerRole] += 4 + RandomLong(0, 1);
			gpGlobals->g.PlayerRoles.rgwDefense[wPlayerRole] += 2 + RandomLong(0, 1);
			gpGlobals->g.PlayerRoles.rgwDexterity[wPlayerRole] += 2 + RandomLong(0, 1);
			gpGlobals->g.PlayerRoles.rgwFleeRate[wPlayerRole] += 2;

			// 增加精力
			gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole] += 6 + RandomLong(0, 6);
		}

		//
		// Reset experience points to zero
		// 将经验值重置为零
		gpGlobals->Exp.rgPrimaryExp[wPlayerRole].wExp = 0;
		gpGlobals->Exp.rgPrimaryExp[wPlayerRole].wLevel =
			gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole];
	}

	// 修行提升时最大体力、真气、精力值限制9999
#define STAT_LIMIT_HMSP(t) { if ((t) > 9999) (t) = 9999; }
	STAT_LIMIT_HMSP(gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole]);
	STAT_LIMIT_HMSP(gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole]);
	STAT_LIMIT_HMSP(gpGlobals->g.PlayerRoles.rgwMaxSP[wPlayerRole]);
#undef STAT_LIMIT_HMSP

	// 修行提升时最大属性值限制90000
#define STAT_LIMIT(t) { if ((t) > 90000) (t) = 90000; }
	STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwAttackStrength[wPlayerRole]);
	STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwMagicStrength[wPlayerRole]);
	STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwDefense[wPlayerRole]);
	STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwDexterity[wPlayerRole]);
	STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwFleeRate[wPlayerRole]);
#undef STAT_LIMIT
}

DWORD
PAL_GetLevelUpBaseExp(
	DWORD		wLevel
)
{
	DWORD wExp = 0;

	if (wLevel <= 899)
	{
		wExp = 55 * wLevel * (wLevel - 1) / 2 + 15;
	}
	else
	{
		wExp = 25000000;
	}
	return wExp;
}

INT
PAL_GetPlayerMaxHP(
	WORD           wPlayerRole
)
/*++
Purpose:

Get the player's maxhp, count in the effect of equipments.

Parameters:

[IN]  wPlayerRole - the player role ID.

Return value:

The total attack strength of the player.

--*/
{
	INT       w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwMaxHP[wPlayerRole];
	}

	if (w < 0)
	{
		w = 0;
	}

	return w;
}

INT
PAL_GetPlayerMaxMP(
	WORD           wPlayerRole
)
/*++
Purpose:

Get the player's maxmp, count in the effect of equipments.

Parameters:

[IN]  wPlayerRole - the player role ID.

Return value:

The total attack strength of the player.

--*/
{
	INT       w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwMaxMP[wPlayerRole];
	}
	if (w < 0)
	{
		w = 0;
	}
	return w;
}

INT
PAL_GetPlayerMaxSP(
	WORD           wPlayerRole
)
/*++
Purpose:

Get the player's maxmp, count in the effect of equipments.

Parameters:

[IN]  wPlayerRole - the player role ID.

Return value:

The total attack strength of the player.

--*/
{
	INT       w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwMaxSP[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwMaxSP[wPlayerRole];
	}
	if (w < 0)
	{
		w = 0;
	}
	return w;
}

INT
PAL_New_GetPlayerLevel(
	WORD           wPlayerRole
)
/*++
  Purpose:

	Get the player's defense value, count in the effect of equipments.

  Parameters:

	[IN]  wPlayerRole - the player role ID.

  Return value:

	The total defense value of the player.

--*/
{
	INT        w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwLevel[wPlayerRole];
	}

	return w;
}

INT
PAL_New_GetPlayerHealth(
	WORD           wPlayerRole
)
/*++
  Purpose:

	Get the player's defense value, count in the effect of equipments.

  Parameters:

	[IN]  wPlayerRole - the player role ID.

  Return value:

	The total defense value of the player.

--*/
{
	INT        w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwHP[wPlayerRole];
	}

	return w;
}

WORD
PAL_New_GetPlayerSorceryResistance(
	WORD			wPlayerRole
)
/*++
	Purpose:Get the player's resistance to Sorcery, count in the effect of equipments.
	Parameters:[IN]  wPlayerRole - the player role ID.
	Return value:The total resistance to Sorcery of the player.
	--*/
{
	WORD       w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwSorceryResistance[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwSorceryResistance[wPlayerRole];
	}

	return min(100, w);
}

WORD
PAL_New_GetPlayerUniqueSkillResistance(
	WORD			wPlayerRole
)
/*++
Purpose:Get the player's resistance to Sorcery, count in the effect of equipments.
Parameters:[IN]  wPlayerRole - the player role ID.
Return value:The total resistance to Sorcery of the player.
--*/
{
	WORD       w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwUniqueSkillResistance[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwUniqueSkillResistance[wPlayerRole];
	}

	return min(100, w);
}

BOOL
PAL_FindEnemyBooty(
	WORD           wScriptEntry,
	WORD           wEventObjectID,
	WORD           wEnemyIndex,
	PAL_POS        pNumPos,
	PAL_POS        pTextPos,
	BOOL           bbJumpScript
)
/*++
  Purpose:

	找出敌人的战利品（其实就是战后脚本得到什么物品）

  Parameters:

	[IN]  wScriptEntry - The script entry to execute.

	[IN]  wEventObjectID - The event object ID which invoked the script.

  Return value:

	物品及其数量

--*/
{
	static WORD       wLastEventObject = 0;

	WORD              wNextScriptEntry;
	BOOL              fEnded;
	LPSCRIPTENTRY     pScript;
	LPEVENTOBJECT     pEvtObj = NULL;

	extern BOOL       g_fUpdatedInBattle; // HACKHACK

	// 04跳转并返回指令执行结果
	BOOL              bJumpResults = FALSE;

	wNextScriptEntry = wScriptEntry;
	fEnded = FALSE;
	g_fUpdatedInBattle = FALSE;

	if (wEventObjectID == 0xFFFF)
	{
		wEventObjectID = wLastEventObject;
	}

	wLastEventObject = wEventObjectID;

	if (wEventObjectID != 0)
	{
		pEvtObj = &(gpGlobals->g.lprgEventObject[wEventObjectID - 1]);
	}

	g_fScriptSuccess = TRUE;

	//
	// Set the default dialog speed.
	// 设置默认对话框速度
	PAL_DialogSetDelayTime(3);

	WORD wEnemyBooty = 0;
	WORD wEnemyBootyNum = 0;

	while (wScriptEntry != 0 && !fEnded)
	{
		pScript = &(gpGlobals->g.lprgScriptEntry[wScriptEntry]);

		switch (pScript->wOperation)
		{
		case 0x0000:
			//
			// Stop running
			//
			fEnded = TRUE;
			break;

		case 0x0001:
			//
			// Stop running and replace the entry with the next line
			//
			fEnded = TRUE;
			wNextScriptEntry = wScriptEntry + 1;
			break;

		case 0x0002:
			//
			// Stop running and replace the entry with the specified one
			//
			if (pScript->rgwOperand[1] == 0 ||
				++(pEvtObj->nScriptIdleFrame) < pScript->rgwOperand[1])
			{
				fEnded = TRUE;
				wNextScriptEntry = pScript->rgwOperand[0];
			}
			else
			{
				//
				// failed
				//
				pEvtObj->nScriptIdleFrame = 0;
				wScriptEntry++;
			}
			break;

		case 0x0003:
			//
			// unconditional jump
			//
			if (pScript->rgwOperand[1] == 0 ||
				++(pEvtObj->nScriptIdleFrame) < pScript->rgwOperand[1])
			{
				wScriptEntry = pScript->rgwOperand[0];
			}
			else
			{
				//
				// failed
				//
				pEvtObj->nScriptIdleFrame = 0;
				wScriptEntry++;
			}
			break;

		case 0x0004:
			//
			// Call script
			//
			bJumpResults = PAL_FindEnemyBooty(pScript->rgwOperand[0], ((pScript->rgwOperand[1] == 0) ? wEventObjectID :
				pScript->rgwOperand[1]), wEnemyIndex, pNumPos, pTextPos, TRUE);
			if (bJumpResults)
				return;
			wScriptEntry++;
			break;

		case 0x001F:
			fEnded = TRUE;
			wEnemyBooty = pScript->rgwOperand[0];
			wEnemyBootyNum = (SHORT)(pScript->rgwOperand[1]);
			break;

		default:
			wScriptEntry++;
			break;
		}
	}

	// 若跳转脚本中没有找到战利品则执行
	if (!bJumpResults)
		if (wEnemyBooty == 0 && !bbJumpScript)
		{
			// 若本函数为非跳转指令的递归调用则执行
			PAL_DrawText(L"无", pTextPos, MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
		}
		else
		{
			PAL_DrawNumber((wEnemyBootyNum == 0) ? 1 : wEnemyBootyNum, 3, pNumPos, kNumColorPink, kNumAlignRight);
			PAL_DrawText(PAL_GetWord(wEnemyBooty), pTextPos, MENUITEM_COLOR_CONFIRMED,
				TRUE, FALSE, FALSE);

			return TRUE;
		}

	return FALSE;
}

INT
PAL_New_GetPlayerIndex(
	WORD		wPlayerRole
)
{
	int		i;

	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		if (wPlayerRole == gpGlobals->rgParty[i].wPlayerRole)
		{
			break;
		}
	}

	if (i > gpGlobals->wMaxPartyMemberIndex)
	{
		return -1;//没有找到
	}
	else
	{
		return i;
	}
}

INT
PAL_New_GetPlayerIndexByParty(
	WORD		wPlayerRole
)
{
	if (wPlayerRole > gpGlobals->wMaxPartyMemberIndex)
	{
		//
		// 超出队伍成员数，查找失败
		//
		return -1;
	}

	return gpGlobals->rgParty[wPlayerRole].wPlayerRole;
}

BOOL
PAL_New_GetTrueByPercentage(
	WORD		wPercentage
)
/*++
Purpose:根据百分比返回真值

Parameters:	[IN]  wPercentage - 百分数

Return value:有（输入%）的可能返回真值，其余返回假
--*/
{
	if (RandomLong(0, 99) < wPercentage)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

INT
PAL_New_GetPlayerPhysicalResistance(
	WORD			wPlayerRole
)
{
	INT       w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwPhysicalResistance[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwPhysicalResistance[wPlayerRole];
	}

	w = max(w, 1);

	return min(100, w);
}

INT
PAL_New_GetPlayerUniqueSkill(
	WORD			wPlayerRole
)
{
	INT       w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwUniqueSkillResistance[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwUniqueSkillResistance[wPlayerRole];
	}

	return min(100, w);
}

VOID
PAL_New_IncreaseExp(
	INT			iExp
)
{
	INT i, w, j;
	WCHAR  s[256];
	DWORD  dwExp;
	BOOL   fLevelUp;
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		w = gpGlobals->rgParty[i].wPlayerRole;
		gpGlobals->Exp.rgPrimaryExp[w].wExp += iExp;
		PAL_StartDialog(kDialogCenterWindow, 0, 0, FALSE);
		PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"%ls%ls%d%ls", PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]), PAL_GetWord(34), iExp, PAL_GetWord(2));
		PAL_ShowDialogText(s);
	}

	// 同步体力真气，血蓝加满
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		fLevelUp = FALSE;
		w = gpGlobals->rgParty[i].wPlayerRole;
		dwExp = gpGlobals->Exp.rgPrimaryExp[w].wExp;
		while (dwExp >= PAL_GetLevelUpBaseExp(gpGlobals->g.PlayerRoles.rgwLevel[w]))
		{
			dwExp -= PAL_GetLevelUpBaseExp(gpGlobals->g.PlayerRoles.rgwLevel[w]);

			if (gpGlobals->g.PlayerRoles.rgwLevel[w] < MAX_LEVELS)
			{
				fLevelUp = TRUE;
				PAL_PlayerLevelUp(w, 1);

				gpGlobals->g.PlayerRoles.rgwHP[w] = PAL_GetPlayerMaxHP(w);
				gpGlobals->g.PlayerRoles.rgwMP[w] = PAL_GetPlayerMaxMP(w);

			}
		}
		gpGlobals->Exp.rgPrimaryExp[w].wExp = (DWORD)dwExp;
		if (fLevelUp)
		{
			// 播放修行晋音效
			AUDIO_PlaySound(127);

			PAL_StartDialog(kDialogCenterWindow, 0, 0, FALSE);
			PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"-%ls-%ls%ls", PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]), PAL_GetWord(STATUS_LABEL_LEVEL), PAL_GetWord(BATTLEWIN_LEVELUP_LABEL));
			PAL_ShowDialogText(s);
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
			j = RandomLong(0, 104);

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

				VIDEO_UpdateScreen(NULL);
				PAL_WaitForKey(3000);
			}

		}
#else
		// 若随机仙术开关关闭
		// Learn all magics at the current level
		// 学习当前级别的所有魔法
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			w = gpGlobals->rgParty[i].wPlayerRole;
			j = 0;
			{
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

						VIDEO_UpdateScreen(NULL);
						PAL_WaitForKey(3000);
					}

					j++;
				}
			}
		}
#endif
	}
}

WORD
PAL_New_GetMovingPlayerIndex(
	VOID
)
{
	return      gpGlobals->rgParty[g_Battle.wMovingPlayerIndex].wPlayerRole;
}

SHORT
PAL_New_GetBattleFieldComplete(
	SHORT			sElem
)
{
	SHORT       sValue;

	if (PAL_New_GetDiagramsEffect(sElem))
		sValue = 10;
	else
		sValue = gpGlobals->g.lprgBattleField[gpGlobals->wNumBattleField].rgsMagicEffect[sElem];

	return min(10, sValue);
}

SHORT
PAL_New_GetBattleField(
	SHORT			sElem
)
{
	SHORT       sValue;

	sValue = gpGlobals->g.lprgBattleField[gpGlobals->wNumBattleField].rgsMagicEffect[sElem];

	return min(10, sValue);
}

BOOL
PAL_New_GetDiagramsEffect(
	SHORT			sElem
)
{
	return gpGlobals->rgsDiagramsEffect[sElem];
}

VOID
PAL_RemoveDiagramsEffect(
	VOID
)
{
	// 关闭所有卦性转换开关
	for (WORD w = 0; w < NUM_MAGIC_ELEMENTAL; w++)
		gpGlobals->rgsDiagramsEffect[w] = FALSE;
}

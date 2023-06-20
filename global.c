/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2019, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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
GLOBALVARS * const  gpGlobals = &_gGlobals;
extern WORD GetSavedTimes(int iSaveSlot);

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
	BOOL *pfIsWIN95
)
{
	FILE *fps[] = { UTIL_OpenRequiredFile("abc.mkf"), UTIL_OpenRequiredFile("map.mkf"), gpGlobals->f.fpF, gpGlobals->f.fpFBP, gpGlobals->f.fpFIRE, gpGlobals->f.fpMGO };
	uint8_t *data = NULL;
	int data_size = 0, dos_score = 0, win_score = 0;
	BOOL result = FALSE;

	for (int i = 0; i < sizeof(fps) / sizeof(FILE *); i++)
	{
		//
		// Find the first non-empty sub-file
		//
		int count = PAL_MKFGetChunkCount(fps[i]), j = 0, size;
		while (j < count && (size = PAL_MKFGetChunkSize(j, fps[i])) < 4) j++;
		if (j >= count) goto PAL_IsWINVersion_Exit;

		//
		// Read the content and check the compression signature
		// Note that this check is not 100% correct, however in incorrect situations,
		// the sub-file will be over 784MB if uncompressed, which is highly unlikely.
		//
		if (data_size < size) data = (uint8_t *)realloc(data, data_size = size);
		PAL_MKFReadChunk(data, data_size, j, fps[i]);
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

	if (pfIsWIN95) *pfIsWIN95 = (win_score == sizeof(fps) / sizeof(FILE *)) ? TRUE : FALSE;

	result = TRUE;

PAL_IsWINVersion_Exit:
	free(data);
	fclose(fps[1]);
	fclose(fps[0]);

	return result;
}

CODEPAGE
PAL_DetectCodePage(
	const char *   filename
)
{
	FILE *fp;
	char *word_buf = NULL;
	long word_len = 0;
	CODEPAGE cp = CP_BIG5;

	if (NULL != (fp = UTIL_OpenFile(filename)))
	{
		fseek(fp, 0, SEEK_END);
		word_len = ftell(fp);
		word_buf = (char *)malloc(word_len);
		fseek(fp, 0, SEEK_SET);
		word_len = fread(word_buf, 1, word_len, fp);
		UTIL_CloseFile(fp);
		// Eliminates null characters so that PAL_MultiByteToWideCharCP works properly
		for (char *ptr = word_buf; ptr < word_buf + word_len; ptr++)
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

  Parameters:

    None.

  Return value:

    0 = success, -1 = error.

--*/
{
   //
   // Open files
   //
   gpGlobals->f.fpFBP = UTIL_OpenRequiredFile("fbp.mkf");
   gpGlobals->f.fpMGO = UTIL_OpenRequiredFile("mgo.mkf");
   gpGlobals->f.fpBALL = UTIL_OpenRequiredFile("ball.mkf");
   gpGlobals->f.fpDATA = UTIL_OpenRequiredFile("data.mkf");
   gpGlobals->f.fpF = UTIL_OpenRequiredFile("f.mkf");
   gpGlobals->f.fpFIRE = UTIL_OpenRequiredFile("fire.mkf");
   gpGlobals->f.fpRGM = UTIL_OpenRequiredFile("rgm.mkf");
   gpGlobals->f.fpSSS = UTIL_OpenRequiredFile("sss.mkf");

   //
   // Retrieve game resource version
   //
//   if (!PAL_IsWINVersion(&gConfig.fIsWIN95)) return -1;

   //
   // Enable AVI playing only when the resource is WIN95
   //
   gConfig.fEnableAviPlay = gConfig.fEnableAviPlay && gConfig.fIsWIN95;

   //
   // Detect game language only when no message file specified
   //
   if (!gConfig.pszMsgFile) PAL_SetCodePage(PAL_DetectCodePage("word.dat"));

   //
   // Set decompress function
   //
  // Decompress = gConfig.fIsWIN95 ? YJ2_Decompress : YJ1_Decompress;
   Decompress = YJ2_Decompress;

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

  Parameters:

    None.

  Return value:

    None.

--*/
{
   //
   // Close all opened files
   //
   UTIL_CloseFile(gpGlobals->f.fpFBP);
   UTIL_CloseFile(gpGlobals->f.fpMGO);
   UTIL_CloseFile(gpGlobals->f.fpBALL);
   UTIL_CloseFile(gpGlobals->f.fpDATA);
   UTIL_CloseFile(gpGlobals->f.fpF);
   UTIL_CloseFile(gpGlobals->f.fpFIRE);
   UTIL_CloseFile(gpGlobals->f.fpRGM);
   UTIL_CloseFile(gpGlobals->f.fpSSS);

   //
   // Free the game data
   //
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
   //
   if (!gConfig.fIsWIN95)
      PAL_FreeObjectDesc(gpGlobals->lpObjectDesc);

   //
   // Clear the instance
   //
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
   const GAMEDATA    *p = &gpGlobals->g;

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
   PAL_MKFReadChunk((LPBYTE)&(p->EnemyPos), sizeof(p->EnemyPos),
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

  Parameters:

    None.

  Return value:

    None.

--*/
{
   GAMEDATA    *p = &gpGlobals->g;
   UINT32       i;

   //
   // Load the default data from the game data files.
   //
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
      //
      for (i = 0; i < MAX_OBJECTS; i++)
      {
         memcpy(&p->rgObject[i], &objects[i], sizeof(OBJECT_DOS));
         p->rgObject[i].rgwData[6] = objects[i].rgwData[5];     // wFlags
         p->rgObject[i].rgwData[5] = 0;                         // wScriptDesc or wReserved2
      }
   }

   PAL_MKFReadChunk((LPBYTE)(&(p->PlayerRoles)), sizeof(PLAYERROLES),
      3, gpGlobals->f.fpDATA);
   DO_BYTESWAP(&(p->PlayerRoles), sizeof(PLAYERROLES));

   //
   // Set some other default data.
   //
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
      gpGlobals->Exp.rgAttackExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
      gpGlobals->Exp.rgMagicPowerExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
      gpGlobals->Exp.rgDefenseExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
      gpGlobals->Exp.rgDexterityExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
      gpGlobals->Exp.rgFleeExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
     gpGlobals->Exp.rgPowerExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
     gpGlobals->Exp.rgWisdom[i].wLevel = p->PlayerRoles.rgwLevel[i];
   }

   gpGlobals->fEnteringScene = TRUE;
}

typedef struct tagSAVEDGAME_COMMON
{
	WORD             wSavedTimes;             // saved times
	WORD             wViewportX, wViewportY;  // viewport location
	WORD             nPartyMember;            // number of members in party
	WORD             wNumScene;               // scene number
	WORD             wPaletteOffset;
	WORD             wPartyDirection;         // party direction
	WORD             wNumMusic;               // music number
	WORD             wNumBattleMusic;         // battle music number
	WORD             wNumBattleField;         // battle field number
	WORD             wScreenWave;             // level of screen waving
	WORD             wBattleSpeed;            // battle speed
	WORD             wCollectValue;           // value of "collected" items
	WORD             wLayer;
	WORD             wChaseRange;
	WORD             wChasespeedChangeCycles;
	WORD             nFollower;
	WORD             wNumPalette;         // current palette number
	WORD             rgwReserved2[2];         // unused
	DWORD            dwCash;                  // amount of cash
	PARTY            rgParty[MAX_PLAYABLE_PLAYER_ROLES];       // player party
	TRAIL            rgTrail[MAX_PLAYABLE_PLAYER_ROLES];       // player trail
	ALLEXPERIENCE    Exp;                     // experience data
	PLAYERROLES      PlayerRoles;
	POISONSTATUS     rgPoisonStatus[MAX_POISONS][MAX_PLAYABLE_PLAYER_ROLES]; // poison status
	INVENTORY        rgInventory[MAX_INVENTORY];               // inventory status
	SCENE            rgScene[MAX_SCENES];
} SAVEDGAME_COMMON, *LPSAVEDGAME_COMMON;

typedef struct tagSAVEDGAME_DOS
{
	WORD             wSavedTimes;             // saved times
	WORD             wViewportX, wViewportY;  // viewport location
	WORD             nPartyMember;            // number of members in party
	WORD             wNumScene;               // scene number
	WORD             wPaletteOffset;
	WORD             wPartyDirection;         // party direction
	WORD             wNumMusic;               // music number
	WORD             wNumBattleMusic;         // battle music number
	WORD             wNumBattleField;         // battle field number
	WORD             wScreenWave;             // level of screen waving
	WORD             wBattleSpeed;            // battle speed
	WORD             wCollectValue;           // value of "collected" items
	WORD             wLayer;
	WORD             wChaseRange;
	WORD             wChasespeedChangeCycles;
	WORD             nFollower;
	WORD             wNumPalette;         // current palette number
	WORD             rgwReserved2[2];         // unused
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
} SAVEDGAME_DOS, *LPSAVEDGAME_DOS;

typedef struct tagSAVEDGAME_WIN
{
	WORD             wSavedTimes;             // saved times
	WORD             wViewportX, wViewportY;  // viewport location
	WORD             nPartyMember;            // number of members in party
	WORD             wNumScene;               // scene number
	WORD             wPaletteOffset;
	WORD             wPartyDirection;         // party direction
	WORD             wNumMusic;               // music number
	WORD             wNumBattleMusic;         // battle music number
	WORD             wNumBattleField;         // battle field number
	WORD             wScreenWave;             // level of screen waving
	WORD             wBattleSpeed;            // battle speed
	WORD             wCollectValue;           // value of "collected" items
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
} SAVEDGAME_WIN, *LPSAVEDGAME_WIN;

static BOOL
PAL_LoadGame_Common(
	int                 iSaveSlot,
	LPSAVEDGAME_COMMON  s,
	size_t              size
)
{
	//
	// Try to open the specified file
	//
	FILE *fp = UTIL_OpenFileAtPath(gConfig.pszSavePath, PAL_va(1, "%d.rpg", iSaveSlot));
	//
	// Read all data from the file and close.
	//
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
	//
	DO_BYTESWAP(s, size);

	//
	// Cash amount is in DWORD, so do a wordswap in Big-Endian.
	//
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	s->dwCash = ((s->dwCash >> 16) | (s->dwCash << 16));
#endif

	//
	// Get common data from the saved game struct.
	//
	gpGlobals->viewport = PAL_XY(s->wViewportX, s->wViewportY);
	gpGlobals->wMaxPartyMemberIndex = s->nPartyMember;
	gpGlobals->wNumScene = s->wNumScene;
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
	gpGlobals->wNumPalette = s->wNumPalette;
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

  Parameters:

    [IN]  szFileName - file name of saved game.

  Return value:

    0 if success, -1 if failed.

--*/
{
   SAVEDGAME_DOS   *s = (SAVEDGAME_DOS*)malloc(sizeof(SAVEDGAME_DOS));
   int                       i;

   //
   // Get all the data from the saved game struct.
   //
   if (!PAL_LoadGame_Common(iSaveSlot, (LPSAVEDGAME_COMMON)s, sizeof(SAVEDGAME_DOS)))
	   return -1;

   //
   // Convert the DOS-style data structure to WIN-style data structure
   //
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

  Parameters:

    [IN]  szFileName - file name of saved game.

  Return value:

    0 if success, -1 if failed.

--*/
{
   SAVEDGAME_WIN   *s = (SAVEDGAME_WIN*)malloc(sizeof(SAVEDGAME_WIN));

   //
   // Get all the data from the saved game struct.
   //
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
	FILE *fp;
	int   i;

	s->wSavedTimes = wSavedTimes;
	s->wViewportX = PAL_X(gpGlobals->viewport);
	s->wViewportY = PAL_Y(gpGlobals->viewport);
	s->nPartyMember = gpGlobals->wMaxPartyMemberIndex;
	s->wNumScene = gpGlobals->wNumScene;
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
	s->wNumPalette = gpGlobals->wNumPalette;
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
	//
	DO_BYTESWAP(s, size);

	//
	// Cash amount is in DWORD, so do a wordswap in Big-Endian.
	//
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	s->dwCash = ((s->dwCash >> 16) | (s->dwCash << 16));
#endif

	//
	// Try writing to file
	//
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
   SAVEDGAME_DOS   *s = (SAVEDGAME_DOS*)malloc(sizeof(SAVEDGAME_DOS));
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
   SAVEDGAME_WIN   *s = (SAVEDGAME_WIN*)malloc(sizeof(SAVEDGAME_WIN));

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
	if (gConfig.fIsWIN95)
		PAL_SaveGame_WIN(iSaveSlot, wSavedTimes);
	else
		PAL_SaveGame_DOS(iSaveSlot, wSavedTimes);
}

VOID
PAL_InitGameData(
   INT         iSaveSlot
)
/*++
  Purpose:

    Initialize the game data (used when starting a new game or loading a saved game).

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
   //
   if (iSaveSlot == 0 || PAL_LoadGame(iSaveSlot) != 0)
   {
      //
      // Cannot load the saved game file. Load the defaults.
      //
      PAL_LoadDefaultGame();
   }

   gpGlobals->fGameStart = TRUE;
   gpGlobals->fNeedToFadeIn = FALSE;
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
 
 Parameters:
 
 [IN]  wObjectID - object number of the item.
 
 Return value:
 
 Counted value.
 
 --*/
{
    int          index;
    int          count;
    int          i,j,w;

    if (wObjectID == 0)
    {
        return FALSE;
    }
    
    index = 0;
    count = 0;
    
    //
    // Search for the specified item in the inventory
    //
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
      //

      if (index >= MAX_INVENTORY)
      {
         //
         // inventory is full. cannot add item
         //
         return FALSE;
      }

	  if (wObjectID <= 60 || wObjectID >= 295)
	  {
		  gpGlobals->rgInventory[index].wItem = wObjectID;
		  if (iNum >= 1)
		  {
			  iNum = 0;
		  }
	  }

      if (fFound)
      {
         gpGlobals->rgInventory[index].nAmount += iNum;
         if (gpGlobals->rgInventory[index].nAmount > 999)
         {
            //
            // Maximum number is 999
            //
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
         if(gpGlobals->rgInventory[index].nAmount == 0 && index == gpGlobals->iCurInvMenuItem && index+1 < MAX_INVENTORY && gpGlobals->rgInventory[index+1].nAmount <= 0)
            gpGlobals->iCurInvMenuItem --;
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

	   if (gpGlobals->rgInventory[i].wItem == 0)
	   {
		   break;
	   }

      if (gpGlobals->rgInventory[i].nAmount > 0)
      {
         gpGlobals->rgInventory[j] = gpGlobals->rgInventory[i];
         j++;
      }
   }

   for (i = 0; i < MAX_INVENTORY; i++)
   {
	   if (gpGlobals->rgInventory[i].wItem <= 60 || gpGlobals->rgInventory[i].wItem >= 295)
	   {
		   gpGlobals->rgInventory[i].nAmount = 0;
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
PAL_IncreaseHPMP(
   WORD          wPlayerRole,
   INT         sHP,
   INT         sMP
)
/*++
  Purpose:

    Increase or decrease player's HP and/or MP.

  Parameters:

    [IN]  wPlayerRole - the number of player role.

    [IN]  sHP - number of HP to be increased (positive value) or decrased
                (negative value).

    [IN]  sMP - number of MP to be increased (positive value) or decrased
                (negative value).

  Return value:

    TRUE if the operation is succeeded, FALSE if not.

--*/
{
   BOOL           fSuccess = FALSE;
   INT           wOrigHP = gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole];
   INT           wOrigMP = gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole];
   //
   // Only care about alive players
   //
   if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] > 0)
   {
      //
      // change HP
      //
      gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] += sHP;

      if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] < 0)
      {
         gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] = 0;
      }
      else if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] >
        PAL_GetPlayerMaxHP(wPlayerRole))
      {
         gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] =
          PAL_GetPlayerMaxHP(wPlayerRole);
      }

      //
      // Change MP
      //
      gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] += sMP;

      if (gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] < 0)
      {
         gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] = 0;
      }
      else if (gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] >
        PAL_GetPlayerMaxMP(wPlayerRole))
      {
         gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] =
          PAL_GetPlayerMaxMP(wPlayerRole);
      }

      //
      // Avoid over treatment
      //
      if (wOrigHP != gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] ||
          wOrigMP != gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole])
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
               PAL_RunTriggerScript(gpGlobals->g.rgObject[w].item.wScriptOnEquip, (INT)i);
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

  Parameters:

    [IN]  wPlayerRole - the player role.

    [IN]  wEquipPart - the part of the equipment.

  Return value:

    None.

--*/
{
   INT       *p;
   int         i, j;

   p = (INT *)(&gpGlobals->rgEquipmentEffect[wEquipPart]); // HACKHACK

   for (i = 0; i < sizeof(PLAYERROLES) / sizeof(PLAYERS); i++)
   {
      p[i * MAX_PLAYER_ROLES + wPlayerRole] = 0;
   }

   //
   // Reset some parameters to default when appropriate
   //
   if (wEquipPart == kBodyPartHand)
   {
      //
      // 删除天罡/醉仙/
      // 
      gpGlobals->rgPlayerStatus[wPlayerRole][kStatusDualAttack] = 0;
     gpGlobals->rgPlayerStatus[wPlayerRole][kStatusBravery] = 0;
   }
   else  if (wEquipPart == kBodyPartFeet)
   {
      //
      //删除仙风云体术
      //
      gpGlobals->rgPlayerStatus[wPlayerRole][kStatusHaste] = 0;

   }
   else if (wEquipPart == kBodyPartWear)
   {
      //
      // Remove all poisons leveled 99
      //

      gpGlobals->rgPlayerStatus[wPlayerRole][kStatusMagic] = 0;

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
   int         i, index, iPoisonIndex;
   WORD        w;

   index = PAL_New_GetPlayerIndex(wPlayerRole);
   WORD wPoisonLevel = gpGlobals->g.rgObject[wPoisonID].poison.wPoisonLevel;

   if (index == -1 || gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0)
   {
      return;
   }

   iPoisonIndex = PAL_New_GetPoisonIndexForPlayer(wPlayerRole, wPoisonID);
   if (iPoisonIndex != -1)
   {
      // already poisoned
#ifdef POISON_STATUS_EXPAND // 增加毒的烈度
      INT iSuccessRate = 100;
      iSuccessRate -= wPoisonLevel * 20;
      iSuccessRate = max(iSuccessRate, 0);
   //   iSuccessRate *= (100 - PAL_GetPlayerPoisonResistance(wPlayerRole)) / 100.0;
      if (PAL_New_GetTrueByPercentage(iSuccessRate))
      {
         gpGlobals->rgPoisonStatus[iPoisonIndex][index].wPoisonIntensity++;
      }
      switch (wPoisonLevel)
      {
      case 0:
      case 1:
      case 2:
         gpGlobals->rgPoisonStatus[iPoisonIndex][index].wPoisonIntensity =
            min(gpGlobals->rgPoisonStatus[iPoisonIndex][index].wPoisonIntensity, 3);
         break;

      default:
         gpGlobals->rgPoisonStatus[iPoisonIndex][index].wPoisonIntensity = 0;
         break;
      }
#endif
      return;
   }

   for (i = 0; i < MAX_POISONS; i++)
   {
      w = gpGlobals->rgPoisonStatus[i][index].wPoisonID;
      if (w == wPoisonID)
      {   // already poisoned
         return;
      }
      else if (w == 0)
      {
         break;
      }
   }

   i = min(i, MAX_POISONS - 1);
   gpGlobals->rgPoisonStatus[i][index].wPoisonID = wPoisonID;
   gpGlobals->rgPoisonStatus[i][index].wPoisonScript = PAL_RunTriggerScript(gpGlobals->g.rgObject[wPoisonID].poison.wPlayerScript, wPlayerRole);

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
#ifdef POISON_STATUS_EXPAND
         memset(&gpGlobals->rgPoisonStatus[i][index], 0, sizeof(gpGlobals->rgPoisonStatus[i][index]));
#else
         gpGlobals->rgPoisonStatus[i][index].wPoisonID = 0;
         gpGlobals->rgPoisonStatus[i][index].wPoisonScript = 0;
#endif
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
#ifdef POISON_STATUS_EXPAND
         memset(&gpGlobals->rgPoisonStatus[i][index], 0, sizeof(gpGlobals->rgPoisonStatus[i][index]));
#else
         gpGlobals->rgPoisonStatus[i][index].wPoisonID = 0;
         gpGlobals->rgPoisonStatus[i][index].wPoisonScript = 0;
#endif
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
         //
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
   INT       w;
   int        i;

   w = gpGlobals->g.PlayerRoles.rgwAttackStrength[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwAttackStrength[wPlayerRole];
   }
   if (w < 0)
   {
      w = 0;
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
   INT       w;
   int        i;

   w = gpGlobals->g.PlayerRoles.rgwMagicStrength[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwMagicStrength[wPlayerRole];
   }
   if (w < 0)
   {
      w = 0;
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
   INT       w;
   int        i;

   w = gpGlobals->g.PlayerRoles.rgwDefense[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwDefense[wPlayerRole];
   }
   if (w < 0)
   {
      w = 0;
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
   INT       w;
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

   if (w < 0)
   {
      w = 0;
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

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    The total flee rate of the player.

--*/
{
   INT       w;
   int        i;

   w = gpGlobals->g.PlayerRoles.rgwFleeRate[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwFleeRate[wPlayerRole];
   }
   if (w < 0)
   {
      w = 0;
   }
   return w;
}

INT
PAL_GetPlayerWisdom(
   WORD           wPlayerRole
)
/*++
Purpose:

Get the player's flee rate, count in the effect of equipments.

Parameters:

[IN]  wPlayerRole - the player role ID.

Return value:

The total flee rate of the player.

--*/
{
   INT       w;
   int        i;

   w = gpGlobals->g.PlayerRoles.rgwWisdom[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwWisdom[wPlayerRole];
   }
   if (w < 0)
   {
      w = 0;
   }
   if (w > 12000)
   {
      w = 12000;
   }
   return w;
}

INT
PAL_GetPlayerPower(
   WORD           wPlayerRole
)
/*++
Purpose:

Get the player's flee rate, count in the effect of equipments.

Parameters:

[IN]  wPlayerRole - the player role ID.

Return value:

The total flee rate of the player.

--*/
{
   INT       w;
   int        i;

   w = gpGlobals->g.PlayerRoles.rgwPower[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwPower[wPlayerRole];
   }
   if (w < 0)
   {
      w = 0;
   }
   if (w > 12000)
   {
      w = 12000;
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
   INT       w;
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
   INT       w;
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

INT
PAL_New_GetPlayerSorceryResistance(
	WORD			wPlayerRole
)
/*++
Purpose:Get the player's resistance to Sorcery, count in the effect of equipments.
Parameters:[IN]  wPlayerRole - the player role ID.
Return value:The total resistance to Sorcery of the player.
--*/
{
	INT       w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwSorceryResistance[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwSorceryResistance[wPlayerRole];
	}

	return min(100, w);
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

	return min(100, w);
}

INT
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
   INT      w;

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

INT
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
   INT      w;

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

  Parameters:

    [IN]  wPlayerRole - the player role ID.

    [IN]  wMagic - the object ID of the magic.

  Return value:

    TRUE if succeeded, FALSE if failed.

--*/
{
   int            i;

   for (i = 0; i < MAX_PLAYER_MAGICS; i++)
   {
      if (gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole] == wMagic)
      {
         //
         // already have this magic
         //
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
      //
      return FALSE;
   }

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
   case kStatusSlow:
   case kStatusStrDown:
   case kStatusAtsDown:
   case kStatusDefDown:
      //
      // for "bad" statuses, don't set the status when we already have it
      //
      if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] != 0 &&
         gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] == 0)
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
   case kStatusMagic:
   case kStatusDefUp:
      //
      // for "good" statuses, reset the status if the status to be set lasts longer
      //
      if((gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] != 0 &&
         gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] < wNumRound) ||
         gpGlobals->rgPlayerStatus[wPlayerRole][kStatusPuppet] !=0 )
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

  Parameters:

    [IN]  wPlayerRole - the player ID.

    [IN]  wStatusID - the status to be set.

  Return value:

    None.

--*/
{
   //
   // Don't remove effects of equipments
   //
   if (gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] <= 999)
   {
      gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] = 0;
   }
}

VOID
PAL_New_RemovePlayerAllStatus(
	WORD         wPlayerRole
)
{
	int x;
	for (x = 0; x < kStatusAll; x++)
	{
		PAL_RemovePlayerStatus(wPlayerRole, x);
	}
}

VOID
PAL_ClearAllPlayerStatus(
   VOID
)
/*++
  Purpose:

    Clear all player status.

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
         //
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
   INT          wNumLevel
)
/*++
  Purpose:

    Increase the player's level by wLevels.

  Parameters:

    [IN]  wPlayerRole - player role ID.

    [IN]  wNumLevel - number of levels to be increased.

  Return value:

    None.

--*/
{
   INT          i;

   //
   // Add the level
   //

   if (wPlayerRole == RoleID_LiXiaoYao)
   {
	   if (gpGlobals->g.PlayerRoles.rgwLevel[RoleID_LiXiaoYao] += wNumLevel)
	   {
		   if (gpGlobals->g.PlayerRoles.rgwLevel[RoleID_LiXiaoYao] > MAX_LEVELS)
		   {
			   gpGlobals->g.PlayerRoles.rgwLevel[RoleID_LiXiaoYao] = MAX_LEVELS;
		   }
		   for (i = 0; i < wNumLevel; i++)
		   {
			   //
			   // Increase player's stats
			   gpGlobals->g.PlayerRoles.rgwMaxHP[RoleID_LiXiaoYao] += 18;
			   gpGlobals->g.PlayerRoles.rgwMaxMP[RoleID_LiXiaoYao] += 10;
			   gpGlobals->g.PlayerRoles.rgwAttackStrength[RoleID_LiXiaoYao] += 7;
			   gpGlobals->g.PlayerRoles.rgwMagicStrength[RoleID_LiXiaoYao] += 4;
			   gpGlobals->g.PlayerRoles.rgwDefense[RoleID_LiXiaoYao] += 7;
			   gpGlobals->g.PlayerRoles.rgwDexterity[RoleID_LiXiaoYao] += 5;
			   gpGlobals->g.PlayerRoles.rgwFleeRate[RoleID_LiXiaoYao] += 4;
			   gpGlobals->g.PlayerRoles.rgwPower[RoleID_LiXiaoYao] += 2;
			   gpGlobals->g.PlayerRoles.rgwWisdom[RoleID_LiXiaoYao] += 2;

		   }
	   }
   }

   if (wPlayerRole == RoleID_ZhaoLingEr)
   {
	   if (gpGlobals->g.PlayerRoles.rgwLevel[RoleID_ZhaoLingEr] += wNumLevel)
	   {
		   if (gpGlobals->g.PlayerRoles.rgwLevel[RoleID_ZhaoLingEr] > MAX_LEVELS)
		   {
			   gpGlobals->g.PlayerRoles.rgwLevel[RoleID_ZhaoLingEr] = MAX_LEVELS;
		   }
		   for (i = 0; i < wNumLevel; i++)
		   {
			   //
			   // Increase player's stats
			   gpGlobals->g.PlayerRoles.rgwMaxHP[RoleID_ZhaoLingEr] += 14;
			   gpGlobals->g.PlayerRoles.rgwMaxMP[RoleID_ZhaoLingEr] += 15;
			   gpGlobals->g.PlayerRoles.rgwAttackStrength[RoleID_ZhaoLingEr] += 4;
			   gpGlobals->g.PlayerRoles.rgwMagicStrength[RoleID_ZhaoLingEr] += 7;
			   gpGlobals->g.PlayerRoles.rgwDefense[RoleID_ZhaoLingEr] += 5;
			   gpGlobals->g.PlayerRoles.rgwDexterity[RoleID_ZhaoLingEr] += 6;
			   gpGlobals->g.PlayerRoles.rgwFleeRate[RoleID_ZhaoLingEr] += 6;
			   gpGlobals->g.PlayerRoles.rgwPower[RoleID_ZhaoLingEr] += 2;
			   gpGlobals->g.PlayerRoles.rgwWisdom[RoleID_ZhaoLingEr] += 2;
		   }
	   }
   }


   if (wPlayerRole == RoleID_LinYueRu)
   {
	   if (gpGlobals->g.PlayerRoles.rgwLevel[RoleID_LinYueRu] += wNumLevel)
	   {
		   if (gpGlobals->g.PlayerRoles.rgwLevel[RoleID_LinYueRu] > MAX_LEVELS)
		   {
			   gpGlobals->g.PlayerRoles.rgwLevel[RoleID_LinYueRu] = MAX_LEVELS;
		   }
		   for (i = 0; i < wNumLevel; i++)
		   {
			   //
			   // Increase player's stats
			   gpGlobals->g.PlayerRoles.rgwMaxHP[RoleID_LinYueRu] += 16;
			   gpGlobals->g.PlayerRoles.rgwMaxMP[RoleID_LinYueRu] += 12;
			   gpGlobals->g.PlayerRoles.rgwAttackStrength[RoleID_LinYueRu] += 6;
			   gpGlobals->g.PlayerRoles.rgwMagicStrength[RoleID_LinYueRu] += 6;
			   gpGlobals->g.PlayerRoles.rgwDefense[RoleID_LinYueRu] += 6;
			   gpGlobals->g.PlayerRoles.rgwDexterity[RoleID_LinYueRu] += 8;
			   gpGlobals->g.PlayerRoles.rgwFleeRate[RoleID_LinYueRu] += 6;
			   gpGlobals->g.PlayerRoles.rgwWisdom[RoleID_LinYueRu] += 2;
			   gpGlobals->g.PlayerRoles.rgwPower[RoleID_LinYueRu] += 2;
		   }
	   }
   }

   if (wPlayerRole == RoleID_ANu)
   {
	   if (gpGlobals->g.PlayerRoles.rgwLevel[RoleID_ANu] += wNumLevel)
	   {
		   if (gpGlobals->g.PlayerRoles.rgwLevel[RoleID_ANu] > MAX_LEVELS)
		   {
			   gpGlobals->g.PlayerRoles.rgwLevel[RoleID_ANu] = MAX_LEVELS;
		   }
		   for (i = 0; i < wNumLevel; i++)
		   {
			   //
			   // Increase player's stats
			   gpGlobals->g.PlayerRoles.rgwMaxHP[RoleID_ANu] += 14;
			   gpGlobals->g.PlayerRoles.rgwMaxMP[RoleID_ANu] += 12;
			   gpGlobals->g.PlayerRoles.rgwAttackStrength[RoleID_ANu] += 5;
			   gpGlobals->g.PlayerRoles.rgwMagicStrength[RoleID_ANu] += 5;
			   gpGlobals->g.PlayerRoles.rgwDefense[RoleID_ANu] += 5;
			   gpGlobals->g.PlayerRoles.rgwDexterity[RoleID_ANu] += 6;
			   gpGlobals->g.PlayerRoles.rgwFleeRate[RoleID_ANu] += 10;
			   gpGlobals->g.PlayerRoles.rgwPower[RoleID_ANu] += 2;
			   gpGlobals->g.PlayerRoles.rgwWisdom[RoleID_ANu] += 2;
		   }
	   }
   }

   if (wPlayerRole == RoleID_WuHou)
   {
	   if (gpGlobals->g.PlayerRoles.rgwLevel[RoleID_WuHou] += wNumLevel)
	   {
		   if (gpGlobals->g.PlayerRoles.rgwLevel[RoleID_WuHou] > MAX_LEVELS)
		   {
			   gpGlobals->g.PlayerRoles.rgwLevel[RoleID_WuHou] = MAX_LEVELS;
		   }
		   for (i = 0; i < wNumLevel; i++)
		   {
			   //
			   // Increase player's stats
			   gpGlobals->g.PlayerRoles.rgwMaxHP[RoleID_WuHou] += 8 + RandomLong(0, 15);
			   gpGlobals->g.PlayerRoles.rgwMaxMP[RoleID_WuHou] += 10 + RandomLong(0, 20);
			   gpGlobals->g.PlayerRoles.rgwAttackStrength[RoleID_WuHou] += 9 + RandomLong(0, 12);
			   gpGlobals->g.PlayerRoles.rgwMagicStrength[RoleID_WuHou] += 7 + RandomLong(0, 19);
			   gpGlobals->g.PlayerRoles.rgwDefense[RoleID_WuHou] += 5 + RandomLong(0, 6);
			   gpGlobals->g.PlayerRoles.rgwDexterity[RoleID_WuHou] += 5 + RandomLong(0, 6);
			   gpGlobals->g.PlayerRoles.rgwFleeRate[RoleID_WuHou] += 10 + RandomLong(0, 6);
			   gpGlobals->g.PlayerRoles.rgwWisdom[RoleID_WuHou] += 6;
			   gpGlobals->g.PlayerRoles.rgwPower[RoleID_WuHou] += 4;
		   }
	   }
   }

#define STAT_LIMIT(t) { if ((t) > 24000) (t) = 24000; }
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwAttackStrength[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwMagicStrength[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwDefense[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwDexterity[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwFleeRate[wPlayerRole]);
#undef STAT_LIMIT

#define STAT_LIMIT(t) { if ((t) > 10000) (t) = 10000; }
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwWisdom[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwPower[wPlayerRole]);
#undef STAT_LIMIT
   //
   // Reset experience points to zero
   //
   gpGlobals->Exp.rgPrimaryExp[wPlayerRole].wExp = 0;
   gpGlobals->Exp.rgPrimaryExp[wPlayerRole].wLevel =
      gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole];

}

WORD
PAL_New_GetPlayerID(
	WORD		wPlayerIndex
)
{
	if (wPlayerIndex > MAX_PLAYERS_IN_PARTY)
	{
		return 0xFFFF;
	}
	else
	{
		return gpGlobals->rgParty[wPlayerIndex].wPlayerRole;
	}
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



//fGetLowest为真找属性最低的角色序号，bLow为假找属性最高的角色序号，
INT
PAL_New_GetPlayerIndexByPara(
	PlayerPara	pp,
	BOOL		fGetLowest)
{
	INT *p = (INT *)(&gpGlobals->g.PlayerRoles);
	INT *p1[MAX_PLAYER_EQUIPMENTS + 1] =
	{
		(INT *)(&gpGlobals->rgEquipmentEffect[0]),
		(INT *)(&gpGlobals->rgEquipmentEffect[1]),
		(INT *)(&gpGlobals->rgEquipmentEffect[2]),
		(INT *)(&gpGlobals->rgEquipmentEffect[3]),
		(INT *)(&gpGlobals->rgEquipmentEffect[4]),
		(INT *)(&gpGlobals->rgEquipmentEffect[5]),
		(INT *)(&gpGlobals->rgEquipmentEffect[6]),

	};
	int i, j, max, maxIndex, min, minIndex, cur;
	INT w = 0;

	w = gpGlobals->rgParty[0].wPlayerRole;
	cur = p[pp * MAX_PLAYER_ROLES + w];
	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		cur += p1[i][pp * MAX_PLAYER_ROLES + w];
	}
	min = cur;
	minIndex = 0;
	max = cur;
	maxIndex = 0;

	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		w = gpGlobals->rgParty[i].wPlayerRole;
		cur = p[pp * MAX_PLAYER_ROLES + w];
		for (j = 0; j <= MAX_PLAYER_EQUIPMENTS; j++)
		{
			cur += p1[j][pp * MAX_PLAYER_ROLES + w];
		}

		if (max < cur)
		{
			max = cur;
			maxIndex = i;
		}
		if (min > cur)
		{
			min = cur;
			minIndex = i;
		}
	}

	if (fGetLowest == TRUE)
	{
		return minIndex;
	}
	else
	{
		return maxIndex;
	}
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
PAL_New_GetPoisonIndexForPlayer(
	WORD			wPlayerRole,
	WORD			wPoisonID
)
/*++
Purpose:    Check if the player is poisoned by the specified poison.

Parameters:		[IN]  wPlayerRole - the player role ID.
[IN]  wPoisonID - the poison to be checked.

Return value:		如果没有中这种毒，返回-1；
如果中毒，则返回该毒在该角色的rgPoisonStatus的位置序号
--*/
{
	int i, index;

	index = PAL_New_GetPlayerIndex(wPlayerRole);
	if (index == -1)
	{
		return -1;
	}

	for (i = 0; i < MAX_POISONS; i++)
	{
		if (gpGlobals->rgPoisonStatus[i][index].wPoisonID == wPoisonID)
		{
			return i;
		}
	}
	return -1;
}


BOOL
PAL_New_IsPlayerPoisoned(
	WORD			wPlayerRole
)
/*++
Purpose:    Check if the player is poisoned.
Parameters:    [IN]  wPlayerRole - the player role ID.
Return value:    TRUE if player is poisoned;
FALSE if not.
--*/
{
	int i, index;

	index = PAL_New_GetPlayerIndex(wPlayerRole);
	if (index == -1)
	{
		return FALSE;
	}

	for (i = 0; i < MAX_POISONS; i++)
	{
		if (gpGlobals->rgPoisonStatus[i][index].wPoisonID != 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}


VOID
PAL_New_SortPoisonsForPlayerByLevel(
	WORD			wPlayerRole
)
{
	int         i, j, index, PoisonNum;
	WORD        wPoisonID1, wPoisonID2;
	WORD        wPoisonLevel1, wPoisonLevel2;
	POISONSTATUS	tempPoison;

	for (index = 0; index <= gpGlobals->wMaxPartyMemberIndex; index++)
	{
		if (gpGlobals->rgParty[index].wPlayerRole == wPlayerRole)break;
	}

	if (index > gpGlobals->wMaxPartyMemberIndex)return; // don't go further

	for (j = 0, PoisonNum = 0; j < MAX_POISONS; j++)
	{
		wPoisonID1 = gpGlobals->rgPoisonStatus[j][index].wPoisonID;
		if (wPoisonID1 == 0) gpGlobals->rgPoisonStatus[j][index].wPoisonScript = 0;
		else PoisonNum++;
	}

	if (PoisonNum < 2)	return;	//中毒数目小于2不用排序


	for (i = 0; i < MAX_POISONS - 1; i++)
	{
		for (j = 0; j < MAX_POISONS - i - 1; j++)
		{
			wPoisonID1 = gpGlobals->rgPoisonStatus[j][index].wPoisonID;
			wPoisonLevel1 = gpGlobals->g.rgObject[wPoisonID1].poison.wPoisonLevel;
			wPoisonID2 = gpGlobals->rgPoisonStatus[j + 1][index].wPoisonID;
			wPoisonLevel2 = gpGlobals->g.rgObject[wPoisonID2].poison.wPoisonLevel;

			if (wPoisonLevel1 < wPoisonLevel2)
			{
				tempPoison = gpGlobals->rgPoisonStatus[j][index];
				gpGlobals->rgPoisonStatus[j][index] = gpGlobals->rgPoisonStatus[j + 1][index];
				gpGlobals->rgPoisonStatus[j + 1][index] = tempPoison;
			}
		}
	}
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

VOID
PAL_New_SortInventory(
)
{
	int         i, j;
	WORD        ItemID1, ItemID2;
	INT		ItemNum;
	INVENTORY   TempItem;
	INVENTORY	TempInventory[MAX_INVENTORY];

	memset(TempInventory, 0, sizeof(TempInventory));

	for (i = 0, j = 0; i < MAX_INVENTORY; i++)
	{
		TempItem = gpGlobals->rgInventory[i];
		if (TempItem.wItem != 0 && TempItem.nAmount != 0)
		{
			TempInventory[j] = TempItem;
			j++;
		}
	}
	ItemNum = j;

	for (i = 0; i < ItemNum; i++)
	{
		for (j = 0; j < ItemNum - i - 1; j++)
		{
			ItemID1 = TempInventory[j].wItem;
			ItemID2 = TempInventory[j + 1].wItem;


			if (ItemID1 > ItemID2)
			{
				TempItem = TempInventory[j];
				TempInventory[j] = TempInventory[j + 1];
				TempInventory[j + 1] = TempItem;
			}
		}
	}

	for (i = 0; i < MAX_INVENTORY; i++)
	{
		gpGlobals->rgInventory[i] = TempInventory[i];
	}

	return;
}

VOID PAL_AutoSaveGame()
{
	WORD wSavedTimes = 0;
	gpGlobals->bCurrentSaveSlot = 10;

	WORD curSavedTimes = GetSavedTimes(10);
	if (curSavedTimes >= wSavedTimes)
	{
		wSavedTimes = curSavedTimes;
	}
	PAL_SaveGame(gpGlobals->bCurrentSaveSlot, wSavedTimes + 1);
}
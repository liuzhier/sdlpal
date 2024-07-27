/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2024, SDLPAL development team.
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
GLOBALVARS * const  gpGlobals = &_gGlobals;

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

#if !PALMOD_Version_DOS
BOOL
PAL_IsWINVersion(
	BOOL *pfIsWIN95
)
{
#if !PALMOD_BULK_MAP
	FILE *fps[] = { UTIL_OpenRequiredFile("abc.mkf"), UTIL_OpenRequiredFile("map.mkf"), gpGlobals->f.fpF, gpGlobals->f.fpFBP, gpGlobals->f.fpFIRE, gpGlobals->f.fpMGO };
#else
   FILE*    fps[6];
   int      i, size = 0;
#endif // !PALMOD_BULK_MAP

	uint8_t *data = NULL;
	int data_size = 0, dos_score = 0, win_score = 0;
	BOOL result = FALSE;

#if !PALMOD_BULK_MAP
	for (int i = 0; i < sizeof(fps) / sizeof(FILE *); i++)
#else
   fps[size++] = UTIL_OpenRequiredFile("abc.mkf");
   if (!PALMOD_CLASSIC || !PALMOD_BULK_MAP) fps[size++] = UTIL_OpenRequiredFile("map.mkf");
   fps[size++] = gpGlobals->f.fpF;
   fps[size++] = gpGlobals->f.fpFBP;
   fps[size++] = gpGlobals->f.fpFIRE;
   fps[size++] = gpGlobals->f.fpMGO;

	for (i = 0; i < size; i++)
#endif // !PALMOD_BULK_MAP
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
#if PALMOD_BULK_MAP
   FILE *fpObject = UTIL_OpenRequiredFile(PALMOD_MainData_PATH "OBJECT");
   fseek(fpObject, 0, SEEK_END);
   data_size = ftell(fpObject);
   UTIL_CloseFile(fpObject);
#else
	data_size = PAL_MKFGetChunkSize(2, gpGlobals->f.fpSSS);
#endif // !PALMOD_BULK_MAP
	if (data_size % sizeof(OBJECT) == 0 && data_size % sizeof(OBJECT_DOS) != 0 && dos_score > 0) goto PAL_IsWINVersion_Exit;
	if (data_size % sizeof(OBJECT_DOS) == 0 && data_size % sizeof(OBJECT) != 0 && win_score > 0) goto PAL_IsWINVersion_Exit;

	if (pfIsWIN95) *pfIsWIN95 = (win_score == sizeof(fps) / sizeof(FILE *)) ? TRUE : FALSE;

	result = TRUE;

PAL_IsWINVersion_Exit:
	free(data);

#if !PALMOD_BULK_MAP
	fclose(fps[1]);
#else
   if (!PALMOD_CLASSIC || !PALMOD_BULK_MAP) fclose(fps[1]);
#endif // !PALMOD_BULK_MAP

	fclose(fps[0]);

	return result;
}
#endif // !PALMOD_Version_DOS

CODEPAGE
PAL_DetectCodePage(
	const char *   filename
)
{
	FILE *fp;
	char *word_buf = NULL;
	size_t word_len;
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
#if PALMOD_BULK_DATA_SSS
   gpGlobals->f.fpDATA = UTIL_OpenRequiredFile("ui.mkf");
#else
   gpGlobals->f.fpDATA = UTIL_OpenRequiredFile("data.mkf");
#endif // !PALMOD_BULK_DATA_SSS
   gpGlobals->f.fpF = UTIL_OpenRequiredFile("f.mkf");
   gpGlobals->f.fpFIRE = UTIL_OpenRequiredFile("fire.mkf");
   gpGlobals->f.fpRGM = UTIL_OpenRequiredFile("rgm.mkf");
#if !PALMOD_BULK_DATA_SSS
   gpGlobals->f.fpSSS = UTIL_OpenRequiredFile("sss.mkf");
#endif // !PALMOD_BULK_DATA_SSS

   //
   // Retrieve game resource version
   //
#if PALMOD_Version_DOS
   gConfig.fIsWIN95 = FALSE;
#else
   if (!PAL_IsWINVersion(&gConfig.fIsWIN95)) return -1;
#endif // PALMOD_Version_DOS

   //
   // Enable AVI playing only when the resource is WIN95
   //
#if !PALMOD_CLASSIC
   gConfig.fEnableAviPlay = gConfig.fEnableAviPlay && gConfig.fIsWIN95;
#endif

   //
   // Detect game language only when no message file specified
   //
   if (!gConfig.pszMsgFile) PAL_SetCodePage(PAL_DetectCodePage("word.dat"));

   //
   // Set decompress function
   //
#if PALMOD_Version_DOS
   Decompress = YJ1_Decompress;
#else
   Decompress = gConfig.fIsWIN95 ? YJ2_Decompress : YJ1_Decompress;
#endif // PALMOD_Version_DOS

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
#if !PALMOD_BULK_DATA_SSS
   UTIL_CloseFile(gpGlobals->f.fpSSS);
#endif // !PALMOD_BULK_DATA_SSS

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

#if !PALMOD_BULK_DATA_SSS
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
#else
   #define LOAD_DATA_GL(filename, type, lptype, ptr, n)                             \
      {                                                                             \
         INT len;                                                                   \
         FILE *fpTmp = UTIL_OpenRequiredFile(filename);                             \
         fseek(fpTmp, 0, SEEK_END);                                                 \
         len = ftell(fpTmp);                                                        \
         ptr = (lptype)malloc(len);                                                 \
         n = len / sizeof(type);                                                    \
         fseek(fpTmp, 0, SEEK_SET);                                                 \
         if (ptr != NULL)                                                           \
         {                                                                          \
            fread(ptr, len, 1, fpTmp);                                              \
         }                                                                          \
         UTIL_CloseFile(fpTmp);                                                     \
         if (ptr == NULL)                                                           \
         {                                                                          \
            TerminateOnError("PAL_InitGlobalGameData(): Memory allocation error!"); \
         }                                                                          \
      }                                                                             \

   #define LOAD_DATA_GLR(filename, ptr)                                             \
      {                                                                             \
         INT len;                                                                   \
         FILE *fpTmp = UTIL_OpenRequiredFile(filename);                             \
         fseek(fpTmp, 0, SEEK_END);                                                 \
         len = ftell(fpTmp);                                                        \
         fseek(fpTmp, 0, SEEK_SET);                                                 \
         if (ptr != NULL)                                                           \
         {                                                                          \
            fread(ptr, len, 1, fpTmp);                                              \
         }                                                                          \
         UTIL_CloseFile(fpTmp);                                                     \
         if (ptr == NULL)                                                           \
         {                                                                          \
            TerminateOnError("PAL_InitGlobalGameData(): Memory allocation error!"); \
         }                                                                          \
      }                                                                             \

#endif // !PALMOD_BULK_DATA_SSS

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
#if !PALMOD_BULK_DATA_SSS
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
#else // !PALMOD_BULK_DATA_SSS
{
   int        len;
   FILE      *fp;

   //
   // If the memory has not been allocated, allocate first.
   //
   if (gpGlobals->g.lprgEventObject == NULL)
   {
      //LOAD_DATA_GL(PALMOD_MainData_PATH "EVENTOBJECT", EVENTOBJECT, LPEVENTOBJECT, gpGlobals->g.lprgEventObject, gpGlobals->g.nEventObject);
      //DO_BYTESWAP(gpGlobals->g.lprgEventObject, sizeof(gpGlobals->g.lprgEventObject));

      LOAD_DATA_GL(PALMOD_MainData_PATH "SCRIPTENTRY", SCRIPTENTRY, LPSCRIPTENTRY, gpGlobals->g.lprgScriptEntry, gpGlobals->g.nScriptEntry);
      DO_BYTESWAP(gpGlobals->g.lprgScriptEntry, sizeof(gpGlobals->g.lprgScriptEntry));

      LOAD_DATA_GL(PALMOD_CoreData_PATH "STORE", STORE, LPSTORE, gpGlobals->g.lprgStore, gpGlobals->g.nStore);
      DO_BYTESWAP(gpGlobals->g.lprgStore, sizeof(gpGlobals->g.lprgStore));

      LOAD_DATA_GL(PALMOD_CoreData_PATH "ENEMY", ENEMY, LPENEMY, gpGlobals->g.lprgEnemy, gpGlobals->g.nEnemy);
      DO_BYTESWAP(gpGlobals->g.lprgEnemy, sizeof(gpGlobals->g.lprgEnemy));

      LOAD_DATA_GL(PALMOD_CoreData_PATH "ENEMYTEAM", ENEMYTEAM, LPENEMYTEAM, gpGlobals->g.lprgEnemyTeam, gpGlobals->g.nEnemyTeam);
      DO_BYTESWAP(gpGlobals->g.lprgEnemyTeam, sizeof(gpGlobals->g.lprgEnemyTeam));

      LOAD_DATA_GL(PALMOD_CoreData_PATH "MAGIC", MAGIC, LPMAGIC, gpGlobals->g.lprgMagic, gpGlobals->g.nMagic);
      DO_BYTESWAP(gpGlobals->g.lprgMagic, sizeof(gpGlobals->g.lprgMagic));

      LOAD_DATA_GL(PALMOD_CoreData_PATH "BATTLEFIELD", BATTLEFIELD, LPBATTLEFIELD, gpGlobals->g.lprgBattleField, gpGlobals->g.nBattleField);
      DO_BYTESWAP(gpGlobals->g.lprgBattleField, sizeof(gpGlobals->g.lprgBattleField));

      LOAD_DATA_GL(PALMOD_CoreData_PATH "LEVELUPMAGIC", LEVELUPMAGIC_ALL, LPLEVELUPMAGIC_ALL, gpGlobals->g.lprgLevelUpMagic, gpGlobals->g.nLevelUpMagic);
      DO_BYTESWAP(gpGlobals->g.lprgLevelUpMagic, sizeof(gpGlobals->g.lprgLevelUpMagic));

      LOAD_DATA_GLR(PALMOD_CoreData_PATH "BattleEffectIndex", gpGlobals->g.rgwBattleEffectIndex);
      DO_BYTESWAP(gpGlobals->g.rgwBattleEffectIndex, sizeof(gpGlobals->g.rgwBattleEffectIndex));

      LOAD_DATA_GLR(PALMOD_CoreData_PATH "ENEMYPOS", &gpGlobals->g.EnemyPos);
      DO_BYTESWAP(&(gpGlobals->EnemyPos), sizeof(gpGlobals->EnemyPos));

      LOAD_DATA_GLR(PALMOD_CoreData_PATH "LEVELUPEXP", gpGlobals->g.rgLevelUpExp);
      DO_BYTESWAP(gpGlobals->rgLevelUpExp, sizeof(gpGlobals->rgLevelUpExp));
   }
}

PAL_FORCE_INLINE char*
PAL_ReadOneLine(
   char* temp,
   int      limit,
   FILE* fp
)
{
   if (fgets(temp, limit, fp))
   {
      size_t n = strlen(temp);
      if (n == limit - 1 && temp[n - 1] != '\n' && !feof(fp))
      {
         // Line too long, try to read it as a whole
         int nn = 2;
         char* tmp = strdup(temp);
         while (!feof(fp))
         {
            if (!(tmp = (char*)realloc(tmp, nn * limit)))
            {
               TerminateOnError("PAL_ReadOneLine(): failed to allocate memory for long line!");
            }
            if (fgets(tmp + n, limit + 1, fp))
            {
               n += strlen(tmp + n);
               if (n < limit - 1 || temp[n - 1] == '\n')
                  break;
               else
                  nn++;
            }
         }
         if (tmp[n - 1] == '\n') tmp[n - 1] = 0;
         return tmp;
      }
      else
      {
         while (n > 0 && (temp[n - 1] == '\n' || temp[n - 1] == '\r')) temp[--n] = 0;
         return temp;
      }
   }
   else
      return NULL;
}
#endif // !PALMOD_BULK_DATA_SSS

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
#if !PALMOD_BULK_DATA_SSS
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
#else
   //LOAD_DATA_GL(PALMOD_MainData_PATH "EVENTOBJECT", EVENTOBJECT, LPEVENTOBJECT, p->lprgEventObject, p->nEventObject);
   //DO_BYTESWAP(p->lprgEventObject, sizeof(p->lprgEventObject));

   //LOAD_DATA_GLR(PALMOD_MainData_PATH "SCENE", p->rgScene);
   //LOAD_DATA_GLR(PALMOD_MainData_PATH "OBJECT", &objects);

   PAL_New_LoadScene();

   LOAD_DATA_GLR(PALMOD_CoreData_PATH "PlayerRoles", &p->PlayerRoles);
   DO_BYTESWAP(&(p->PlayerRoles), sizeof(PLAYERROLES));
#endif // !PALMOD_BULK_DATA_SSS

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
   gpGlobals->nFollower = 0;
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
	WORD             rgwReserved2[3];         // unused
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
#if PALMOD_Version_DOS
   return PAL_LoadGame_DOS(iSaveSlot);
#else
	return gConfig.fIsWIN95 ? PAL_LoadGame_WIN(iSaveSlot) : PAL_LoadGame_DOS(iSaveSlot);
#endif
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
	size_t i;

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

#if !PALMOD_BULK_DATA_SSS
	i = PAL_MKFGetChunkSize(0, gpGlobals->f.fpSSS);
#else
   FILE *fpEvent = UTIL_OpenRequiredFile(PALMOD_MainData_PATH "EVENTOBJECT");
   fseek(fpEvent, 0, SEEK_END);
   i = ftell(fpEvent);
   UTIL_CloseFile(fpEvent);
#endif // !PALMOD_BULK_DATA_SSS
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
PAL_ReloadInNextTick(
    INT           iSaveSlot
)
/*++
  Purpose:

    Reload the game IN NEXT TICK, avoid reentrant problems.

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

  Parameters:

    [IN]  iSaveSlot - Slot of saved game.

  Return value:

    None.

--*/
{
   PAL_InitGlobalGameData();

   gpGlobals->bCurrentSaveSlot = (BYTE)iSaveSlot;

#if PD_Player_Status_Index_error
   //
   // Using incorrect logic to save the status information of a role.
   //
   PAL_New_SaveErrorStatus();
#endif

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

#if !PD_LoadSave_NoResetItemCursor
   gpGlobals->iCurInvMenuItem = 0;
#if PD_Menu_KeyLeftOrRight_NextLine
   gpGlobals->iCurSellMenuItem = 0;
#endif // PD_Menu_KeyLeftOrRight_NextLine
#endif // PD_LoadSave_NoResetItemCursor
   gpGlobals->fInBattle = FALSE;

#if PD_Player_Status_Index_error
   //
   // Using incorrect logic to read the status information of a role.
   //
   PAL_New_LoadErrorStatus();
#else
   memset(gpGlobals->rgPlayerStatus, 0, sizeof(gpGlobals->rgPlayerStatus));
#endif

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
PAL_GetItemIndexToInventory(
   WORD          wObjectID,
   INT          *index
)
/*++
  Purpose:

    Search for the specified item in the inventory.

  Parameters:

    [IN]  wObjectID - object number of the item.

    [IN]  index - use a pointer to receive the retrieved inventory index.

  Return value:

    TRUE if found it, FALSE if not found it.

--*/
{
   BOOL         fFound = FALSE;

   *index = 0;

   while (*index < MAX_INVENTORY)
   {
      if (gpGlobals->rgInventory[*index].wItem == wObjectID)
      {
         fFound = TRUE;
         break;
      }
      else if (gpGlobals->rgInventory[*index].wItem == 0)
      {
         break;
      }
      (*index)++;
   }

   return fFound;
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
   fFound = PAL_GetItemIndexToInventory(wObjectID, &index);

#if PD_Add_Item_Not_End_Place
   if (iNum >= 0)
#else
   if (iNum > 0)
#endif
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

      if (fFound)
      {
         gpGlobals->rgInventory[index].nAmount += iNum;
         if (gpGlobals->rgInventory[index].nAmount > 99)
         {
            //
            // Maximum number is 99
            //
            gpGlobals->rgInventory[index].nAmount = 99;
         }
      }
      else
      {
         gpGlobals->rgInventory[index].wItem = wObjectID;
         if (iNum > 99)
         {
            iNum = 99;
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
#if !PD_Del_Item_Menu_Cursor_Move_Prev
         if (gpGlobals->rgInventory[index].nAmount == 0 && index == gpGlobals->iCurInvMenuItem && index + 1 < MAX_INVENTORY && gpGlobals->rgInventory[index + 1].nAmount <= 0)
            gpGlobals->iCurInvMenuItem--;
#endif
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
      //removed detect zero then break code, due to incompatible with save file hacked by palmod

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
PAL_IncreaseHPMP(
   WORD          wPlayerRole,
   SHORT         sHP,
   SHORT         sMP
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
   WORD           wOrigHP = gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole];
   WORD           wOrigMP = gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole];

   //
   // Only care about alive players
   //
   if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] > 0)
   {
      //
      // change HP
      //
      gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] += sHP;

      if ((SHORT)(gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole]) < 0)
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

      if ((SHORT)(gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole]) < 0)
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
      // Avoid over treatment
      //
      if (wOrigHP != gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] ||
          wOrigMP != gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole])
         fSuccess = TRUE;
   }

#if PD_Role_Repeat_Not_Display_HP_Loss
   if (fSuccess)
   {
      PAL_ChangeHPMP(g_Battle.wChangePlayerIndex, gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] - wOrigHP,
         gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] - wOrigMP, kBattleChangeHPMPCommit);
   }
#endif // PD_Role_Repeat_Not_Display_HP_Loss

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
               PAL_RunTriggerScript(gpGlobals->g.rgObject[w].item.wScriptOnEquip, (WORD)i);
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
   WORD       *p;
   int         i, j;

   p = (WORD *)(&gpGlobals->rgEquipmentEffect[wEquipPart]); // HACKHACK

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
      // reset the dual attack status
      //
      gpGlobals->rgPlayerStatus[wPlayerRole][kStatusDualAttack] = 0;
   }
   else if (wEquipPart == kBodyPartWear)
   {
      //
      // Remove all poisons leveled 99
      //
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
		  PAL_RunTriggerScript(gpGlobals->g.rgObject[wPoisonID].poison.wPlayerScript, wPlayerRole);
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

#if PD_Fix_Poison_Level
      if (w == 0)
      {
         //
         // Skip empty PoisonID
         //
         continue;
      }
#endif // PD_Fix_Poison_Level

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

WORD
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
   WORD       w;
   int        i;

   w = gpGlobals->g.PlayerRoles.rgwAttackStrength[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwAttackStrength[wPlayerRole];
   }

   return w;
}

WORD
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
   WORD       w;
   int        i;

   w = gpGlobals->g.PlayerRoles.rgwMagicStrength[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwMagicStrength[wPlayerRole];
   }

   return w;
}

WORD
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
   WORD       w;
   int        i;

   w = gpGlobals->g.PlayerRoles.rgwDefense[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwDefense[wPlayerRole];
   }

   return w;
}

WORD
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
   WORD       w;
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

WORD
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
   WORD       w;
   int        i;

   w = gpGlobals->g.PlayerRoles.rgwFleeRate[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwFleeRate[wPlayerRole];
   }

   return w;
}

WORD
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
   WORD       w;
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

WORD
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
   WORD       w;
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

BOOL
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
   BOOL           fSuccess = TRUE;

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
      if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0)
      {
         if (gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] < wNumRound)
         {
            gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] = wNumRound;
         }
      }
      else
      {
         fSuccess = FALSE;
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

   return fSuccess;
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
   WORD          wNumLevel
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
   WORD          i;

   //
   // Add the level
   //
   gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole] += wNumLevel;
   if (gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole] > MAX_LEVELS)
   {
      gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole] = MAX_LEVELS;
   }

   for (i = 0; i < wNumLevel; i++)
   {
      //
      // Increase player's stats
      //
      gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole] += 10 + RandomLong(0, 8);
      gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole] += 8 + RandomLong(0, 6);
      gpGlobals->g.PlayerRoles.rgwAttackStrength[wPlayerRole] += 4 + RandomLong(0, 1);
      gpGlobals->g.PlayerRoles.rgwMagicStrength[wPlayerRole] += 4 + RandomLong(0, 1);
      gpGlobals->g.PlayerRoles.rgwDefense[wPlayerRole] += 2 + RandomLong(0, 1);
      gpGlobals->g.PlayerRoles.rgwDexterity[wPlayerRole] += 2 + RandomLong(0, 1);
      gpGlobals->g.PlayerRoles.rgwFleeRate[wPlayerRole] += 2;
   }

#define STAT_LIMIT(t) { if ((t) > 999) (t) = 999; }
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwAttackStrength[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwMagicStrength[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwDefense[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwDexterity[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwFleeRate[wPlayerRole]);
#undef STAT_LIMIT

   //
   // Reset experience points to zero
   //
   gpGlobals->Exp.rgPrimaryExp[wPlayerRole].wExp = 0;
   gpGlobals->Exp.rgPrimaryExp[wPlayerRole].wLevel =
      gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole];
}

#if PD_Battle_ShowEnemyStatus
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

   [IN]  wEnemyIndex - The event index in the team.

   [IN]  pNumPos - The pos of draw num.

   [IN]  pNumPos - The pos of draw text.

   [IN]  bbJumpScript - Determine return value..

  Return value:

   物品及其数量

--*/
{
   static WORD       wLastEventObject = 0;

   WORD              wNextScriptEntry;
   BOOL              fEnded;
   LPSCRIPTENTRY     pScript;
   LPEVENTOBJECT     pEvtObj = NULL;

   // 04跳转并返回指令执行结果
   BOOL              bJumpResults = FALSE;

   wNextScriptEntry = wScriptEntry;
   fEnded = FALSE;

   if (wEventObjectID == 0xFFFF)
   {
      wEventObjectID = wLastEventObject;
   }

   wLastEventObject = wEventObjectID;

   if (wEventObjectID != 0)
   {
#if PALMOD_BULK_DATA_SSS
      pEvtObj = &(gpGlobals->g.lprgEventObject[PAL_New_GetSceneEventObject(wEventObjectID - 1)]);
#else
      pEvtObj = &(gpGlobals->g.lprgEventObject[wEventObjectID - 1]);
#endif // PALMOD_BULK_DATA_SSS
   }

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
            return FALSE;
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
         PAL_DrawText(L"\x65E0", pTextPos, MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
      }
      else
      {
         PAL_DrawNumber((wEnemyBootyNum == 0) ? 1 : wEnemyBootyNum, 3, pNumPos, kNumColorCyan, kNumAlignRight);
         PAL_DrawText(PAL_GetWord(wEnemyBooty), pTextPos, MENUITEM_COLOR_CONFIRMED,
            TRUE, FALSE, FALSE);

         return TRUE;
      }

   return FALSE;
}

SHORT
PAL_New_GetEnemyLevel(
   WORD		wEnemyIndex
)
{
   WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
   if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
   {
      return 0;
   }

   SHORT s = g_Battle.rgEnemy[wEnemyIndex].e.wLevel;
   s += 1;

   if (g_Battle.fIsBoss && s >= 30)
   {
      s += 5;
   }

   // 敌方修行最小为1
   s = max(s, 1);
   s = min(s, 0x7FFF);

   // 敌方修行最大999
   s = min(s, 999);
   return (INT)s;
}

SHORT
PAL_New_GetEnemyAttackStrength(
   WORD		wEnemyIndex
)
{
   WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;

   if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
   {
      return 0;
   }

   SHORT s = g_Battle.rgEnemy[wEnemyIndex].e.wAttackStrength;

   s += (g_Battle.rgEnemy[wEnemyIndex].e.wLevel + 6) * 6;

   s = max(s, 0);
   s = min(s, 0xFFFF);

   return s;
}

SHORT
PAL_New_GetEnemyMagicStrength(
   WORD		wEnemyIndex
)
{
   WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;

   if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
   {
      return 0;
   }

   SHORT s = g_Battle.rgEnemy[wEnemyIndex].e.wMagicStrength;

   s += (g_Battle.rgEnemy[wEnemyIndex].e.wLevel + 6) * 6;

   s = max(s, 0);
   s = min(s, 0xFFFF);

   return s;
}

SHORT
PAL_New_GetEnemyDefense(
   WORD		wEnemyIndex
)
{
   WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;

   if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
   {
      return 0;
   }

   SHORT s = g_Battle.rgEnemy[wEnemyIndex].e.wDefense;

   s += (g_Battle.rgEnemy[wEnemyIndex].e.wLevel + 6) * 4;

   s = max(s, 0);
   s = min(s, 0xFFFF);

   return s;
}

SHORT
PAL_New_GetEnemyDexterity(
   WORD		wEnemyIndex
)
{
   WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;

   if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
   {
      return 0;
   }

   SHORT s = g_Battle.rgEnemy[wEnemyIndex].e.wDexterity;

   s += (g_Battle.rgEnemy[wEnemyIndex].e.wLevel + 6) * 3;

   s = max(s, 0);
   s = min(s, 0xFFFF);

   return s;
}

SHORT
PAL_New_GetEnemyFleeRate(
   WORD		wEnemyIndex
)
{
   WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;

   if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
   {
      return 0;
   }

   SHORT s = g_Battle.rgEnemy[wEnemyIndex].e.wFleeRate;

   s += (g_Battle.rgEnemy[wEnemyIndex].e.wLevel + 6) * 2;

   s = max(s, 0);
   s = min(s, 0xFFFF);

   return s;
}

SHORT
PAL_New_GetEnemySorceryResistance(
   WORD		wEnemyIndex
)
{
   WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
   if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
   {
      return 0;
   }

   SHORT w = gpGlobals->g.rgObject[wObjectID].enemy.wResistanceToSorcery * 10;

   return min(w, 100);
}

SHORT
PAL_New_GetEnemyPoisonResistance(
   WORD		wEnemyIndex
)
{
   WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
   if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
   {
      return 0;
   }

   SHORT w = g_Battle.rgEnemy[wEnemyIndex].e.wPoisonResistance;

   if (g_Battle.fIsBoss)
   {
      if (w == 0)
      {
         w += 15;
      }
      else
      {
         w += 10;
      }
   }

   return min(w, 100);
}

SHORT
PAL_New_GetEnemyPhysicalResistance(
   WORD		wEnemyIndex
)
{
   WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
   if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
   {
      return 0;
   }

   SHORT w = g_Battle.rgEnemy[wEnemyIndex].e.wPhysicalResistance;

   if (g_Battle.fIsBoss)
   {
      if (w == 0)
      {
         w += 15;
      }
      else
      {
         w += 10;
      }
   }
   else
   {
      w += 10;
   }


   return min(w, 100);
}

SHORT
PAL_New_GetEnemyElementalResistance(
   WORD		wEnemyIndex,
   INT		iAttrib
)
{
   WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
   if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
   {
      return 0;
   }

   SHORT w = g_Battle.rgEnemy[wEnemyIndex].e.wElemResistance[iAttrib];

   return min(w, 100);
}
#endif // PD_Battle_ShowEnemyStatus

#if PD_Battle_ShowMoreMessages
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

SHORT
PAL_New_GetPlayerPhysicalResistance(
   WORD			wPlayerRole
)
{
   INT        w;
   int        i;

   w = gpGlobals->g.PlayerRoles.rgwPhysicalResistance[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwPhysicalResistance[wPlayerRole];
   }

   return min(100, w);
}

INT
PAL_New_GetPlayerSorceryResistance(
   WORD			wPlayerRole
)
/*++
   Purpose:

      Get the player's resistance to Sorcery, count in the effect of equipments.

   Parameters:

      [IN]  wPlayerRole - the player role ID.

   Return value:

      The total resistance to Sorcery of the player.

--*/
{
   INT        w;
   int        i;

   w = gpGlobals->g.PlayerRoles.rgwSorceryResistance[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwSorceryResistance[wPlayerRole];
   }

   return min(100, w);
}
#endif // PD_Battle_ShowMoreMessages

#if PD_Player_Status_Index_error
VOID
PAL_New_SaveErrorStatus(
   VOID
)
{
   INT  i, j;
   WORD wPlayerRole;
   BOOL fCompleted[MAX_PLAYER_ROLES] = { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE };

   //
   // Back up the status of all roles.
   //
   for (i = 0; i < MAX_PLAYABLE_PLAYER_ROLES; i++)
   {
      wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

      if (wPlayerRole >= MAX_PLAYER_ROLES || fCompleted[wPlayerRole]) continue;

      for (j = kStatusConfused; j <= kStatusSilence; j++)
      {
         //
         // Only objects with negative buffs should be allocated incorrectly.
         //
         gpGlobals->rgPlayerStatusError[i][j] = gpGlobals->rgPlayerStatus[wPlayerRole][j];
      }

      fCompleted[wPlayerRole] = TRUE;
   }
}

VOID
PAL_New_LoadErrorStatus(
   VOID
)
{
   INT  i, j;
   WORD wPlayerRole;
   BOOL fCompleted[MAX_PLAYER_ROLES] = { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE };

   //
   // Restore the state of all roles.
   //
   for (i = 0; i < MAX_PLAYABLE_PLAYER_ROLES; i++)
   {
      wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

      if (wPlayerRole >= MAX_PLAYER_ROLES || fCompleted[wPlayerRole]) continue;

      for (j = 0; j < kStatusAll; j++)
      {
         gpGlobals->rgPlayerStatus[wPlayerRole][j] = gpGlobals->rgPlayerStatusError[i][j];
      }

      fCompleted[wPlayerRole] = TRUE;
   }
}
#endif // PD_Player_Status_Index_error

#if PALMOD_BULK_DATA_SSS
VOID
PAL_New_LoadScene(
   VOID
)
{
   GAMEDATA   *p = &gpGlobals->g;
   FILE       *fpThisScene;
   LPSTR       lpszTextContent;
   CHAR        temp[512], hexStr[5];
   INT         i, iValue, j, k, l, len;
   const WORD  nSceneParam = sizeof(SCENE) / sizeof(WORD) - 1, nObjectParam = sizeof(EVENTOBJECT) / sizeof(WORD);

   p->nEventObject = MAX_EVENT_OBJECTS;
   l = sizeof(EVENTOBJECT) * MAX_EVENT_OBJECTS;
   p->lprgEventObject = UTIL_malloc(l);
   memset(p->lprgEventObject, 0, l);

   for (i = 1; i < MAX_SCENES; i++)
   {
      fpThisScene = UTIL_OpenRequiredFile(PAL_va(1, "%s%s%s%d.TXT", gConfig.pszGamePath,
         PAL_NATIVE_PATH_SEPARATOR, PALMOD_MainData_SCENE_PATH, i));

      if (fpThisScene == NULL)
      {
         //
         // Assuming that the file numbers are continuous here
         //
         break;
      }

      len = 0;

      while ((lpszTextContent = PAL_ReadOneLine(temp, 512, fpThisScene)) != NULL)
      {
         k = 1;
         
         switch (*lpszTextContent)
         {
         case '#':
            {
               for (j = 0; j < nSceneParam; j++)
               {
                  while (strncmp(lpszTextContent + k, "\t", 1) != 0 &&
                     strncmp(lpszTextContent + k, "\0", 1) != 0)
                     k++;

                  if (strncmp(lpszTextContent + k, "\0", 1) == 0) break;

                  k++;

                  switch (j)
                  {
                  case 0:
                     sscanf(lpszTextContent + k, "%d", &iValue);
                     break;

                  case 1:
                  case 2:
                  default:
                  {
                     strncpy(hexStr, lpszTextContent + k, 4);
                     hexStr[4] = '\0';
                     iValue = strtoul(hexStr, NULL, 16);
                  }
                  break;
                  }

                  ((WORD*)&(p->rgScene[i - 1]))[j] = iValue;
               };
            }
            break;

         case '@':
            {
               for (j = 0; j < nObjectParam; j++)
               {
                  while (strncmp(lpszTextContent + k, "\t", 1) != 0 &&
                     strncmp(lpszTextContent + k, "\0", 1) != 0)
                     k++;

                  if (strncmp(lpszTextContent + k, "\0", 1) == 0) break;

                  k++;

                  switch (j)
                  {
                  case 0:
                  case 1:
                  case 2:
                  case 3:
                  case 6:
                  case 7:
                  case 8:
                  case 9:
                  case 10:
                  case 11:
                  case 12:
                  case 13:
                  case 14:
                  case 15:
                     sscanf(lpszTextContent + k, "%d", &iValue);
                     break;

                  case 4:
                  case 5:
                  default:
                     {
                        strncpy(hexStr, lpszTextContent + k, 4);
                        hexStr[4] = '\0';
                        iValue = strtoul(hexStr, NULL, 16);
                     }
                     break;
                  }

                  ((WORD*)&(p->lprgEventObject[(i - 1) * MAX_SCENE_EVENT_OBJECTS + len]))[j] = iValue;
               };

               len++;
            }
            break;

         default:
            break;
         }
      }

      ((WORD*)&(p->rgScene[i - 1]))[nSceneParam] = len;
      UTIL_CloseFile(fpThisScene);
   }
   
   DO_BYTESWAP(p->rgScene, sizeof(p->rgScene));
   DO_BYTESWAP(p->lprgEventObject, l);
}

WORD
PAL_New_GetSceneEventObjectIndex(
   VOID
)
{
   return (gpGlobals->wNumScene - 1) * MAX_SCENE_EVENT_OBJECTS;
}

WORD
PAL_New_GetSceneEventObject(
   WORD     wEventObjectID
)
{
   return PAL_New_GetSceneEventObjectIndex() + wEventObjectID;
}

WORD
PAL_New_GetSceneEventObjectWithScript(
   WORD     wOperand
)
{
   return PAL_New_GetSceneEventObject(wOperand & (MAX_SCENE_EVENT_OBJECTS - 1));
}
#endif

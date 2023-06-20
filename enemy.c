#include "main.h"
#include <math.h>


VOID
PAL_New_CurePoisonForEnemyByKind(
	WORD		wEnemyIndex,
	WORD		wPoisonID
)
{
	int j;

	WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
	if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
	{
		return;
	}

	for (j = 0; j < MAX_POISONS; j++)
	{
		if (g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonID == wPoisonID)
		{
#ifdef POISON_STATUS_EXPAND
			memset(&g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j], 0, sizeof(g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j]));
#else
			g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonID = 0;
			g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonScript = 0;
#endif
			break;
		}
	}
}

VOID
PAL_New_CurePoisonForEnemyByLevel(
	WORD		wEnemyIndex,
	WORD		wMaxLevel
)
{
	int		j;
	WORD       w;

	WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
	if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
	{
		return;
	}

	for (j = 0; j < MAX_POISONS; j++)
	{
		w = g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonID;
		if (gpGlobals->g.rgObject[w].poison.wPoisonLevel <= wMaxLevel)
		{

#ifdef POISON_STATUS_EXPAND
			memset(&g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j], 0, sizeof(g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j]));
#else
			g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonID = 0;
			g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonScript = 0;
#endif
			break;
		}
	}
}


INT
PAL_New_GetPoisonIndexForEnemy(
WORD		wEnemyIndex,
WORD		wPoisonID
)
{
	int i;

	WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
	if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
	{
		return 0;
	}

	for (i = 0; i < MAX_POISONS; i++)
	{
		if (g_Battle.rgEnemy[wEnemyIndex].rgPoisons[i].wPoisonID == wPoisonID)
		{
			return i;
		}
	}
	return -1;
}

VOID
PAL_New_AddPoisonForEnemy(
	WORD		wEnemyIndex,
	WORD		wPoisonID
)
/*++
Purpose:   	对敌方增加指定的毒
Parameters:    [IN]  wEnemyIndex - 敌人在队伍中的序号.
[IN]  wPoisonID - the poison to be added.
Return value:    None.
注：不可以在该函数内进行毒排序，会造成脚本执行混乱
--*/
{
	int         j;
	WORD        w;

	WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
	if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
	{
		return;
	}

	for (j = 0; j < MAX_POISONS; j++)
	{
		if (g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonID == wPoisonID)
		{
#ifdef POISON_STATUS_EXPAND // 增加毒的烈度
			INT iSuccessRate = 100;
			WORD wPoisonLevel = gpGlobals->g.rgObject[wPoisonID].poison.wPoisonLevel;
			iSuccessRate -= wPoisonLevel * 20;
			iSuccessRate = max(iSuccessRate, 0);
			iSuccessRate *= 2;
			if (PAL_New_GetTrueByPercentage(iSuccessRate))
			{
				g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonIntensity++;
			}
			switch (wPoisonLevel)
			{
			case 0:
			case 1:
			case 2:
				g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonIntensity =
					min(g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonIntensity, 3);
				break;
			case 3:
				g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonIntensity =
					min(g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonIntensity, 0);
				break;
			default:
				g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonIntensity =
					min(g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonIntensity, 1);
				break;
			}
#endif
			return;
		}
	}

	for (j = 0; j < MAX_POISONS; j++)
	{
		w = g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonID;
		if (w == 0)
		{
			break;
		}
	}

	j = min(j, MAX_POISONS - 1);
	g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonID = wPoisonID;
	g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonScript =
		PAL_RunTriggerScript(gpGlobals->g.rgObject[wPoisonID].poison.wEnemyScript, wEnemyIndex);

#ifdef POISON_STATUS_EXPAND // 增加毒的烈度
	g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonIntensity = 0;
#endif
}


VOID 
PAL_New_SortPoisonsForEnemyByLevel(
WORD		wEnemyIndex
)
{
	int         i, j, PoisonNum;
	WORD        wPoisonID1, wPoisonID2;
	WORD        wPoisonLevel1, wPoisonLevel2;
	POISONSTATUS	tempPoison;

	WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
	if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
	{
		return;
	}

	for (j = 0, PoisonNum = 0; j < MAX_POISONS; j++)
	{
		wPoisonID1 = g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonID;
		if (wPoisonID1 == 0)
		{
			g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonID = 0;
		}
		else
		{
			PoisonNum++;
		}
	}

	if (PoisonNum < 2)		//中毒数目小于2不用排序
	{
		return;
	}

	for (i = 0; i < MAX_POISONS - 1; i++)
	{
		for (j = 0; j < MAX_POISONS - i - 1; j++)
		{
			wPoisonID1 = g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j].wPoisonID;
			wPoisonLevel1 = gpGlobals->g.rgObject[wPoisonID1].poison.wPoisonLevel;
			wPoisonID2 = g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j + 1].wPoisonID;
			wPoisonLevel2 = gpGlobals->g.rgObject[wPoisonID2].poison.wPoisonLevel;

			if (wPoisonLevel1 < wPoisonLevel2)
			{
				tempPoison = g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j];
				g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j] = g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j + 1];
				g_Battle.rgEnemy[wEnemyIndex].rgPoisons[j + 1] = tempPoison;
			}
		}
	}
	return;
}

WORD 
PAL_New_GetEnemySorceryResistance(
WORD		wEnemyIndex
)
{
	WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
	if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
	{
		return 0;
	}

	WORD w = g_Battle.rgEnemy[wEnemyIndex].e.wResistanceToSorcery * 10;

	if (g_Battle.fIsBoss)
	{
		if (w == 0)
		{
			w += 20;
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

INT
PAL_New_GetEnemyPoisonResistance(
WORD		wEnemyIndex
)
{
	WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
	if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
	{
		return 0;
	}

	INT w = g_Battle.rgEnemy[wEnemyIndex].e.wPoisonResistance * 10;

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

INT
PAL_New_GetEnemyPhysicalResistance(
WORD		wEnemyIndex
)
{
	WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
	if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
	{
		return 0;
	}

	INT w = g_Battle.rgEnemy[wEnemyIndex].e.wPhysicalResistance * 10;

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

INT
PAL_New_GetEnemyElementalResistance(
WORD		wEnemyIndex,
INT			iAttrib
)
{
	WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
	if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
	{
		return 0;
	}

	INT w = g_Battle.rgEnemy[wEnemyIndex].e.wElemResistance[iAttrib] * 10;

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


VOID
PAL_New_SetEnemyStatus(
	WORD		wEnemyIndex,
	WORD		wStatusID,
	WORD		wNumRound
)
{
	WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
	if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
	{
		return;
	}

	WORD wNumRoundNow = g_Battle.rgEnemy[wEnemyIndex].rgwStatus[wStatusID];

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
	case kStatusPuppet:
	case kStatusBravery:
	case kStatusProtect:
	case kStatusDualAttack:
	case kStatusHaste:
	{
		if (wNumRoundNow == 0)
		{
			g_Battle.rgEnemy[wEnemyIndex].rgwStatus[wStatusID] = wNumRound;
		}
		else if (wNumRoundNow <= 2)
		{
			g_Battle.rgEnemy[wEnemyIndex].rgwStatus[wStatusID]++;
		}
		break;
	}

	default:
		assert(FALSE);
		break;
	}
}

INT
PAL_New_GetEnemyLevel(
	WORD		wEnemyIndex
)
{
	WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
	if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
	{
		return 0;
	}

	INT s = (INT)g_Battle.rgEnemy[wEnemyIndex].e.wLevel;
	s += 1;

	if (g_Battle.fIsBoss && s >= 30)
	{
		s += 5;
	}

	s = max(s, 0);
	s = min(s, 0xFFFFFFFF);
	return (INT)s;
}

INT
PAL_New_GetEnemyAttackStrength(
WORD		wEnemyIndex
)
{
	WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
	if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
	{
		return 0;
	}

	INT s = (INT)g_Battle.rgEnemy[wEnemyIndex].e.wAttackStrength ;
	s += PAL_New_GetEnemyLevel(wEnemyIndex) * 8.5;

	if (PAL_New_GetEnemyLevel(wEnemyIndex) > 30 && PAL_New_GetEnemyLevel(wEnemyIndex) <= 89)
	{
		s *= 1.1;
	}
	else if (PAL_New_GetEnemyLevel(wEnemyIndex) >= 90)
	{
		s *= 1.15;
	}
	s = max(s, 0);
	s = min(s, 0xFFFFFFFF);
	return (INT)s;
}

INT
PAL_New_GetEnemyMagicStrength(
WORD		wEnemyIndex
)
{
	WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
	if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
	{
		return 0;
	}

	INT s = (INT)g_Battle.rgEnemy[wEnemyIndex].e.wMagicStrength ;
	s += PAL_New_GetEnemyLevel(wEnemyIndex) * 8;

	if (PAL_New_GetEnemyLevel(wEnemyIndex) > 30 && PAL_New_GetEnemyLevel(wEnemyIndex) <= 89)
	{
		s *= 1.1;
	}
	else if (PAL_New_GetEnemyLevel(wEnemyIndex) >= 90)
	{
		s *= 1.15;
	}
	s = max(s, 0);
	s = min(s, 0xFFFFFFFF);
	return (INT)s;
}

INT
PAL_New_GetEnemyDefense(
WORD		wEnemyIndex
)
{
	WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
	if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
	{
		return 0;
	}

	INT s = (INT)g_Battle.rgEnemy[wEnemyIndex].e.wDefense;
	s += PAL_New_GetEnemyLevel(wEnemyIndex) * 5;
	if ( PAL_New_GetEnemyLevel(wEnemyIndex) > 30 && PAL_New_GetEnemyLevel(wEnemyIndex) <=89)
	{
		s *= 1.1;
	}
	else if (PAL_New_GetEnemyLevel(wEnemyIndex) >= 90)
	{
		s *= 1.15;
	}
	s = max(s, 0);
	s = min(s, 0xFFFFFFFF);
	return (INT)s;
}

INT 
PAL_New_GetEnemyDexterity(
WORD		wEnemyIndex
)
{
	WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
	if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
	{
		return 0;
	}

	INT s = (INT)g_Battle.rgEnemy[wEnemyIndex].e.wDexterity;
	s += PAL_New_GetEnemyLevel(wEnemyIndex) * 5;
	if (PAL_New_GetEnemyLevel(wEnemyIndex) > 20 && PAL_New_GetEnemyLevel(wEnemyIndex) <=89)
	{
		s *= 1.18;
	}
	else if (PAL_New_GetEnemyLevel(wEnemyIndex) >= 90)
	{
		s *= 1.25;
	}

	s = max(s, 0);
	s = min(s, 0xFFFFFFFF);
	return (INT)s;
}

INT
PAL_New_GetEnemyFleeRate(
WORD		wEnemyIndex
)
{
	WORD wObjectID = g_Battle.rgEnemy[wEnemyIndex].wObjectID;
	if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || wObjectID == 0)
	{
		return 0;
	}

	INT s = (INT)(g_Battle.rgEnemy[wEnemyIndex].e.wFleeRate);
	s += PAL_New_GetEnemyLevel(wEnemyIndex) * 4;
	if (PAL_New_GetEnemyLevel(wEnemyIndex) > 20)
	{
		s *= 1.15;
	}

	s = max(s, 0);
	s = min(s, 0xFFFFFFFF);
	return (INT)s;
}
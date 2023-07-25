/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2023, SDLPAL development team.
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
// Based on PALx Project by palxex.
// Copyright (c) 2006-2008, Pal Lockheart <palxex@gmail.com>.
//

#include "main.h"

BOOL            g_fScriptSuccess = TRUE;
static int      g_iCurEquipPart = -1;

static void __pal_team_up(WORD* members) {
	//
	// Set the player party
	//
	gpGlobals->wMaxPartyMemberIndex = 0;

	for (int i = 0; i < MAX_PLAYERS_IN_PARTY; i++)
	{
		if (members[i] != 0)
		{
			gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex].wPlayerRole = (WORD)(members[i] - 1);
			gpGlobals->wMaxPartyMemberIndex++;
		}
	}

	if (gpGlobals->wMaxPartyMemberIndex == 0)
	{
		// HACK for Dream 2.11
		gpGlobals->rgParty[0].wPlayerRole = 0;
		gpGlobals->wMaxPartyMemberIndex = 1;
	}

	gpGlobals->wMaxPartyMemberIndex--;

	//
	// Reload the player sprites
	//
	PAL_SetLoadFlags(kLoadPlayerSprite);
	PAL_LoadResources();

	memset(gpGlobals->rgPoisonStatus, 0, sizeof(gpGlobals->rgPoisonStatus));
	PAL_UpdateEquipments();
}


static WORD
PAL_NEW_CheckAndGetLegalEnemyTarget(
	WORD wEnemyIndex
)
/*++
Check if the object is a legitimate enemy target
--*/
{
	WORD		i;
	if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || g_Battle.rgEnemy[wEnemyIndex].wObjectID == 0)
	{
		do
		{
			i = (WORD)RandomLong(0, g_Battle.wMaxEnemyIndex);
		} while (g_Battle.rgEnemy[i].wObjectID == 0);

		return i;
	}
	else
	{
		return wEnemyIndex;
	}
}

static WORD
PAL_NEW_CheckAndGetLegalPlayerTarget(
	WORD wPlayerRole
)
/*++
Check if the object is a legitimate target of our role
--*/
{
	INT		i;
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		if (wPlayerRole == gpGlobals->rgParty[i].wPlayerRole)
		{
			return wPlayerRole;
		}
	}

	if (i > gpGlobals->wMaxPartyMemberIndex)
	{
		i = RandomLong(0, gpGlobals->wMaxPartyMemberIndex);
		wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;
	}
	return wPlayerRole;
}

static BOOL
PAL_NPCWalkTo(
	WORD           wEventObjectID,
	INT            x,
	INT            y,
	INT            h,
	INT            iSpeed
)
/*++
  Purpose:

	Make the specified event object walk to the map position specified by (x, y, h)
	at the speed of iSpeed.

  Parameters:

	[IN]  wEventObjectID - the event object to move.

	[IN]  x - Column number of the tile.

	[IN]  y - Line number in the map.

	[IN]  h - Each line in the map has two lines of tiles, 0 and 1.
			  (See map.h for details.)

	[IN]  iSpeed - the speed to move.

  Return value:

	TRUE if the event object has successfully moved to the specified position,
	FALSE if still need more moving.

--*/
{
	LPEVENTOBJECT    pEvtObj;
	int              xOffset, yOffset;

	pEvtObj = &(gpGlobals->g.lprgEventObject[wEventObjectID - 1]);

	xOffset = (x * 32 + h * 16) - pEvtObj->x;
	yOffset = (y * 16 + h * 8) - pEvtObj->y;

	if (yOffset < 0)
	{
		pEvtObj->wDirection = ((xOffset < 0) ? kDirWest : kDirNorth);
	}
	else
	{
		pEvtObj->wDirection = ((xOffset < 0) ? kDirSouth : kDirEast);
	}

	if (abs(xOffset) < iSpeed * 2 || abs(yOffset) < iSpeed * 2)
	{
		pEvtObj->x = x * 32 + h * 16;
		pEvtObj->y = y * 16 + h * 8;
	}
	else
	{
		PAL_NPCWalkOneStep(wEventObjectID, iSpeed);
	}

	if (pEvtObj->x == x * 32 + h * 16 && pEvtObj->y == y * 16 + h * 8)
	{
		pEvtObj->wCurrentFrameNum = 0;
		return TRUE;
	}

	return FALSE;
}

static VOID
PAL_PartyWalkTo(
	INT            x,
	INT            y,
	INT            h,
	INT            iSpeed
)
/*++
  Purpose:

	Make the party walk to the map position specified by (x, y, h)
	at the speed of iSpeed.

  Parameters:

	[IN]  x - Column number of the tile.

	[IN]  y - Line number in the map.

	[IN]  h - Each line in the map has two lines of tiles, 0 and 1.
			  (See map.h for details.)

	[IN]  iSpeed - the speed to move.

  Return value:

	None.

--*/
{
	int           xOffset, yOffset, i, dx, dy;
	DWORD         t;

	xOffset = x * 32 + h * 16 - PAL_X(gpGlobals->viewport) - PAL_X(gpGlobals->partyoffset);
	yOffset = y * 16 + h * 8 - PAL_Y(gpGlobals->viewport) - PAL_Y(gpGlobals->partyoffset);

	t = 0;

	while (xOffset != 0 || yOffset != 0)
	{
		PAL_DelayUntil(t);

		t = SDL_GetTicks() + FRAME_TIME;

		//
		// Store trail
		//
		for (i = 3; i >= 0; i--)
		{
			gpGlobals->rgTrail[i + 1] = gpGlobals->rgTrail[i];
		}
		gpGlobals->rgTrail[0].wDirection = gpGlobals->wPartyDirection;
		gpGlobals->rgTrail[0].x = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
		gpGlobals->rgTrail[0].y = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

		if (yOffset < 0)
		{
			gpGlobals->wPartyDirection = ((xOffset < 0) ? kDirWest : kDirNorth);
		}
		else
		{
			gpGlobals->wPartyDirection = ((xOffset < 0) ? kDirSouth : kDirEast);
		}

		dx = PAL_X(gpGlobals->viewport);
		dy = PAL_Y(gpGlobals->viewport);

		if (abs(xOffset) <= iSpeed * 2)
		{
			dx += xOffset;
		}
		else
		{
			dx += iSpeed * (xOffset < 0 ? -2 : 2);
		}

		if (abs(yOffset) <= iSpeed)
		{
			dy += yOffset;
		}
		else
		{
			dy += iSpeed * (yOffset < 0 ? -1 : 1);
		}

		//
		// Move the viewport
		//
		gpGlobals->viewport = PAL_XY(dx, dy);

		PAL_UpdatePartyGestures(TRUE);
		PAL_GameUpdate(FALSE);
		PAL_MakeScene();
		VIDEO_UpdateScreen(NULL);

		xOffset = x * 32 + h * 16 - PAL_X(gpGlobals->viewport) - PAL_X(gpGlobals->partyoffset);
		yOffset = y * 16 + h * 8 - PAL_Y(gpGlobals->viewport) - PAL_Y(gpGlobals->partyoffset);
	}

	PAL_UpdatePartyGestures(FALSE);
}

static VOID
PAL_PartyRideEventObject(
	WORD           wEventObjectID,
	INT            x,
	INT            y,
	INT            h,
	INT            iSpeed
)
/*++
  Purpose:

	Move the party to the specified position, riding the specified event object.

  Parameters:

	[IN]  wEventObjectID - the event object to be ridden.

	[IN]  x - Column number of the tile.

	[IN]  y - Line number in the map.

	[IN]  h - Each line in the map has two lines of tiles, 0 and 1.
			  (See map.h for details.)

	[IN]  iSpeed - the speed to move.

  Return value:

	TRUE if the party and event object has successfully moved to the specified
	position, FALSE if still need more moving.

--*/
{
	int              xOffset, yOffset, dx, dy, i;
	DWORD            t;
	LPEVENTOBJECT    p;

	p = &(gpGlobals->g.lprgEventObject[wEventObjectID - 1]);

	xOffset = x * 32 + h * 16 - PAL_X(gpGlobals->viewport) - PAL_X(gpGlobals->partyoffset);
	yOffset = y * 16 + h * 8 - PAL_Y(gpGlobals->viewport) - PAL_Y(gpGlobals->partyoffset);

	t = 0;

	while (xOffset != 0 || yOffset != 0)
	{
		PAL_DelayUntil(t);

		t = SDL_GetTicks() + FRAME_TIME;

		if (yOffset < 0)
		{
			gpGlobals->wPartyDirection = ((xOffset < 0) ? kDirWest : kDirNorth);
		}
		else
		{
			gpGlobals->wPartyDirection = ((xOffset < 0) ? kDirSouth : kDirEast);
		}

		if (abs(xOffset) > iSpeed * 2)
		{
			dx = iSpeed * (xOffset < 0 ? -2 : 2);
		}
		else
		{
			dx = xOffset;
		}

		if (abs(yOffset) > iSpeed)
		{
			dy = iSpeed * (yOffset < 0 ? -1 : 1);
		}
		else
		{
			dy = yOffset;
		}

		//
		// Store trail
		//
		for (i = 3; i >= 0; i--)
		{
			gpGlobals->rgTrail[i + 1] = gpGlobals->rgTrail[i];
		}

		gpGlobals->rgTrail[0].wDirection = gpGlobals->wPartyDirection;
		gpGlobals->rgTrail[0].x = PAL_X(gpGlobals->viewport) + dx + PAL_X(gpGlobals->partyoffset);
		gpGlobals->rgTrail[0].y = PAL_Y(gpGlobals->viewport) + dy + PAL_Y(gpGlobals->partyoffset);

		//
		// Move the viewport
		//
		gpGlobals->viewport =
			PAL_XY(PAL_X(gpGlobals->viewport) + dx, PAL_Y(gpGlobals->viewport) + dy);

		p->x += dx;
		p->y += dy;

		PAL_GameUpdate(FALSE);
		PAL_MakeScene();
		VIDEO_UpdateScreen(NULL);

		xOffset = x * 32 + h * 16 - PAL_X(gpGlobals->viewport) - PAL_X(gpGlobals->partyoffset);
		yOffset = y * 16 + h * 8 - PAL_Y(gpGlobals->viewport) - PAL_Y(gpGlobals->partyoffset);
	}
}

static VOID
PAL_MonsterChasePlayer(
	WORD         wEventObjectID,
	WORD         wSpeed,
	WORD         wChaseRange,
	BOOL         fFloating
)
/*++
  Purpose:

	Make the specified event object chase the players.

  Parameters:

	[IN]  wEventObjectID - the event object ID of the monster.

	[IN]  wSpeed - the speed of chasing.

	[IN]  wChaseRange - sensitive range of the monster.

	[IN]  fFloating - TRUE if monster is floating (i.e., ignore the obstacles)

  Return value:

	None.

--*/
{
	LPEVENTOBJECT    pEvtObj = &gpGlobals->g.lprgEventObject[wEventObjectID - 1];
	WORD             wMonsterSpeed = 0, prevx, prevy;
	int              x, y, i, j, l;

	if (gpGlobals->wChaseRange != 0)
	{
		x = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset) - pEvtObj->x;
		y = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset) - pEvtObj->y;

		if (x == 0)
		{
			x = RandomLong(0, 1) ? -1 : 1;
		}

		if (y == 0)
		{
			y = RandomLong(0, 1) ? -1 : 1;
		}

		prevx = pEvtObj->x;
		prevy = pEvtObj->y;

		i = prevx % 32;
		j = prevy % 16;

		prevx /= 32;
		prevy /= 16;
		l = 0;

		if (i + j * 2 >= 16)
		{
			if (i + j * 2 >= 48)
			{
				prevx++;
				prevy++;
			}
			else if (32 - i + j * 2 < 16)
			{
				prevx++;
			}
			else if (32 - i + j * 2 < 48)
			{
				l = 1;
			}
			else
			{
				prevy++;
			}
		}

		prevx = prevx * 32 + l * 16;
		prevy = prevy * 16 + l * 8;

		//
		// Is the party near to the event object?
		//
		if (abs(x) + abs(y) * 2 < wChaseRange * 32 * gpGlobals->wChaseRange)
		{
			if (x < 0)
			{
				if (y < 0)
				{
					pEvtObj->wDirection = kDirWest;
				}
				else
				{
					pEvtObj->wDirection = kDirSouth;
				}
			}
			else
			{
				if (y < 0)
				{
					pEvtObj->wDirection = kDirNorth;
				}
				else
				{
					pEvtObj->wDirection = kDirEast;
				}
			}

			if (x != 0)
			{
				x = pEvtObj->x + x / abs(x) * 16;
			}
			else
			{
				x = pEvtObj->x;
			}

			if (y != 0)
			{
				y = pEvtObj->y + y / abs(y) * 8;
			}
			else
			{
				y = pEvtObj->y;
			}

			if (fFloating)
			{
				wMonsterSpeed = wSpeed;
			}
			else
			{
				if (!PAL_CheckObstacle(PAL_XY(x, y), TRUE, wEventObjectID))
				{
					wMonsterSpeed = wSpeed;
				}
				else
				{
					pEvtObj->x = prevx;
					pEvtObj->y = prevy;
				}

				for (l = 0; l < 4; l++)
				{
					switch (l)
					{
					case 0:
						pEvtObj->x -= 4;
						pEvtObj->y += 2;
						break;

					case 1:
						pEvtObj->x -= 4;
						pEvtObj->y -= 2;
						break;

					case 2:
						pEvtObj->x += 4;
						pEvtObj->y -= 2;
						break;

					case 3:
						pEvtObj->x += 4;
						pEvtObj->y += 2;
						break;
					}

					if (PAL_CheckObstacle(PAL_XY(pEvtObj->x, pEvtObj->y), FALSE, 0))
					{
						pEvtObj->x = prevx;
						pEvtObj->y = prevy;
					}
				}
			}
		}
	}

	PAL_NPCWalkOneStep(wEventObjectID, wMonsterSpeed);
}

VOID
PAL_AdditionalCredits(
	VOID
)
/*++
  Purpose:

	Show the additional credits.

  Parameters:

	None.

  Return value:

	None.

--*/
{
	LPCWSTR rgszcps[][CP_MAX] = {
		// Traditional Chinese, Simplified Chinese
		{ L"", L"", /*L""*/ },
		{ L"         經典特別篇   ",
		  L"         经典特别篇   ",
		  //L"   \x30AF\x30E9\x30B7\x30C3\x30AF\x7279\x5225\x7DE8  "
		},
		{ L"", L"", /*L""*/ },
		{ L"", L"", /*L""*/ },
		{ L"", L"", /*L""*/ },
		{ L"", L"", /*L""*/ },
		{ L"", L"", /*L""*/ },
		{ L"", L"", /*L""*/ },
		{ L"   本程式是自由軟體，按照 GNU General",
		  L"   本程序是自由软件，按照 GNU General",
		  //L" \x3053\x306E\x30D7\x30ED\x30B0\x30E9\x30E0\x306F\x81EA\x7531\x30BD\x30D5\x30C8\x30A6\x30A7\x30A2\x3067\x3059\x3001"
		},
		{ L"   Public License v3 或更高版本發佈",
		  L"   Public License v3 或更高版本发布",
		  //L" GNU General Public License v3 \x306E\x4E0B\x3067"
		},
		{ L"", L"", /*L" \x914D\x5E03\x3055\x308C\x3066\x3044\x307E\x3059\x3002"*/ },
		{ L"                    ...按 Enter 結束",
		  L"                    ...按 Enter 结束",
		  //L"      ...Enter\x30AD\x30FC\x3092\x62BC\x3057\x3066\x7D42\x4E86\x3057\x307E\x3059"
		},
	};

	LPCWSTR rgszStrings[] = {
	   L"  SDLPAL (http://sdlpal.github.io/)",
 #ifdef PAL_CLASSIC
	   L"%ls(" WIDETEXT(__DATE__) L")",
 #else
	   L"                        (" WIDETEXT(__DATE__) L")",
 #endif
	   L" ",
	   L"    (c) 2009-2011, Wei Mingzhi",
	   L"        <whistler_wmz@users.sf.net>.",
	   L"    (c) 2011-2021, SDLPAL Team",
	   L"%ls",  // Porting information line 1
	   L"%ls",  // Porting information line 2
	   L"%ls",  // GNU line 1
	   L"%ls",  // GNU line 2
	   L"%ls",  // GNU line 3
	   L"%ls",  // Press Enter to continue
	};

	int        i = 0;

	PAL_DrawOpeningMenuBackground();

	for (i = 0; i < 12; i++)
	{
		WCHAR buffer[48];
		PAL_swprintf(buffer, sizeof(buffer) / sizeof(WCHAR), rgszStrings[i], gConfig.pszMsgFile ? g_rcCredits[i] : rgszcps[i][PAL_GetCodePage()]);
		PAL_DrawText(buffer, PAL_XY(0, 2 + i * 16), DESCTEXT_COLOR, TRUE, FALSE, FALSE);
	}

	PAL_SetPalette(0, FALSE);
	VIDEO_UpdateScreen(NULL);

	PAL_WaitForKey(0);
}

static WORD
PAL_InterpretInstruction(
	WORD           wScriptEntry,
	WORD           wEventObjectID
)
/*++
  Purpose:

	Interpret and execute one instruction in the script.

  Parameters:

	[IN]  wScriptEntry - The script entry to execute.

	[IN]  wEventObjectID - The event object ID which invoked the script.

  Return value:

	The address of the next script instruction to execute.

--*/
{
	LPEVENTOBJECT          pEvtObj, pCurrent;
	LPSCRIPTENTRY          pScript;
	int                    iPlayerRole, i, j, x, y;
	WORD                   w, wCurEventObjectID;
	WORD                   wPlayerRole;
	WCHAR s[256] = L"";

	pScript = &(gpGlobals->g.lprgScriptEntry[wScriptEntry]);

	if (wEventObjectID != 0)
	{
		pEvtObj = &(gpGlobals->g.lprgEventObject[wEventObjectID - 1]);
	}
	else
	{
		pEvtObj = NULL;
	}

	if (pScript->rgwOperand[0] == 0 || pScript->rgwOperand[0] == 0xFFFF)
	{
		pCurrent = pEvtObj;
		wCurEventObjectID = wEventObjectID;
	}
	else
	{
		i = pScript->rgwOperand[0] - 1;
		if (i > 0x9000)
		{
			// HACK for Dream 2.11 to avoid crash
			i -= 0x9000;
		}
		pCurrent = &(gpGlobals->g.lprgEventObject[i]);
		wCurEventObjectID = pScript->rgwOperand[0];
	}

	if (pScript->rgwOperand[0] < MAX_PLAYABLE_PLAYER_ROLES)
	{
		iPlayerRole = gpGlobals->rgParty[pScript->rgwOperand[0]].wPlayerRole;
	}
	else
	{
		iPlayerRole = gpGlobals->rgParty[0].wPlayerRole;
	}

	switch (pScript->wOperation)
	{
	case 0x000B:
	case 0x000C:
	case 0x000D:
	case 0x000E:
		//
		// walk one step
		//
	{
		if (pEvtObj == NULL)
		{
			break;
		}
		pEvtObj->wDirection = pScript->wOperation - 0x000B;
		PAL_NPCWalkOneStep(wEventObjectID, 2);
		break;
	}

	case 0x000F:
		//
		// Set the direction and/or gesture for event object
		//
		if (pScript->rgwOperand[0] != 0xFFFF)
		{
			pEvtObj->wDirection = pScript->rgwOperand[0];
		}
		if (pScript->rgwOperand[1] != 0xFFFF)
		{
			pEvtObj->wCurrentFrameNum = pScript->rgwOperand[1];
		}
		break;

	case 0x0010:
		//
		// Walk straight to the specified position
		//
		if (!PAL_NPCWalkTo(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
			pScript->rgwOperand[2], 3))
		{
			wScriptEntry--;
		}
		break;

	case 0x0011:
		//
		// Walk straight to the specified position, at a lower speed
		//
		if ((wEventObjectID & 1) ^ (gpGlobals->dwFrameNum & 1))
		{
			if (!PAL_NPCWalkTo(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
				pScript->rgwOperand[2], 4))
			{
				wScriptEntry--;
			}
		}
		else
		{
			wScriptEntry--;
		}
		break;

	case 0x0012:
		//
		// Set the position of the event object, relative to the party
		//
		pCurrent->x =
			pScript->rgwOperand[1] + PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
		pCurrent->y =
			pScript->rgwOperand[2] + PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);
		break;

	case 0x0013:
		//
		// Set the position of the event object
		//
		pCurrent->x = pScript->rgwOperand[1];
		pCurrent->y = pScript->rgwOperand[2];
		break;

	case 0x0014:
		//
		// Set the gesture of the event object
		//
		pEvtObj->wCurrentFrameNum = pScript->rgwOperand[0];
		pEvtObj->wDirection = kDirSouth;
		break;

	case 0x0015:
		//
		// Set the direction and gesture for a party member
		//
		gpGlobals->wPartyDirection = pScript->rgwOperand[0];
		gpGlobals->rgParty[pScript->rgwOperand[2]].wFrame =
			gpGlobals->wPartyDirection * 3 + pScript->rgwOperand[1];
		break;

	case 0x0016:
		//
		// Set the direction and gesture for an event object
		//
		if (pScript->rgwOperand[0] != 0)
		{
			pCurrent->wDirection = pScript->rgwOperand[1];
			pCurrent->wCurrentFrameNum = pScript->rgwOperand[2];
		}
		break;

	case 0x0017:
		//
		// set the player's extra attribute
		//
	{
		INT* p;

		i = pScript->rgwOperand[0] - 0xB;

		p = (INT*)(&gpGlobals->rgEquipmentEffect[i]); // HACKHACK

		p[pScript->rgwOperand[1] * MAX_PLAYER_ROLES + wEventObjectID] =
			(SHORT)pScript->rgwOperand[2];
	}
	break;

	case 0x0018:
		//
		// Equip the selected item
		//
		i = pScript->rgwOperand[0] - 0x0B;
		g_iCurEquipPart = i;

		//
		// The wEventObjectID parameter here should indicate the player role
		//
		PAL_RemoveEquipmentEffect(wEventObjectID, i);

		if (gpGlobals->g.PlayerRoles.rgwEquipment[i][wEventObjectID] != pScript->rgwOperand[1])
		{
			w = gpGlobals->g.PlayerRoles.rgwEquipment[i][wEventObjectID];
			gpGlobals->g.PlayerRoles.rgwEquipment[i][wEventObjectID] = pScript->rgwOperand[1];

			PAL_AddItemToInventory(pScript->rgwOperand[1], -1);

			if (w != 0)
			{
				PAL_AddItemToInventory(w, 1);
			}

			gpGlobals->wLastUnequippedItem = w;
		}
		break;

	case 0x0019:
		//
		// Increase/decrease the player's attribute
		//
	{
		INT* p = (INT*)(&gpGlobals->g.PlayerRoles); // HACKHACK

		if (pScript->rgwOperand[2] == 0)
		{
			iPlayerRole = wEventObjectID;
		}
		else
		{
			iPlayerRole = pScript->rgwOperand[2] - 1;
		}

		p[pScript->rgwOperand[0] * MAX_PLAYER_ROLES + iPlayerRole] +=
			(SHORT)pScript->rgwOperand[1];
	}
	break;

	case 0x001A:
		//
		// Set player's stat
		//
	{
		INT* p = (INT*)(&gpGlobals->g.PlayerRoles); // HACKHACK

		if (g_iCurEquipPart != -1)
		{
			//
			// In the progress of equipping items
			//
			p = (INT*)&(gpGlobals->rgEquipmentEffect[g_iCurEquipPart]);
		}

		if (pScript->rgwOperand[2] == 0)
		{
			//
			// Apply to the current player. The wEventObjectID should
			// indicate the player role.
			//
			iPlayerRole = wEventObjectID;
		}
		else
		{
			iPlayerRole = pScript->rgwOperand[2] - 1;
		}

		p[pScript->rgwOperand[0] * MAX_PLAYER_ROLES + iPlayerRole] =
			(SHORT)pScript->rgwOperand[1];
	}
	break;

	case 0x001B:
		//
		// Increase/decrease player's HP
		//
		if (pScript->rgwOperand[0])
		{
			g_fScriptSuccess = FALSE;
			//
			// Apply to everyone
			//
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				w = gpGlobals->rgParty[i].wPlayerRole;
				if (PAL_IncreaseHPMP(w, (SHORT)(pScript->rgwOperand[1]), 0))
					g_fScriptSuccess = TRUE;
			}
		}
		else
		{
			//
			// Apply to one player. The wEventObjectID parameter should indicate the player role.
			//
			if (!PAL_IncreaseHPMP(wEventObjectID, (SHORT)(pScript->rgwOperand[1]), 0))
			{
				g_fScriptSuccess = FALSE;
			}
		}
		break;

	case 0x001C:
		//
		// Increase/decrease player's MP
		//
		if (pScript->rgwOperand[0])
		{
			//
			// Apply to everyone
			//
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				w = gpGlobals->rgParty[i].wPlayerRole;
				PAL_IncreaseHPMP(w, 0, (SHORT)(pScript->rgwOperand[1]));
			}
		}
		else
		{
			//
			// Apply to one player. The wEventObjectID parameter should indicate the player role.
			//
			if (!PAL_IncreaseHPMP(wEventObjectID, 0, (SHORT)(pScript->rgwOperand[1])))
			{
				g_fScriptSuccess = FALSE;
			}
		}
		break;

	case 0x001D:
		//
		// Increase/decrease player's HP and MP
		//
		if (pScript->rgwOperand[0])
		{
			//
			// Apply to everyone
			//
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				w = gpGlobals->rgParty[i].wPlayerRole;
				PAL_IncreaseHPMP(w,
					(SHORT)(pScript->rgwOperand[1]), (SHORT)(pScript->rgwOperand[2]));
			}
		}
		else
		{
			//
			// Apply to one player. The wEventObjectID parameter should indicate the player role.
			//
			if (!PAL_IncreaseHPMP(wEventObjectID,
				(SHORT)(pScript->rgwOperand[1]), (SHORT)(pScript->rgwOperand[2])))
			{
				g_fScriptSuccess = FALSE;
			}
		}
		break;

	case 0x001E:
		//
		// Increase or decrease cash by the specified amount
		//
		if ((SHORT)(pScript->rgwOperand[0]) < 0 &&
			gpGlobals->dwCash < (WORD)(-(SHORT)(pScript->rgwOperand[0])))
		{
			//
			// not enough cash
			//
			wScriptEntry = pScript->rgwOperand[1] - 1;
		}
		else
		{
			gpGlobals->dwCash += (SHORT)(pScript->rgwOperand[0]);
		}
		break;

	case 0x001F:
		//
		// Add item to inventory
		//
		PAL_AddItemToInventory(pScript->rgwOperand[0], (SHORT)(pScript->rgwOperand[1]));
		break;

	case 0x0020:
		//
		// Remove item from inventory
		//
		x = pScript->rgwOperand[1];
		if (x == 0)
		{
			x = 1;
		}
		if (x <= PAL_CountItem(pScript->rgwOperand[0]) || pScript->rgwOperand[2] == 0)
		{
			if (!PAL_AddItemToInventory(pScript->rgwOperand[0], -x))
			{
				//
				// Try removing equipped item
				//
				for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
				{
					w = gpGlobals->rgParty[i].wPlayerRole;

					for (j = 0; j < MAX_PLAYER_EQUIPMENTS; j++)
					{
						if (gpGlobals->g.PlayerRoles.rgwEquipment[j][w] == pScript->rgwOperand[0])
						{
							PAL_RemoveEquipmentEffect(w, j);
							gpGlobals->g.PlayerRoles.rgwEquipment[j][w] = 0;

							if (--x == 0)
							{
								i = 9999;
								break;
							}
						}
					}
				}
			}
		}
		else
			wScriptEntry = pScript->rgwOperand[2] - 1;
		break;

	case 0x0021:
		//
		// Inflict damage to the enemy
		//
		if (pScript->rgwOperand[0])
		{
			//
			// Inflict damage to all enemies
			//
			for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
			{
				if (g_Battle.rgEnemy[i].wObjectID != 0)
				{
					g_Battle.rgEnemy[i].e.wHealth -= pScript->rgwOperand[1];
				}
			}
		}
		else
		{
			//
			// Inflict damage to one enemy
			//
			g_Battle.rgEnemy[wEventObjectID].e.wHealth -= pScript->rgwOperand[1];
		}
		break;

	case 0x0022:
		//
		// Revive player
		//
		if (pScript->rgwOperand[0])
		{
			//
			// Apply to everyone
			//
			g_fScriptSuccess = FALSE;

			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				w = gpGlobals->rgParty[i].wPlayerRole;
				if (gpGlobals->g.PlayerRoles.rgwHP[w] == 0)
				{
					gpGlobals->g.PlayerRoles.rgwHP[w] =
						PAL_GetPlayerMaxHP(w) * pScript->rgwOperand[1] / 10;

					PAL_CurePoisonByLevel(w, 10);
					for (x = 0; x < kStatusAll; x++)
					{
						PAL_RemovePlayerStatus(w, x);
					}

					g_fScriptSuccess = TRUE;
				}
			}
		}
		else
		{
			//
			// Apply to one player
			//
			if (gpGlobals->g.PlayerRoles.rgwHP[wEventObjectID] == 0)
			{
				gpGlobals->g.PlayerRoles.rgwHP[wEventObjectID] =
					PAL_GetPlayerMaxHP(wEventObjectID) * pScript->rgwOperand[1] / 10;

				PAL_CurePoisonByLevel(wEventObjectID, 10);
				for (x = 0; x < kStatusAll; x++)
				{
					PAL_RemovePlayerStatus(wEventObjectID, x);
				}
			}
			else
			{
				g_fScriptSuccess = FALSE;
			}
		}
		break;

	case 0x0023:
		//
		// Remove equipment from the specified player
		//
		if (pScript->rgwOperand[1] == 0)
		{
			//
			// Remove all equipments
			//
			for (i = 0; i < MAX_PLAYER_EQUIPMENTS; i++)
			{
				w = gpGlobals->g.PlayerRoles.rgwEquipment[i][iPlayerRole];
				if (w != 0)
				{
					PAL_AddItemToInventory(w, 1);
					gpGlobals->g.PlayerRoles.rgwEquipment[i][iPlayerRole] = 0;
				}
				PAL_RemoveEquipmentEffect(iPlayerRole, i);
			}
		}
		else
		{
			w = gpGlobals->g.PlayerRoles.rgwEquipment[pScript->rgwOperand[1] - 1][iPlayerRole];
			if (w != 0)
			{
				PAL_RemoveEquipmentEffect(iPlayerRole, pScript->rgwOperand[1] - 1);
				PAL_AddItemToInventory(w, 1);
				gpGlobals->g.PlayerRoles.rgwEquipment[pScript->rgwOperand[1] - 1][iPlayerRole] = 0;
			}
		}
		break;

	case 0x0024:
		//
		// Set the autoscript entry address for an event object
		//
		if (pScript->rgwOperand[0] != 0)
		{
			pCurrent->wAutoScript = pScript->rgwOperand[1];
		}
		break;

	case 0x0025:
		//
		// Set the trigger script entry address for an event object
		//
		if (pScript->rgwOperand[0] != 0)
		{
			pCurrent->wTriggerScript = pScript->rgwOperand[1];
		}
		break;

	case 0x0026:
		//
		// Show the buy item menu
		//
		PAL_MakeScene();
		VIDEO_UpdateScreen(NULL);
		PAL_BuyMenu(pScript->rgwOperand[0]);
		break;

	case 0x0027:
		//
		// Show the sell item menu
		//
		PAL_MakeScene();
		VIDEO_UpdateScreen(NULL);
		PAL_SellMenu();
		break;

	case 0x0028:
		/*
		28 参数1 参数2      敌方中毒指令
		参数1 是否全体
		参数2 中毒代号
		作用      敌方选定对象或全体中参数2代表的毒
		*/
	{
		BOOL fAll = (pScript->rgwOperand[0] == 0) ? FALSE : TRUE;
		WORD wPoisonID = pScript->rgwOperand[1];
		BOOL fJump = FALSE;
		BOOL fAlwaysSuccess = FALSE;
		WORD wPoisonResistance = 0;
		INT iSuccessRate = 0;
		WORD wBaseSuccessRate = 100;

		if (g_Battle.fPlayerMoving)
		{
			wPlayerRole = gpGlobals->rgParty[g_Battle.wMovingPlayerIndex].wPlayerRole;
			wPlayerRole = PAL_NEW_CheckAndGetLegalPlayerTarget(wPlayerRole);
		}

#ifdef ADD_SOME_POISONS_SUCCESSFULLY_ANYTIME
		if (gpGlobals->g.rgObject[pScript->rgwOperand[1]].poison.wPoisonLevel >= 99)
		{
			fAlwaysSuccess = TRUE;
		}
#endif

		if (fAll)
		{
			// 全体
			for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
			{
				w = g_Battle.rgEnemy[i].wObjectID;

				if (w != 0)
				{
					wPoisonResistance = PAL_New_GetEnemyPoisonResistance(i);
					iSuccessRate = wBaseSuccessRate - wPoisonResistance;

					if (fAlwaysSuccess || PAL_New_GetTrueByPercentage(iSuccessRate))
					{
						PAL_New_AddPoisonForEnemy(i, wPoisonID);
					}

#ifdef ADD_POISON_FAIL_THEN_JUMP
					else if (pScript->rgwOperand[2] != 0x0000 && pScript->rgwOperand[2] != 0xffff)
					{
						fJump = TRUE;
					}
#endif
				}
			}
		}
		else
		{
			//单体
			wEventObjectID = PAL_NEW_CheckAndGetLegalEnemyTarget(wEventObjectID);
			wPoisonResistance = PAL_New_GetEnemyPoisonResistance(wEventObjectID);
			iSuccessRate = wBaseSuccessRate - wPoisonResistance;

			if (fAlwaysSuccess || PAL_New_GetTrueByPercentage(iSuccessRate))
			{
				PAL_New_AddPoisonForEnemy(wEventObjectID, wPoisonID);
			}

#ifdef ADD_POISON_FAIL_THEN_JUMP
			else if (pScript->rgwOperand[2] != 0x0000 && pScript->rgwOperand[2] != 0xffff)
			{
				fJump = TRUE;
			}
		}

		if (fJump == TRUE)
		{
			wScriptEntry = pScript->rgwOperand[2] - 1;

		}
#else
	}
#endif
		break;
	}

	case 0x0029:
		//
		// Apply poison to player
		// 我方单人或者全体中毒
	{
		BOOL fAll = (pScript->rgwOperand[0] == 0) ? FALSE : TRUE;
		WORD wPoisonID = pScript->rgwOperand[1];
		BOOL fJump = FALSE;
		BOOL fAlwaysSuccess = FALSE;
		INT wPoisonResistance = 0;

#ifdef ADD_SOME_POISONS_SUCCESSFULLY_ANYTIME
		if (gpGlobals->g.rgObject[pScript->rgwOperand[1]].poison.wPoisonLevel >= 5)
		{
			fAlwaysSuccess = TRUE;
		}
#endif

		if (fAll)
		{	// Apply to everyone
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				w = gpGlobals->rgParty[i].wPlayerRole;
				wPoisonResistance = PAL_GetPlayerPoisonResistance(w);
				if (fAlwaysSuccess || !PAL_New_GetTrueByPercentage(wPoisonResistance))
				{
					PAL_AddPoisonForPlayer(w, pScript->rgwOperand[1]);
					if (pScript->rgwOperand[1] == 556)
					{
						PAL_SetPlayerStatus(w, 11, 320);
					}
					if (pScript->rgwOperand[1] == 559)
					{
						PAL_SetPlayerStatus(w, 10, 320);
					}
					if (pScript->rgwOperand[1] == 557)
					{
						PAL_SetPlayerStatus(w, 12, 320);
					}
					if (pScript->rgwOperand[1] == 558)
					{
						PAL_SetPlayerStatus(w, 13, 320);
					}
				}
#ifdef ADD_POISON_FAIL_THEN_JUMP
				else if (pScript->rgwOperand[2] != 0x0000 && pScript->rgwOperand[2] != 0xffff)
				{
					fJump = TRUE;
				}
#endif
			}
		}
		else
		{	// Apply to one player
			wPoisonResistance = PAL_GetPlayerPoisonResistance(wEventObjectID);
			if (fAlwaysSuccess || !PAL_New_GetTrueByPercentage(wPoisonResistance))
			{
				PAL_AddPoisonForPlayer(wEventObjectID, pScript->rgwOperand[1]);
				if (pScript->rgwOperand[1] == 556)
				{
					PAL_SetPlayerStatus(wEventObjectID, 11, 320);
				}
				if (pScript->rgwOperand[1] == 559)
				{
					PAL_SetPlayerStatus(wEventObjectID, 10, 320);
				}
				if (pScript->rgwOperand[1] == 557)
				{
					PAL_SetPlayerStatus(wEventObjectID, 12, 320);
				}
				if (pScript->rgwOperand[1] == 558)
				{
					PAL_SetPlayerStatus(wEventObjectID, 13, 320);
				}
			}
#ifdef ADD_POISON_FAIL_THEN_JUMP
			else if (pScript->rgwOperand[2] != 0x0000 && pScript->rgwOperand[2] != 0xffff)
			{
				fJump = TRUE;
			}
		}

		if (fJump == TRUE)
		{
			wScriptEntry = pScript->rgwOperand[2] - 1;
		}
#else
	}
#endif
		break;
	}

	case 0x002A:
	{
		/*
		2A 参数1 参数2      敌方解毒指令
		参数1 是否全体
		参数2 中毒代号
		作用      敌方选定对象或全体解除参数2代表的毒
		*/
		BOOL fAll = (pScript->rgwOperand[0] == 0) ? FALSE : TRUE;
		WORD wPoisonID = pScript->rgwOperand[1];

		if (fAll)
		{	// Apply to all enemies
			for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
			{
				if (g_Battle.rgEnemy[i].wObjectID != 0)
				{
					PAL_New_CurePoisonForEnemyByKind(i, wPoisonID);
				}
			}
		}
		else
		{	// Apply to one enemy
			PAL_New_CurePoisonForEnemyByKind(wEventObjectID, wPoisonID);
		}

		break;
	}

	case 0x002B:
		//
		// Cure poison by object ID for player
		//
		if (pScript->rgwOperand[0])
		{
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				w = gpGlobals->rgParty[i].wPlayerRole;
				PAL_CurePoisonByKind(w, pScript->rgwOperand[1]);
			}
		}
		else
		{
			PAL_CurePoisonByKind(wEventObjectID, pScript->rgwOperand[1]);
		}
		break;

	case 0x002C:
		//
		// Cure poisons by level
		//
		if (pScript->rgwOperand[0])
		{
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				w = gpGlobals->rgParty[i].wPlayerRole;
				PAL_CurePoisonByLevel(w, pScript->rgwOperand[1]);
			}
		}
		else
		{
			PAL_CurePoisonByLevel(wEventObjectID, pScript->rgwOperand[1]);
		}
		break;

	case 0x002D:
		//设置我方状态
		// 参数1 状态代号
		// 参数2 整数
		// 参数3 脚本地址
	{
		BOOL fAll = (pScript->rgwOperand[0] == 0) ? FALSE : TRUE;
		BOOL fAlwaysSuccess = FALSE;
		INT wSorceryResistance = PAL_New_GetPlayerSorceryResistance(wEventObjectID);
		INT iSuccessRate = 0;
		WORD wBaseSuccessRate = 100;
		WORD wStatusID = pScript->rgwOperand[0];
		WORD wNumRound = pScript->rgwOperand[1];

#ifdef ADD_SOME_STATUSES_SUCCESSFULLY_ANYTIME
		if (wStatusID >= 4 && wStatusID <= 9 || pScript->rgwOperand[2] == 0xffff)	//有益状态总是成功
		{
			fAlwaysSuccess = TRUE;
		}
#endif
		iSuccessRate = wBaseSuccessRate - wSorceryResistance;
		if (fAlwaysSuccess || PAL_New_GetTrueByPercentage(iSuccessRate))
		{   //仅对不良状态有抗性  //有益状态直接加
			PAL_SetPlayerStatus(wEventObjectID, wStatusID, wNumRound);
		}
		else if (pScript->rgwOperand[2] != 0x0000 && pScript->rgwOperand[2] != 0xffff)
		{
			wScriptEntry = pScript->rgwOperand[2] - 1;
		}

		break;
	}

	case 0x002E:
		/*
			设置敌方状态
		*/
	{
		BOOL fAlwaysSuccess = FALSE;
		BOOL fSorceryIsFull = FALSE;
		INT wSorceryResistance = 0.0;
		INT iSuccessRate = 0.0;
		INT wBaseSuccessRate = 100.0;
		WORD wStatusID = pScript->rgwOperand[0];
		WORD wNumRound = pScript->rgwOperand[1];

		wEventObjectID = PAL_NEW_CheckAndGetLegalEnemyTarget(wEventObjectID);
		wSorceryResistance = PAL_New_GetEnemySorceryResistance(wEventObjectID);
		fSorceryIsFull = wSorceryResistance >= 100.0 ? TRUE : FALSE;

		if (g_Battle.fPlayerMoving)
		{
			wPlayerRole = gpGlobals->rgParty[g_Battle.wMovingPlayerIndex].wPlayerRole;
			wPlayerRole = PAL_NEW_CheckAndGetLegalPlayerTarget(wPlayerRole);	//如果
		}

		wPlayerRole = gpGlobals->rgParty[g_Battle.wMovingPlayerIndex].wPlayerRole;
		INT str = PAL_GetPlayerActualFleeRate(wPlayerRole) / 50.0;
		if (str >= 40.0)
		{
			str = 40.0;
		}
		if (wSorceryResistance == 100)
		{
			iSuccessRate = wBaseSuccessRate - wSorceryResistance;
		}
		else
		{
			iSuccessRate = wBaseSuccessRate + str - wSorceryResistance;
		}
		if (fAlwaysSuccess || !fSorceryIsFull && PAL_New_GetTrueByPercentage(iSuccessRate))
		{
			PAL_New_SetEnemyStatus(wEventObjectID, wStatusID, wNumRound);
		}
		else
		{
			wScriptEntry = pScript->rgwOperand[2] - 1;
		}

		break;
	}

	case 0x002F:
		//
		// Remove player's status
		//
		PAL_RemovePlayerStatus(wEventObjectID, pScript->rgwOperand[0]);
		break;

	case 0x0030:
		//
		// Increase player's stat temporarily by percent
		//
	{
		INT* p;
		p = (INT*)(&gpGlobals->rgEquipmentEffect[kBodyPartExtra]); // HACKHACK
		INT* p1;
		p1 = (INT*)(&gpGlobals->g.PlayerRoles);

		if (pScript->rgwOperand[2] == 0)
		{
			iPlayerRole = wEventObjectID;
		}
		else
		{
			iPlayerRole = pScript->rgwOperand[2] - 1;
		}

		p[pScript->rgwOperand[0] * MAX_PLAYER_ROLES + iPlayerRole] =
			p1[pScript->rgwOperand[0] * MAX_PLAYER_ROLES + iPlayerRole] *
			(SHORT)pScript->rgwOperand[1] / 100;
	}
	break;

	case 0x0031:
		//
		// Change battle sprite temporarily for player
		//
		gpGlobals->rgEquipmentEffect[kBodyPartExtra].rgwSpriteNumInBattle[wEventObjectID] =
			pScript->rgwOperand[0];
		break;

	case 0x0033:
		//
		// collect the enemy for items
		//
		if (g_Battle.rgEnemy[wEventObjectID].e.wCollectValue != 0)
		{
			gpGlobals->wCollectValue +=
				g_Battle.rgEnemy[wEventObjectID].e.wCollectValue;
		}
		else
		{
			wScriptEntry = pScript->rgwOperand[0] - 1;
		}
		break;

	case 0x0034:
		//
		// Transform collected enemies into items
		//
		if (gpGlobals->wCollectValue > 0)
		{
			WCHAR s[256];

#ifdef PAL_CLASSIC
			i = RandomLong(1, gpGlobals->wCollectValue);
			if (i > 9)
			{
				i = 9;
			}
#else
			i = RandomLong(1, 9);
			if (i > gpGlobals->wCollectValue)
			{
				i = gpGlobals->wCollectValue;
			}
#endif

			gpGlobals->wCollectValue -= i;
			i--;

			PAL_AddItemToInventory(gpGlobals->g.lprgStore[0].rgwItems[i], 1);

			g_TextLib.iDialogShadow = 5;
			PAL_StartDialogWithOffset(kDialogCenterWindow, 0, 0, FALSE, 0, -10);
			PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"%ls@%ls@", PAL_GetWord(42),
				PAL_GetWord(gpGlobals->g.lprgStore[0].rgwItems[i]));
			LPCBITMAPRLE pBG = PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX);
			INT iBGWidth = PAL_RLEGetWidth(pBG), iBGHeight = PAL_RLEGetHeight(pBG);
			INT iBG_X = (320 - iBGWidth) / 2, iBG_Y = (200 - iBGHeight) / 2;
			PAL_POS pos = PAL_XY(iBG_X, iBG_Y);
			SDL_Rect rect = { iBG_X, iBG_Y, iBGWidth, iBGHeight };
			PAL_RLEBlitToSurface(pBG, gpScreen, pos);

			WORD wObject = gpGlobals->g.lprgStore[0].rgwItems[i];
			static WORD wPrevImageIndex = 0xFFFF;
			static BYTE bufImage[2048];
			if (gpGlobals->g.rgObject[wObject].item.wBitmap != wPrevImageIndex)
			{
				if (PAL_MKFReadChunk(bufImage, 2048,
					gpGlobals->g.rgObject[wObject].item.wBitmap, gpGlobals->f.fpBALL) > 0)
				{
					wPrevImageIndex = gpGlobals->g.rgObject[wObject].item.wBitmap;
				}
				else
				{
					wPrevImageIndex = 0xFFFF;
				}
			}
			if (wPrevImageIndex != 0xFFFF)
			{
				PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(PAL_X(pos) + 8, PAL_Y(pos) + 7));
			}

			VIDEO_UpdateScreen(&rect);

			PAL_ShowDialogText(s);
			g_TextLib.iDialogShadow = 0;
		}
		else
		{
			wScriptEntry = pScript->rgwOperand[0] - 1;
		}
		break;

	case 0x0035:
		//
		// Shake the screen
		//
		i = pScript->rgwOperand[1];
		if (i == 0)
		{
			i = 3;
		}
		VIDEO_ShakeScreen(pScript->rgwOperand[0], i);
		if (!pScript->rgwOperand[0])
		{
			VIDEO_UpdateScreen(NULL);
		}
		break;

	case 0x0036:
		//
		// Set the current playing RNG animation
		//
		gpGlobals->iCurPlayingRNG = pScript->rgwOperand[0];
		break;

	case 0x0037:
		//
		// Play RNG animation
		//
		PAL_RNGPlay(gpGlobals->iCurPlayingRNG,
			pScript->rgwOperand[0],
			pScript->rgwOperand[1] > 0 ? pScript->rgwOperand[1] : -1,
			pScript->rgwOperand[2] > 0 ? pScript->rgwOperand[2] : 16);
		break;

	case 0x0038:
		//
		// Teleport the party out of the scene
		//
		if (!gpGlobals->fInBattle &&
			gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wScriptOnTeleport != 0)
		{
			PAL_RunTriggerScript(gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wScriptOnTeleport, 0xFFFF);
		}
		else
		{
			//
			// failed
			//
			g_fScriptSuccess = FALSE;
			wScriptEntry = pScript->rgwOperand[0] - 1;
		}
		break;

	case 0x0039:
		//
		// Drain HP from enemy
		//
	{
		if (pScript->rgwOperand[0])
		{
			//
			// Inflict damage to all enemies
			//
			for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
			{
				if (g_Battle.rgEnemy[i].wObjectID != 0)
				{
					g_Battle.rgEnemy[i].e.wHealth -= pScript->rgwOperand[1];
				}
			}
			for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
			{
				w = gpGlobals->rgParty[j].wPlayerRole;

				if (gpGlobals->g.PlayerRoles.rgwHP[w] > 0)
				{
					gpGlobals->g.PlayerRoles.rgwHP[w] += pScript->rgwOperand[2];
				}
				if (gpGlobals->g.PlayerRoles.rgwHP[w] > PAL_GetPlayerMaxHP(w))
				{
					gpGlobals->g.PlayerRoles.rgwHP[w] = PAL_GetPlayerMaxHP(w);
				}
			}
		}
		else
		{
			//
			// Inflict damage to one enemy
			//
			w = gpGlobals->rgParty[g_Battle.wMovingPlayerIndex].wPlayerRole;
			g_Battle.rgEnemy[wEventObjectID].e.wHealth -= pScript->rgwOperand[1];
			gpGlobals->g.PlayerRoles.rgwHP[w] += pScript->rgwOperand[2];

			if (gpGlobals->g.PlayerRoles.rgwHP[w] > PAL_GetPlayerMaxHP(w))
			{
				gpGlobals->g.PlayerRoles.rgwHP[w] = PAL_GetPlayerMaxHP(w);
			}
		}
		break;
	}

	case 0x003A:
		//
		// Player flee from the battle
		//
		//if (g_Battle.fIsBoss)
		//{
		//	//
		//	// Cannot flee from bosses
		//	//
		//	wScriptEntry = pScript->rgwOperand[0] - 1;
		//}
		//else
	{
		PAL_BattlePlayerEscape();
	}
	break;

	case 0x003F:
		//
		// Ride the event object to the specified position, at a low speed
		//
		PAL_PartyRideEventObject(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
			pScript->rgwOperand[2], 5);
		break;

	case 0x0040:
		//
		// set the trigger method for a event object
		//
		if (pScript->rgwOperand[0] != 0)
		{
			pCurrent->wTriggerMode = pScript->rgwOperand[1];
		}
		break;

	case 0x0041:
		//
		// Mark the script as failed
		//
		g_fScriptSuccess = FALSE;
		break;

	case 0x0042:
		//
		// Simulate a magic for player
		//
		i = (SHORT)(pScript->rgwOperand[2]) - 1;
		if (i < 0)
		{
			i = wEventObjectID;
		}
		PAL_BattleSimulateMagic(i, pScript->rgwOperand[0], pScript->rgwOperand[1]);
		break;

	case 0x0043:
		//
		// Set background music
		//
		gpGlobals->wNumMusic = pScript->rgwOperand[0];
		AUDIO_PlayMusic(pScript->rgwOperand[0], pScript->rgwOperand[1] != 1, (pScript->rgwOperand[1] == 3 && pScript->rgwOperand[0] != 9) ? 3.0f : 0.0f);
		break;

	case 0x0044:
		//
		// Ride the event object to the specified position, at the normal speed
		//
		PAL_PartyRideEventObject(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
			pScript->rgwOperand[2], 12);
		break;

	case 0x0045:
		//
		// Set battle music
		//
		gpGlobals->wNumBattleMusic = pScript->rgwOperand[0];
		break;

	case 0x0046:
		//
		// Set the party position on the map
		//
	{
		int xOffset, yOffset, x, y;

		xOffset =
			((gpGlobals->wPartyDirection == kDirWest || gpGlobals->wPartyDirection == kDirSouth)
				? 16 : -16);
		yOffset =
			((gpGlobals->wPartyDirection == kDirWest || gpGlobals->wPartyDirection == kDirNorth)
				? 8 : -8);

		x = pScript->rgwOperand[0] * 32 + pScript->rgwOperand[2] * 16;
		y = pScript->rgwOperand[1] * 16 + pScript->rgwOperand[2] * 8;

		x -= PAL_X(gpGlobals->partyoffset);
		y -= PAL_Y(gpGlobals->partyoffset);

		gpGlobals->viewport = PAL_XY(x, y);

		x = PAL_X(gpGlobals->partyoffset);
		y = PAL_Y(gpGlobals->partyoffset);

		for (i = 0; i < MAX_PLAYABLE_PLAYER_ROLES; i++)
		{
			gpGlobals->rgParty[i].x = x;
			gpGlobals->rgParty[i].y = y;
			gpGlobals->rgTrail[i].x = x + PAL_X(gpGlobals->viewport);
			gpGlobals->rgTrail[i].y = y + PAL_Y(gpGlobals->viewport);
			gpGlobals->rgTrail[i].wDirection = gpGlobals->wPartyDirection;

			x += xOffset;
			y += yOffset;
		}
	}
	break;

	case 0x0047:
		//
		// Play sound effect
		//
		AUDIO_PlaySound(pScript->rgwOperand[0]);
		break;

	case 0x0049:
		//
		// Set the state of event object
		//
		if (pScript->rgwOperand[0] != 0)
			pCurrent->sState = pScript->rgwOperand[1];
		break;

	case 0x004A:
		//
		// Set the current battlefield
		//
		gpGlobals->wNumBattleField = pScript->rgwOperand[0];
		break;

	case 0x004B:
		//
		// Nullify the event object for a short while
		// __DEBUG__暂时将事件对象置为空
		//pEvtObj->sState *= -1;
		//pEvtObj->sVanishTime = 3000;

		//pEvtObj->sState = 0;
		//pEvtObj->sVanishTime = 30000;

		pEvtObj->sVanishTime = -15;
		break;

	case 0x004C:
		//
		// chase the player
		//
		i = pScript->rgwOperand[0]; // max. distance
		j = pScript->rgwOperand[1]; // speed

		if (i == 0)
		{
			i = 10;
		}

		if (j == 0)
		{
			j = 5;
		}

		PAL_MonsterChasePlayer(wEventObjectID, j, i, pScript->rgwOperand[2]);
		break;

	case 0x004D:
		//
		// wait for any key
		//
		PAL_WaitForKey(0);
		break;

	case 0x004E:
		//
		// Load the last saved game
		//
		PAL_FadeOut(1);
		PAL_ReloadInNextTick(gpGlobals->bCurrentSaveSlot);
		return 0; // don't go further

	case 0x004F:
		//
		// Fade the screen to red color (game over)
		//
		PAL_FadeToRed();
		break;

	case 0x0050:
		//
		// screen fade out
		//
		if (gpGlobals->fIsNotFaded)
		{
			gpGlobals->fIsNotFaded = FALSE;;
			break;
		}

		VIDEO_UpdateScreen(NULL);
		PAL_FadeOut(pScript->rgwOperand[0] ? pScript->rgwOperand[0] : 1);
		gpGlobals->fNeedToFadeIn = TRUE;
		break;

	case 0x0051:
		//
		// screen fade in
		//
		VIDEO_UpdateScreen(NULL);
		PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette,
			((SHORT)(pScript->rgwOperand[0]) > 0) ? pScript->rgwOperand[0] : 1);
		gpGlobals->fNeedToFadeIn = FALSE;
		break;

	case 0x0052:
		//
		// hide the event object for a while, default 800 frames
		//
		pEvtObj->sState *= -1;
		pEvtObj->sVanishTime = 1300;
		//if (gpGlobals->fDoAutoSave == TRUE)
		//{
		//	PAL_AutoSaveGame();
		//	gpGlobals->fDoAutoSave = FALSE;
		//}
		break;

	case 0x0053:
		//
		// use the day palette
		//
		gpGlobals->fNightPalette = FALSE;
		break;

	case 0x0054:
		//
		// use the night palette
		//
		gpGlobals->fNightPalette = TRUE;
		break;

	case 0x0055:
		//
		// Add magic to a player
		//
		i = pScript->rgwOperand[1];
		if (i == 0)
		{
			i = wEventObjectID;
		}
		else
		{
			i--;
		}
		PAL_AddMagic(i, pScript->rgwOperand[0]);
		break;

	case 0x0056:
		//
		// Remove magic from a player
		//
		i = pScript->rgwOperand[1];
		if (i == 0)
		{
			i = wEventObjectID;
		}
		else
		{
			i--;
		}
		PAL_RemoveMagic(i, pScript->rgwOperand[0]);
		break;

	case 0x0057:
		//
		// Set the base damage of magic according to MP value
		// 根据真气体力设定仙术的基础伤害值
	{
		INT iBaseDamage = 0;
		INT iCostHP = gpGlobals->g.PlayerRoles.rgwHP[wEventObjectID];
		INT iCostMP = gpGlobals->g.PlayerRoles.rgwMP[wEventObjectID];

		i = ((pScript->rgwOperand[1] = 10));
		j = gpGlobals->g.rgObject[pScript->rgwOperand[0]].magic.wMagicNumber;
		iBaseDamage = (iCostMP + iCostHP) * i;

		gpGlobals->g.lprgMagic[j].wBaseDamage = iBaseDamage;
		gpGlobals->g.PlayerRoles.rgwMP[wEventObjectID] -= iCostMP;
		gpGlobals->g.PlayerRoles.rgwHP[wEventObjectID] -= iCostHP;
		gpGlobals->g.PlayerRoles.rgwHP[wEventObjectID] = 1;
		break;
	}

	case 0x0058:
		//
		// Jump if there is less than the specified number of the specified items
		// in the inventory
		//
		if (PAL_GetItemAmount(pScript->rgwOperand[0]) < (SHORT)(pScript->rgwOperand[1]))
		{
			wScriptEntry = pScript->rgwOperand[2] - 1;
		}
		break;

	case 0x0059:
		//
		// Change to the specified scene
		//
		if (pScript->rgwOperand[0] > 0 && pScript->rgwOperand[0] <= MAX_SCENES &&
			gpGlobals->wNumScene != pScript->rgwOperand[0])
		{
			//
			// Set data to load the scene in the next frame
			//
			// 自动存档
			//gpGlobals->fDoAutoSave = TRUE;
			gpGlobals->wNumScene = pScript->rgwOperand[0];
			PAL_SetLoadFlags(kLoadScene);
			gpGlobals->fEnteringScene = TRUE;
			gpGlobals->wLayer = 0;
		}
		break;

	case 0x005A:
		//
		// Halve the player's HP
		// The wEventObjectID parameter here should indicate the player role
		//
		gpGlobals->g.PlayerRoles.rgwHP[wEventObjectID] /= 2;
		break;

	case 0x005B:
		//
		// Halve the enemy's HP
		//
		w = g_Battle.rgEnemy[wEventObjectID].e.wHealth / 2 + 1;
		if (w > pScript->rgwOperand[0])
		{
			w = pScript->rgwOperand[0];
		}
		g_Battle.rgEnemy[wEventObjectID].e.wHealth -= pScript->rgwOperand[0];
		break;

	case 0x005C:
		//
		// Hide for a while
		//
		g_Battle.iHidingTime = -(INT)(pScript->rgwOperand[0]);
		break;

	case 0x005D:
		//
		// Jump if player doesn't have the specified poison
		//
		if (!PAL_IsPlayerPoisonedByKind(wEventObjectID, pScript->rgwOperand[0]))
		{
			wScriptEntry = pScript->rgwOperand[1] - 1;
		}
		break;

	case 0x005E:
		//
		// Jump if enemy doesn't have the specified poison
		//
		for (i = 0; i < MAX_POISONS; i++)
		{
			if (g_Battle.rgEnemy[wEventObjectID].rgPoisons[i].wPoisonID == pScript->rgwOperand[0])
			{
				break;
			}
		}

		if (i >= MAX_POISONS)
		{
			wScriptEntry = pScript->rgwOperand[1] - 1;
		}
		break;

	case 0x005F:
		//
		// Kill the player immediately
		// The wEventObjectID parameter here should indicate the player role
		//
		gpGlobals->g.PlayerRoles.rgwHP[wEventObjectID] = 0;
		break;

	case 0x0060:
		//
		// Immediate KO of the enemy
		//
		g_Battle.rgEnemy[wEventObjectID].e.wHealth = 0;
		break;

	case 0x0061:
		//
		// Jump if player is not poisoned
		// 之前用的函数不能正确判断等级为0的毒，比如赤毒
		if (PAL_New_IsPlayerPoisoned(wEventObjectID) == FALSE)
		{
			wScriptEntry = pScript->rgwOperand[0] - 1;
		}
		break;

	case 0x0062:
		//
		// Pause enemy chasing for a while
		//
		gpGlobals->wChasespeedChangeCycles = pScript->rgwOperand[0];
		gpGlobals->wChaseRange = 0;
		break;

	case 0x0063:
		//
		// Speed up enemy chasing for a while
		//
		gpGlobals->wChasespeedChangeCycles = pScript->rgwOperand[0];
		gpGlobals->wChaseRange = 3;
		break;

	case 0x0064:
		//
		// Jump if enemy's HP is more than the specified percentage
		//
		if ((g_Battle.rgEnemy[wEventObjectID].e.wHealth) * 100 >
			(g_Battle.rgEnemy[wEventObjectID].iMaxHealth) * pScript->rgwOperand[0])
		{
			wScriptEntry = pScript->rgwOperand[1] - 1;
		}
		break;

	case 0x0065:
		//
		// Set the player's sprite
		//
		gpGlobals->g.PlayerRoles.rgwSpriteNum[pScript->rgwOperand[0]] = pScript->rgwOperand[1];
		if (!gpGlobals->fInBattle && pScript->rgwOperand[2])
		{
			PAL_SetLoadFlags(kLoadPlayerSprite);
			PAL_LoadResources();
		}
		break;

	case 0x0066:
		//
		// Throw weapon to enemy
		//
		w = pScript->rgwOperand[1] * 5;
		w += PAL_GetPlayerActualAttackStrength(gpGlobals->rgParty[g_Battle.wMovingPlayerIndex].wPlayerRole) * RandomFloat(0, 3);

		if (gpGlobals->rgPlayerStatus[gpGlobals->rgParty[g_Battle.wMovingPlayerIndex].wPlayerRole][kStatusBravery] > 0)
		{
			w *= 2;
		}

		PAL_BattleSimulateMagic((SHORT)wEventObjectID, pScript->rgwOperand[0], w);
		break;

	case 0x0067:
		//
		// Enemy use magic
		//
		g_Battle.rgEnemy[wEventObjectID].e.wMagic = pScript->rgwOperand[0];
		g_Battle.rgEnemy[wEventObjectID].e.wMagicRate = ((pScript->rgwOperand[1] == 0) ? 10 : pScript->rgwOperand[1]);
		break;

	case 0x0068:
		//
		// Jump if it's enemy's turn
		//
		if (g_Battle.fEnemyMoving)
		{
			wScriptEntry = pScript->rgwOperand[0] - 1;
		}
		break;

	case 0x0069:
		//
		// Enemy escape in battle
		//
		PAL_BattleEnemyEscape();
		break;

	case 0x006A:
		//
		// Steal from the enemy
		//
		// PAL_BattleStealFromEnemy(wEventObjectID, pScript->rgwOperand[0]);
		PAL_BattleStealFromEnemy(wEventObjectID, 10);
		break;

	case 0x006B:
		//
		// Blow away enemies
		//
		g_Battle.iBlow = (SHORT)(pScript->rgwOperand[0]);
		break;

	case 0x006C:
		//
		// Walk the NPC in one step
		//
		pCurrent->x += (SHORT)(pScript->rgwOperand[1]);
		pCurrent->y += (SHORT)(pScript->rgwOperand[2]);
		PAL_NPCWalkOneStep(wCurEventObjectID, 0);
		break;

	case 0x006D:
		//
		// Set the enter script and teleport script for a scene
		//
		if (pScript->rgwOperand[0])
		{
			if (pScript->rgwOperand[1])
			{
				gpGlobals->g.rgScene[pScript->rgwOperand[0] - 1].wScriptOnEnter =
					pScript->rgwOperand[1];
			}

			if (pScript->rgwOperand[2])
			{
				gpGlobals->g.rgScene[pScript->rgwOperand[0] - 1].wScriptOnTeleport =
					pScript->rgwOperand[2];
			}

			if (pScript->rgwOperand[1] == 0 && pScript->rgwOperand[2] == 0)
			{
				gpGlobals->g.rgScene[pScript->rgwOperand[0] - 1].wScriptOnEnter = 0;
				gpGlobals->g.rgScene[pScript->rgwOperand[0] - 1].wScriptOnTeleport = 0;
			}
		}
		break;

	case 0x006E:
		//
		// Move the player to the specified position in one step
		//
		for (i = 3; i >= 0; i--)
		{
			gpGlobals->rgTrail[i + 1] = gpGlobals->rgTrail[i];
		}
		gpGlobals->rgTrail[0].wDirection = gpGlobals->wPartyDirection;
		gpGlobals->rgTrail[0].x = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
		gpGlobals->rgTrail[0].y = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

		gpGlobals->viewport = PAL_XY(
			PAL_X(gpGlobals->viewport) + (SHORT)(pScript->rgwOperand[0]),
			PAL_Y(gpGlobals->viewport) + (SHORT)(pScript->rgwOperand[1]));

		gpGlobals->wLayer = pScript->rgwOperand[2] * 8;

		if (pScript->rgwOperand[0] != 0 || pScript->rgwOperand[1] != 0)
		{
			PAL_UpdatePartyGestures(TRUE);
		}
		break;

	case 0x006F:
		//
		// Sync the state of current event object with another event object
		//
		if (pCurrent->sState == (SHORT)(pScript->rgwOperand[1]))
		{
			pEvtObj->sState = (SHORT)(pScript->rgwOperand[1]);
		}
		break;

	case 0x0070:
		//
		// Walk the party to the specified position
		//
		PAL_PartyWalkTo(pScript->rgwOperand[0], pScript->rgwOperand[1], pScript->rgwOperand[2], 3);
		break;

	case 0x0071:
		//
		// Wave the screen
		//
		gpGlobals->wScreenWave = pScript->rgwOperand[0];
		gpGlobals->sWaveProgression = (SHORT)(pScript->rgwOperand[1]);
		break;

	case 0x0073:
		//
		// Fade the screen to scene
		//
		VIDEO_BackupScreen(gpScreen);
		PAL_MakeScene();
		VIDEO_FadeScreen(pScript->rgwOperand[0]);
		break;

	case 0x0074:
		//
		// Jump if not all players are full HP MP
		//
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			w = gpGlobals->rgParty[i].wPlayerRole;
			if (gpGlobals->g.PlayerRoles.rgwHP[w] < PAL_GetPlayerMaxHP(w) || gpGlobals->g.PlayerRoles.rgwMP[w] < PAL_GetPlayerMaxMP(w))
			{
				wScriptEntry = pScript->rgwOperand[0] - 1;
				break;
			}
		}
		break;

	case 0x0075:
		//
		// Set the player party
		//
		gpGlobals->wMaxPartyMemberIndex = 0;

		if (pScript->rgwOperand[0] >= 0x0010) {
			WORD pattern = pScript->rgwOperand[0];
			UTIL_LogOutput(LOGLEVEL_DEBUG, "Special party pattern: %.4x", pattern);
			WORD party[4] = { 0, 1, 2, 4 };
			if (pattern >= 0x0020)
			{

				party[0] = 1;
				party[1] = 0;
				party[2] = 2;
				party[3] = 4;
			}
			if (pattern >= 0x0030)
			{
				party[0] = 2;
				party[1] = 1;
				party[2] = 0;
				party[3] = 4;
			}
			if (pattern >= 0x0040)
			{
				party[0] = 4;
				party[1] = 0;
				party[2] = 2;
				party[3] = 1;
			}

			for (i = 0; i < 4; i++) {
				gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex].wPlayerRole = party[i];
				gpGlobals->wMaxPartyMemberIndex++;
			}
		}
		else {
			for (i = 0; i < 3; i++) {
				if (pScript->rgwOperand[i] != 0) {
					gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex].wPlayerRole =
						pScript->rgwOperand[i] - 1;

					gpGlobals->wMaxPartyMemberIndex++;
				}
			}
		}

		if (gpGlobals->wMaxPartyMemberIndex == 0)
		{
			// HACK for Dream 2.11
			gpGlobals->rgParty[0].wPlayerRole = 0;
			gpGlobals->wMaxPartyMemberIndex = 1;
		}

		gpGlobals->wMaxPartyMemberIndex--;

		//
		// Reload the player sprites
		//
		PAL_SetLoadFlags(kLoadPlayerSprite);
		PAL_LoadResources();

		memset(gpGlobals->rgPoisonStatus, 0, sizeof(gpGlobals->rgPoisonStatus));
		PAL_UpdateEquipments();
		break;

	case 0x0076:
		//
		// Show FBP picture
		//
		if (gConfig.fIsWIN95)
		{
			SDL_FillRect(gpScreen, NULL, 0);
			VIDEO_UpdateScreen(NULL);
		}
		else
		{
			PAL_EndingSetEffectSprite(0);
			PAL_ShowFBP(pScript->rgwOperand[0], pScript->rgwOperand[1]);
		}
		break;

	case 0x0077:
		//
		// Stop current playing music
		//
		AUDIO_PlayMusic(0, FALSE,
			(pScript->rgwOperand[0] == 0) ? 2.0f : (FLOAT)(pScript->rgwOperand[0]) * 3);
		gpGlobals->wNumMusic = 0;
		break;

	case 0x0078:
		//
		// FIXME: ???
		//
		//gpGlobals->fDoAutoSave = TRUE;
		break;

	case 0x0079:
		//
		// Jump if the specified player is in the party
		//
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			if (gpGlobals->g.PlayerRoles.rgwName[gpGlobals->rgParty[i].wPlayerRole] ==
				pScript->rgwOperand[0])
			{
				wScriptEntry = pScript->rgwOperand[1] - 1;
				break;
			}
		}
		break;

	case 0x007A:
		//
		// Walk the party to the specified position, at a higher speed
		//
		PAL_PartyWalkTo(pScript->rgwOperand[0], pScript->rgwOperand[1], pScript->rgwOperand[2], 6);
		break;

	case 0x007B:
		//
		// Walk the party to the specified position, at the highest speed
		//
		PAL_PartyWalkTo(pScript->rgwOperand[0], pScript->rgwOperand[1], pScript->rgwOperand[2], 8);
		break;

	case 0x007C:
		//
		// Walk straight to the specified position
		//
		if ((wEventObjectID & 1) ^ (gpGlobals->dwFrameNum & 1))
		{
			if (!PAL_NPCWalkTo(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
				pScript->rgwOperand[2], 8))
			{
				wScriptEntry--;
			}
		}
		else
		{
			wScriptEntry--;
		}
		break;

	case 0x007D:
		//
		// Move the event object
		//
		pCurrent->x += (SHORT)(pScript->rgwOperand[1]);
		pCurrent->y += (SHORT)(pScript->rgwOperand[2]);
		break;

	case 0x007E:
		//
		// Set the layer of event object
		//
		pCurrent->sLayer = (SHORT)(pScript->rgwOperand[1]);
		break;

	case 0x007F:
		//
		// Move the viewport
		//
		if (pScript->rgwOperand[0] == 0 && pScript->rgwOperand[1] == 0)
		{
			//
			// Move the viewport back to normal state
			//
			x = gpGlobals->rgParty[0].x - 160;
			y = gpGlobals->rgParty[0].y - 112;

			gpGlobals->viewport =
				PAL_XY(PAL_X(gpGlobals->viewport) + x, PAL_Y(gpGlobals->viewport) + y);
			gpGlobals->partyoffset = PAL_XY(160, 112);

			for (i = 0; i <= (short)gpGlobals->wMaxPartyMemberIndex + gpGlobals->nFollower; i++)
			{
				gpGlobals->rgParty[i].x -= x;
				gpGlobals->rgParty[i].y -= y;
			}

			if (pScript->rgwOperand[2] != 0xFFFF)
			{
				PAL_MakeScene();
				VIDEO_UpdateScreen(NULL);
			}
		}
		else
		{
			DWORD time;

			i = 0;

			x = (SHORT)(pScript->rgwOperand[0]);
			y = (SHORT)(pScript->rgwOperand[1]);

			time = SDL_GetTicks() + FRAME_TIME;

			do
			{
				if (pScript->rgwOperand[2] == 0xFFFF)
				{
					x = PAL_X(gpGlobals->viewport);
					y = PAL_Y(gpGlobals->viewport);

					gpGlobals->viewport =
						PAL_XY(pScript->rgwOperand[0] * 32 - 160, pScript->rgwOperand[1] * 16 - 112);

					x -= PAL_X(gpGlobals->viewport);
					y -= PAL_Y(gpGlobals->viewport);

					for (j = 0; j <= (short)gpGlobals->wMaxPartyMemberIndex + gpGlobals->nFollower; j++)
					{
						gpGlobals->rgParty[j].x += x;
						gpGlobals->rgParty[j].y += y;
					}
				}
				else
				{
					gpGlobals->viewport =
						PAL_XY(PAL_X(gpGlobals->viewport) + x, PAL_Y(gpGlobals->viewport) + y);
					gpGlobals->partyoffset =
						PAL_XY(PAL_X(gpGlobals->partyoffset) - x, PAL_Y(gpGlobals->partyoffset) - y);

					for (j = 0; j <= (short)gpGlobals->wMaxPartyMemberIndex + gpGlobals->nFollower; j++)
					{
						gpGlobals->rgParty[j].x -= x;
						gpGlobals->rgParty[j].y -= y;
					}
				}

				if (pScript->rgwOperand[2] != 0xFFFF)
				{
					PAL_GameUpdate(FALSE);
				}

				PAL_MakeScene();
				VIDEO_UpdateScreen(NULL);

				//
				// Delay for one frame
				//
				PAL_DelayUntil(time);
				time = SDL_GetTicks() + FRAME_TIME;
			} while (++i < (SHORT)(pScript->rgwOperand[2]));
		}
		break;

	case 0x0080:
		//
		// Toggle day/night palette
		//
		gpGlobals->fNightPalette = !(gpGlobals->fNightPalette);
		PAL_PaletteFade(gpGlobals->wNumPalette, gpGlobals->fNightPalette,
			!(pScript->rgwOperand[0]));
		break;

	case 0x0081:
		//
		// Jump if the player is not facing the specified event object
		//
	{
		if (pScript->rgwOperand[0] <= gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex ||
			pScript->rgwOperand[0] > gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex)
		{
			//
			// The event object is not in the current scene
			//
			wScriptEntry = pScript->rgwOperand[2] - 1;
			g_fScriptSuccess = FALSE;
			break;
		}

		x = pCurrent->x;
		y = pCurrent->y;

		x +=
			((gpGlobals->wPartyDirection == kDirWest || gpGlobals->wPartyDirection == kDirSouth)
				? 16 : -16);
		y +=
			((gpGlobals->wPartyDirection == kDirWest || gpGlobals->wPartyDirection == kDirNorth)
				? 8 : -8);

		x -= PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
		y -= PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

		if (abs(x) + abs(y * 2) < pScript->rgwOperand[1] * 32 + 16 && gpGlobals->g.lprgEventObject[pScript->rgwOperand[0] - 1].sState > 0)
		{
			if (pScript->rgwOperand[1] > 0)
			{
				//
				// Change the trigger mode so that the object can be triggered in next frame
				//
				pCurrent->wTriggerMode = kTriggerTouchNormal + pScript->rgwOperand[1];
			}
		}
		else
		{
			wScriptEntry = pScript->rgwOperand[2] - 1;
			g_fScriptSuccess = FALSE;
		}
	}
	break;

	case 0x0082:
		//
		// Walk straight to the specified position, at a high speed
		//
		if (!PAL_NPCWalkTo(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
			pScript->rgwOperand[2], 14))
		{
			wScriptEntry--;
		}
		break;

	case 0x0083:
		//
		// Jump if event object is not in the specified zone of the current event object
		//
		if (pScript->rgwOperand[0] <= gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex ||
			pScript->rgwOperand[0] > gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex)
		{
			//
			// The event object is not in the current scene
			//
			wScriptEntry = pScript->rgwOperand[2] - 1;
			g_fScriptSuccess = FALSE;
			break;
		}

		x = pEvtObj->x - pCurrent->x;
		y = pEvtObj->y - pCurrent->y;

		if (abs(x) + abs(y * 2) >= pScript->rgwOperand[1] * 32 + 16)
		{
			wScriptEntry = pScript->rgwOperand[2] - 1;
			g_fScriptSuccess = FALSE;
		}
		break;

	case 0x0084:
		//
		// Place the item which player used as an event object to the scene
		//
		if (pScript->rgwOperand[0] <= gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex ||
			pScript->rgwOperand[0] > gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex)
		{
			//
			// The event object is not in the current scene
			//
			wScriptEntry = pScript->rgwOperand[2] - 1;
			g_fScriptSuccess = FALSE;
			break;
		}

		x = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
		y = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

		x +=
			((gpGlobals->wPartyDirection == kDirWest || gpGlobals->wPartyDirection == kDirSouth)
				? -16 : 16);
		y +=
			((gpGlobals->wPartyDirection == kDirWest || gpGlobals->wPartyDirection == kDirNorth)
				? -8 : 8);

		if (PAL_CheckObstacle(PAL_XY(x, y), FALSE, 0))
		{
			wScriptEntry = pScript->rgwOperand[2] - 1;
			g_fScriptSuccess = FALSE;
		}
		else
		{
			pCurrent->x = x;
			pCurrent->y = y;
			pCurrent->sState = (SHORT)(pScript->rgwOperand[1]);
		}
		break;

	case 0x0085:
		//
		// Delay for a period
		//
		UTIL_Delay(pScript->rgwOperand[0] * 80);
		break;

	case 0x0086:
		//
		// Jump if the specified item is not equipped
		//
		y = FALSE;
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			w = gpGlobals->rgParty[i].wPlayerRole;
			for (x = 0; x < MAX_PLAYER_EQUIPMENTS; x++)
			{
				if (gpGlobals->g.PlayerRoles.rgwEquipment[x][w] == pScript->rgwOperand[0])
				{
					y = TRUE;
					i = 999;
					break;
				}
			}
		}
		if (!y)
		{
			wScriptEntry = pScript->rgwOperand[2] - 1;
		}
		break;

	case 0x0087:
		//
		// Animate the event object
		//
		PAL_NPCWalkOneStep(wCurEventObjectID, 0);
		break;

	case 0x0088:
		//
		// Set the base damage of magic according to amount of money
		//
		i = ((gpGlobals->dwCash > 20000) ? 20000 : gpGlobals->dwCash);
		gpGlobals->dwCash -= i;
		j = gpGlobals->g.rgObject[pScript->rgwOperand[0]].magic.wMagicNumber;
		gpGlobals->g.lprgMagic[j].wBaseDamage = i * 2 / 4;
		break;

	case 0x0089:
		//
		// Set the battle result
		//
		g_Battle.BattleResult = pScript->rgwOperand[0];
		break;

	case 0x008A:
		//
		// Enable Auto-Battle for next battle
		//
		gpGlobals->fAutoBattle = TRUE;
		break;

	case 0x008B:
		//
		// change the current palette
		//
		gpGlobals->wNumPalette = pScript->rgwOperand[0];
		if (!gpGlobals->fNeedToFadeIn)
		{
			PAL_SetPalette(gpGlobals->wNumPalette, FALSE);
		}
		break;

	case 0x008C:
		//
		// Fade from/to color
		//
		PAL_ColorFade(pScript->rgwOperand[1], (BYTE)(pScript->rgwOperand[0]),
			pScript->rgwOperand[2]);
		gpGlobals->fNeedToFadeIn = FALSE;
		break;

	case 0x008D:
		//
		// Increase player's level
		//
		PAL_PlayerLevelUp(wEventObjectID, pScript->rgwOperand[0]);

		gpGlobals->g.PlayerRoles.rgwHP[wEventObjectID] = PAL_GetPlayerMaxHP(wEventObjectID);
		gpGlobals->g.PlayerRoles.rgwMP[wEventObjectID] = PAL_GetPlayerMaxMP(wEventObjectID);

		w = gpGlobals->rgParty[i].wPlayerRole;
		j = 0;
		{
			while (j < gpGlobals->g.nLevelUpMagic)
			{
				if (gpGlobals->g.lprgLevelUpMagic[j].m[wEventObjectID].wMagic == 0 ||
					gpGlobals->g.lprgLevelUpMagic[j].m[wEventObjectID].wLevel > gpGlobals->g.PlayerRoles.rgwLevel[wEventObjectID])
				{
					j++;
					continue;
				}

				if (PAL_AddMagic(wEventObjectID, gpGlobals->g.lprgLevelUpMagic[j].m[wEventObjectID].wMagic))
				{
					PAL_StartDialog(kDialogCenterWindow, 0, 0, FALSE);
					PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"%ls%ls%ls", PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[wEventObjectID]), PAL_GetWord(BATTLEWIN_ADDMAGIC_LABEL), PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[j].m[wEventObjectID].wMagic));
					PAL_ShowDialogText(s);
				}

				j++;
			}
		}
		break;

	case 0x008F:
		//
		// Halve the cash amount
		// 消耗所有金钱
		gpGlobals->dwCash = 0;
		break;

	case 0x0090:
		//
		// Set the object script
		//
		gpGlobals->g.rgObject[pScript->rgwOperand[0]].rgwData[2 + pScript->rgwOperand[2]] =
			pScript->rgwOperand[1];
		break;

	case 0x0091:
		//
		// Jump if the enemy is not first of same kind
		//
	{
		int self_pos = 0;
		int count = 0;
		if (gpGlobals->fInBattle)
		{
			for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
			{
				if (g_Battle.rgEnemy[i].wObjectID == g_Battle.rgEnemy[wEventObjectID].wObjectID)
				{
					count++;
					if (i == wEventObjectID)
						self_pos = count;
				}
			}
		}
		if (self_pos > 1)
			wScriptEntry = pScript->rgwOperand[0] - 1;
	}
	break;

	case 0x0092:
		//
		// Show a magic-casting animation for a player in battle
		//
		if (gpGlobals->fInBattle)
		{
			if (pScript->rgwOperand[0] != 0)
			{
				PAL_BattleShowPlayerPreMagicAnim(pScript->rgwOperand[0] - 1, FALSE);
				g_Battle.rgPlayer[pScript->rgwOperand[0] - 1].wCurrentFrame = 6;
			}

			for (i = 0; i < 5; i++)
			{
				for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
				{
					g_Battle.rgPlayer[j].iColorShift = i * 2;
				}
				PAL_BattleDelay(1, 0, TRUE);
			}
			VIDEO_BackupScreen(g_Battle.lpSceneBuf);
			PAL_BattleUpdateFighters();
			PAL_BattleMakeScene();
			PAL_BattleFadeScene();
		}
		break;

	case 0x0093:
		//
		// Fade the screen. Update scene in the process.
		//
		PAL_SceneFade(gpGlobals->wNumPalette, gpGlobals->fNightPalette,
			(SHORT)(pScript->rgwOperand[0]));
		gpGlobals->fNeedToFadeIn = ((SHORT)(pScript->rgwOperand[0]) < 0);
		break;

	case 0x0094:
		//
		// Jump if the state of event object is the specified one
		//
		if (pCurrent->sState == (SHORT)(pScript->rgwOperand[1]))
		{
			wScriptEntry = pScript->rgwOperand[2] - 1;
		}
		break;

	case 0x0095:
		//
		// Jump if the current scene is the specified one
		//
		if (gpGlobals->wNumScene == pScript->rgwOperand[0])
		{
			wScriptEntry = pScript->rgwOperand[1] - 1;
		}
		break;

	case 0x0096:
		//
		// Show the ending animation
		//
		if (!gConfig.fIsWIN95)
			PAL_EndingAnimation();
		break;

	case 0x0097:
		//
		// Ride the event object to the specified position, at a higher speed
		//
		PAL_PartyRideEventObject(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
			pScript->rgwOperand[2], 8);
		break;

	case 0x0098:
		//
		// Set follower of the party
		//
		j = 0;
		for (i = 0; i < 2; i++)
			if (pScript->rgwOperand[i] > 0)
			{
				int curFollower = j = i + 1;
				gpGlobals->nFollower = curFollower;
				gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + curFollower].wPlayerRole = pScript->rgwOperand[i];

				PAL_SetLoadFlags(kLoadPlayerSprite);
				PAL_LoadResources();

				//
				// Update the position and gesture for the follower
				//
				gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + curFollower].x =
					gpGlobals->rgTrail[3 + i].x - PAL_X(gpGlobals->viewport);
				gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + curFollower].y =
					gpGlobals->rgTrail[3 + i].y - PAL_Y(gpGlobals->viewport);
				gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + curFollower].wFrame =
					gpGlobals->rgTrail[3 + i].wDirection * 3;
			}
		if (j == 0)
		{
			gpGlobals->nFollower = 0;
		}
		break;

	case 0x0099:
		//
		// Change the map for the specified scene
		//
		if (pScript->rgwOperand[0] == 0xFFFF)
		{
			gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wMapNum = pScript->rgwOperand[1];
			PAL_SetLoadFlags(kLoadScene);
			PAL_LoadResources();
		}
		else
		{
			gpGlobals->g.rgScene[pScript->rgwOperand[0] - 1].wMapNum = pScript->rgwOperand[1];
		}
		break;

	case 0x009A:
		//
		// Set the state for multiple event objects
		//
		for (i = pScript->rgwOperand[0]; i <= pScript->rgwOperand[1]; i++)
		{
			gpGlobals->g.lprgEventObject[i - 1].sState = pScript->rgwOperand[2];
		}
		break;

	case 0x009B:
		//
		// Fade to the current scene
		// FIXME: This is obviously wrong
		//
		VIDEO_BackupScreen(gpScreen);
		PAL_MakeScene();
		VIDEO_FadeScreen(2);
		break;

	case 0x009C:
		//
		// Enemy duplicate itself
		//
		w = 0;

		for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
		{
			if (g_Battle.rgEnemy[i].wObjectID != 0)
			{
				w++;
			}
		}

		if (w != 1 || g_Battle.rgEnemy[wCurEventObjectID].e.wHealth <= 1)
		{
			//
			// Division is only possible when only 1 enemy left
			// health too low also cannot division
			//
			if (pScript->rgwOperand[1] != 0)
			{
				wScriptEntry = pScript->rgwOperand[1] - 1;
			}
			break;
		}

		w = pScript->rgwOperand[0];
		if (w == 0)
		{
			w = 1;
		}
		x = w + 1;
		y = w;

		//division does not limited by original team layout
		for (i = 0; i < MAX_ENEMIES_IN_TEAM; i++)
		{
			if (w > 0 && g_Battle.rgEnemy[i].wObjectID == 0)
			{
				w--;

				//notice: MAX MAY VARYING IN DIVISION!
				memset(&(g_Battle.rgEnemy[i]), 0, sizeof(BATTLEENEMY));

				g_Battle.rgEnemy[i].wObjectID = g_Battle.rgEnemy[wEventObjectID].wObjectID;
				g_Battle.rgEnemy[i].e = g_Battle.rgEnemy[wEventObjectID].e;
				g_Battle.rgEnemy[i].iMaxHealth = g_Battle.rgEnemy[wEventObjectID].iMaxHealth;
				g_Battle.rgEnemy[i].iPrevHP = g_Battle.rgEnemy[wEventObjectID].iPrevHP;
				g_Battle.rgEnemy[i].e.wHealth = (g_Battle.rgEnemy[wEventObjectID].e.wHealth + y) / x;
				g_Battle.rgEnemy[i].wScriptOnTurnStart = g_Battle.rgEnemy[wEventObjectID].wScriptOnTurnStart;
				g_Battle.rgEnemy[i].wScriptOnBattleEnd = g_Battle.rgEnemy[wEventObjectID].wScriptOnBattleEnd;
				g_Battle.rgEnemy[i].wScriptOnReady = g_Battle.rgEnemy[wEventObjectID].wScriptOnReady;

				g_Battle.rgEnemy[i].state = kFighterWait;
				g_Battle.rgEnemy[i].flTimeMeter = 50;
				g_Battle.rgEnemy[i].iColorShift = 0;

			}
		}
		g_Battle.rgEnemy[wCurEventObjectID].e.wHealth = (g_Battle.rgEnemy[wEventObjectID].e.wHealth + y) / x;
		g_Battle.rgEnemy[wEventObjectID].e.wHealth = g_Battle.rgEnemy[wCurEventObjectID].e.wHealth;

		w = 0;
		for (i = 0; i < MAX_ENEMIES_IN_TEAM; i++)
			if (g_Battle.rgEnemy[i].wObjectID != 0)
				w = i;
		g_Battle.wMaxEnemyIndex = w;

		PAL_LoadBattleSprites();

		for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
		{
			if (g_Battle.rgEnemy[i].wObjectID == 0)
			{
				continue;
			}
			g_Battle.rgEnemy[i].pos = g_Battle.rgEnemy[wEventObjectID].pos;
		}

		for (i = 0; i < 10; i++)
		{
			for (j = 0; j <= g_Battle.wMaxEnemyIndex; j++)
			{
				x = (PAL_X(g_Battle.rgEnemy[j].pos) + PAL_X(g_Battle.rgEnemy[j].posOriginal)) / 2;
				y = (PAL_Y(g_Battle.rgEnemy[j].pos) + PAL_Y(g_Battle.rgEnemy[j].posOriginal)) / 2;

				g_Battle.rgEnemy[j].pos = PAL_XY(x, y);
			}

			PAL_BattleDelay(1, 0, TRUE);
		}

		PAL_BattleUpdateFighters();
		PAL_BattleDelay(1, 0, TRUE);
		break;

	case 0x009D:
		break;

	case 0x009E:
		//
		// Enemy summons another monster
		//
	{
		WORD wEmptyEnemySiteNum = 0;
		WORD wNewEnemyID = pScript->rgwOperand[0];//召唤怪物的id
		WORD wNewEnemyNum = max(pScript->rgwOperand[1], 1);//召唤怪物的数量

		//wNewEnemyID = g_Battle.rgEnemy[wEventObjectID].wObjectID;

		for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
		{
			if (g_Battle.rgEnemy[i].wObjectID == 0)
			{
				wEmptyEnemySiteNum++;
			}
		}

		if (wEmptyEnemySiteNum < wNewEnemyNum || !PAL_New_IfEnemyCanMove(wEventObjectID))
		{
			if (pScript->rgwOperand[2] != 0)
			{
				// 召唤失败，则跳转
				wScriptEntry = pScript->rgwOperand[2] - 1;
			}
		}
		else
		{
			for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
			{
				if (g_Battle.rgEnemy[i].wObjectID == 0)
				{
					memset(&(g_Battle.rgEnemy[i]), 0, sizeof(BATTLEENEMY));

					g_Battle.rgEnemy[i].wObjectID = wNewEnemyID;
					g_Battle.rgEnemy[i].e = gpGlobals->g.lprgEnemy[gpGlobals->g.rgObject[wNewEnemyID].enemy.wEnemyID];
					//g_Battle.rgEnemy[i].e.wHealth = g_Battle.rgEnemy[wEventObjectID].e.wHealth;
#ifdef STRENGTHEN_ENEMY
					//
					// __DEBUG__困难难度下加强敌人
					//
					if (gpGlobals->wDifficultyNum != 0)
						g_Battle.rgEnemy[i] = PAL_New_StrengthenEnemy(g_Battle.rgEnemy[i]);
					else if (gpGlobals->wDifficultyNum == 0)
						g_Battle.rgEnemy[i].e.wHealth *= 0.7f;
					g_Battle.rgEnemy[i].iPrevHP = g_Battle.rgEnemy[i].e.wHealth;
#endif

					g_Battle.rgEnemy[i].state = kFighterWait;
					g_Battle.rgEnemy[i].wScriptOnTurnStart = gpGlobals->g.rgObject[wNewEnemyID].enemy.wScriptOnTurnStart;
					g_Battle.rgEnemy[i].wScriptOnBattleEnd = gpGlobals->g.rgObject[wNewEnemyID].enemy.wScriptOnBattleEnd;
					g_Battle.rgEnemy[i].wScriptOnReady = gpGlobals->g.rgObject[wNewEnemyID].enemy.wScriptOnReady;
					g_Battle.rgEnemy[i].flTimeMeter = 50;
					g_Battle.rgEnemy[i].iColorShift = 8;

					wNewEnemyNum--;
					if (wNewEnemyNum <= 0)
					{
						break;
					}
				}
			}

			VIDEO_BackupScreen(g_Battle.lpSceneBuf);
			PAL_LoadBattleSprites();
			PAL_BattleMakeScene();
			AUDIO_PlaySound(212);
			PAL_BattleFadeScene();

			// avoid releasing gesture disappears before summon done
			PAL_BattleDelay(2, 0, TRUE);

			for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
			{
				g_Battle.rgEnemy[i].iColorShift = 0;
			}

			VIDEO_BackupScreen(g_Battle.lpSceneBuf);
			PAL_BattleMakeScene();
			PAL_BattleFadeScene();
		}
		break;
	}

	case 0x009F:
		//
		// Enemy transforms into something else
		//
	{
		WORD wEnemyIndex = wEventObjectID;
		WORD wNewEnemyID = pScript->rgwOperand[0];

		if (PAL_New_IfEnemyCanMove(wEnemyIndex))
		{
			WORD wPrveLevel = g_Battle.rgEnemy[wEnemyIndex].e.wLevel;
			WORD wPrevCollectValue = g_Battle.rgEnemy[wEnemyIndex].e.wCollectValue;
			WORD wPrevExp = g_Battle.rgEnemy[wEnemyIndex].e.wExp;
			w = g_Battle.rgEnemy[wEnemyIndex].e.wHealth;

			g_Battle.rgEnemy[wEnemyIndex].wObjectID = wNewEnemyID;
			g_Battle.rgEnemy[wEnemyIndex].e =
				gpGlobals->g.lprgEnemy[gpGlobals->g.rgObject[wNewEnemyID].enemy.wEnemyID];

			g_Battle.rgEnemy[wEnemyIndex].e.wLevel = max(g_Battle.rgEnemy[wEnemyIndex].e.wLevel, wPrveLevel);
			g_Battle.rgEnemy[wEnemyIndex].e.wCollectValue = max(g_Battle.rgEnemy[wEnemyIndex].e.wCollectValue, wPrevCollectValue);
			g_Battle.rgEnemy[wEnemyIndex].e.wExp = max(g_Battle.rgEnemy[wEnemyIndex].e.wExp, wPrevExp);
			g_Battle.rgEnemy[wEnemyIndex].e.wHealth = w;
			g_Battle.rgEnemy[wEnemyIndex].wCurrentFrame = 0;
			g_Battle.rgEnemy[wEnemyIndex].wScriptOnBattleEnd =
				gpGlobals->g.rgObject[wNewEnemyID].enemy.wScriptOnBattleEnd; //修正怪物变身后的战后脚本

			for (i = 0; i < MAX_ENEMIES_IN_TEAM; i++)
			{
				g_Battle.rgEnemy[wEnemyIndex].iColorShift = i;
				PAL_BattleDelay(1, 0, FALSE);
			}

			g_Battle.rgEnemy[wEnemyIndex].iColorShift = 0;

			AUDIO_PlaySound(212);
			VIDEO_BackupScreen(g_Battle.lpSceneBuf);
			PAL_LoadBattleSprites();
			PAL_BattleMakeScene();
			PAL_BattleFadeScene();
		}
		break;
	}

	case 0x00A0:
		//
		// Quit game
		//
		if (gConfig.fIsWIN95)
			PAL_EndingScreen();
		PAL_AdditionalCredits();
		PAL_Shutdown(0);
		break;

	case 0x00A1:
		//
		// Set the positions of all party members to the same as the first one
		//
		for (i = 0; i < MAX_PLAYABLE_PLAYER_ROLES; i++)
		{
			gpGlobals->rgTrail[i].wDirection = gpGlobals->wPartyDirection;
			gpGlobals->rgTrail[i].x = gpGlobals->rgParty[0].x + PAL_X(gpGlobals->viewport);
			gpGlobals->rgTrail[i].y = gpGlobals->rgParty[0].y + PAL_Y(gpGlobals->viewport);
		}
		for (i = 1; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			gpGlobals->rgParty[i].x = gpGlobals->rgParty[0].x;
			gpGlobals->rgParty[i].y = gpGlobals->rgParty[0].y - 1;
		}
		PAL_UpdatePartyGestures(FALSE);
		break;

	case 0x00A2:
		//
		// Jump to one of the following instructions randomly
		//
		wScriptEntry += RandomLong(0, pScript->rgwOperand[0] - 1);
		break;

	case 0x00A3:
		//
		// Play CD music. Use the RIX music for fallback.
		//
		gpGlobals->wNumMusic = pScript->rgwOperand[1];
		if (AUDIO_CD_Available())
		{
			int numTrack = (SHORT)pScript->rgwOperand[0];
			if (!AUDIO_PlayCDTrack(numTrack == -1 ? -2 : numTrack))
				AUDIO_PlayMusic(pScript->rgwOperand[1], TRUE, 0);
		}
		else
			AUDIO_PlayMusic(pScript->rgwOperand[1], TRUE, 0);
		break;

	case 0x00A4:
		//
		// Scroll FBP to the screen
		//
		if (!gConfig.fIsWIN95)
		{
			if (pScript->rgwOperand[0] == 68)
			{
				//
				// HACKHACK: to make the ending picture show correctly
				//
				PAL_ShowFBP(69, 0);
			}
			PAL_ScrollFBP(pScript->rgwOperand[0], pScript->rgwOperand[2], TRUE);
		}
		break;

	case 0x00A5:
		//
		// Show FBP picture with sprite effects
		//
		if (!gConfig.fIsWIN95)
		{
			if (pScript->rgwOperand[1] != 0xFFFF)
			{
				PAL_EndingSetEffectSprite(pScript->rgwOperand[1]);
			}
			PAL_ShowFBP(pScript->rgwOperand[0], pScript->rgwOperand[2]);
		}
		break;

	case 0x00A6:
		//
		// backup screen
		//
		VIDEO_BackupScreen(gpScreen);
		break;

	case 0x00A8:
		// Remove character enhancements
		PAL_RemovePlayerStatus(wEventObjectID, 5);
		PAL_RemovePlayerStatus(wEventObjectID, 6);
		PAL_RemovePlayerStatus(wEventObjectID, 7);
		PAL_RemovePlayerStatus(wEventObjectID, 8);
		PAL_RemovePlayerStatus(wEventObjectID, 9);
		PAL_RemovePlayerStatus(wEventObjectID, 14);
		PAL_RemoveEquipmentEffect(wEventObjectID, kBodyPartExtra);
		break;

	case 0x00A9:
		//
		//All of us are silent
		//
	{
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			w = gpGlobals->rgParty[i].wPlayerRole;
			if (RandomLong(0, 100) > PAL_New_GetPlayerSorceryResistance(w))
			{
				PAL_SetPlayerStatus(w, 3, 3);
			}
		}
		break;
	}

	case 0x00AA:
		//
		// Set the direction and/or gesture for event object
		//
		if (pScript->rgwOperand[0] != 0)
		{
			pCurrent->wDirection = pScript->rgwOperand[1];
		}
		if (pScript->rgwOperand[2] != 0)
		{
			pCurrent->wCurrentFrameNum = pScript->rgwOperand[2];
		}
		break;

	case 0x00AB:
		//
		// Percentage Increase/decrease player's HP 
		//
		if (pScript->rgwOperand[0])
		{
			//
			// Apply to everyone
			//
			g_fScriptSuccess = FALSE;
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				w = gpGlobals->rgParty[i].wPlayerRole;
				if (PAL_IncreaseHPMP(w, PAL_GetPlayerMaxHP(w) * (SHORT)pScript->rgwOperand[1] / 100, 0))
					g_fScriptSuccess = TRUE;
			}
		}
		else
		{
			//
			// Apply to one player
			//
			if (!PAL_IncreaseHPMP(wEventObjectID, PAL_GetPlayerMaxHP(wEventObjectID) * (SHORT)pScript->rgwOperand[1] / 100, 0))
			{
				g_fScriptSuccess = FALSE;
			}
		}
		break;

	case 0x00AC:
		//
		// Percentage Increase/decrease player's MP
		//
		if (pScript->rgwOperand[0])
		{
			//
			// Apply to everyone
			//
			g_fScriptSuccess = FALSE;
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				w = gpGlobals->rgParty[i].wPlayerRole;
				if (PAL_IncreaseHPMP(w, 0, PAL_GetPlayerMaxMP(w) * (SHORT)pScript->rgwOperand[1] / 100))
					g_fScriptSuccess = TRUE;
			}
		}
		else
		{
			//
			// Apply to one player
			//
			if (!PAL_IncreaseHPMP(wEventObjectID, 0, PAL_GetPlayerMaxMP(wEventObjectID) * (SHORT)pScript->rgwOperand[1] / 100))
			{
				g_fScriptSuccess = FALSE;
			}
		}
		break;

	case 0x00AD:
		//
		// 打开灵葫商店
		//
		PAL_MakeScene();
		VIDEO_UpdateScreen(NULL);
		PAL_BuyLingHu(0);
		break;

	case 0x00AE:
		//
		// 指定对象置帧
		//
		if (pScript->rgwOperand[0] != 0)
		{
			pCurrent->wCurrentFrameNum = pScript->rgwOperand[1];
			pCurrent->wDirection = kDirSouth;
		}
		break;

	case 0x00AF:
	{
		//
		//我方全体百分比增加临时属性
		//
		INT* p;
		p = (INT*)(&gpGlobals->rgEquipmentEffect[kBodyPartExtra]); // HACKHACK
		INT* p1;
		p1 = (INT*)(&gpGlobals->g.PlayerRoles);


		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			w = gpGlobals->rgParty[i].wPlayerRole;
			p[pScript->rgwOperand[0] * MAX_PLAYER_ROLES + w] =
				p1[pScript->rgwOperand[0] * MAX_PLAYER_ROLES + w] * 0.3;
		}
	}
	break;

	case 0x00b0:
		//
		// 张四哥划船
		//
		PAL_PartyRideEventObject(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
			pScript->rgwOperand[2], 2);
		break;

	case 0x00b1:
		//
		// 本对象慢速走到
		//
		if ((wEventObjectID & 1) ^ (gpGlobals->dwFrameNum & 1))
		{
			if (!PAL_NPCWalkTo(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
				pScript->rgwOperand[2], 2))
			{
				wScriptEntry--;
			}
		}
		else
		{
			wScriptEntry--;
		}
		break;

	case 0x00B2:
		//
		// Percentage of blood loss
		//
	{
		UINT w;
		if (PAL_IsPlayerPoisonedByKind(wEventObjectID, 563) || PAL_GetPlayerPoisonResistance(wEventObjectID) == 100)
		{
			w = g_Battle.rgEnemy[wEventObjectID].iMaxHealth * 0.03;
		}
		else
		{
			w = g_Battle.rgEnemy[wEventObjectID].iMaxHealth * 0.06;
		}
		if (gpGlobals->g.PlayerRoles.rgwHP[RoleID_ANu] > 0)
		{
			g_Battle.rgEnemy[wEventObjectID].e.wHealth -= w;
		}
		else
		{
			PAL_New_CurePoisonForEnemyByKind(wEventObjectID, 629);
		}

		if (g_Battle.rgEnemy[wEventObjectID].e.wHealth > g_Battle.rgEnemy[wEventObjectID].iMaxHealth)
		{
			g_Battle.rgEnemy[wEventObjectID].e.wHealth = 0;
		}
		break;
	}

	case 0x00B3:
		//
		// 
		gpGlobals->g.PlayerRoles.rgwHP[wEventObjectID] /= 2;
		gpGlobals->g.PlayerRoles.rgwMP[wEventObjectID] /= 2;
		break;

	case 0x00B4:
		//
		// Increase/decrease player's HP
		//
		if (pScript->rgwOperand[0])
		{
			g_fScriptSuccess = FALSE;
			//
			// Apply to everyone
			//
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				w = gpGlobals->rgParty[i].wPlayerRole;
				if (PAL_IncreaseHPMP(w, 600 + PAL_GetPlayerMaxHP(w) * 0.25, 0))
					g_fScriptSuccess = TRUE;
			}
		}
		break;

	case 0x00B5:
		//
		// Jump if player's HP is more than the specified percentage
		//
		if ((gpGlobals->g.PlayerRoles.rgwHP[wEventObjectID]) * 100 >
			(PAL_GetPlayerMaxHP(wEventObjectID) * pScript->rgwOperand[0]))
		{
			wScriptEntry = pScript->rgwOperand[1] - 1;
		}
		break;

	case 0x00B6:
		//
		// 偷取我方道具
		//
	{
		srand(time(NULL));
		int  index = 61 + rand() % (259 - 61 + 1);
		int c = 200 + rand() % (3000 - 200);

		if (index != 93 || index != 113 || index != 114 || index != 118 || index != 145 || index != 168)
		{
			if (PAL_GetItemAmount(index) >= 1)
			{
				PAL_AddItemToInventory(index, -1);
				PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"%ls @%ls@", L"丢失", PAL_GetWord(index));
				PAL_StartDialog(kDialogCenterWindow, 0, 0, FALSE);
				PAL_ShowDialogText(s);
			}
			else if (PAL_GetItemAmount(index) >= 1)
			{
				PAL_AddItemToInventory(index, -1);
				PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"%ls @%ls@", L"丢失", PAL_GetWord(index));
				PAL_StartDialog(kDialogCenterWindow, 0, 0, FALSE);
				PAL_ShowDialogText(s);
			}
			else if (gpGlobals->dwCash > 3000)
			{
				gpGlobals->dwCash -= c;
				PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"@%ls@%d@%ls@", L"丢失", c, PAL_GetWord(10));
				PAL_StartDialog(kDialogCenterWindow, 0, 0, FALSE);
				PAL_ShowDialogText(s);
			}
			else
			{
				PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"%ls", L"偷窃失败");
				PAL_StartDialog(kDialogCenterWindow, 0, 0, FALSE);
				PAL_ShowDialogText(s);
			}
		}
		break;
	}

	case 0x00b7:
		//
		// 敌方全体百分比恢复血量
		//

		for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
		{
			if (g_Battle.rgEnemy[i].wObjectID != 0)
			{
				g_Battle.rgEnemy[i].e.wHealth += g_Battle.rgEnemy[i].iMaxHealth * 0.1;
			}
			if (g_Battle.rgEnemy[i].e.wHealth > g_Battle.rgEnemy[i].iMaxHealth)
			{
				g_Battle.rgEnemy[i].e.wHealth = g_Battle.rgEnemy[i].iMaxHealth;
			}

		}
		break;

	case 0x00b8:
		//提升一半防御
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			WORD w = gpGlobals->rgParty[i].wPlayerRole;
			PAL_SetPlayerStatus(w, 14, 99);
		}
		break;


	case 0x00B9:
		//
		// 当我方死亡，敌方回复体力
		//
		//
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			w = gpGlobals->rgParty[i].wPlayerRole;
			if (gpGlobals->g.PlayerRoles.rgwHP[w] == 0)
			{
				g_Battle.rgEnemy[wEventObjectID].e.wHealth += (PAL_GetPlayerMaxHP(w) * 3);
			}
			else if (gpGlobals->g.PlayerRoles.rgwHP[w] == 0 && g_Battle.rgEnemy[wEventObjectID].e.wLevel >= 150)
			{
				g_Battle.rgEnemy[wEventObjectID].e.wHealth += (PAL_GetPlayerMaxHP(w) * 6);
			}

			if (g_Battle.rgEnemy[wEventObjectID].e.wHealth > g_Battle.rgEnemy[wEventObjectID].iMaxHealth)
			{
				g_Battle.rgEnemy[wEventObjectID].e.wHealth = g_Battle.rgEnemy[wEventObjectID].iMaxHealth;
			}
		}
		break;


	case 0x00BA:
		//
		// 获得经验值
		//
		//
	{
		WCHAR s[256];
		DWORD dwExp;
		BOOL   fLevelUp;
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			w = gpGlobals->rgParty[i].wPlayerRole;
			gpGlobals->Exp.rgPrimaryExp[w].wExp += pScript->rgwOperand[0] * 2;
			PAL_StartDialog(kDialogCenterWindow, 0, 0, FALSE);
			PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"%ls%ls%d%ls", PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]), PAL_GetWord(34), pScript->rgwOperand[0] * 2, PAL_GetWord(2));
			PAL_ShowDialogText(s);
		}

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
				PAL_StartDialog(kDialogCenterWindow, 0, 0, FALSE);
				PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"%ls%ls%ls", PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]), PAL_GetWord(STATUS_LABEL_LEVEL), PAL_GetWord(BATTLEWIN_LEVELUP_LABEL));
				PAL_ShowDialogText(s);
			}
		}
	}

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
					PAL_StartDialog(kDialogCenterWindow, 0, 0, FALSE);
					PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"%ls%ls%ls", PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]), PAL_GetWord(BATTLEWIN_ADDMAGIC_LABEL), PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[j].m[w].wMagic));
					PAL_ShowDialogText(s);
				}

				j++;
			}
		}
	}
	break;

	case 0x00bb:
		//
		//为我方全体添加所有增益状态，驱散所有负面状态
		//
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			w = gpGlobals->rgParty[i].wPlayerRole;
			PAL_CurePoisonByLevel(w, 10);
			PAL_RemovePlayerStatus(w, 0);
			PAL_RemovePlayerStatus(w, 1);
			PAL_RemovePlayerStatus(w, 2);
			PAL_RemovePlayerStatus(w, 3);
			PAL_RemovePlayerStatus(w, 10);
			PAL_RemovePlayerStatus(w, 11);
			PAL_RemovePlayerStatus(w, 12);
			PAL_RemovePlayerStatus(w, 13);
			PAL_SetPlayerStatus(w, 5, 6);
			PAL_SetPlayerStatus(w, 6, 6);
			PAL_SetPlayerStatus(w, 7, 6);
			PAL_SetPlayerStatus(w, 8, 6);
			PAL_SetPlayerStatus(w, 9, 6);
		}
		break;

	case 0x00bc:
		//
		//为我方全体添加身法加快
		//
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			w = gpGlobals->rgParty[i].wPlayerRole;
			PAL_SetPlayerStatus(w, 7, 7);
		}
		break;

	case 0x00bd:
		//
		//Percentage Increase/decrease player's HP and MP
		//
		if (pScript->rgwOperand[0])
		{
			//
			// Apply to everyone
			//
			g_fScriptSuccess = FALSE;
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				w = gpGlobals->rgParty[i].wPlayerRole;
				if (PAL_IncreaseHPMP(w, PAL_GetPlayerMaxHP(w) * (SHORT)pScript->rgwOperand[1] / 100, PAL_GetPlayerMaxMP(w) * (SHORT)pScript->rgwOperand[2] / 100))
					g_fScriptSuccess = TRUE;
			}
		}
		else
		{
			//
			// Apply to one player 
			//
			if (!PAL_IncreaseHPMP(wEventObjectID, PAL_GetPlayerMaxHP(wEventObjectID) * (SHORT)pScript->rgwOperand[1] / 100, PAL_GetPlayerMaxMP(wEventObjectID) * (SHORT)pScript->rgwOperand[2] / 100))
			{
				g_fScriptSuccess = FALSE;
			}
		}
		break;

	case 0x00be:
		//
		// Walk the party to the specified position
		//
		PAL_PartyWalkTo(pScript->rgwOperand[0], pScript->rgwOperand[1], pScript->rgwOperand[2], 3);
		break;

	default:
		TerminateOnError("SCRIPT: Invalid Instruction at %4x: (%4x - %4x, %4x, %4x)",
			wScriptEntry, pScript->wOperation, pScript->rgwOperand[0],
			pScript->rgwOperand[1], pScript->rgwOperand[2]);
		break;
	}

	return wScriptEntry + 1;
}

PAL_FORCE_INLINE
INT
MESSAGE_GetSpan(
	WORD* pwScriptEntry
)
/*++
 Purpose:

 Get the final span of a message block which started from message index of wScriptEntry

 Parameters:

 [IN]  pwScriptEntry - The pointer of script entry which starts the message block, must be a 0xffff command.

 Return value:

 The final span of the message block.

 --*/
{
	int currentScriptEntry = *pwScriptEntry;
	int result = 0;
	int beginning = 1;
	int firstMsgIndex, lastMsgIndex;

	// ensure the command is 0xFFFF
	assert(gpGlobals->g.lprgScriptEntry[currentScriptEntry].wOperation == 0xFFFF);

	firstMsgIndex = lastMsgIndex = gpGlobals->g.lprgScriptEntry[currentScriptEntry].rgwOperand[0];

	//
	// If the NEXT command is 0xFFFF, but the message index is not continuous or not incremental,
	// this MESSAGE block shoud end at THIS command.
	//
	if (gpGlobals->g.lprgScriptEntry[currentScriptEntry + 1].wOperation == 0xFFFF && gpGlobals->g.lprgScriptEntry[currentScriptEntry + 1].rgwOperand[0] != lastMsgIndex + 1)
		currentScriptEntry++;
	else
		while ((gpGlobals->g.lprgScriptEntry[currentScriptEntry].wOperation == 0xFFFF &&
			(!beginning ? gpGlobals->g.lprgScriptEntry[currentScriptEntry].rgwOperand[0] == lastMsgIndex + 1 : 1))
			|| gpGlobals->g.lprgScriptEntry[currentScriptEntry].wOperation == 0x008E)
		{
			if (gpGlobals->g.lprgScriptEntry[currentScriptEntry].wOperation == 0xFFFF)
				lastMsgIndex = gpGlobals->g.lprgScriptEntry[currentScriptEntry].rgwOperand[0];
			currentScriptEntry++;
			beginning = 0;
		}

	result = lastMsgIndex - firstMsgIndex;
	assert(result >= 0);
	*pwScriptEntry = currentScriptEntry;
	return result;
}

WORD
PAL_RunTriggerScript(
	WORD           wScriptEntry,
	WORD           wEventObjectID
)
/*++
  Purpose:

	Runs a trigger script.

  Parameters:

	[IN]  wScriptEntry - The script entry to execute.

	[IN]  wEventObjectID - The event object ID which invoked the script.

  Return value:

	The entry point of the script.

--*/
{
	static WORD       wLastEventObject = 0;

	WORD              wNextScriptEntry;
	BOOL              fEnded;
	LPSCRIPTENTRY     pScript;
	LPEVENTOBJECT     pEvtObj = NULL;
	int               i;

	extern BOOL       g_fUpdatedInBattle; // HACKHACK

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
	//
	PAL_DialogSetDelayTime(2);

	while (wScriptEntry != 0 && !fEnded)
	{
		pScript = &(gpGlobals->g.lprgScriptEntry[wScriptEntry]);

		UTIL_LogOutput(LOGLEVEL_DEBUG, "[SCRIPT] %.4x: %.4x %.4x %.4x %.4x\n", wScriptEntry,
			pScript->wOperation, pScript->rgwOperand[0],
			pScript->rgwOperand[1], pScript->rgwOperand[2]);

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
			PAL_RunTriggerScript(pScript->rgwOperand[0],
				((pScript->rgwOperand[1] == 0) ? wEventObjectID : pScript->rgwOperand[1]));
			wScriptEntry++;
			break;

		case 0x0005:
			//
			// Redraw screen
			//
			PAL_ClearDialog(TRUE);

			if (PAL_DialogIsPlayingRNG())
			{
				VIDEO_RestoreScreen(gpScreen);
			}
			else if (gpGlobals->fInBattle)
			{
				PAL_BattleMakeScene();
				VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);
				VIDEO_UpdateScreen(NULL);
			}
			else
			{
				if (pScript->rgwOperand[2])
				{
					PAL_UpdatePartyGestures(FALSE);
				}

				PAL_MakeScene();

				VIDEO_UpdateScreen(NULL);
				UTIL_Delay((pScript->rgwOperand[1] == 0) ? 60 : (pScript->rgwOperand[1] * 60));
			}

			wScriptEntry++;
			break;

		case 0x0006:
			//
			// Jump to the specified address by the specified rate
			//
			if (RandomLong(1, 100) >= pScript->rgwOperand[0])
			{
				wScriptEntry = pScript->rgwOperand[1];
				continue;
			}
			else
			{
				wScriptEntry++;
			}
			break;

		case 0x0007:
			//
			// Start battle
			//

			//
			// __DEBUG__隐藏触发战斗的敌人
			//
			//if (pScript->rgwOperand[2] && pEvtObj->wSpriteNum != 0x010A)
			//{
			//	//
			//	// 不要淡入淡出
			//	//
			//	gpGlobals->fIsNotFaded = TRUE;
			//	pEvtObj->sState = 0;
			//	// 
			//	// 自动存档
			//	//
			//	gpGlobals->fDoAutoSave = TRUE;
			//}

			i = PAL_StartBattle(pScript->rgwOperand[0], !pScript->rgwOperand[2]);

			//
			// 鹿的战斗单独处理
			//
			//if (pEvtObj->wSpriteNum == 0x010A)
			//	i = kBattleResultFleed;

			if (i == kBattleResultLost && pScript->rgwOperand[1] != 0)
			{
				wScriptEntry = pScript->rgwOperand[1];
			}
			else if (i == kBattleResultFleed && pScript->rgwOperand[2] != 0)
			{
				wScriptEntry = pScript->rgwOperand[2];
			}
			else
			{
				wScriptEntry++;
			}
			gpGlobals->fAutoBattle = FALSE;
			break;

		case 0x0008:
			//
			// Replace the entry with the next instruction
			//
			wScriptEntry++;
			wNextScriptEntry = wScriptEntry;
			break;

		case 0x0009:
			//
			// wait for the specified number of frames
			//
		{
			DWORD        time;

			PAL_ClearDialog(TRUE);

			time = SDL_GetTicks() + FRAME_TIME;

			for (i = 0; i < (pScript->rgwOperand[0] ? pScript->rgwOperand[0] : 1); i++)
			{
				PAL_DelayUntil(time);

				time = SDL_GetTicks() + FRAME_TIME;

				if (pScript->rgwOperand[2])
				{
					PAL_UpdatePartyGestures(FALSE);
				}

				PAL_GameUpdate(pScript->rgwOperand[1] ? TRUE : FALSE);
				PAL_MakeScene();
				VIDEO_UpdateScreen(NULL);
			}
		}
		wScriptEntry++;
		break;

		case 0x000A:
			//
			// Goto the specified address if player selected no
			//
			PAL_ClearDialog(FALSE);

			if (!PAL_ConfirmMenu())
			{
				wScriptEntry = pScript->rgwOperand[0];
			}
			else
			{
				wScriptEntry++;
			}
			break;

		case 0x003B:
			//
			// Show dialog in the middle part of the screen
			//
			PAL_ClearDialog(TRUE);
			PAL_StartDialog(kDialogCenter, (BYTE)pScript->rgwOperand[0], 0,
				pScript->rgwOperand[2] ? TRUE : FALSE);
			wScriptEntry++;
			break;

		case 0x003C:
			//
			// Show dialog in the upper part of the screen
			//
			PAL_ClearDialog(TRUE);
			PAL_StartDialog(kDialogUpper, (BYTE)pScript->rgwOperand[1],
				pScript->rgwOperand[0], pScript->rgwOperand[2] ? TRUE : FALSE);
			wScriptEntry++;
			break;

		case 0x003D:
			//
			// Show dialog in the lower part of the screen
			//
			PAL_ClearDialog(TRUE);
			PAL_StartDialog(kDialogLower, (BYTE)pScript->rgwOperand[1],
				pScript->rgwOperand[0], pScript->rgwOperand[2] ? TRUE : FALSE);
			wScriptEntry++;
			break;

		case 0x003E:
			//
			// Show text in a window at the center of the screen
			//
			PAL_ClearDialog(TRUE);
			//	 g_TextLib.iDialogShadow = 5;
			PAL_StartDialogWithOffset(kDialogCenterWindow, 0, 0, FALSE, 0, -10);
			//    PAL_StartDialog(kDialogCenterWindow, (BYTE)pScript->rgwOperand[0], 0, FALSE);
			wScriptEntry++;
			break;

		case 0x008E:
			//
			// Restore the screen
			//
			PAL_ClearDialog(TRUE);
			VIDEO_RestoreScreen(gpScreen);
			VIDEO_UpdateScreen(NULL);
			wScriptEntry++;
			break;

		case 0xFFFF:
			//
			// Print dialog text
			//
			if (gConfig.pszMsgFile)
			{
				int msgSpan = MESSAGE_GetSpan(&wScriptEntry);
				int idx = 0, iMsg;
				while ((iMsg = PAL_GetMsgNum(pScript->rgwOperand[0], msgSpan, idx++)) >= 0)
				{
					if (iMsg == 0)
					{
						//
						// Restore the screen
						//
						PAL_ClearDialog(TRUE);
						VIDEO_RestoreScreen(gpScreen);
						VIDEO_UpdateScreen(NULL);
					}
					else
						PAL_ShowDialogText(PAL_GetMsg(iMsg));
				}
			}
			else
			{
				PAL_ShowDialogText(PAL_GetMsg(pScript->rgwOperand[0]));
				wScriptEntry++;
			}
			break;

			//
			//接下来是新增加的命令
			//
		case 0x00FF:
		{
			// 新命令：随机跳转到之后的n条后的地址中的一条，
			// 执行这一条后，从这条命令开始处的n条后继续执行
			// n为 pScript->rgwOperand[0]的值
			WORD wNewScriptEntry = wScriptEntry + RandomLong(1, pScript->rgwOperand[0]);
			WORD wJumpScriptEntry = PAL_InterpretInstruction(wNewScriptEntry, wEventObjectID);

			wScriptEntry += pScript->rgwOperand[0] + 1;

			break;
		}

		default:
			PAL_ClearDialog(TRUE);
			wScriptEntry = PAL_InterpretInstruction(wScriptEntry, wEventObjectID);
			break;
		}
	}

	PAL_EndDialog();
	g_iCurEquipPart = -1;

	return wNextScriptEntry;
}

WORD
PAL_RunAutoScript(
	WORD           wScriptEntry,
	WORD           wEventObjectID
)
/*++
  Purpose:

	Runs the autoscript of the specified event object.

  Parameters:

	[IN]  wScriptEntry - The script entry to execute.

	[IN]  wEventObjectID - The event object ID which invoked the script.

  Return value:

	The address of the next script instruction to execute.

--*/
{
	LPSCRIPTENTRY          pScript;
	LPEVENTOBJECT          pEvtObj;

begin:
	pScript = &(gpGlobals->g.lprgScriptEntry[wScriptEntry]);
	pEvtObj = &(gpGlobals->g.lprgEventObject[wEventObjectID - 1]);

	UTIL_LogOutput(LOGLEVEL_DEBUG, "[AUTOSCRIPT] %04x %.4x: %.4x %.4x %.4x %.4x\n", wEventObjectID, wScriptEntry,
		pScript->wOperation, pScript->rgwOperand[0],
		pScript->rgwOperand[1], pScript->rgwOperand[2]);

	//
	// For autoscript, we should interpret one instruction per frame (except
	// jumping) and save the address of next instruction.
	//
	switch (pScript->wOperation)
	{
	case 0x0000:
		//
		// Stop running
		//
		break;

	case 0x0001:
		//
		// Stop running and replace the entry with the next line
		//
		wScriptEntry++;
		break;

	case 0x0002:
		//
		// Stop running and replace the entry with the specified one
		//
		if (pScript->rgwOperand[1] == 0 ||
			++(pEvtObj->wScriptIdleFrameCountAuto) < pScript->rgwOperand[1])
		{
			wScriptEntry = pScript->rgwOperand[0];
		}
		else
		{
			pEvtObj->wScriptIdleFrameCountAuto = 0;
			wScriptEntry++;
		}
		break;

	case 0x0003:
		//
		// unconditional jump
		//
		if (pScript->rgwOperand[1] == 0 ||
			++(pEvtObj->wScriptIdleFrameCountAuto) < pScript->rgwOperand[1])
		{
			wScriptEntry = pScript->rgwOperand[0];
			goto begin;
		}
		else
		{
			pEvtObj->wScriptIdleFrameCountAuto = 0;
			wScriptEntry++;
		}
		break;

	case 0x0004:
		//
		// Call subroutine
		//
		PAL_RunTriggerScript(pScript->rgwOperand[0],
			pScript->rgwOperand[1] ? pScript->rgwOperand[1] : wEventObjectID);
		wScriptEntry++;
		break;

	case 0x0006:
		//
		// jump to the specified address by the specified rate
		//
		if (RandomLong(1, 100) >= pScript->rgwOperand[0])
		{
			if (pScript->rgwOperand[1] != 0)
			{
				wScriptEntry = pScript->rgwOperand[1];
				goto begin;
			}
		}
		else
		{
			wScriptEntry++;
		}
		break;

	case 0x0009:
		//
		// Wait for a certain number of frames
		//
		if (++(pEvtObj->wScriptIdleFrameCountAuto) >= pScript->rgwOperand[0])
		{
			//
			// waiting ended; go further
			//
			pEvtObj->wScriptIdleFrameCountAuto = 0;
			wScriptEntry++;
		}
		break;

	case 0xFFFF:
		if (gConfig.fIsWIN95)
		{
			int XBase = (wEventObjectID & PAL_ITEM_DESC_BOTTOM) ? 71 : gConfig.ScreenLayout.MagicDescMsgPos;
			int YBase = (wEventObjectID & PAL_ITEM_DESC_BOTTOM) ? 151 - gConfig.ScreenLayout.ExtraItemDescLines * 16 : 3;
			int iDescLine = (wEventObjectID & ~PAL_ITEM_DESC_BOTTOM);

			if (gConfig.pszMsgFile)
			{
				int msgSpan = MESSAGE_GetSpan(&wScriptEntry);
				int idx = 0, iMsg;
				while ((iMsg = PAL_GetMsgNum(pScript->rgwOperand[0], msgSpan, idx++)) >= 0)
				{
					if (iMsg > 0)
					{
						PAL_DrawText(PAL_GetMsg(iMsg), PAL_XY(XBase, iDescLine * 16 + YBase), DESCTEXT_COLOR, TRUE, FALSE, FALSE);
						iDescLine++;
					}
				}
			}
			else
			{
				PAL_DrawText(PAL_GetMsg(pScript->rgwOperand[0]), PAL_XY(XBase, iDescLine * 16 + YBase), DESCTEXT_COLOR, TRUE, FALSE, FALSE);
				wScriptEntry++;
			}
		}
		else
		{
			wScriptEntry++;
		}
		break;

	case 0x00A7:
		wScriptEntry++;
		break;

	default:
		//
		// Other operations
		//
		wScriptEntry = PAL_InterpretInstruction(wScriptEntry, wEventObjectID);
		break;
	}

	return wScriptEntry;
}

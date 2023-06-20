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
// Portions based on PALx Project by palxex.
// Copyright (c) 2006, Pal Lockheart <palxex@gmail.com>.
//

#include "main.h"

#define MAX_SPRITE_TO_DRAW         2048

typedef struct tagSPRITE_TO_DRAW
{
	LPCBITMAPRLE     lpSpriteFrame; // pointer to the frame bitmap
	PAL_POS          pos;           // position on the scene
	int              iLayer;        // logical layer
} SPRITE_TO_DRAW;

static SPRITE_TO_DRAW    g_rgSpriteToDraw[MAX_SPRITE_TO_DRAW];
static int               g_nSpriteToDraw;

static VOID
PAL_AddSpriteToDraw(
	LPCBITMAPRLE     lpSpriteFrame,
	int              x,
	int              y,
	int              iLayer
)
/*++
   Purpose:

	 Add a sprite to our list of drawing.

   Parameters:

	 [IN]  lpSpriteFrame - the bitmap of the sprite frame.

	 [IN]  x - the X coordinate on the screen.

	 [IN]  y - the Y coordinate on the screen.

	 [IN]  iLayer - the layer of the sprite.

   Return value:

	 None.

--*/
{
	assert(g_nSpriteToDraw < MAX_SPRITE_TO_DRAW);

	g_rgSpriteToDraw[g_nSpriteToDraw].lpSpriteFrame = lpSpriteFrame;
	g_rgSpriteToDraw[g_nSpriteToDraw].pos = PAL_XY(x, y);
	g_rgSpriteToDraw[g_nSpriteToDraw].iLayer = iLayer;

	g_nSpriteToDraw++;
}

static VOID
PAL_CalcCoverTiles(
	SPRITE_TO_DRAW* lpSpriteToDraw
)
/*++
   Purpose:

	 Calculate all the tiles which may cover the specified sprite. Add the tiles
	 into our list as well.

   Parameters:

	 [IN]  lpSpriteToDraw - pointer to SPRITE_TO_DRAW struct.

   Return value:

	 None.

--*/
{
	int             x, y, i, l, iTileHeight;
	LPCBITMAPRLE    lpTile;

	const int       sx = PAL_X(gpGlobals->viewport) + PAL_X(lpSpriteToDraw->pos) - lpSpriteToDraw->iLayer / 2;
	const int       sy = PAL_Y(gpGlobals->viewport) + PAL_Y(lpSpriteToDraw->pos) - lpSpriteToDraw->iLayer;
	const int       sh = ((sx % 32) ? 1 : 0);

	const int       width = PAL_RLEGetWidth(lpSpriteToDraw->lpSpriteFrame);
	const int       height = PAL_RLEGetHeight(lpSpriteToDraw->lpSpriteFrame);

	int             dx = 0;
	int             dy = 0;
	int             dh = 0;

	//
	// Loop through all the tiles in the area of the sprite.
	//
	for (y = (sy - height - 15) / 16; y <= sy / 16; y++)
	{
		for (x = (sx - width / 2) / 32; x <= (sx + width / 2) / 32; x++)
		{
			for (i = ((x == (sx - width / 2) / 32) ? 0 : 3); i < 5; i++)
			{
				//
				// Scan tiles in the following form (* = to scan):
				//
				// . . . * * * . . .
				//  . . . * * . . . .
				//
				switch (i)
				{
				case 0:
					dx = x;
					dy = y;
					dh = sh;
					break;

				case 1:
					dx = x - 1;
					break;

				case 2:
					dx = (sh ? x : (x - 1));
					dy = (sh ? (y + 1) : y);
					dh = 1 - sh;
					break;

				case 3:
					dx = x + 1;
					dy = y;
					dh = sh;
					break;

				case 4:
					dx = (sh ? (x + 1) : x);
					dy = (sh ? (y + 1) : y);
					dh = 1 - sh;
					break;
				}

				for (l = 0; l < 2; l++)
				{
					lpTile = PAL_MapGetTileBitmap(dx, dy, dh, l, PAL_GetCurrentMap());
					iTileHeight = (signed char)PAL_MapGetTileHeight(dx, dy, dh, l, PAL_GetCurrentMap());

					//
					// Check if this tile may cover the sprites
					//
					if (lpTile != NULL && iTileHeight > 0 && (dy + iTileHeight) * 16 + dh * 8 >= sy)
					{
						//
						// This tile may cover the sprite
						//
						PAL_AddSpriteToDraw(lpTile,
							dx * 32 + dh * 16 - 16 - PAL_X(gpGlobals->viewport),
							dy * 16 + dh * 8 + 7 + l + iTileHeight * 8 - PAL_Y(gpGlobals->viewport),
							iTileHeight * 8 + l);
					}
				}
			}
		}
	}
}

static VOID
PAL_SceneDrawSprites(
	VOID
)
/*++
   Purpose:

	 Draw all the sprites to scene.

   Parameters:

	 None.

   Return value:

	 None.

--*/
{
	int i, x, y, vy;

	g_nSpriteToDraw = 0;

	//
	// Put all the sprites to be drawn into our array.
	//

	//
	// Players
	//
	for (i = 0; i <= (short)gpGlobals->wMaxPartyMemberIndex + gpGlobals->nFollower; i++)
	{
		LPCBITMAPRLE lpBitmap =
			PAL_SpriteGetFrame(PAL_GetPlayerSprite((BYTE)i), gpGlobals->rgParty[i].wFrame);

		if (lpBitmap == NULL)
		{
			continue;
		}

		//
		// Add it to our array
		//
		PAL_AddSpriteToDraw(lpBitmap,
			gpGlobals->rgParty[i].x - PAL_RLEGetWidth(lpBitmap) / 2,
			gpGlobals->rgParty[i].y + gpGlobals->wLayer + 10,
			gpGlobals->wLayer + 6);

		//
		// Calculate covering tiles on the map
		//
		PAL_CalcCoverTiles(&g_rgSpriteToDraw[g_nSpriteToDraw - 1]);
	}

	//
	// Event Objects (Monsters/NPCs/others)
	//
	for (i = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex;
		i < gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex; i++)
	{
		LPCBITMAPRLE     lpFrame;
		LPCSPRITE        lpSprite;

		LPEVENTOBJECT    lpEvtObj = &(gpGlobals->g.lprgEventObject[i]);

		int              iFrame;

		if (lpEvtObj->sState == kObjStateHidden || lpEvtObj->sVanishTime > 0 ||
			lpEvtObj->sState < 0)
		{
			continue;
		}

		//
		// Get the sprite
		//
		lpSprite = PAL_GetEventObjectSprite((WORD)i + 1);
		if (lpSprite == NULL)
		{
			continue;
		}

		iFrame = lpEvtObj->wCurrentFrameNum;
		if (lpEvtObj->nSpriteFrames == 3)
		{
			//
			// walking character
			//
			if (iFrame == 2)
			{
				iFrame = 0;
			}

			if (iFrame == 3)
			{
				iFrame = 2;
			}
		}

		lpFrame = PAL_SpriteGetFrame(lpSprite,
			lpEvtObj->wDirection * lpEvtObj->nSpriteFrames + iFrame);

		if (lpFrame == NULL)
		{
			continue;
		}

		//
		// Calculate the coordinate and check if outside the screen
		//
		x = (SHORT)lpEvtObj->x - PAL_X(gpGlobals->viewport);
		x -= PAL_RLEGetWidth(lpFrame) / 2;

		if (x >= 320 || x < -(int)PAL_RLEGetWidth(lpFrame))
		{
			//
			// outside the screen; skip it
			//
			continue;
		}

		y = (SHORT)lpEvtObj->y - PAL_Y(gpGlobals->viewport);
		y += lpEvtObj->sLayer * 8 + 9;

		vy = y - PAL_RLEGetHeight(lpFrame) - lpEvtObj->sLayer * 8 + 2;
		if (vy >= 200 || vy < -(int)PAL_RLEGetHeight(lpFrame))
		{
			//
			// outside the screen; skip it
			//
			continue;
		}

		//
		// Add it into the array
		//
		PAL_AddSpriteToDraw(lpFrame, x, y, lpEvtObj->sLayer * 8 + 2);

		//
		// Calculate covering map tiles
		//
		PAL_CalcCoverTiles(&g_rgSpriteToDraw[g_nSpriteToDraw - 1]);
	}

	//
	// All sprites are now in our array; sort them by their vertical positions.
	//
	for (x = 0; x < g_nSpriteToDraw - 1; x++)
	{
		SPRITE_TO_DRAW           tmp;
		BOOL                     fSwap = FALSE;

		for (y = 0; y < g_nSpriteToDraw - 1 - x; y++)
		{
			if (PAL_Y(g_rgSpriteToDraw[y].pos) > PAL_Y(g_rgSpriteToDraw[y + 1].pos))
			{
				fSwap = TRUE;

				tmp = g_rgSpriteToDraw[y];
				g_rgSpriteToDraw[y] = g_rgSpriteToDraw[y + 1];
				g_rgSpriteToDraw[y + 1] = tmp;
			}
		}

		if (!fSwap)
		{
			break;
		}
	}

	//
	// Draw all the sprites to the screen.
	//
	for (i = 0; i < g_nSpriteToDraw; i++)
	{
		SPRITE_TO_DRAW* p = &g_rgSpriteToDraw[i];

		x = PAL_X(p->pos);
		y = PAL_Y(p->pos) - PAL_RLEGetHeight(p->lpSpriteFrame) - p->iLayer;

		PAL_RLEBlitToSurface(p->lpSpriteFrame, gpScreen, PAL_XY(x, y));
	}
}

VOID
PAL_ApplyWave(
	SDL_Surface* lpSurface
)
/*++
   Purpose:

	 Apply screen waving effect when needed.

   Parameters:

	 [OUT] lpSurface - the surface to be proceed.

   Return value:

	 None.

--*/
{
	int                  wave[32];
	int                  i, a, b;
	static int           index = 0;
	LPBYTE               p;
	BYTE                 buf[320];

	gpGlobals->wScreenWave += gpGlobals->sWaveProgression;

	if (gpGlobals->wScreenWave == 0 || gpGlobals->wScreenWave >= 256)
	{
		//
		// No need to wave the screen
		// 不需要波动屏幕
		gpGlobals->wScreenWave = 0;
		gpGlobals->sWaveProgression = 0;
		return;
	}

	//
	// Calculate the waving offsets.
	// 计算波动偏移。
	a = 0;
	b = 60 + 8;

	for (i = 0; i < 16; i++)
	{
		b -= 8;
		a += b;

		//
		// WARNING: assuming the screen width is 320
		// 警告：假设屏幕宽度为320
		wave[i] = a * gpGlobals->wScreenWave / 256;
		wave[i + 16] = 320 - wave[i];
	}

	//
	// Apply the effect.
	// WARNING: only works with 320x200 8-bit surface.
	// 应用效果。
	// 警告：仅适用于320x200 8位表面。
	//
	a = index;
	p = (LPBYTE)(lpSurface->pixels);

	//
	// Loop through all lines in the screen buffer.
	// 循环浏览屏幕缓冲区中的所有行。
	for (i = 0; i < 200; i++)
	{
		b = wave[a];

		if (b > 0)
		{
			//
			// Do a shift on the current line with the calculated offset.
			// 用计算出的偏移量在当前行上进行偏移。
			memcpy(buf, p, b);
			//memmove(p, p + b, 320 - b);
			memmove(p, &p[b], 320 - b);
			//memcpy(p + 320 - b, buf, b);
			memcpy(&p[320 - b], buf, b);
		}

		a = (a + 1) % 32;
		p += lpSurface->pitch;
	}

	index = (index + 1) % 32;
}

VOID
PAL_MakeScene(
	VOID
)
/*++
   Purpose:

	 Draw the scene of the current frame to the screen. Both the map and
	 the sprites are handled here.

   Parameters:

	 None.

   Return value:

	 None.

--*/
{
	static SDL_Rect         rect = { 0, 0, 320, 200 };

	//
	// Step 1: Draw the complete map, for both of the layers.
	//
	rect.x = PAL_X(gpGlobals->viewport);
	rect.y = PAL_Y(gpGlobals->viewport);

	PAL_MapBlitToSurface(PAL_GetCurrentMap(), gpScreen, &rect, 0);
	PAL_MapBlitToSurface(PAL_GetCurrentMap(), gpScreen, &rect, 1);

	//
	// Step 2: Apply screen waving effects.
	//
	PAL_ApplyWave(gpScreen);

	//
	// Step 3: Draw all the sprites.
	//
	PAL_SceneDrawSprites();

	// 绘制驱魔香剩余时间
	if (gpGlobals->wChaseRange != 1)
	{
		PAL_DrawNumber(gpGlobals->wChasespeedChangeCycles, 3, PAL_XY(300, 0), kNumColorYellow, kNumAlignRight);
	}

	// 绘制当前灵葫值
	if (gpGlobals->wCollectValue != 0)
	{
		PAL_DrawNumber(gpGlobals->wCollectValue, 20, PAL_XY(198, 190), kNumColorCyan, kNumAlignRight);
	}

	//
	// Check if we need to fade in.
	//
	if (gpGlobals->fNeedToFadeIn)
	{
		VIDEO_UpdateScreen(NULL);
		PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
		gpGlobals->fNeedToFadeIn = FALSE;
	}
}

BOOL
PAL_CheckObstacle(
	PAL_POS         pos,
	BOOL            fCheckEventObjects,
	WORD            wSelfObject
)
/*++
   Purpose:

	 Check if the specified location has obstacle or not.

   Parameters:

	 [IN]  pos - the position to check.

	 [IN]  fCheckEventObjects - TRUE if check for event objects, FALSE if only
		   check for the map.

	 [IN]  wSelfObject - the event object which will be skipped.

   Return value:

	 TRUE if the location is obstacle, FALSE if not.

--*/
{
	int x, y, h, xr, yr;
	int blockX = PAL_X(gpGlobals->partyoffset) / 32, blockY = PAL_Y(gpGlobals->partyoffset) / 16;

	//
	// Check if the map tile at the specified position is blocking
	//
	x = PAL_X(pos) / 32;
	y = PAL_Y(pos) / 16;
	h = 0;

	//
	// Avoid walk out of range, look out of map
	//
	if (x < blockX || x >= 2048 || y < blockY || y >= 2048)
	{
		return TRUE;
	}

	xr = PAL_X(pos) % 32;
	yr = PAL_Y(pos) % 16;

	if (xr + yr * 2 >= 16)
	{
		if (xr + yr * 2 >= 48)
		{
			x++;
			y++;
		}
		else if (32 - xr + yr * 2 < 16)
		{
			x++;
		}
		else if (32 - xr + yr * 2 < 48)
		{
			h = 1;
		}
		else
		{
			y++;
		}
	}

	if (PAL_MapTileIsBlocked(x, y, h, PAL_GetCurrentMap()))
	{
		return TRUE;
	}

	if (fCheckEventObjects)
	{
		//
		// Loop through all event objects in the current scene
		//
		int i;
		for (i = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex;
			i < gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex; i++)
		{
			LPEVENTOBJECT p = &(gpGlobals->g.lprgEventObject[i]);
			if (i == wSelfObject - 1)
			{
				//
				// Skip myself
				//
				continue;
			}

			//
			// Is this object a blocking one?
			//
			if (p->sState >= kObjStateBlocker)
			{
				//
				// Check for collision
				//
				if (abs(p->x - PAL_X(pos)) + abs(p->y - PAL_Y(pos)) * 2 < 16)
				{
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

VOID
PAL_UpdatePartyGestures(
	BOOL             fWalking
)
/*++
   Purpose:

	 Update the gestures of all the party members.
	 更新所有队员的手势

   Parameters:

	 [IN]  fWalking - whether the party is walking or not.
	 无论队伍是否在行走

   Return value:

	 None.
	 无

--*/
{
	static int       s_iThisStepFrame = 0;
	int              iStepFrameFollower = 0, iStepFrameLeader = 0;
	int              i;

	if (fWalking)
	{
		//
		// Update the gesture for party leader
		// 更新领队的手势
		s_iThisStepFrame = (s_iThisStepFrame + 1) % 4;
		if (s_iThisStepFrame & 1)
		{
			iStepFrameLeader = (s_iThisStepFrame + 1) / 2;
			iStepFrameFollower = 3 - iStepFrameLeader;
		}
		else
		{
			iStepFrameLeader = 0;
			iStepFrameFollower = 0;
		}

		gpGlobals->rgParty[0].x = PAL_X(gpGlobals->partyoffset);
		gpGlobals->rgParty[0].y = PAL_Y(gpGlobals->partyoffset);

		if (gpGlobals->g.PlayerRoles.rgwWalkFrames[gpGlobals->rgParty[0].wPlayerRole] == 4)
		{
			gpGlobals->rgParty[0].wFrame = gpGlobals->wPartyDirection * 4 + s_iThisStepFrame;
		}
		else
		{
			gpGlobals->rgParty[0].wFrame = gpGlobals->wPartyDirection * 3 + iStepFrameLeader;
		}

		//
		// Update the gestures and positions for other party members
		// 更新其他党员的手势和姿势
		for (i = 1; i <= (short)gpGlobals->wMaxPartyMemberIndex; i++)
		{
			gpGlobals->rgParty[i].x = gpGlobals->rgTrail[1].x - PAL_X(gpGlobals->viewport);
			gpGlobals->rgParty[i].y = gpGlobals->rgTrail[1].y - PAL_Y(gpGlobals->viewport);

			// 这里改为了对所有队员的判断，代码来自：仙剑奇侠传DOS版圆梦终曲MOD
			if (i == 1)
			{
				gpGlobals->rgParty[i].x +=
					((gpGlobals->rgTrail[1].wDirection == kDirWest || gpGlobals->rgTrail[1].wDirection == kDirSouth) ? 16 : -16);
				gpGlobals->rgParty[i].y +=
					((gpGlobals->rgTrail[1].wDirection == kDirWest || gpGlobals->rgTrail[1].wDirection == kDirNorth) ? 8 : -8);
			}
			else if (i == 2)
			{
				gpGlobals->rgParty[i].x +=
					(gpGlobals->rgTrail[1].wDirection == kDirEast || gpGlobals->rgTrail[1].wDirection == kDirWest) ? -16 : 16;
				gpGlobals->rgParty[i].y += 8;
			}
			else if (i == 3)
			{
				SHORT dx = 0, dy = 0;
				if (gpGlobals->rgTrail[1].wDirection == kDirSouth || gpGlobals->rgTrail[1].wDirection == kDirWest) {
					dx = -16;
				}
				else {
					dx = 16;
				}
				if (gpGlobals->rgTrail[1].wDirection == kDirWest || gpGlobals->rgTrail[1].wDirection == kDirNorth) {
					dy = -8;
				}
				else {
					dy = 8;
				}
				gpGlobals->rgParty[i].x += dx;
				gpGlobals->rgParty[i].y += dy;
			}
			else
			{
				gpGlobals->rgParty[i].x +=
					((gpGlobals->rgTrail[1].wDirection == kDirWest || gpGlobals->rgTrail[1].wDirection == kDirSouth) ? 16 : -16);
				gpGlobals->rgParty[i].y +=
					((gpGlobals->rgTrail[1].wDirection == kDirWest || gpGlobals->rgTrail[1].wDirection == kDirNorth) ? 8 : -8);
			}

			//
			// Adjust the position if there is obstacle
			// 如果有障碍物，调整位置
			if (PAL_CheckObstacle(PAL_XY(gpGlobals->rgParty[i].x + PAL_X(gpGlobals->viewport),
				gpGlobals->rgParty[i].y + PAL_Y(gpGlobals->viewport)), TRUE, 0))
			{
				gpGlobals->rgParty[i].x = gpGlobals->rgTrail[1].x - PAL_X(gpGlobals->viewport);
				gpGlobals->rgParty[i].y = gpGlobals->rgTrail[1].y - PAL_Y(gpGlobals->viewport);
			}

			//
			// Update gesture for this party member
			// 更新此参与方成员的手势
			if (gpGlobals->g.PlayerRoles.rgwWalkFrames[gpGlobals->rgParty[i].wPlayerRole] == 4)
			{
				gpGlobals->rgParty[i].wFrame = gpGlobals->rgTrail[2].wDirection * 4 + s_iThisStepFrame;
			}
			else
			{
				gpGlobals->rgParty[i].wFrame = gpGlobals->rgTrail[2].wDirection * 3 + iStepFrameLeader;
			}
		}

		for (i = 1; i <= gpGlobals->nFollower; i++)
		{
			//
			// Update the position and gesture for the follower
			// 更新跟随者的位置和姿势
			gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + i].x =
				gpGlobals->rgTrail[2 + i].x - PAL_X(gpGlobals->viewport);
			gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + i].y =
				gpGlobals->rgTrail[2 + i].y - PAL_Y(gpGlobals->viewport);
			gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + i].wFrame =
				gpGlobals->rgTrail[2 + i].wDirection * 3 + iStepFrameFollower;
		}
	}
	else
	{
		//
		// Player is not moved. Use the "standing" gesture instead of "walking" one.
		// 若队员未移动，则用“站着”的姿势代替“走着”的手势。
		i = gpGlobals->g.PlayerRoles.rgwWalkFrames[gpGlobals->rgParty[0].wPlayerRole];
		if (i == 0)
		{
			i = 3;
		}
		gpGlobals->rgParty[0].wFrame = gpGlobals->wPartyDirection * i;

		for (i = 1; i <= (short)gpGlobals->wMaxPartyMemberIndex; i++)
		{
			int f = gpGlobals->g.PlayerRoles.rgwWalkFrames[gpGlobals->rgParty[i].wPlayerRole];
			if (f == 0)
			{
				f = 3;
			}
			gpGlobals->rgParty[i].wFrame = gpGlobals->rgTrail[2].wDirection * f;
		}

		for (i = 1; i <= gpGlobals->nFollower; i++)
		{
			gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + i].wFrame =
				gpGlobals->rgTrail[2 + i].wDirection * 3;
		}

		s_iThisStepFrame &= 2;
		s_iThisStepFrame ^= 2;
	}
}

VOID
PAL_UpdateParty(
	VOID
)
/*++
   Purpose:

	 Update the location and walking gesture of all the party members.
	 更新所有队员的位置和行走帧。

   Parameters:

	 None.

   Return value:

	 None.

--*/
{
	int              xSource, ySource, xTarget, yTarget, xOffset, yOffset, i, xOffsetSide, yOffsetSide;

	//
	// Has user pressed one of the arrow keys?
	// 用户是否按下了其中一个方向键？
	if (g_InputState.dir != kDirUnknown)
	{
		// 若玩家改变方向，则延迟清除
		if (gpGlobals->wPartyDirection != g_InputState.dir)
			g_InputState.dwDetourDelay = 0;

		// 判断并记录上次按键
		switch (g_InputState.dir)
		{
		case kDirSouth:
			g_InputState.isSouth = TRUE;
			break;

		case kDirWest:
			g_InputState.isWest = TRUE;
			break;

		case kDirNorth:
			g_InputState.isSouth = FALSE;
			break;

		case kDirEast:
			g_InputState.isWest = FALSE;
			break;
		}

		xOffset = ((g_InputState.dir == kDirWest || g_InputState.dir == kDirSouth) ? -16 : 16);
		yOffset = ((g_InputState.dir == kDirWest || g_InputState.dir == kDirNorth) ? -8 : 8);

		xSource = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
		ySource = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

		xTarget = xSource + xOffset;
		yTarget = ySource + yOffset;

		gpGlobals->wPartyDirection = g_InputState.dir;

		//
		// Check for obstacles on the destination location
		// 检查目的地是否有障碍物
		if (!PAL_CheckObstacle(PAL_XY(xTarget, yTarget), TRUE, 0))
		{
			//
			// Player will actually be moved. Store trail.
			// 玩家实际上会被移动。存储跟踪
			for (i = 3; i >= 0; i--)
			{
				gpGlobals->rgTrail[i + 1] = gpGlobals->rgTrail[i];
			}

			gpGlobals->rgTrail[0].wDirection = g_InputState.dir;
			gpGlobals->rgTrail[0].x = xSource;
			gpGlobals->rgTrail[0].y = ySource;

			//
			// Move the viewport
			// 视角跟随领队移动
			gpGlobals->viewport =
				PAL_XY(PAL_X(gpGlobals->viewport) + xOffset, PAL_Y(gpGlobals->viewport) + yOffset);

			//
			// Update gestures
			// 更新行走帧
			PAL_UpdatePartyGestures(TRUE);

			return; // don't go further 不要走得更远
		}
		else
		{
			// 初始化值
			xOffset = 0;
			yOffset = 0;

			//
			// 试图绕过障碍物。。。。
			//
			switch (g_InputState.dir)
			{
			case kDirSouth:
				//
				// 南：右下
				// 先计算右手路段坐标
				xOffsetSide = xSource - 16;
				yOffsetSide = ySource - 8;

				// 判断右手路段
				if (!PAL_CheckObstacle(PAL_XY(xOffsetSide, yOffsetSide), TRUE, 0))
					// 若右手边路段允许同行则判断右手前方路段
					if (!PAL_CheckObstacle(PAL_XY(xOffsetSide - 16, ySource), TRUE, 0))
					{
						xOffset = -16;
						yOffset = -8;
					}

				// 再计算右方路段坐标
				xOffsetSide = xSource + 16;
				yOffsetSide = ySource + 8;

				// 然后是判断左手路段
				if (!PAL_CheckObstacle(PAL_XY(xOffsetSide, yOffsetSide), TRUE, 0))
					// 若左手路段允许同行则判断左前方路段
					if (!PAL_CheckObstacle(PAL_XY(xSource, yOffsetSide + 8), TRUE, 0))
						// 若两条路都可通行，则取最近按键的方向,否则直接写入
						if (xOffset == 0 && yOffset == 0 || !g_InputState.isWest)
						{
							xOffset = 16;
							yOffset = 8;
						}
				break;

			case kDirWest:
				//
				// 东：左上
				// 先计算右手路段坐标
				xOffsetSide = xSource + 16;
				yOffsetSide = ySource - 8;

				// 判断右手路段
				if (!PAL_CheckObstacle(PAL_XY(xOffsetSide, yOffsetSide), TRUE, 0))
					// 若右手边路段允许同行则判断右手前方路段
					if (!PAL_CheckObstacle(PAL_XY(xSource, yOffsetSide - 8), TRUE, 0))
					{
						xOffset = 16;
						yOffset = -8;
					}

				// 再计算右方路段坐标
				xOffsetSide = xSource - 16;
				yOffsetSide = ySource + 8;

				// 然后是判断左手路段
				if (!PAL_CheckObstacle(PAL_XY(xOffsetSide, yOffsetSide), TRUE, 0))
					// 若左手路段允许同行则判断左前方路段
					if (!PAL_CheckObstacle(PAL_XY(xOffsetSide - 16, ySource), TRUE, 0))
						// 若两条路都可通行，则取最近按键的方向,否则直接写入
						if (xOffset == 0 && yOffset == 0 || g_InputState.isSouth)
						{
							xOffset = -16;
							yOffset = 8;
						}
				break;

			case kDirNorth:
				//
				// 北：右上
				// 先计算右手路段坐标
				xOffsetSide = xSource + 16;
				yOffsetSide = ySource + 8;

				// 判断右手路段
				if (!PAL_CheckObstacle(PAL_XY(xOffsetSide, yOffsetSide), TRUE, 0))
					// 若右手边路段允许同行则判断右手前方路段
					if (!PAL_CheckObstacle(PAL_XY(xOffsetSide + 16, ySource), TRUE, 0))
					{
						xOffset = 16;
						yOffset = 8;
					}

				// 再计算右方路段坐标
				xOffsetSide = xSource - 16;
				yOffsetSide = ySource - 8;

				// 然后是判断左手路段
				if (!PAL_CheckObstacle(PAL_XY(xOffsetSide, yOffsetSide), TRUE, 0))
					// 若左手路段允许同行则判断左前方路段
					if (!PAL_CheckObstacle(PAL_XY(xSource, yOffsetSide - 8), TRUE, 0))
						// 若两条路都可通行，则取最近按键的方向,否则直接写入
						if (xOffset == 0 && yOffset == 0 || g_InputState.isWest)
						{
							xOffset = -16;
							yOffset = -8;
						}
				break;

			case kDirEast:
				//
				// 西：右下
				// 先计算右手路段坐标
				xOffsetSide = xSource - 16;
				yOffsetSide = ySource + 8;

				// 判断右手路段
				if (!PAL_CheckObstacle(PAL_XY(xOffsetSide, yOffsetSide), TRUE, 0))
					// 若右手边路段允许同行则判断右手前方路段
					if (!PAL_CheckObstacle(PAL_XY(xSource, yOffsetSide + 8), TRUE, 0))
					{
						xOffset = -16;
						yOffset = 8;
					}

				// 再计算右方路段坐标
				xOffsetSide = xSource + 16;
				yOffsetSide = ySource - 8;

				// 然后是判断左手路段
				if (!PAL_CheckObstacle(PAL_XY(xOffsetSide, yOffsetSide), TRUE, 0))
					// 若左手路段允许同行则判断左前方路段
					if (!PAL_CheckObstacle(PAL_XY(xOffsetSide + 16, ySource), TRUE, 0))
						// 若两条路都可通行，则取最近按键的方向,否则直接写入
						if (xOffset == 0 && yOffset == 0 || !g_InputState.isSouth)
						{
							xOffset = 16;
							yOffset = -8;
						}
				break;
			}

			// 是否需要绕行（这里的延迟因为玩家需要获取物资会无意识的绕行）
			if (xOffset == 0 && yOffset == 0 || g_InputState.dwDetourDelay < 4)
			{
				// 延迟叠加
				g_InputState.dwDetourDelay++;

				// 更新队员行走帧
				PAL_UpdatePartyGestures(FALSE);

				// 禁止通行
				return;
			}

			// 延迟清除，这里不清除则直走时一直处于绕行状态
			//g_InputState.dwDetourDelay = 0;

			gpGlobals->wPartyDirection = g_InputState.dir;

			//
			// Player will actually be moved. Store trail.
			// 玩家实际上会被移动。存储跟踪
			for (i = 3; i >= 0; i--)
			{
				gpGlobals->rgTrail[i + 1] = gpGlobals->rgTrail[i];
			}

			gpGlobals->rgTrail[0].wDirection = g_InputState.dir;
			gpGlobals->rgTrail[0].x = xSource;
			gpGlobals->rgTrail[0].y = ySource;

			//
			// Move the viewport
			// 视角跟随领队移动
			gpGlobals->viewport =
				PAL_XY(PAL_X(gpGlobals->viewport) + xOffset, PAL_Y(gpGlobals->viewport) + yOffset);

			//
			// Update gestures
			// 更新行走帧
			PAL_UpdatePartyGestures(TRUE);

			// 更新队员行走帧
			return; // don't go further 不要走得更远
		}
	}

	PAL_UpdatePartyGestures(FALSE);
}

VOID
PAL_NPCWalkOneStep(
	WORD          wEventObjectID,
	INT           iSpeed
)
/*++
  Purpose:

	Move and animate the specified event object (NPC).

  Parameters:

	[IN]  wEventObjectID - the event object to move.

	[IN]  iSpeed - speed of the movement.

  Return value:

	None.

--*/
{
	LPEVENTOBJECT        p;

	//
	// Check for invalid parameters
	//
	if (wEventObjectID == 0 || wEventObjectID > gpGlobals->g.nEventObject)
	{
		return;
	}

	p = &(gpGlobals->g.lprgEventObject[wEventObjectID - 1]);

	//
	// Move the event object by the specified direction
	//
	p->x += ((p->wDirection == kDirWest || p->wDirection == kDirSouth) ? -2 : 2) * iSpeed;
	p->y += ((p->wDirection == kDirWest || p->wDirection == kDirNorth) ? -1 : 1) * iSpeed;

	//
	// Update the gesture
	//
	if (p->nSpriteFrames > 0)
	{
		p->wCurrentFrameNum++;
		p->wCurrentFrameNum %= (p->nSpriteFrames == 3 ? 4 : p->nSpriteFrames);
	}
	else if (p->nSpriteFramesAuto > 0)
	{
		p->wCurrentFrameNum++;
		p->wCurrentFrameNum %= p->nSpriteFramesAuto;
	}
}

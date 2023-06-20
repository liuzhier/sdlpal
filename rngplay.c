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
// Portions based on PalLibrary by Lou Yihua <louyihua@21cn.com>.
// Copyright (c) 2006-2007, Lou Yihua.
//

#include "main.h"

static INT
PAL_RNGReadFrame(
	LPBYTE          lpBuffer,
	UINT            uiBufferSize,
	UINT            uiRngNum,
	UINT            uiFrameNum,
	FILE* fpRngMKF
)
/*++
  Purpose:

	Read a frame from a RNG animation.
	从RNG动画中读取帧。

  Parameters:

	[OUT] lpBuffer - pointer to the destination buffer.
	指向目标缓冲区的指针。

	[IN]  uiBufferSize - size of the destination buffer.
	目标缓冲区的大小。

	[IN]  uiRngNum - the number of the RNG animation in the MKF archive.
	MKF存档中RNG动画的编号。

	[IN]  uiFrameNum - frame number in the RNG animation.
	RNG动画中的帧数。

	[IN]  fpRngMKF - pointer to the fopen'ed MKF file.
	指向已打开 MKF文件的指针。

  Return value:

	Integer value which indicates the size of the chunk.
	-1 if there are error in parameters.
	-2 if buffer size is not enough.
	指示块大小的整数值。
	-1  若参数有错误。
	-2  若缓冲区大小不够。

--*/
{
	UINT         uiOffset = 0;
	UINT         uiSubOffset = 0;
	UINT         uiNextOffset = 0;
	UINT         uiChunkCount = 0;
	INT          iChunkLen = 0;

	if (lpBuffer == NULL || fpRngMKF == NULL || uiBufferSize == 0)
	{
		return -1;
	}

	//
	// Get the total number of chunks.
	// 获取MKF子文件的总数，若小于指定的子文件的编号则跳出
	uiChunkCount = PAL_MKFGetChunkCount(fpRngMKF);
	if (uiRngNum >= uiChunkCount)
	{
		return -1;
	}

	//
	// Get the offset of the chunk.
	// 获取指定块的偏移量。
	fseek(fpRngMKF, 4 * uiRngNum, SEEK_SET);
	PAL_fread(&uiOffset, sizeof(UINT), 1, fpRngMKF);
	PAL_fread(&uiNextOffset, sizeof(UINT), 1, fpRngMKF);
	uiOffset = SDL_SwapLE32(uiOffset);
	uiNextOffset = SDL_SwapLE32(uiNextOffset);

	//
	// Get the length of the chunk.
	// 获取块的长度。
	iChunkLen = uiNextOffset - uiOffset;
	if (iChunkLen != 0)
	{
		fseek(fpRngMKF, uiOffset, SEEK_SET);
	}
	else
	{
		return -1;
	}

	//
	// Get the number of sub chunks.
	// 获取指定块的子块的数量。
	PAL_fread(&uiChunkCount, sizeof(UINT), 1, fpRngMKF);
	uiChunkCount = (SDL_SwapLE32(uiChunkCount) - 4) / 4;
	if (uiFrameNum >= uiChunkCount)
	{
		return -1;
	}

	//
	// Get the offset of the sub chunk.
	// 获取子块的偏移量。
	fseek(fpRngMKF, uiOffset + 4 * uiFrameNum, SEEK_SET);
	PAL_fread(&uiSubOffset, sizeof(UINT), 1, fpRngMKF);
	PAL_fread(&uiNextOffset, sizeof(UINT), 1, fpRngMKF);
	uiSubOffset = SDL_SwapLE32(uiSubOffset);
	uiNextOffset = SDL_SwapLE32(uiNextOffset);

	//
	// Get the length of the sub chunk.
	// 获取子块的长度。
	iChunkLen = uiNextOffset - uiSubOffset;
	if ((UINT)iChunkLen > uiBufferSize)
	{
		return -2;
	}

	if (iChunkLen != 0)
	{
		fseek(fpRngMKF, uiOffset + uiSubOffset, SEEK_SET);
		return (int)fread(lpBuffer, 1, iChunkLen, fpRngMKF);
	}

	return -1;
}

static INT
PAL_RNGBlitToSurface(
	const uint8_t* rng,
	int              length,
	SDL_Surface* lpDstSurface
)
/*++
  Purpose:

	Blit one frame in an RNG animation to an SDL surface.
	The surface should contain the last frame of the RNG, or blank if it's the first
	frame.

	NOTE: Assume the surface is already locked, and the surface is a 320x200 8-bit one.

	将RNG动画中的一帧Blit到SDL曲面。
	曲面应包含RNG的最后一帧，如果是第一帧，则为空白框架

	注：假设曲面已锁定，且曲面为320x200 8位曲面。

  Parameters:

	[IN]  rng - Pointer to the RNG data.
	指向rng数据的指针。

	[IN]  length - Length of the RNG data.
	RNG数据的长度。

	[OUT] lpDstSurface - pointer to the destination SDL surface.
	指向目标SDL曲面的指针。

  Return value:

	0 = success, -1 = error.
	0=成功，-1=错误。

--*/
{
	int                   ptr = 0;
	int                   dst_ptr = 0;
	uint16_t              wdata = 0;
	int                   x, y, i, n;

	//
	// Check for invalid parameters.
	// 检查参数是否无效。
	if (lpDstSurface == NULL || length < 0)
	{
		return -1;
	}

	//
	// Draw the frame to the surface.
	// FIXME: Dirty and ineffective code, needs to be cleaned up
	// 将框架绘制到曲面。
	// 修复：肮脏且无效的代码，需要清理
	//
	while (ptr < length)
	{
		uint8_t data = rng[ptr++];
		switch (data)
		{
		case 0x00:
		case 0x13:
			//
			// End
			//
			goto end;

		case 0x02:
			dst_ptr += 2;
			break;

		case 0x03:
			data = rng[ptr++];
			dst_ptr += (data + 1) * 2;
			break;

		case 0x04:
			wdata = rng[ptr] | (rng[ptr + 1] << 8);
			ptr += 2;
			dst_ptr += ((unsigned int)wdata + 1) * 2;
			break;

		case 0x0a:
			x = dst_ptr % 320;
			y = dst_ptr / 320;
			((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
			if (++x >= 320)
			{
				x = 0;
				++y;
			}
			((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
			dst_ptr += 2;

		case 0x09:
			x = dst_ptr % 320;
			y = dst_ptr / 320;
			((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
			if (++x >= 320)
			{
				x = 0;
				++y;
			}
			((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
			dst_ptr += 2;

		case 0x08:
			x = dst_ptr % 320;
			y = dst_ptr / 320;
			((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
			if (++x >= 320)
			{
				x = 0;
				++y;
			}
			((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
			dst_ptr += 2;

		case 0x07:
			x = dst_ptr % 320;
			y = dst_ptr / 320;
			((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
			if (++x >= 320)
			{
				x = 0;
				++y;
			}
			((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
			dst_ptr += 2;

		case 0x06:
			x = dst_ptr % 320;
			y = dst_ptr / 320;
			((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
			if (++x >= 320)
			{
				x = 0;
				++y;
			}
			((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
			dst_ptr += 2;
			break;

		case 0x0b:
			data = *(rng + ptr++);
			for (i = 0; i <= data; i++)
			{
				x = dst_ptr % 320;
				y = dst_ptr / 320;
				((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
				if (++x >= 320)
				{
					x = 0;
					++y;
				}
				((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
				dst_ptr += 2;
			}
			break;

		case 0x0c:
			wdata = rng[ptr] | (rng[ptr + 1] << 8);
			ptr += 2;
			for (i = 0; i <= wdata; i++)
			{
				x = dst_ptr % 320;
				y = dst_ptr / 320;
				((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
				if (++x >= 320)
				{
					x = 0;
					++y;
				}
				((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
				dst_ptr += 2;
			}
			break;

		case 0x0d:
		case 0x0e:
		case 0x0f:
		case 0x10:
			for (i = 0; i < data - (0x0d - 2); i++)
			{
				x = dst_ptr % 320;
				y = dst_ptr / 320;
				((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr];
				if (++x >= 320)
				{
					x = 0;
					++y;
				}
				((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr + 1];
				dst_ptr += 2;
			}
			ptr += 2;
			break;

		case 0x11:
			data = *(rng + ptr++);
			for (i = 0; i <= data; i++)
			{
				x = dst_ptr % 320;
				y = dst_ptr / 320;
				((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr];
				if (++x >= 320)
				{
					x = 0;
					++y;
				}
				((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr + 1];
				dst_ptr += 2;
			}
			ptr += 2;
			break;

		case 0x12:
			n = (rng[ptr] | (rng[ptr + 1] << 8)) + 1;
			ptr += 2;
			for (i = 0; i < n; i++)
			{
				x = dst_ptr % 320;
				y = dst_ptr / 320;
				((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr];
				if (++x >= 320)
				{
					x = 0;
					++y;
				}
				((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr + 1];
				dst_ptr += 2;
			}
			ptr += 2;
			break;
		}
	}

end:
	return 0;
}

VOID
PAL_RNGPlay(
	INT           iNumRNG,
	INT           iStartFrame,
	INT           iEndFrame,
	INT           iSpeed
)
/*++
  Purpose:

	Play a RNG movie.

  Parameters:

	[IN]  iNumRNG - number of the RNG movie.

	[IN]  iStartFrame - start frame number.

	[IN]  iEndFrame - end frame number.

	[IN]  iSpeed - speed of playing.

  Return value:

	None.

--*/
{
	double         iDelay = (double)SDL_GetPerformanceFrequency() / (iSpeed == 0 ? 16 : iSpeed);
	uint8_t* rng = (uint8_t*)malloc(65000);
	uint8_t* buf = (uint8_t*)malloc(65000);
	FILE* fp = UTIL_OpenRequiredFile("rng.mkf");

	for (double iTime = SDL_GetPerformanceCounter(); rng && buf && iStartFrame != iEndFrame; iStartFrame++)
	{
		iTime += iDelay;
		//
		// Read, decompress and render the frame
		// 读取、解压缩和渲染帧
		if (PAL_RNGReadFrame(buf, 65000, iNumRNG, iStartFrame, fp) < 0 ||
			PAL_RNGBlitToSurface(rng, Decompress(buf, rng, 65000), gpScreen) == -1)
		{
			//
			// Failed to get the frame, don't go further
			// 无法获取框架，直接跳出
			break;
		}

		//
		// Update the screen
		// 更新整个屏幕
		VIDEO_UpdateScreen(NULL);

		//
		// Fade in the screen if needed
		// 如果需要则淡入屏幕
		if (gpGlobals->fNeedToFadeIn)
		{
			PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
			gpGlobals->fNeedToFadeIn = FALSE;
		}

		//
		// Delay for a while
		// 延迟一段时间
		PAL_DelayUntilPC(iTime);
	}

	fclose(fp);
	free(rng);
	free(buf);
}

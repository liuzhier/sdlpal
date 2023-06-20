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

#include "palcommon.h"
#include "global.h"
#include "palcfg.h"

PAL_FORCE_INLINE
BYTE
PAL_CalcShadowColor(
	BYTE bSourceColor
)
{
	return ((bSourceColor & 0xF0) | ((bSourceColor & 0x0F) >> 1));
}

INT
PAL_RLEBlitToSurface(
	LPCBITMAPRLE      lpBitmapRLE,
	SDL_Surface* lpDstSurface,
	PAL_POS           pos
)
{
	return PAL_RLEBlitToSurfaceWithShadow(lpBitmapRLE, lpDstSurface, pos, FALSE);
}

INT
PAL_RLEBlitToSurfaceWithShadow(
	LPCBITMAPRLE      lpBitmapRLE,
	SDL_Surface* lpDstSurface,
	PAL_POS           pos,
	BOOL              bShadow
)
/*++
  Purpose:

	Blit an RLE-compressed bitmap to an SDL surface.
	NOTE: Assume the surface is already locked, and the surface is a 8-bit one.
	将RLE压缩位图Blit到SDL曲面。
	注意：假设曲面已锁定，并且曲面是8位曲面。

  Parameters:

	[IN]  lpBitmapRLE - pointer to the RLE-compressed bitmap to be decoded.
	指向要解码的RLE压缩位图的指针。

	[OUT] lpDstSurface - pointer to the destination SDL surface.
	指向目标SDL曲面的指针。

	[IN]  pos - position of the destination area.
	目标区域的位置。

	[IN]  bShadow - flag to mention whether blit source color or just shadow.
	标记，指明是blit源颜色还是仅阴影。

  Return value:

	0 = success, -1 = error.

--*/
{
	UINT          i, j, k, sx;
	INT           x, y;
	UINT          uiLen = 0;
	UINT          uiWidth = 0;
	UINT          uiHeight = 0;
	UINT          uiSrcX = 0;         // 该图像的当前正在绘制的像素点 相对于 欲绘制点 POS 的 X 轴偏移（也就是正在绘制该图像当前行的第几个像素）
	BYTE          T;
	INT           dx = PAL_X(pos);    // 欲将该图像绘制到屏幕的 X 轴坐标
	INT           dy = PAL_Y(pos);    // 欲将该图像绘制到屏幕的 Y 轴坐标
	LPBYTE        p;

	//
	// Check for NULL pointer.
	// 检查空指针，空指针取消绘制
	if (lpBitmapRLE == NULL || lpDstSurface == NULL)
	{
		return -1;
	}

	//
	// Skip the 0x00000002 in the file header.
	// 跳过文件头中的0x00000002。
	if (lpBitmapRLE[0] == 0x02 && lpBitmapRLE[1] == 0x00 &&
		lpBitmapRLE[2] == 0x00 && lpBitmapRLE[3] == 0x00)
	{
		lpBitmapRLE += 4;
	}

	//
	// Get the width and height of the bitmap.
	// 获取位图的宽度和高度。
	uiWidth = lpBitmapRLE[0] | (lpBitmapRLE[1] << 8);
	uiHeight = lpBitmapRLE[2] | (lpBitmapRLE[3] << 8);

	//
	// Check whether bitmap intersects the surface.
	// 检查位图是否与曲面相交。
	// （这里判断要绘制的图像最终绘制位置，若不在屏幕内，就算绘制出来，
	// 用户也是看不到图像的，本次绘制无意义，故直接取消了绘制）
	if (uiWidth + dx <= 0 || dx >= lpDstSurface->w ||
		uiHeight + dy <= 0 || dy >= lpDstSurface->h)
	{
		goto end;
	}

	//
	// Calculate the total length of the bitmap.
	// The bitmap is 8-bpp, each pixel will use 1 byte.
	// 计算位图的总长度。
	// 位图为8-bpp，每个像素将使用1个字节。
	uiLen = uiWidth * uiHeight;

	//
	// Start decoding and blitting the bitmap.
	// 开始对位图进行解码和块传输。
	lpBitmapRLE += 4;
	for (i = 0; i < uiLen;)
	{
		// 指针迭代器，读取 RLE 的下一个字节
		T = *lpBitmapRLE++;

		// 解码 RLE
		if ((T & 0x80) && T <= 0x80 + uiWidth)
		{
			i += T - 0x80;
			uiSrcX += T - 0x80;
			if (uiSrcX >= uiWidth)
			{
				uiSrcX -= uiWidth;
				dy++;
			}
		}
		else
		{
			//
			// Prepare coordinates.
			// 准备坐标
			j = 0;
			sx = uiSrcX;        // 当前像素在图像里的编号？
			x = dx + uiSrcX;    // 当前像素点相对于屏幕的 POSX
			y = dy;             // 当前像素点相对于屏幕的 POSY

			//
			// Skip the points which are out of the surface.
			// 跳过曲面外的点。（不去绘制那些超出屏幕范围的像素点）
			if (y < 0)
			{
				j += -y * uiWidth;
				y = 0;
			}
			else if (y >= lpDstSurface->h)
			{
				// No more pixels needed, break out
				// 不再需要像素，跳出（若实际绘制的 POSY 超出屏幕范围）
				goto end;
			}

			while (j < T)
			{
				//
				// Skip the points which are out of the surface.
				// 跳过曲面外的点。（不去绘制那些超出屏幕范围的像素点）
				if (x < 0)
				{
					j += -x;
					if (j >= T) break;
					sx += -x;
					x = 0;
				}
				else if (x >= lpDstSurface->w)
				{
					j += uiWidth - sx;
					x -= sx;
					sx = 0;
					y++;
					if (y >= lpDstSurface->h)
					{
						goto end; // No more pixels needed, break out
					}
					continue;
				}

				//
				// Put the pixels in row onto the surface
				//
				k = T - j;
				if (lpDstSurface->w - x < k) k = lpDstSurface->w - x;
				if (uiWidth - sx < k) k = uiWidth - sx;
				sx += k;
				p = ((LPBYTE)lpDstSurface->pixels) + y * lpDstSurface->pitch;
				if (bShadow)
				{
					j += k;
					for (; k != 0; k--)
					{
						p[x] = PAL_CalcShadowColor(p[x]);
						x++;
					}
				}
				else
				{
					for (; k != 0; k--)
					{
						p[x] = lpBitmapRLE[j];
						j++;
						x++;
					}
				}

				if (sx >= uiWidth)
				{
					sx -= uiWidth;
					x -= uiWidth;
					y++;
					if (y >= lpDstSurface->h)
					{
						goto end; // No more pixels needed, break out
					}
				}
			}
			lpBitmapRLE += T;
			i += T;
			uiSrcX += T;
			while (uiSrcX >= uiWidth)
			{
				uiSrcX -= uiWidth;
				dy++;
			}
		}
	}

end:
	//
	// Success
	//
	return 0;
}

INT
PAL_RLEBlitWithColorShift(
	LPCBITMAPRLE      lpBitmapRLE,
	SDL_Surface* lpDstSurface,
	PAL_POS           pos,
	INT               iColorShift
)
/*++
  Purpose:

	Blit an RLE-compressed bitmap to an SDL surface.
	NOTE: Assume the surface is already locked, and the surface is a 8-bit one.

  Parameters:

	[IN]  lpBitmapRLE - pointer to the RLE-compressed bitmap to be decoded.

	[OUT] lpDstSurface - pointer to the destination SDL surface.

	[IN]  pos - position of the destination area.

	[IN]  iColorShift - shift the color by this value.

  Return value:

	0 = success, -1 = error.

--*/
{
	UINT          i, j, k, sx;
	INT           x, y;
	UINT          uiLen = 0;
	UINT          uiWidth = 0;
	UINT          uiHeight = 0;
	UINT          uiSrcX = 0;
	BYTE          T, b;
	INT           dx = PAL_X(pos);
	INT           dy = PAL_Y(pos);
	LPBYTE        p;

	//
	// Check for NULL pointer.
	//
	if (lpBitmapRLE == NULL || lpDstSurface == NULL)
	{
		return -1;
	}

	//
	// Skip the 0x00000002 in the file header.
	//
	if (lpBitmapRLE[0] == 0x02 && lpBitmapRLE[1] == 0x00 &&
		lpBitmapRLE[2] == 0x00 && lpBitmapRLE[3] == 0x00)
	{
		lpBitmapRLE += 4;
	}

	//
	// Get the width and height of the bitmap.
	//
	uiWidth = lpBitmapRLE[0] | (lpBitmapRLE[1] << 8);
	uiHeight = lpBitmapRLE[2] | (lpBitmapRLE[3] << 8);

	//
	// Check whether bitmap intersects the surface.
	//
	if (uiWidth + dx <= 0 || dx >= lpDstSurface->w ||
		uiHeight + dy <= 0 || dy >= lpDstSurface->h)
	{
		goto end;
	}

	//
	// Calculate the total length of the bitmap.
	// The bitmap is 8-bpp, each pixel will use 1 byte.
	//
	uiLen = uiWidth * uiHeight;

	//
	// Start decoding and blitting the bitmap.
	//
	lpBitmapRLE += 4;
	for (i = 0; i < uiLen;)
	{
		T = *lpBitmapRLE++;
		if ((T & 0x80) && T <= 0x80 + uiWidth)
		{
			i += T - 0x80;
			uiSrcX += T - 0x80;
			if (uiSrcX >= uiWidth)
			{
				uiSrcX -= uiWidth;
				dy++;
			}
		}
		else
		{
			//
			// Prepare coordinates.
			//
			j = 0;
			sx = uiSrcX;
			x = dx + uiSrcX;
			y = dy;

			//
			// Skip the points which are out of the surface.
			//
			if (y < 0)
			{
				j += -y * uiWidth;
				y = 0;
			}
			else if (y >= lpDstSurface->h)
			{
				goto end; // No more pixels needed, break out
			}

			while (j < T)
			{
				//
				// Skip the points which are out of the surface.
				//
				if (x < 0)
				{
					j += -x;
					if (j >= T) break;
					sx += -x;
					x = 0;
				}
				else if (x >= lpDstSurface->w)
				{
					j += uiWidth - sx;
					x -= sx;
					sx = 0;
					y++;
					if (y >= lpDstSurface->h)
					{
						goto end; // No more pixels needed, break out
					}
					continue;
				}

				//
				// Put the pixels in row onto the surface
				//
				k = T - j;
				if (lpDstSurface->w - x < k) k = lpDstSurface->w - x;
				if (uiWidth - sx < k) k = uiWidth - sx;
				sx += k;
				p = ((LPBYTE)lpDstSurface->pixels) + y * lpDstSurface->pitch;
				for (; k != 0; k--)
				{
					b = (lpBitmapRLE[j] & 0x0F);
					if ((INT)b + iColorShift > 0x0F)
					{
						b = 0x0F;
					}
					else if ((INT)b + iColorShift < 0)
					{
						b = 0;
					}
					else
					{
						b += iColorShift;
					}

					p[x] = (b | (lpBitmapRLE[j] & 0xF0));
					j++;
					x++;
				}

				if (sx >= uiWidth)
				{
					sx -= uiWidth;
					x -= uiWidth;
					y++;
					if (y >= lpDstSurface->h)
					{
						goto end; // No more pixels needed, break out
					}
				}
			}
			lpBitmapRLE += T;
			i += T;
			uiSrcX += T;
			while (uiSrcX >= uiWidth)
			{
				uiSrcX -= uiWidth;
				dy++;
			}
		}
	}

end:
	//
	// Success
	//
	return 0;
}

INT
PAL_RLEBlitMonoColor(
	LPCBITMAPRLE      lpBitmapRLE,
	SDL_Surface* lpDstSurface,
	PAL_POS           pos,
	BYTE              bColor,
	INT               iColorShift
)
/*++
  Purpose:

	Blit an RLE-compressed bitmap to an SDL surface in mono-color form.
	NOTE: Assume the surface is already locked, and the surface is a 8-bit one.

  Parameters:

	[IN]  lpBitmapRLE - pointer to the RLE-compressed bitmap to be decoded.

	[OUT] lpDstSurface - pointer to the destination SDL surface.

	[IN]  pos - position of the destination area.

	[IN]  bColor - the color to be used while drawing.

	[IN]  iColorShift - shift the color by this value.

  Return value:

	0 = success, -1 = error.

--*/
{
	UINT          i, j, k, sx;
	INT           x, y;
	UINT          uiLen = 0;
	UINT          uiWidth = 0;
	UINT          uiHeight = 0;
	UINT          uiSrcX = 0;
	BYTE          T, b;
	INT           dx = PAL_X(pos);
	INT           dy = PAL_Y(pos);
	LPBYTE        p;

	//
	// Check for NULL pointer.
	//
	if (lpBitmapRLE == NULL || lpDstSurface == NULL)
	{
		return -1;
	}

	//
	// Skip the 0x00000002 in the file header.
	//
	if (lpBitmapRLE[0] == 0x02 && lpBitmapRLE[1] == 0x00 &&
		lpBitmapRLE[2] == 0x00 && lpBitmapRLE[3] == 0x00)
	{
		lpBitmapRLE += 4;
	}

	//
	// Get the width and height of the bitmap.
	//
	uiWidth = lpBitmapRLE[0] | (lpBitmapRLE[1] << 8);
	uiHeight = lpBitmapRLE[2] | (lpBitmapRLE[3] << 8);

	//
	// Check whether bitmap intersects the surface.
	//
	if (uiWidth + dx <= 0 || dx >= lpDstSurface->w ||
		uiHeight + dy <= 0 || dy >= lpDstSurface->h)
	{
		goto end;
	}

	//
	// Calculate the total length of the bitmap.
	// The bitmap is 8-bpp, each pixel will use 1 byte.
	//
	uiLen = uiWidth * uiHeight;

	//
	// Start decoding and blitting the bitmap.
	//
	lpBitmapRLE += 4;
	bColor &= 0xF0;
	for (i = 0; i < uiLen;)
	{
		T = *lpBitmapRLE++;
		if ((T & 0x80) && T <= 0x80 + uiWidth)
		{
			i += T - 0x80;
			uiSrcX += T - 0x80;
			if (uiSrcX >= uiWidth)
			{
				uiSrcX -= uiWidth;
				dy++;
			}
		}
		else
		{
			//
			// Prepare coordinates.
			//
			j = 0;
			sx = uiSrcX;
			x = dx + uiSrcX;
			y = dy;

			//
			// Skip the points which are out of the surface.
			//
			if (y < 0)
			{
				j += -y * uiWidth;
				y = 0;
			}
			else if (y >= lpDstSurface->h)
			{
				goto end; // No more pixels needed, break out
			}

			while (j < T)
			{
				//
				// Skip the points which are out of the surface.
				//
				if (x < 0)
				{
					j += -x;
					if (j >= T) break;
					sx += -x;
					x = 0;
				}
				else if (x >= lpDstSurface->w)
				{
					j += uiWidth - sx;
					x -= sx;
					sx = 0;
					y++;
					if (y >= lpDstSurface->h)
					{
						goto end; // No more pixels needed, break out
					}
					continue;
				}

				//
				// Put the pixels in row onto the surface
				//
				k = T - j;
				if (lpDstSurface->w - x < k) k = lpDstSurface->w - x;
				if (uiWidth - sx < k) k = uiWidth - sx;
				sx += k;
				p = ((LPBYTE)lpDstSurface->pixels) + y * lpDstSurface->pitch;
				for (; k != 0; k--)
				{
					b = lpBitmapRLE[j] & 0x0F;
					if ((INT)b + iColorShift > 0x0F)
					{
						b = 0x0F;
					}
					else if ((INT)b + iColorShift < 0)
					{
						b = 0;
					}
					else
					{
						b += iColorShift;
					}

					p[x] = (b | bColor);
					j++;
					x++;
				}

				if (sx >= uiWidth)
				{
					sx -= uiWidth;
					x -= uiWidth;
					y++;
					if (y >= lpDstSurface->h)
					{
						goto end; // No more pixels needed, break out
					}
				}
			}
			lpBitmapRLE += T;
			i += T;
			uiSrcX += T;
			while (uiSrcX >= uiWidth)
			{
				uiSrcX -= uiWidth;
				dy++;
			}
		}
	}

end:
	//
	// Success
	//
	return 0;
}

INT
PAL_FBPBlitToSurface(
	LPBYTE            lpBitmapFBP,
	SDL_Surface* lpDstSurface
)
/*++
  Purpose:

	Blit an uncompressed bitmap in FBP.MKF to an SDL surface.
	NOTE: Assume the surface is already locked, and the surface is a 8-bit 320x200 one.

  Parameters:

	[IN]  lpBitmapFBP - pointer to the RLE-compressed bitmap to be decoded.

	[OUT] lpDstSurface - pointer to the destination SDL surface.

  Return value:

	0 = success, -1 = error.

--*/
{
	int       x, y;
	LPBYTE    p;

	if (lpBitmapFBP == NULL || lpDstSurface == NULL ||
		lpDstSurface->w != 320 || lpDstSurface->h != 200)
	{
		return -1;
	}

	//
	// simply copy everything to the surface
	//
	for (y = 0; y < 200; y++)
	{
		p = (LPBYTE)(lpDstSurface->pixels) + y * lpDstSurface->pitch;
		for (x = 0; x < 320; x++)
		{
			*(p++) = *(lpBitmapFBP++);
		}
	}

	return 0;
}

INT
PAL_RLEGetWidth(
	LPCBITMAPRLE    lpBitmapRLE
)
/*++
  Purpose:

	Get the width of an RLE-compressed bitmap.

  Parameters:

	[IN]  lpBitmapRLE - pointer to an RLE-compressed bitmap.

  Return value:

	Integer value which indicates the height of the bitmap.

--*/
{
	if (lpBitmapRLE == NULL)
	{
		return 0;
	}

	//
	// Skip the 0x00000002 in the file header.
	//
	if (lpBitmapRLE[0] == 0x02 && lpBitmapRLE[1] == 0x00 &&
		lpBitmapRLE[2] == 0x00 && lpBitmapRLE[3] == 0x00)
	{
		lpBitmapRLE += 4;
	}

	//
	// Return the width of the bitmap.
	//
	return lpBitmapRLE[0] | (lpBitmapRLE[1] << 8);
}

INT
PAL_RLEGetHeight(
	LPCBITMAPRLE       lpBitmapRLE
)
/*++
  Purpose:

	Get the height of an RLE-compressed bitmap.

  Parameters:

	[IN]  lpBitmapRLE - pointer of an RLE-compressed bitmap.

  Return value:

	Integer value which indicates the height of the bitmap.

--*/
{
	if (lpBitmapRLE == NULL)
	{
		return 0;
	}

	//
	// Skip the 0x00000002 in the file header.
	//
	if (lpBitmapRLE[0] == 0x02 && lpBitmapRLE[1] == 0x00 &&
		lpBitmapRLE[2] == 0x00 && lpBitmapRLE[3] == 0x00)
	{
		lpBitmapRLE += 4;
	}

	//
	// Return the height of the bitmap.
	//
	return lpBitmapRLE[2] | (lpBitmapRLE[3] << 8);
}

WORD
PAL_SpriteGetNumFrames(
	LPCSPRITE       lpSprite
)
/*++
  Purpose:

	Get the total number of frames of a sprite.
	获取敌方单位的总帧数。

  Parameters:

	[IN]  lpSprite - pointer to the sprite.
	指向敌方单位的指针。

  Return value:

	Number of frames of the sprite.
	敌方单位的帧数。

--*/
{
	if (lpSprite == NULL)
	{
		return 0;
	}

	return (lpSprite[0] | (lpSprite[1] << 8)) - 1;
}

LPCBITMAPRLE
PAL_SpriteGetFrame(
	LPCSPRITE       lpSprite,
	INT             iFrameNum
)
/*++
  Purpose:

	Get the pointer to the specified frame from a sprite.
	获取指向敌方单位帧数的指针。

  Parameters:

	[IN]  lpSprite - pointer to the sprite.
	指向敌方单位的指针

	[IN]  iFrameNum - number of the frame.
	帧的编号。

  Return value:

	Pointer to the specified frame. NULL if the frame does not exist.
	指向指定帧的指针。如果帧数不存在，则为NULL。

--*/
{
	int imagecount, offset;

	if (lpSprite == NULL)
	{
		return NULL;
	}

	//
	// Hack for broken sprites like the Bloody-Mouth Bug
	//
 //   imagecount = (lpSprite[0] | (lpSprite[1] << 8)) - 1;
	imagecount = (lpSprite[0] | (lpSprite[1] << 8));

	if (iFrameNum < 0 || iFrameNum >= imagecount)
	{
		//
		// The frame does not exist
		// 帧数不存在则跳出
		return NULL;
	}

	//
	// Get the offset of the frame
	// 获取帧的偏移
	iFrameNum <<= 1;
	offset = ((lpSprite[iFrameNum] | (lpSprite[iFrameNum + 1] << 8)) << 1);

	if (offset == 0x18444)
		offset = (WORD)offset;

	return &lpSprite[offset];
}

INT
PAL_MKFGetChunkCount(
	FILE* fp
)
/*++
  Purpose:

	Get the number of chunks in an MKF archive.
	获取MKF存档中的块数。

  Parameters:

	[IN]  fp - pointer to an fopen'ed MKF file.
	指向已打开的MKF文件的指针。

  Return value:

	Integer value which indicates the number of chunks in the specified MKF file.
	指定的MKF文件中块数的整数值。

--*/
{
	INT iNumChunk;
	if (fp == NULL)
	{
		return 0;
	}

	fseek(fp, 0, SEEK_SET);
	if (fread(&iNumChunk, sizeof(INT), 1, fp) == 1)
		return (SDL_SwapLE32(iNumChunk) - 4) >> 2;
	else
		return 0;
}

INT
PAL_MKFGetChunkSize(
	UINT    uiChunkNum,
	FILE* fp
)
/*++
  Purpose:

	Get the size of a chunk in an MKF archive.
	获取MKF存档中指定块的大小。

  Parameters:

	[IN]  uiChunkNum - the number of the chunk in the MKF archive.
	MKF存档中块的编号。


	[IN]  fp - pointer to the fopen'ed MKF file.
	指向已打开的MKF文件的指针。


  Return value:

	Integer value which indicates the size of the chunk.
	-1 if the chunk does not exist.
	指示块大小的整数值。
	-1  若指定的块不存在。

--*/
{
	UINT    uiOffset = 0;
	UINT    uiNextOffset = 0;
	UINT    uiChunkCount = 0;

	//
	// Get the total number of chunks.
	// 获取块的总数。
	uiChunkCount = PAL_MKFGetChunkCount(fp);
	if (uiChunkNum >= uiChunkCount)
	{
		return -1;
	}

	//
	// Get the offset of the specified chunk and the next chunk.
	// 获取指定块和下一块的偏移量。
	fseek(fp, 4 * uiChunkNum, SEEK_SET);
	PAL_fread(&uiOffset, sizeof(UINT), 1, fp);
	PAL_fread(&uiNextOffset, sizeof(UINT), 1, fp);

	uiOffset = SDL_SwapLE32(uiOffset);
	uiNextOffset = SDL_SwapLE32(uiNextOffset);

	//
	// Return the length of the chunk.
	// 返回块的长度。
	return uiNextOffset - uiOffset;
}

INT
PAL_MKFReadChunk(
	LPBYTE          lpBuffer,
	UINT            uiBufferSize,
	UINT            uiChunkNum,
	FILE* fp
)
/*++
  Purpose:

	Read a chunk from an MKF archive into lpBuffer.
	将MKF归档文件中的块读取到lpBuffer中。

  Parameters:

	[OUT] lpBuffer - pointer to the destination buffer.
	指向目标缓冲区的指针。

	[IN]  uiBufferSize - size of the destination buffer.
	目标缓冲区的大小。

	[IN]  uiChunkNum - the number of the chunk in the MKF archive to read.
	MKF存档文件中要读取的区块数。

	[IN]  fp - pointer to the fopen'ed MKF file.
	指向已打开的MKF文件的指针。

  Return value:

	Integer value which indicates the size of the chunk.
	-1 if there are error in parameters.
	-2 if buffer size is not enough.
	指示块大小的整数值。
	-1  若参数有错误。
	-2，若缓冲区大小不够。

--*/
{
	UINT     uiOffset = 0;
	UINT     uiNextOffset = 0;
	UINT     uiChunkCount;
	UINT     uiChunkLen;

	if (lpBuffer == NULL || fp == NULL || uiBufferSize == 0)
	{
		return -1;
	}

	//
	// Get the total number of chunks.
	// 获取块的总数。
	uiChunkCount = PAL_MKFGetChunkCount(fp);
	if (uiChunkNum >= uiChunkCount)
	{
		return -1;
	}

	//
	// Get the offset of the chunk.
	// 获取块的偏移量。
	fseek(fp, 4 * uiChunkNum, SEEK_SET);
	PAL_fread(&uiOffset, 4, 1, fp);
	PAL_fread(&uiNextOffset, 4, 1, fp);
	uiOffset = SDL_SwapLE32(uiOffset);
	uiNextOffset = SDL_SwapLE32(uiNextOffset);

	//
	// Get the length of the chunk.
	// 获取块的长度。
	uiChunkLen = uiNextOffset - uiOffset;

	if (uiChunkLen > uiBufferSize)
	{
		return -2;
	}

	if (uiChunkLen != 0)
	{
		fseek(fp, uiOffset, SEEK_SET);
		return (int)fread(lpBuffer, 1, uiChunkLen, fp);
	}

	return -1;
}

INT
PAL_MKFGetDecompressedSize(
	UINT    uiChunkNum,
	FILE* fp
)
/*++
  Purpose:

	Get the decompressed size of a compressed chunk in an MKF archive.
	获取MKF存档中指定的压缩块解压缩后的大小。

  Parameters:

	[IN]  uiChunkNum - the number of the chunk in the MKF archive.
	MKF存档中块的编号。

	[IN]  fp - pointer to the fopen'ed MKF file.
	指向已打开的MKF文件的指针。

  Return value:

	Integer value which indicates the size of the chunk.
	-1 if the chunk does not exist.
	指定的块的大小整数值。
	-1  若指定的块不存在。

--*/
{
	DWORD         buf[2];
	UINT          uiOffset;
	UINT          uiChunkCount;

	if (fp == NULL)
	{
		return -1;
	}

	//
	// Get the total number of chunks.
	// 获取子区块的总数。
	uiChunkCount = PAL_MKFGetChunkCount(fp);
	if (uiChunkNum >= uiChunkCount)
	{
		return -1;
	}

	//
	// Get the offset of the chunk.
	// 获取块的偏移量。
	fseek(fp, 4 * uiChunkNum, SEEK_SET);
	PAL_fread(&uiOffset, 4, 1, fp);
	uiOffset = SDL_SwapLE32(uiOffset);

	//
	// Read the header.
	// 读取文件头部信息（应该是所有子文件的总偏移）
	fseek(fp, uiOffset, SEEK_SET);
	if (gConfig.fIsWIN95)
	{
		PAL_fread(buf, sizeof(DWORD), 1, fp);
		buf[0] = SDL_SwapLE32(buf[0]);

		return (INT)buf[0];
	}
	else
	{
		PAL_fread(buf, sizeof(DWORD), 2, fp);
		buf[0] = SDL_SwapLE32(buf[0]);
		buf[1] = SDL_SwapLE32(buf[1]);

		return (buf[0] != 0x315f4a59) ? -1 : (INT)buf[1];
	}
}

INT
PAL_MKFDecompressChunk(
	LPBYTE          lpBuffer,
	UINT            uiBufferSize,
	UINT            uiChunkNum,
	FILE* fp
)
/*++
  Purpose:

	Decompress a compressed chunk from an MKF archive into lpBuffer.
	将MKF存档中的压缩块解压缩到lpBuffer中。

  Parameters:

	[OUT] lpBuffer - pointer to the destination buffer.
	指向目标缓冲区的指针。

	[IN]  uiBufferSize - size of the destination buffer.
	目标缓冲区的大小。

	[IN]  uiChunkNum - the number of the chunk in the MKF archive to read.
	MKF存档文件中要读取的区块数。

	[IN]  fp - pointer to the fopen'ed MKF file.
	指向已打开 MKF文件的指针。

  Return value:

	Integer value which indicates the size of the chunk.
	-1 if there are error in parameters, or buffer size is not enough.
	-3 if cannot allocate memory for decompression.
	指示块大小的整数值。
	-1  若参数中存在错误或缓冲区大小不足。
	-3  若无法分配内存进行解压缩。

--*/
{
	LPBYTE          buf;
	int             len;

	// 获取MKF指定区块大小(区块号，文件名称)
	len = PAL_MKFGetChunkSize(uiChunkNum, fp);

	// 若文件不存在则跳出
	if (len <= 0)
	{
		return len;
	}

	// 获取字节指针，指向子文件在MKF文件中的偏移值，内存指针
	buf = (LPBYTE)malloc(len);
	if (buf == NULL)
	{
		return -3;
	}

	// 读取MKF的区块（输出到的流，区块长度，区块号，文件名称）
	PAL_MKFReadChunk(buf, len, uiChunkNum, fp);

	// 解YJ_1/2，返回数据长度，一石二鸟
	len = Decompress(buf, lpBuffer, uiBufferSize);
	free(buf);

	return len;
}

LPBYTE
PAL_New_DoubleMKFGetFrame(
	LPBYTE          lpBuffer,
	UINT            uiSpriteNum,
	UINT            uiFrameNum,
	FILE* fp
)
/*++
  Purpose:

	Decompress a compressed chunk from an MKF archive into lpBuffer.
	将MKF存档中的压缩块解压缩到lpBuffer中。

  Parameters:

	[OUT] lpBuffer - pointer to the destination buffer.
	指向目标缓冲区的指针。

	[IN]  uiBufferSize - size of the destination buffer.
	目标缓冲区的大小。

	[IN]  uiChunkNum - the number of the chunk in the MKF archive to read.
	MKF存档文件中要读取的区块数。

	[IN]  fp - pointer to the fopen'ed MKF file.
	指向已打开 MKF文件的指针。

  Return value:

	Integer value which indicates the size of the chunk.
	-1 if there are error in parameters, or buffer size is not enough.
	-3 if cannot allocate memory for decompression.
	指示块大小的整数值。
	-1  若参数中存在错误或缓冲区大小不足。
	-3  若无法分配内存进行解压缩。

--*/
{
	UINT         uiOffset = 0;
	UINT         uiSubOffset = 0;
	UINT         uiNextOffset = 0;
	UINT         uiChunkCount = 0;
	UINT         uiBufferSize = 65535;
	INT          iChunkLen = 0;

	if (lpBuffer == NULL || fp == NULL || uiBufferSize == 0)
	{
		return NULL;
	}

	//
	// Get the total number of chunks.
	// 获取MKF子文件的总数，若小于指定的子文件的编号则跳出
	uiChunkCount = PAL_MKFGetChunkCount(fp);
	if (uiSpriteNum >= uiChunkCount)
	{
		return NULL;
	}

	//
	// Get the offset of the chunk.
	// 获取指定块的偏移量。
	fseek(fp, 4 * uiSpriteNum, SEEK_SET);
	PAL_fread(&uiOffset, sizeof(UINT), 1, fp);
	PAL_fread(&uiNextOffset, sizeof(UINT), 1, fp);
	uiOffset = SDL_SwapLE32(uiOffset);
	uiNextOffset = SDL_SwapLE32(uiNextOffset);

	//
	// Get the length of the chunk.
	// 获取块的长度。
	iChunkLen = uiNextOffset - uiOffset;
	if (iChunkLen != 0)
	{
		fseek(fp, uiOffset, SEEK_SET);
	}
	else
	{
		return NULL;
	}

	//
	// Get the number of sub chunks.
	// 获取指定块的子块的数量。
	PAL_fread(&uiChunkCount, sizeof(UINT), 1, fp);
	uiChunkCount = (SDL_SwapLE32(uiChunkCount) - 4) / 4;
	if (uiFrameNum >= uiChunkCount)
	{
		return NULL;
	}

	//
	// Get the offset of the sub chunk.
	// 获取子块的偏移量。
	fseek(fp, uiOffset + 4 * uiFrameNum, SEEK_SET);
	PAL_fread(&uiSubOffset, sizeof(UINT), 1, fp);
	PAL_fread(&uiNextOffset, sizeof(UINT), 1, fp);
	uiSubOffset = SDL_SwapLE32(uiSubOffset);
	uiNextOffset = SDL_SwapLE32(uiNextOffset);

	//
	// Get the length of the sub chunk.
	// 获取子块的长度。
	iChunkLen = uiNextOffset - uiSubOffset;
	if ((UINT)iChunkLen > uiBufferSize)
	{
		return NULL;
	}

	if (iChunkLen != 0)
	{
		fseek(fp, uiOffset + uiSubOffset, SEEK_SET);
		(int)fread(lpBuffer, 1, iChunkLen, fp);
		return lpBuffer;
	}

	return NULL;
}


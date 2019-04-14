
/*
 * XKOBO, a video-oriented game
 * Copyright (C) 1995,1996  Akira Higuchi
 *     a-higuti@math.hokudai.ac.jp
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 */

#ifndef XKOBO_H_MAP
#define XKOBO_H_MAP

#include "config.h"

#define SITE_MAX       1024

#define SPACE          (1<<8)
#define WALL           1
#define CORE           (1<<4)
#define U_MASK         (1<<0)
#define R_MASK         (1<<1)
#define D_MASK         (1<<2)
#define L_MASK         (1<<3)
#define HARD           (1<<5)
#define HIT_MASK       (CORE | U_MASK | R_MASK | D_MASK | L_MASK)

#define	IS_SPACE(x)	((x) & SPACE)

class _map
{
  public:
	void init();
	void make_maze(int x, int y, int difx, int dify);
	void convert(unsigned ratio);	/* ratio < 64 */
	inline unsigned short &pos(int x, int y)
	{
		x &= MAP_SIZEX - 1;
		y &= MAP_SIZEY - 1;
		return data[(y << MAP_SIZEX_LOG2) + x];
	}
	void clearpos(int x, int y);
  protected:
#if 0	//gcc 2.96 fails to understand that these are constants...
	static const int sx_log2 = MAP_SIZEX_LOG2;
	static const int sy_log2 = MAP_SIZEY_LOG2;
	static const int sx = 1 << sx_log2;
	static const int sy = 1 << sy_log2;
#endif
	int sitex[SITE_MAX];
	int sitey[SITE_MAX];
	int site_max;
	unsigned short data[1 << (MAP_SIZEX_LOG2 + MAP_SIZEY_LOG2)];
	int maze_pop();
	void maze_push(int x, int y);
	void maze_move_and_push(int x, int y, int d);
	int maze_judge(int cx, int cy, int dx, int dy, int x, int y);
};

#endif				// XKOBO_H_MAP

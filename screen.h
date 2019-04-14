/*
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 1995, 1996, Akira Higuchi
 * Copyright (C) 2001, 2002, David Olofson
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
 */

#ifndef _KOBO_SCREEN_H_
#define _KOBO_SCREEN_H_

#include "map.h"

class window_t;

class _screen
{
  protected:
	static int scene_num;
	static int level;
	static int generate_count;
	static _map map;
	static int translate(int n);
	static int scene_max;
	static int pixel_f0, pixel_f1, pixel_f2, pixel_s[8];
	static int show_title;
	static float _fps;
	static float scroller_speed;
	static float target_speed;
	static int hi_sc[10];
	static int hi_st[10];
	static char hi_nm[10][20];
  public:
	static void init();
	static void init_scene(int sc);
	static int prepare();
	static void generate_fixed_enemies();
	static int get_chip_number(int x, int y);
	static void set_chip_number(int x, int y, int n);
	static void render_background(window_t * win);
	static void title();
	static void init_highscores();
	static void highscores(int t);
	static void credits(int t);
	static void scroller();
	static void usage();
	static void fps(float f);
	static float fps()	{ return _fps; }
};

extern _screen screen;

#endif	//_KOBO_SCREEN_H_

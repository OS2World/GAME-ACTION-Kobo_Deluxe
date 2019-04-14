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

#include <math.h>

#include "kobo.h"
#include "screen.h"
#include "manage.h"
#include "options.h"
#include "enemies.h"
#include "myship.h"
#include "radar.h"
#include "scenes.h"
#include "config.h"
#include "random.h"
#include "version.h"

int _screen::scene_max;
int _screen::scene_num;
int _screen::level;
int _screen::generate_count;
_map _screen::map;
int _screen::pixel_f0 = -1;
int _screen::pixel_f1 = -1;
int _screen::pixel_f2 = -1;
int _screen::pixel_s[8];
int _screen::show_title = 0;
float _screen::_fps = 40;
float _screen::scroller_speed = SCROLLER_SPEED;
float _screen::target_speed = SCROLLER_SPEED;

int _screen::hi_sc[10];
int _screen::hi_st[10];
char _screen::hi_nm[10][20];

/* Translate "map code" into tile index */
int _screen::translate(int n)
{
	if(IS_SPACE(n))
		return (n & 0xff) + 64;

	int x = n;
	int y = 7;
	if(n & HARD)
	{
		y = 6;
		if(n & U_MASK)
			x = 8;
		else if(n & R_MASK)
			x = 9;
		else if(n & D_MASK)
			x = 10;
		else if(n & L_MASK)
			x = 11;
	}
	else if(n & CORE)
	{
		y = 6;
		x = 15;
	}
	return y * 16 + x - 104;
}


void _screen::init()
{
	scene_max = 0;
	while(scene[scene_max].ratio != -1)
		scene_max++;
	if(pixel_f0 < 0)
	{
		pixel_f0 = wbase.map_rgb(200, 255, 255);
		pixel_f1 = wbase.map_rgb(140, 200, 200);
		pixel_f2 = wbase.map_rgb(100, 200, 100);
		int i;
		for(i = 0; i < 8; i++)
		{
			pixel_s[i] = wbase.map_rgb(160,
					60 + i * 16, 200 - i * 20);
		}
	}
}


/*
FIXME: This should be replaced with some nice scrolling stuff.
*/
void _screen::title()
{
	wchip.foreground(pixel_f1);
	wchip.sprite((wchip.width() - 208)/2, 40, B_LOGO, 0);
	wchip.font(B_NORMAL_FONT);
	wchip.center(115, "v" KOBO_VERSION);
	if(prefs.cmd_cheat && (manage.count & 0x10))
		wchip.center(130, "CHEAT MODE");
	wchip.foreground(pixel_f2);
}


void _screen::init_highscores()
{
	for(unsigned int i = 0; i < 10; ++i)
		if(i < scorefile.highs)
		{
			hi_sc[i] = scorefile.high_tbl[i].score;
			hi_st[i] = scorefile.high_tbl[i].end_scene;
			strncpy(hi_nm[i], scorefile.high_tbl[i].name, 19);
			hi_nm[i][19] = 0;
		}
		else
		{
			hi_sc[i] = 0;
			hi_st[i] = 0;
			strcpy(hi_nm[i], "---");
		}

}


void _screen::highscores(int t)
{
	int i, y;

	wchip.font(B_BIG_FONT);
	wchip.center(30, "HALL OF FAME");
	wchip.sprite(30, 32, B_SPRITES, 48 + (t/100) % 8);
	wchip.sprite(wchip.width()-30-16, 32, B_SPRITES, 48 + (t/100) % 8);

	wchip.font(B_NORMAL_FONT);
#if 0
	wchip.string(35, 50, "Score");
	wchip.string(85, 50, "Stage");
	wchip.string(135, 50, "Name");
#endif
	wchip.center(50, "(Score/Stage/Name)");
	wchip.font(B_MEDIUM_FONT);
	float yo = t * (11*18+100) * 256.0 / 12500.0 - PIXEL2CS(110);
	for(i = 0, y = 65; i < 10; ++i, y += 18)
	{
		static char s[20];
		float xo, cy;
		int real_y = PIXEL2CS(y) - (int)yo;
		if(real_y < PIXEL2CS(55) || real_y > PIXEL2CS(165))
			continue;
		cy = (PIXEL2CS(y) - yo) - PIXEL2CS(65 + 5*18/2);
		xo = cy*cy*cy*cy*cy * 1e-16;
		snprintf(s, 16, "%d", hi_sc[i]);
		wchip.center_token_fxp(PIXEL2CS(90)+(int)xo, real_y, s);
		snprintf(s, 16, "%d", hi_st[i]);
		wchip.center_token_fxp(PIXEL2CS((90+125)/2)+(int)xo, real_y, s, -1);
		wchip.string_fxp(PIXEL2CS(125)+(int)xo, real_y, hi_nm[i]);
	}
}


void _screen::credits(int t)
{
	wchip.font(B_NORMAL_FONT);
	wchip.center(30, "KOBO DELUXE was created by...");

	if(t % 4000 < 3700)
		switch(t / 4000)
		{
		  case 0:
			wchip.font(B_BIG_FONT);
			wchip.center(80, "DAVID OLOFSON");
			wchip.font(B_NORMAL_FONT);
			t = 6 * (t % 4000) / 3700;
			if(t > 0)
				wchip.center(100, "SDL Port");
			if(t > 1)
				wchip.center(110, "New Audio & GFX Engines");
			if(t > 2)
				wchip.center(120, "Additional SFX, GFX & Music");
			break;
		  case 1:
			wchip.font(B_BIG_FONT);
			wchip.center(80, "AKIRA HIGUCHI");
			wchip.font(B_NORMAL_FONT);
			t = 6 * (t % 4000) / 3700;
			if(t > 0)
				wchip.center(100, "XKobo - The Original Game");
			break;
		  case 2:
			wchip.font(B_BIG_FONT);
			wchip.center(80, "MASANAO IZUMO");
			wchip.font(B_NORMAL_FONT);
			t = 6 * (t % 4000) / 3700;
			if(t > 0)
				wchip.center(100, "Original Sounds");
			if(t > 1)
				wchip.center(110, "Original Audio Engine");
			break;
		}

	wchip.font(B_MEDIUM_FONT);
	wchip.center(145, "See scroller below for");
	wchip.center(160, "Additional Credits & Thanks");
}


void _screen::scroller()
{
	/*
	 * Adjust scroller speed according to
	 * frame rate, for readability.
	 */
	if(_fps < 30)
		target_speed = SCROLLER_SPEED / 2;
	else if(_fps > 40)
		target_speed = SCROLLER_SPEED;

	scroller_speed += (target_speed - scroller_speed) * 0.05;

	static const char scrolltext[] =
			"Welcome to KOBO DELUXE, an enhanced version of "
			"Akira Higuchi's fabulous X-Window game XKOBO. "
			"     "
			"This version uses SDL, the Simple DirectMedia "
			"Layer (http://www.libsdl.org/) for graphics, "
			"sound and input, and should be easy to port "
			"to most reasonable platforms. "
			"     "
			"KOBO DELUXE has been known to hinder productivity on: "
			"     "
			"  -  "
			"GNU/Linux/x86  -  "
			"FreeBSD/x86  -  "
			"NetBSD/x86  -  "
			"Windows 95/98/ME  -  "
			"Windows 2000/XP  -  "
			"Solaris/x86  -  "
			"Solaris/SPARC  -  "
			"Mac OS X/PPC  -  "
			"BeOS/x86  -  "
			"BeOS/PPC  -  "
			"AmigaOS  -  "
			"     "
			"Any help in the Cause of Infiltrating Further "
			"Platforms is Greatly Appreciated!  Mac OS Classic "
			"and various Handheld Devices are of Particular "
			"Interest. "
			"                        "
			"Additional Credits & Thanks to: "
			"      Samuel Hart (Joystick Support)"
			"      Max Horn (Mac OS X & Build Script Patches)"
			"      Jeremy Sheeley (Player Profiles)"
			"      Tsuyoshi Iguchi (FreeBSD, NetBSD)"
			"      G. Low (Solaris)"
			"      Gerry Jo \"Trick\" Jellestad (Testing & Ideas)"
			"      \"Riki\" (Intel Compiler)"
			"      Andreas Spaangberg (Sun Compiler & Bug Spotting)"
			"      \"SixK\" (Amiga Port)"
			"      Sam Lantinga & Others (SDL)"
			"      Members of the SDL Mailing List"
			"      My Girlfriend, Ingela (Ideas & Putting up "
			"with my Around-the-Clock Hacking)"
			"                        "
			"Additional Thanks from Akira Higuchi Go To:"
			"      Bruce Cheng"
			"      Christoph Lameter"
			"      Davide Rossi"
			"      Eduard Martinescu"
			"      Elan Feingold"
			"      Helmut Hoenig"
			"      Jeff Epler"
			"      Joe Ramey"
			"      Joey Hess"
			"      Michael Sterrett"
			"      Mihail Iotov"
			"      Shoichi Nakayama"
			"      Thomas Marsh"
			"      Torsten Wolnik"
			"                        "
			"Now, if you were around the so called "
			"\"Demo Scene\" back in the days of the "
			"glorious C64, Amiga and Atari ST machines, "
			"you're about to see a familiar term... :-)"
			"                        "
			"         <WRAP>         ";
	static const char *stp = scrolltext;
	static int pos = -PIXEL2CS(SCREEN_WIDTH);
	static int t = 0;
	static int offs = 0;
	char buf[2] = {0, 0};
	int nt = (int)SDL_GetTicks();
	int dt = nt - t;
	t = nt;
	if(dt > 100)
		dt = 100;
#if 1
	static int fdt = 0;
	fdt += ((dt<<8) - fdt) >> 2;
#else
	int fdt = dt<<8;
#endif
	pos += (fdt * PIXEL2CS((int)scroller_speed) / 1000) >> 8;
	wchip.font(B_BIG_FONT);
	wchip.string_fxp(-pos + offs, PIXEL2CS(190), stp);

	/*
	 * Chop away characters at the left edge
	 */
	buf[0] = *stp;
	int cw = wchip.textwidth(buf);
	if(CS2PIXEL(-pos + offs) < -cw)
	{
		offs += PIXEL2CS(cw);
		++stp;
		if(*stp == 0)
		{
			pos = -PIXEL2CS(SCREEN_WIDTH);
			stp = scrolltext;
			offs = 0;
		}
	}
}


void _screen::usage()
{
	int t = SDL_GetTicks();
	float ft = t * 0.001;
	int y = PIXEL2CS(180) + (int)floor(PIXEL2CS(5)*sin(ft * 6));

	if(t % 3000 < 2500)
	{
		wchip.font(B_BIG_FONT);
		switch((t/3000) % 6)
		{
		  case 0:
			wchip.center_fxp(y, "---- INSTRUCTIONS ----");
			break;
		  case 1:
			wchip.center_fxp(y, "Press SPACE to Start Game!");
			break;
		  case 2:
			wchip.center_fxp(y, "Use +/- to Select Start Stage");
			break;
		  case 3:
			wchip.center_fxp(y, "Hit Backspace to Rebuild Stage");
			break;
		  case 4:
			wchip.center_fxp(y, "Hit RETURN for Options");
			break;
		  case 5:
			wchip.center_fxp(y, "(Don't) Hit ESC to Quit");
			break;
		}
	}
}


void _screen::init_scene(int sc)
{
	map.init();
	if(sc < 0)
	{
		/*
		 * Intro mode
		 */
		show_title = 1;
		myship.off();
		enemies.off();
		if(sc == INTRO_SCENE)
		{
			/*
			 * Plain intro - no map
			 */
			radar.prepare(1);	/* Clear radar */
			scene_num = level = 19;
		}
		else
		{
			/*
			 * Map selection - show current map
			 */
			radar.prepare(0);
			scene_num = -(sc+1) % scene_max;
			level = -(sc+1) / scene_max;
		}
	}
	else
	{
		/*
		 * In-game mode
		 */
		gengine.period(0);
		show_title = 0;
		scene_num = sc % scene_max;
		level = sc / scene_max;
	}

	_scene *s = &scene[scene_num];
	int i;
	for(i = 0; i < s->base_max; i++)
		map.make_maze(s->base[i].x, s->base[i].y, s->base[i].h,
				s->base[i].v);
	map.convert(s->ratio);

	/* No radar map in intro mode! */
	if(INTRO_SCENE == sc)
		radar.prepare(1);
	else
		radar.prepare(0);

	generate_count = 0;
}


int _screen::prepare()
{
	if(scene_num < 0)
		return 0;
	_scene *s = &scene[scene_num];
	int i, j;
	int count_core = 0;
	int c = 0;

	int interval_1, interval_2;
	interval_1 = (s->ek1_interval) >> level;
	interval_2 = (s->ek2_interval) >> level;
	if(interval_1 < 4)
		interval_1 = 4;
	if(interval_2 < 4)
		interval_2 = 4;
	enemies.set_ekind_to_generate(s->ek1, interval_1, s->ek2,
			interval_2);

	wchip.clear();
	for(i = 0; i < MAP_SIZEX; i++)
		for(j = 0; j < MAP_SIZEY; j++)
		{
			int m = map.pos(i, j);
			if((m == U_MASK) || (m == R_MASK) || (m == D_MASK)
					|| (m == L_MASK))
			{
				enemies.make(&cannon, i * 16 + 8,
						j * 16 + 8);
				c++;
			}
			else if(m == CORE)
			{
				enemies.make(&core, i * 16 + 8,
						j * 16 + 8);
				count_core++;
				c++;
			}
		}

	myship.set_position(s->startx << 4, s->starty << 4);

	return count_core;
}


void _screen::generate_fixed_enemies()
{
	static int sint[16] =
			{ 0, 12, 23, 30, 32, 30, 23, 12, 0, -12, -23, -30,
				-32, -30, -23, -12 };
	static int cost[16] =
			{ 32, 30, 23, 12, 0, -12, -23, -30, -32, -30, -23,
				-12, 0, 12, 23, 30 };
	_scene *s = &scene[scene_num];
	if(generate_count < s->enemy_max)
	{
		int j;
		for(j = 0; j < s->enemy[generate_count].num; j++)
		{
			int sp = s->enemy[generate_count].speed;
			int x, y, h, v, t;
			x = gamerand.get() % (WORLD_SIZEX - VIEWLIMIT * 2);
			y = gamerand.get() % (WORLD_SIZEY - VIEWLIMIT * 2);
			x -= (WORLD_SIZEX / 2 - VIEWLIMIT);
			y -= (WORLD_SIZEY / 2 - VIEWLIMIT);
			if(x < 0)
				x -= VIEWLIMIT;
			else
				x += VIEWLIMIT;
			if(y < 0)
				y -= VIEWLIMIT;
			else
				y += VIEWLIMIT;
			x += myship.get_x();
			y += myship.get_y();

			t = gamerand.get(4);
			h = PIXEL2CS(sp * sint[t]) / 64;
			v = PIXEL2CS(sp * cost[t]) / 64;
			enemies.make(s->enemy[generate_count].kind, x, y,
					h, v);
		}
		generate_count++;
	}
	if(generate_count >= s->enemy_max)
		generate_count = 0;
}


int _screen::get_chip_number(int x, int y)
{
	return map.pos(x, y);
}


void _screen::set_chip_number(int x, int y, int n)
{
	if(n == 0)
	{
		map.clearpos(x, y);
		radar.update(x, y);
	}
	else
		map.pos(x, y) = n;

}


void _screen::render_background(window_t *win)
{
	if(!win)
		return;

	int vx, vy, xo, yo, x, y, xmax, ymax;
	int mx, my;
	vx = gengine.xoffs(LAYER_BASES);
	vy = gengine.yoffs(LAYER_BASES);

	/* 
	 * Start exactly at the top-left corner
	 * of the tile visible in the top-left
	 * corner of the display window.
	 */
	xo = vx & (PIXEL2CS(CHIP_SIZEX) - 1);
	yo = vy & (PIXEL2CS(CHIP_SIZEY) - 1);
	mx = CS2PIXEL(vx >> CHIP_SIZEX_LOG2);
	my = CS2PIXEL(vy >> CHIP_SIZEY_LOG2);
	ymax = ((WSIZE+CS2PIXEL(yo)) >> CHIP_SIZEY_LOG2) + 1;
	xmax = ((WSIZE+CS2PIXEL(xo)) >> CHIP_SIZEX_LOG2) + 1;

	for(y = 0; y < ymax; ++y)
		for(x = 0; x < xmax; ++x)
		{
			int n = map.pos(mx + x, my + y);
#if 0
			if(IS_SPACE(n))
			{
				win->foreground(win->map_rgb(0,0,0));
				win->fillrect_fxp(PIXEL2CS(x<<CHIP_SIZEX_LOG2) - xo,
						PIXEL2CS(y<<CHIP_SIZEX_LOG2) - yo,
						PIXEL2CS(CHIP_SIZEX),
						PIXEL2CS(CHIP_SIZEY));
			}
			else
#endif
			win->sprite_fxp(PIXEL2CS(x<<CHIP_SIZEX_LOG2) - xo,
					PIXEL2CS(y<<CHIP_SIZEX_LOG2) - yo,
					B_TILES, translate(n));
		}
}


void _screen::fps(float f)
{
	_fps = f;
}

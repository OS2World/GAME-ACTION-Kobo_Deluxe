/*(GPL)
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

#define	DBG(x)	x

#undef	DEBUG_OUT

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef DEBUG
/*For the oscilloscope*/
#include "a_globals.h"
/*For VU and voice info*/
#include "a_struct.h"
#endif
}

#include "config.h"
#include "kobo.h"
#include "states.h"
#include "screen.h"
#include "manage.h"
#include "score.h"
#include "radar.h"
#include "gamectl.h"
#include "random.h"
#include "version.h"
#include "options.h"
#include "myship.h"
#include "enemies.h"
#include "audio.h"
#include "a_wca.h"

#ifdef DEBUG
int audio_vismode = 0;
#endif

skobo_gfxengine_t gengine;
filemapper_t fmap;
prefs_t prefs;

SDL_Joystick *joystick = NULL;
int js_lr = DEFAULT_JOY_LR;
int js_ud = DEFAULT_JOY_UD;
int js_fire = DEFAULT_JOY_FIRE;
int js_start = DEFAULT_JOY_START;

int mouse_x = 0, mouse_y = 0;
int mouse_left = 0, mouse_middle = 0, mouse_right = 0;

window_t wbase;
window_t wchip;
window_t wradar;
window_t whealth;

display_t dhigh;
display_t dscore;
display_t dstage;
display_t dships;

int exit_game = 0;
int exit_game_fast = 0;


#define	MAX_FPS_RESULTS	64
static int fps_count = 0;
static int fps_starttime = 0;
static int fps_nextresult = 0;
static int fps_lastresult = 0;
static float *fps_results = NULL;
display_t dfps;

static void _print_fps_results()
{
	int i, r = fps_nextresult;
	if(fps_lastresult != MAX_FPS_RESULTS-1)
		r = 0;
	for(i = 0; i < fps_lastresult; ++i)
	{
		printf("%.1f fps\n", fps_results[r++]);
		if(r >= MAX_FPS_RESULTS)
			r = 0;
	}

	free(fps_results);
	fps_nextresult = 0;
	fps_lastresult = 0;
	fps_results = NULL;
}


int intscale = 1;
int xoffs = 0;
int yoffs = 0;


void flush_events()
{
	SDL_Event e;
	while(SDL_PollEvent(&e))
	{
		switch(e.type)
		{
		  case SDL_QUIT:
			exit_game_fast = 1;
			break;
		}
	}
}


int quit_requested()
{
	flush_events();
	if(exit_game_fast)
		return 1;
	return SDL_QuitRequested();
}


#define	NIBBLE_W	4
#define	NIBBLE_H	4
#define	NIBBLE_TILES	((SCREEN_WIDTH/NIBBLE_W+1)*(SCREEN_HEIGHT/NIBBLE_H+1))

/*
 * Tools:
 *	-1: Random tool
 *	0:  Black 4x4 square
 *	1+: Brushes from "brushes.png";
 *		1: Soft black 16 pixel round brush
 *		2: (4/8)x4 tilable romboid brush
 *		3: Tilable garbage brush
 *		4: Vertical "D" brush
 */
void nibble(int tool = -1)
{
	int i;
	int x[NIBBLE_TILES];
	int y[NIBBLE_TILES];

	if(tool < 0)
		tool = (pubrand.get(20) + SDL_GetTicks()) % 5;

	/* Clear */
	for(i = 0; i < NIBBLE_TILES; ++i)
		x[i] = y[i] = -1;

	/* Fill in */
	int ind = 0;
	int xx = -NIBBLE_W / 2;
	int yy = -NIBBLE_H / 2;
	for(i = 0; i < NIBBLE_TILES; ++i)
	{
		ind = pubrand.get() % NIBBLE_TILES;
		if(ind >= NIBBLE_TILES)
			ind = 0;
		while(x[ind] != -1)
			if(++ind >= NIBBLE_TILES)
				ind = 0;
		x[ind] = xx;
		y[ind] = yy;
		xx += NIBBLE_W;
		if(xx >= SCREEN_WIDTH + NIBBLE_W / 2)
		{
			xx = -NIBBLE_W / 2;
			yy += NIBBLE_H;
		}
	}

	/* Clear 4000 tiles/second, until all are done. */
	wbase.foreground(wbase.map_rgb(0x000000));
	ind = 0;
	int last_index;
	int t = SDL_GetTicks();
	while(ind < NIBBLE_TILES)
	{
		int nt = SDL_GetTicks();
		int dt = nt - t;
		t = nt;
		last_index = ind;	/* For double buffer mode */
		for(i = 0; i < dt * 4; ++i)
		{
			if(ind >= NIBBLE_TILES)
				break;
			switch (tool)
			{
			  case 0:
				wbase.fillrect(x[ind] + NIBBLE_W / 2,
						y[ind] + NIBBLE_H / 2,
						NIBBLE_W, NIBBLE_H);
				break;
			  default:
				wbase.sprite(x[ind] - 8 + NIBBLE_W / 2,
						y[ind] - 8 +
						NIBBLE_H / 2, B_BRUSHES,
						tool - 1);
				break;
			}
			++ind;
		}
		gengine.invalidate();
		gengine.flip();
		if(gengine.buffering() >= GFX_BUFFER_DOUBLE)
		{
			for(i = 0; i < dt * 4; ++i)
			{
				if(last_index >= NIBBLE_TILES)
					break;
				switch (tool)
				{
				  case 0:
					wbase.fillrect(x[last_index] +
							NIBBLE_W / 2,
							y[last_index] +
							NIBBLE_H / 2,
							NIBBLE_W,
							NIBBLE_H);
					break;
				  default:
					wbase.sprite(x[last_index] - 8 +
							NIBBLE_W / 2,
							y[last_index] - 8 +
							NIBBLE_H / 2,
							B_BRUSHES,
							tool - 1);
					break;
				}
				++last_index;
			}
		}
	}
	wbase.fillrect(0, 0, wbase.width(), wbase.height());
	gengine.invalidate();
	gengine.flip();
	if(gengine.buffering() >= GFX_BUFFER_DOUBLE)
	{
		wbase.fillrect(0, 0, wbase.width(), wbase.height());
		gengine.invalidate();
		gengine.flip();
	}
	SDL_Delay(200);
}




void put_usage()
{
	printf("\nKobo Deluxe %s\n", KOBO_VERSION);
	printf("Usage: kobodl -hiscores\n");
	printf("       kobodl -highscores\n");
	printf("       kobodl -showcfg\n");
	printf("       kobodl [-noframe]\n");
	printf("              [-fps]\n");
	printf("              [-debug]\n");
	printf("              [-[no]fullscreen]\n");
	printf("              [-buffer <n>] (default: 0)\n");
	printf("              [-depth <n>] (default: 0)\n");
	printf("              [-[no]nofilter]\n");
	printf("              [-[no]cheat]\n");
	printf("              [-[no]indicator]\n");
	printf("              [-wait <n>] (default: 30)\n");
	printf("              [-[no]nosound]\n");
#ifdef HAVE_OSS
	printf("              [-[no]oss]\n");
#endif
	printf("              [-latency <n>] (default: 50)\n");
	printf("              [-vol <n>] (default: 100)\n");
	printf("              [-reverb <n>] (default: 100)\n");
	printf("              [-threshold <n>] (default: 100)\n");
	printf("              [-release <n>] (default: 100)\n");
	printf("              [-bgm <bgm index>]\n");
	printf("              [-files <kobo data dir path>]\n");
	printf("              [-gfx <gfx dir path>]\n");
	printf("              [-scores <scores dir path>]\n");
	printf("              [-[no]joystick]\n");
	printf("\n");
	exit(1);
}


void setup_dirs(char *xpath)
{
	fmap.exepath(xpath);

	fmap.addpath("DATA", KOBO_DATA_DIR);

	/*
	 * Graphics data
	 */
	/* Current dir; from within the build tree */
	fmap.addpath("GFX", "./data");
	/* Real data dir */
	fmap.addpath("GFX", "DATA>>gfx");
	/* Current dir */
	fmap.addpath("GFX", "./gfx");

	/*
	 * Sound data
	 */
	/* Current dir; from within the build tree */
	fmap.addpath("SFX", "./data");
	/* Real data dir */
	fmap.addpath("SFX", "DATA>>sfx");
	/* Current dir */
	fmap.addpath("SFX", "./sfx");

	/*
	 * Score files (user and global)
	 */
	fmap.addpath("SCORES", KOBO_SCORE_DIR);
	/* 'scores' in current dir (For importing scores, perhaps...) */
// (Disabled for now, since filemapper_t can't tell
// when it hits the same dir more than once...)
//	fmap.addpath("SCORES", "./scores");

	/*
	 * Configuration files
	 */
	fmap.addpath("CONFIG", KOBO_CONFIG_DIR);
	/* System local */
	fmap.addpath("CONFIG", SYSCONF_DIR);
	/* In current dir (last resort) */
	fmap.addpath("CONFIG", "./");
}


void add_dirs(prefs_t *p)
{
	char buf[300];
	if(p->dir[0])
	{
		char *upath = fmap.sys2unix(p->dir);
		snprintf(buf, 300, "%s/sfx", upath);
		fmap.addpath("SFX", buf, 1);
		snprintf(buf, 300, "%s/gfx", upath);
		fmap.addpath("GFX", buf, 1);
		snprintf(buf, 300, "%s/scores", upath);
		fmap.addpath("SCORES", buf, 1);
		snprintf(buf, 300, "%s", upath);
		fmap.addpath("CONFIG", buf, 0);
	}

	if(p->sfxdir[0])
		fmap.addpath("SFX", fmap.sys2unix(p->sfxdir), 1);

	if(p->gfxdir[0])
		fmap.addpath("GFX", fmap.sys2unix(p->gfxdir), 1);

	if(p->scoredir[0])
		fmap.addpath("SCORES", fmap.sys2unix(p->scoredir), 1);
}


int init_display(prefs_t *p)
{
	gengine.title("Kobo Deluxe v" VERSION, "kobodl");
	gengine.driver((gfx_drivers_t)p->videodriver);
	gengine.scale(p->width/SCREEN_WIDTH, p->height/SCREEN_HEIGHT);

//We don't support non-integer scaling ratios yet, so we just center
//a window of the next smaller supported resolution...
	intscale = p->width/SCREEN_WIDTH;
//	if(p->videodriver == GFX_DRIVER_SDL2D)
//	{
		xoffs = (p->width/intscale - SCREEN_WIDTH) / 2;
		yoffs = (p->height/intscale - SCREEN_HEIGHT) / 2;
//	}
//	else
//		xoffs = yoffs = 0;

	//Add thin black border around the game "screen" in windowed mode.
	if(p->fullscreen)
		gengine.size(p->width, p->height);
	else
	{
		int border = 4;	//No, it's not intended to scale! :-)
		xoffs += border/intscale;
		yoffs += border/intscale;
		gengine.size(p->width+border*2, p->height+border*2);
	}
	gengine.mode(0, p->fullscreen);
	gengine.buffering((gfx_buffermodes_t)(p->buffermode), GFX_SYNC_FLIP);
	gengine.cursor(0);
	gengine.period(p->wait_msec);
	gengine.interpolation(p->filter);
	gengine.scroll_ratio(LAYER_OVERLAY, 0.0, 0.0);
	gengine.scroll_ratio(LAYER_PLAYER, 1.0, 1.0);
	gengine.scroll_ratio(LAYER_ENEMIES, 1.0, 1.0);
	gengine.scroll_ratio(LAYER_BASES, 1.0, 1.0);
	gengine.scroll_ratio(LAYER_GROUND, 1.0, 1.0);
	gengine.scroll_ratio(LAYER_STARS, 0.5, 0.5);
	gengine.wrap(MAP_SIZEX*CHIP_SIZEX, MAP_SIZEY*CHIP_SIZEY);
	wbase.init(&gengine);
	wbase.place(xoffs, yoffs, SCREEN_WIDTH, SCREEN_HEIGHT);
	gengine.dither(p->use_dither, p->broken_rgba8);
	return gengine.open(2048);
}


void show_progress(prefs_t *p)
{
	const char *fn;
	int buffers;

	gengine.colorkey(255, 0, 0);
	gengine.clampcolor(0, 0, 0, 0);
	gengine.scalemode((gfx_scalemodes_t) p->scalemode);
	gengine.dither(p->use_dither, p->broken_rgba8);

	fn = fmap.get("GFX>>loading.png");
	if(fn)
		gengine.loadimage(B_LOADING, fn);

	if(gengine.buffering() >= GFX_BUFFER_DOUBLE)
		buffers = 2;
	else
		buffers = 1;
	while(buffers--)
	{
		wbase.background(wbase.map_rgb(0x000000));
		wbase.clear();
		wbase.sprite((wbase.width() - 158) / 2, 60, B_LOADING, 0);
		gengine.invalidate();
		gengine.flip();
	}
}


void progress(int percent)
{
	int buffers;
	int x, y, w, h;

	if(gengine.buffering() >= GFX_BUFFER_DOUBLE)
		buffers = 2;
	else
		buffers = 1;
	while(buffers--)
	{
		x = 0;
		w = wbase.width() * percent / 100;
		if(w < 4)
			return;

		h = 16;
		y = wbase.height() - h;

		SDL_Rect r;
		r.x = x;
		r.y = y;
		r.w = w;
		r.h = h;

		wbase.foreground(wbase.map_rgb(0x000099));
		wbase.rectangle(x, y, w, h);

		++x;
		++y;
		w -= 2;
		h -= 2;
		wbase.foreground(wbase.map_rgb( 0x0000cc));
		wbase.rectangle(x, y, w, h);

		++x;
		++y;
		w -= 2;
		h -= 2;
		wbase.foreground(wbase.map_rgb( 0x0000ff));
		wbase.fillrect(x, y, w, h);

		wbase.invalidate(&r);
		gengine.flip();
	}
}


#define	TUBE_LENGTH	52

#define	PROGRESS(x)	progress(x); if(quit_requested()) return -999;
int load_graphics(prefs_t *p)
{
	const char *fn;
	SDL_Rect r;

	show_progress(p);

	fn = fmap.get("GFX>>tiles.pcx");
	if(!fn || gengine.loadtiles(B_TILES, 16, 16, fn) < 0)
		return -1;
	PROGRESS(10);
	fn = fmap.get("GFX>>sprites.pcx");
	if(!fn || gengine.loadsprites(B_SPRITES, 16, 16, fn) < 0)
		return -2;
	PROGRESS(15);
	fn = fmap.get("GFX>>bullets.pcx");
	if(!fn || gengine.loadsprites(B_BULLETS, 8, 8, fn) < 0)
		return -3;
	PROGRESS(20);
	fn = fmap.get("GFX>>bigship.pcx");
	if(!fn || gengine.loadsprites(B_BIGSHIP, 32, 32, fn) < 0)
		return -4;
	PROGRESS(25);

	gengine.clampcolor(0, 0, 0, 255);
	fn = fmap.get("GFX>>screen.png");
	if(!fn || gengine.loadimage(B_SCREEN, fn) < 0)
		return -5;
	PROGRESS(40);
	r.x = (8 + MARGIN) * intscale;
	r.y = MARGIN * intscale;
	r.w = r.h = WSIZE * intscale;
	if(gengine.loadrect(B_FRAME, B_SCREEN, 0, &r) < 0)
		return -6;
	r.x = (8 + 240 + 1) * intscale;
	r.y = 8 * intscale;
	r.w = 64 * intscale;
	r.h = 18 * intscale;
	if(gengine.loadrect(B_HIGH_BACK, B_SCREEN, 0, &r) < 0)
		return -26;
	r.y += (18 + 6) * intscale;
	if(gengine.loadrect(B_SCORE_BACK, B_SCREEN, 0, &r) < 0)
		return -27;
	r.y += (18 + 6) * intscale;
	r.w = MAP_SIZEX * intscale;
	r.h = MAP_SIZEY * intscale;
	if(gengine.loadrect(B_RADAR_BACK, B_SCREEN, 0, &r) < 0)
		return -28;
	r.y += (MAP_SIZEY + 6) * intscale;
	r.w = 38 * intscale;
	r.h = 18 * intscale;
	if(gengine.loadrect(B_SHIPS_BACK, B_SCREEN, 0, &r) < 0)
		return -29;
	r.y += (18 + 6) * intscale;
	if(gengine.loadrect(B_STAGE_BACK, B_SCREEN, 0, &r) < 0)
		return -30;
	PROGRESS(45);

	gengine.clampcolor(0, 0, 0, 0);

	fn = fmap.get("GFX>>logo3.png");
	if(!fn || gengine.loadimage(B_LOGO, fn) < 0)
		return -7;
	PROGRESS(60);

	fn = fmap.get("GFX>>font2b.png");
	if(!fn || gengine.loadfont(B_NORMAL_FONT, fn) < 0)
		return -8;
	PROGRESS(70);

	fn = fmap.get("GFX>>font4b.png");
	if(!fn || gengine.loadfont(B_MEDIUM_FONT, fn) < 0)
		return -9;
	PROGRESS(75);

	fn = fmap.get("GFX>>font3b.png");
	if(!fn || gengine.loadfont(B_BIG_FONT, fn) < 0)
		return -9;
	PROGRESS(80);

	fn = fmap.get("GFX>>counter.png");
	if(!fn || gengine.loadfont(B_COUNTER_FONT, fn) < 0)
		return -10;
	PROGRESS(85);

	fn = fmap.get("GFX>>brushes.png");
	if(!fn || gengine.loadsprites(B_BRUSHES, 16, 16, fn) < 0)
		return -11;
	PROGRESS(87);

	fn = fmap.get("GFX>>tube1.png");
	if(!fn || gengine.loadimage(B_TEMP, fn) < 0)
		return -12;
	r.x = (60 - TUBE_LENGTH) * intscale / 2;
	r.y = 0;
	r.w = r.h = TUBE_LENGTH * intscale;
	if(gengine.loadrect(B_BIG_TUBE, B_TEMP, 0, &r) < 0)
		return -13;
	gengine.unload(B_TEMP);
	PROGRESS(90);

	fn = fmap.get("GFX>>tube2.png");
	if(!fn || gengine.loadimage(B_TEMP, fn) < 0)
		return -14;
	r.x = (60 - TUBE_LENGTH) * intscale / 2;
	r.y = 0;
	r.w = r.h = TUBE_LENGTH * intscale;
	if(gengine.loadrect(B_SMALL_TUBE, B_TEMP, 0, &r) < 0)
		return -15;
	gengine.unload(B_TEMP);
	PROGRESS(100);

	nibble();
	return 0;
}
#undef	PROGRESS


void build_screen()
{
	wbase.font(B_NORMAL_FONT);
	wbase.foreground(wbase.map_rgb(0xffffff));
	wbase.background(wbase.map_rgb(0x000000));

	whealth.init(&gengine);
	whealth.place(xoffs + 5, yoffs + 56, 2, 128);

	wchip.init(&gengine);
	wchip.place(xoffs + 8 + MARGIN, yoffs + MARGIN, WSIZE,
			WSIZE, MAP_SIZEX * CHIP_SIZEX,
			MAP_SIZEY * CHIP_SIZEY);
	wchip.font(B_NORMAL_FONT);
	wchip.foreground(wbase.map_rgb(0xffffff));
	wchip.background(wbase.map_rgb(0x000000));
	gengine.output(&wchip);

	dhigh.init(&gengine);
	dhigh.place(xoffs + 8 + 240 + 1, yoffs + 8, 64, 18);
	dhigh.font(B_NORMAL_FONT);
	dhigh.bgimage(B_HIGH_BACK, 0);
	dhigh.caption("HIGHSCORE");
	dhigh.text("000000000");

	dscore.init(&gengine);
	dscore.place(dhigh.x(), dhigh.y2() + 6, 64, 18);
	dscore.font(B_NORMAL_FONT);
	dscore.bgimage(B_SCORE_BACK, 0);
	dscore.caption("SCORE");
	dscore.text("000000000");

	wradar.init(&gengine);
	wradar.place(dhigh.x(),
			yoffs + (SCREEN_HEIGHT - MAP_SIZEY) / 2,
			MAP_SIZEX, MAP_SIZEY);
	wradar.bgimage(B_RADAR_BACK, 0);

	dships.init(&gengine);
	dships.place(dhigh.x(), wradar.y2() + 6, 38, 18);
	dships.font(B_NORMAL_FONT);
	dships.bgimage(B_SHIPS_BACK, 0);
	dships.caption("SHIPS");
	dships.text("000");

	dstage.init(&gengine);
	dstage.place(dhigh.x(), dships.y2() + 6, 38, 18);
	dstage.font(B_NORMAL_FONT);
	dstage.bgimage(B_STAGE_BACK, 0);
	dstage.caption("STAGE");
	dstage.text("000");

	if(prefs.cmd_fps)
	{
		dfps.init(&gengine);
		dfps.place(xoffs + wbase.width() - 32,
				yoffs + wbase.height() - 18, 32, 18);
		dfps.color(wbase.map_rgb(0x000000));
		dfps.font(B_NORMAL_FONT);
		dfps.caption("FPS");
	}

	int buffers;
	if(gengine.buffering() >= GFX_BUFFER_DOUBLE)
		buffers = 2;
	else
		buffers = 1;
	while(buffers--)
	{
		wbase.sprite(0, 0, B_SCREEN, 0);
		dhigh.render();
		dscore.render();
		dships.render();
		dstage.render();
		if(prefs.cmd_fps)
			dfps.render();
		gengine.invalidate();
		gengine.flip();
	}
}


static int sounds_loaded = 0;
static int music_loaded = 0;

int load_sounds(prefs_t *p)
{
	const char *ap;
//	int i;
	if(!p->use_sound)
		return 0;

	show_progress(p);

	ap = fmap.get("SFX>>", FM_DIR);
	if(!ap)
	{
		fprintf(stderr, "Couldn't find audio data directory!\n");
		return -1;
	}
	audio_set_path(ap);

	progress(p->use_music ? 10 : 50);

	if(!sounds_loaded)
	{
		audio_wave_load(0, "sfx.agw", 0);
		sounds_loaded = 1;
	}
	progress(p->use_music ? 30 : 70);

	if(p->use_music && !music_loaded)
	{
		audio_wave_load(0, "music.agw", 0);
		music_loaded = 1;
	}
	progress(90);

//	if(p->cmd_debug)
//		audio_wave_load(0, "edit.agw", 0);
#if 0
	/* Build a set of basic waveforms */
	wca_reset();
	wca_val(WCA_FREQUENCY, 523.25);	/* C */
	audio_wave_format(32, AF_MONO16, 33488);
	wca_val(WCA_AMPLITUDE, 1.0);
	audio_wave_blank(32, 256, 1);	/* Two periods, for retrig/randtrig */
	for(i = 33; i <= 43; ++i)
		audio_wave_clone(32, i);

	wca_val(WCA_MOD1, 0.0);
	wca_osc(32, WCA_SINE, WCA_ADD);
	wca_osc(33, WCA_HALFSINE, WCA_ADD);
	wca_osc(34, WCA_RECTSINE, WCA_ADD);
	wca_val(WCA_MOD1, 0.5);
	wca_osc(35, WCA_RECTSINE, WCA_ADD);
	wca_val(WCA_MOD1, 0.1);
	wca_osc(36, WCA_PULSE, WCA_ADD);
	wca_val(WCA_MOD1, 0.2);
	wca_osc(37, WCA_PULSE, WCA_ADD);
	wca_val(WCA_MOD1, 0.3);
	wca_osc(38, WCA_PULSE, WCA_ADD);
	wca_val(WCA_MOD1, 0.4);
	wca_osc(39, WCA_PULSE, WCA_ADD);
	wca_val(WCA_MOD1, 0.5);
	wca_osc(40, WCA_PULSE, WCA_ADD);
	wca_val(WCA_MOD1, 0.0);
	wca_osc(41, WCA_TRIANGLE, WCA_ADD);
	wca_osc(42, WCA_NOISE, WCA_ADD);

	wca_val(WCA_AMPLITUDE, 0.7);
	wca_osc(43, WCA_SINE, WCA_ADD);
	wca_val(WCA_AMPLITUDE, 0.3);
	wca_osc(43, WCA_NOISE, WCA_ADD);

	audio_wave_format(31, AF_STEREO16, 33488);
	audio_wave_blank(31, 128, 1);	/* Two periods, for retrig/randtrig */
	wca_val(WCA_FREQUENCY, 523.25);	/* C */
	wca_val(WCA_AMPLITUDE, 1.0);
	wca_val(WCA_MOD1, 0.0);
	wca_osc(31, WCA_SINE, WCA_ADD);
#endif

	audio_wave_prepare(-1);
	progress(100);

	return 0;
}



void init_intro_music()
{
	int i;
	for(i = 16; i < AUDIO_MAX_CHANNELS; ++i)
		audio_channel_control(i, -1, ACC_GROUP, SOUND_GROUP_INTRO);
}


void init_ingame_music()
{
	int i;
	for(i = 16; i < AUDIO_MAX_CHANNELS; ++i)
		audio_channel_control(i, -1, ACC_GROUP, SOUND_GROUP_MUSIC);
}


void init_audio(prefs_t *p)
{
	int i;
	if(!p->use_sound)
		return;

	if(audio_start(p->samplerate, p->latency, p->use_oss, p->cmd_midi,
			p->cmd_pollaudio) < 0)
		fprintf(stderr, "Couldn't initialize audio;"
				" disabling sound effects.\n");

	//Music and fanfares
	audio_channel_control(0, -1, ACC_GROUP, SOUND_GROUP_MUSIC);
	audio_channel_control(1, -1, ACC_GROUP, SOUND_GROUP_MUSIC);

	//Sound effects
	for(i = 2; i < 16; ++i)
		audio_channel_control(i, -1, ACC_GROUP, SOUND_GROUP_SFX);

	init_intro_music();

	sound_wrap(MAP_SIZEX*CHIP_SIZEX, MAP_SIZEY*CHIP_SIZEY);
	sound_scale(VIEWLIMIT * 3 / 2, VIEWLIMIT);

	audio_master_volume((float)p->volume/100.0);
	audio_group_controlf(SOUND_GROUP_INTRO, ACC_VOLUME,
			(float)p->intro_vol/100.0);
	audio_group_controlf(SOUND_GROUP_SFX, ACC_VOLUME,
			(float)p->sfx_vol/100.0);
	audio_group_controlf(SOUND_GROUP_MUSIC, ACC_VOLUME,
			(float)p->music_vol/100.0);
	audio_set_limiter((float)p->threshold / 100.0,
			(float)p->release / 100.0);
	audio_quality((audio_quality_t)p->mixquality);

	/* Bus 7: Sound effects */
	audio_bus_control(0, 1, ABC_FX_TYPE, AFX_NONE);
	audio_bus_controlf(0, 0, ABC_SEND_MASTER, 1.0);
	audio_bus_controlf(0, 0, ABC_SEND_BUS_7, 0.5); /* Always a little rvb! */

	/* Bus 8: Our "Master Reverb Bus" */
	audio_master_reverb((float)p->reverb/100.0);

/* These configurations require 128 ksample delay buffer for 32+ kHz! */
#ifdef MEGAREVERB
	audio_bus_control(0, ABC_FX1_TYPE, AFX_NONE);
	audio_bus_controlf(0, ABC_DRY_MASTER, 1.0);
	audio_bus_controlf(0, ABC_DRY_BUS_2, 1.0);

	audio_bus_control(2, ABC_FX1_TYPE, AFX_REVERB);
	audio_bus_controlf(2, ABC_FX1_PARAM_1, 0.8);
	audio_bus_controlf(2, ABC_FX1_PARAM_2, 0.9);
	audio_bus_control(2, ABC_FX2_TYPE, AFX_REVERB);
	audio_bus_controlf(2, ABC_FX2_PARAM_1, 1.1);
	audio_bus_controlf(2, ABC_FX2_PARAM_2, 1.3);
	audio_bus_controlf(2, ABC_DRY_MASTER, 0.0);
	audio_bus_controlf(2, ABC_WET_MASTER, 0.3);
	audio_bus_controlf(2, ABC_WET_BUS_3, 2.0);

	audio_bus_control(3, ABC_FX1_TYPE, AFX_REVERB);
	audio_bus_controlf(3, ABC_FX1_PARAM_1, 1.0);
	audio_bus_controlf(3, ABC_FX1_PARAM_2, 1.0);
	audio_bus_control(3, ABC_FX2_TYPE, AFX_REVERB);
	audio_bus_controlf(3, ABC_FX2_PARAM_1, 1.5);
	audio_bus_controlf(3, ABC_FX2_PARAM_2, 1.7);
	audio_bus_controlf(3, ABC_DRY_MASTER, 0.0);
	audio_bus_controlf(3, ABC_WET_MASTER, 2.0);
#endif
#ifdef MEGAREVERB2
	audio_bus_control(0, ABC_FX1_TYPE, AFX_NONE);
	audio_bus_controlf(0, ABC_DRY_MASTER, 1.0);
	audio_bus_controlf(0, ABC_DRY_BUS_2, 1.0);

	audio_bus_control(2, ABC_FX1_TYPE, AFX_REVERB);
	audio_bus_controlf(2, ABC_FX1_PARAM_1, 0.2);
	audio_bus_controlf(2, ABC_FX1_PARAM_2, 0.3);
	audio_bus_control(2, ABC_FX2_TYPE, AFX_REVERB);
	audio_bus_controlf(2, ABC_FX2_PARAM_1, 0.4);
	audio_bus_controlf(2, ABC_FX2_PARAM_2, 0.5);
	audio_bus_controlf(2, ABC_DRY_MASTER, 0.0);
	audio_bus_controlf(2, ABC_WET_MASTER, 0.3);
	audio_bus_controlf(2, ABC_WET_BUS_3, 2.0);

	audio_bus_control(3, ABC_FX1_TYPE, AFX_REVERB);
	audio_bus_controlf(3, ABC_FX1_PARAM_1, 0.6);
	audio_bus_controlf(3, ABC_FX1_PARAM_2, 0.7);
	audio_bus_control(3, ABC_FX2_TYPE, AFX_REVERB);
	audio_bus_controlf(3, ABC_FX2_PARAM_1, 0.8);
	audio_bus_controlf(3, ABC_FX2_PARAM_2, 0.9);
	audio_bus_controlf(3, ABC_DRY_MASTER, 0.0);
	audio_bus_controlf(3, ABC_WET_MASTER, 2.0);
#endif

	play_init();
}


void close_audio()
{
	audio_close();
	sounds_loaded = 0;
	music_loaded = 0;
}


int init_js(prefs_t *p)
{
	/* Activate Joystick sub-sys if we are using it */
	if(p->use_joystick)
	{
		if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
		{
			fprintf(stderr, "Error setting up joystick!\n");
			return -1;
		}

		if(SDL_NumJoysticks() > 0)
		{
			SDL_JoystickEventState(SDL_ENABLE);
			joystick = SDL_JoystickOpen(0);
			if(!joystick)
			{
				SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
				return -2;
			}
		}
		else
		{
			fprintf(stderr, "No joysticks found!\n");
			joystick = NULL;
			return -3;
		}

	}
	return 0;
}


void close_js()
{
	if(!SDL_WasInit(SDL_INIT_JOYSTICK))
		return;

	if(!joystick)
		return;

	if(SDL_JoystickOpened(0))
		SDL_JoystickClose(joystick);
	joystick = NULL;

	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}


void load_config(prefs_t *p)
{
	FILE *f = fmap.fopen(KOBO_CONFIG_DIR KOBO_CONFIG_FILE, "r");
	if(f)
	{
		p->read(f);
		fclose(f);
	}
}


void save_config(prefs_t *p)
{
	FILE *f = fmap.fopen(KOBO_CONFIG_DIR KOBO_CONFIG_FILE, "w");
	if(f)
	{
		p->write(f);
		fclose(f);
	}
}


void brutal_quit()
{
	if(exit_game_fast)
	{
		fprintf(stderr, "Second try quitting; using brutal method!\n");
		atexit(SDL_Quit);
		exit(1);
	}

	exit_game_fast = 1;
	gengine.stop();
}


void pause_game()
{
	if(gsm.current() != &st_pause_game)
		gsm.press(BTN_PAUSE);
}


/*----------------------------------------------------------
	Various handlers...
----------------------------------------------------------*/

void skobo_gfxengine_t::frame()
{
	if(!gsm.current())
	{
		fprintf(stderr, "INTERNAL ERROR: No gamestate!\n");
		exit_game = 1;
		stop();
		return;
	}
	if(exit_game || manage.aborted())
	{
		stop();
		return;
	}

	/*
	 * Process input
	 */
	SDL_Event ev;
	while(SDL_PollEvent(&ev))
	{
		int k, ms;
		switch (ev.type)
		{
		  case SDL_KEYDOWN:
			switch(ev.key.keysym.sym)
			{
#ifdef PROFILE_AUDIO
			  case SDLK_F10:
				++audio_vismode;
				break;
			  case SDLK_F12:
				audio_print_info();
				break;
#endif
			  case SDLK_DELETE:
				if(prefs.cmd_debug)
				{
					manage.ships = 1;
					myship.hit(1000);
				}
				break;
			  case SDLK_RETURN:
				ms = SDL_GetModState();
				if(ms & (KMOD_CTRL | KMOD_SHIFT | KMOD_META))
					break;
				if(!(ms & KMOD_ALT))
					break;
				pause_game();
				prefs.fullscreen = !prefs.fullscreen;
				prefs.changed = 1;
				global_status |= OS_RELOAD_GRAPHICS |
						OS_RESTART_VIDEO;
				stop();
				return;
			  case SDLK_PRINT:
			  case SDLK_SYSREQ:
//			  case SDLK_p:
				gengine.screenshot();
				break;
			  default:
				break;
			}
			k = gamecontrol.map(ev.key.keysym.sym);
			gamecontrol.press(k);
			gsm.press(k, ev.key.keysym.unicode);
			break;
		  case SDL_KEYUP:
			k = gamecontrol.map(ev.key.keysym.sym);
			if(k == SDLK_PAUSE)
			{
				gamecontrol.press(BTN_PAUSE);
				gsm.press(BTN_PAUSE);
			}
			else
			{
				gamecontrol.release(k);
				gsm.release(k);
			}
			break;
		  case SDL_VIDEOEXPOSE:
			build_screen();
			radar.prepare(-1);
			break;
		  case SDL_QUIT:
			/*gsm.press(BTN_CLOSE);*/
			brutal_quit();
			break;
		  case SDL_JOYBUTTONDOWN:
			if(ev.jbutton.button == js_fire)
			{
				gamecontrol.press(BTN_FIRE);
				gsm.press(BTN_FIRE);
			}
			else if(ev.jbutton.button == js_start)
			{
				gamecontrol.press(BTN_START);
				gsm.press(BTN_START);
			}
			break;
		  case SDL_JOYBUTTONUP:
			if(ev.jbutton.button == js_fire)
			{
				gamecontrol.release(BTN_FIRE);
				gsm.release(BTN_FIRE);
			}
			break;
		  case SDL_JOYAXISMOTION:
			// FIXME: We will want to allow these to be
			// redefined, but for now, this works ;-)
			if(ev.jaxis.axis == js_lr)
			{
				if(ev.jaxis.value < -3200)
				{
					gamecontrol.press(BTN_LEFT);
					gsm.press(BTN_LEFT);
				}
				else if(ev.jaxis.value > 3200)
				{
					gamecontrol.press(BTN_RIGHT);
					gsm.press(BTN_RIGHT);
				}
				else
				{
					gamecontrol.release(BTN_LEFT);
					gamecontrol.release(BTN_RIGHT);
					gsm.release(BTN_LEFT);
					gsm.release(BTN_RIGHT);
				}

			}
			else if(ev.jaxis.axis == js_ud)
			{
				if(ev.jaxis.value < -3200)
				{
					gamecontrol.press(BTN_UP);
					gsm.press(BTN_UP);
				}
				else if(ev.jaxis.value > 3200)
				{
					gamecontrol.press(BTN_DOWN);
					gsm.press(BTN_DOWN);
				}
				else
				{
					gamecontrol.release(BTN_UP);
					gamecontrol.release(BTN_DOWN);
					gsm.release(BTN_UP);
					gsm.release(BTN_DOWN);
				}

			}
		  case SDL_MOUSEMOTION:
			mouse_x = ev.motion.x / intscale;
			mouse_y = ev.motion.y / intscale;
			if(prefs.use_mouse)
				gamecontrol.mouse_position(
						mouse_x - ::xoffs - 8 - MARGIN - WSIZE/2,
						mouse_y - ::yoffs - MARGIN - WSIZE/2);
			break;
		  case SDL_MOUSEBUTTONDOWN:
			mouse_x = ev.button.x / intscale;
			mouse_y = ev.button.y / intscale;
			gsm.press(BTN_FIRE);
			if(prefs.use_mouse)
			{
				gamecontrol.mouse_position(
						mouse_x - ::xoffs - 8 - MARGIN - WSIZE/2,
						mouse_y - ::yoffs - MARGIN - WSIZE/2);
				switch(ev.button.button)
				{
				  case SDL_BUTTON_LEFT:
					mouse_left = 1;
					break;
				  case SDL_BUTTON_MIDDLE:
					mouse_middle = 1;
					break;
				  case SDL_BUTTON_RIGHT:
					mouse_right = 1;
					break;
				}
				gamecontrol.press(BTN_FIRE);
			}
			break;
		  case SDL_MOUSEBUTTONUP:
			mouse_x = ev.button.x / intscale;
			mouse_y = ev.button.y / intscale;
			if(prefs.use_mouse)
			{
				gamecontrol.mouse_position(
						mouse_x - ::xoffs - 8 - MARGIN - WSIZE/2,
						mouse_y - ::yoffs - MARGIN - WSIZE/2);
				switch(ev.button.button)
				{
				  case SDL_BUTTON_LEFT:
					mouse_left = 0;
					break;
				  case SDL_BUTTON_MIDDLE:
					mouse_middle = 0;
					break;
				  case SDL_BUTTON_RIGHT:
					mouse_right = 0;
					break;
				}
			}
			if(!mouse_left && !mouse_middle && !mouse_right)
			{
				if(prefs.use_mouse)
					gamecontrol.release(BTN_FIRE);
				gsm.release(BTN_FIRE);
			}
			break;
		}
	}
	gamecontrol.process();

	/*
	 * Run the current gamestate for one frame
	 */
	gsm.frame();
}


void skobo_gfxengine_t::pre_render()
{
	audio_run();
	gsm.pre_render(&wchip);
}


#ifdef DEBUG
static void draw_osc(int mode)
{
	int mx = oscframes;
	if(mx > wchip.width())
		mx = wchip.width();
	int yo = wchip.height() - 40;
	wchip.foreground(wchip.map_rgb(0x000099));
	wchip.fillrect(0, yo, wchip.width(), 1);
	wchip.foreground(wchip.map_rgb(0x990000));
	wchip.fillrect(0, yo - 32, wchip.width(), 1);
	wchip.fillrect(0, yo + 31, wchip.width(), 1);

	switch(mode)
	{
	  case 0:
		wchip.foreground(wchip.map_rgb(0x009900));
		for(int s = 0; s < mx; ++s)
			wchip.point(s, ((oscbufl[s]+oscbufr[s]) >> 11) + yo);
		break;
	  case 1:
		wchip.foreground(wchip.map_rgb(0x009900));
		for(int s = 0; s < mx; ++s)
			wchip.point(s, (oscbufl[s] >> 10) + yo);
		wchip.foreground(wchip.map_rgb(0xcc0000));
		for(int s = 0; s < mx; ++s)
			wchip.point(s, (oscbufr[s] >> 10) + yo);
		break;
	  case 2:
		wchip.foreground(wchip.map_rgb(0x009900));
		for(int s = 0; s < mx; ++s)
			wchip.point(s>>1, (oscbufl[s] >> 10) + yo);
		wchip.foreground(wchip.map_rgb(0xcc0000));
		for(int s = 0; s < mx; ++s)
			wchip.point((s>>1) + (mx>>1), (oscbufr[s] >> 10) + yo);
		wchip.fillrect(mx/2, yo-34, 1, 68);
		break;
	}

	wchip.foreground(wchip.map_rgb(0xcc0000));
	wchip.fillrect(0, yo - 32, 6, limiter.attenuation >> 11);
}


static void draw_vu(void)
{
	int xo = (wchip.width() - 5 * AUDIO_MAX_VOICES) / 2;
	int yo = wchip.height() - 50;
	wchip.foreground(wchip.map_rgb(0x000000));
	for(int s = 4; s < 40; s += 4)
		wchip.fillrect(xo, yo+s, 5 * AUDIO_MAX_VOICES-1, 1);
	wchip.foreground(wchip.map_rgb(0x333333));
	wchip.fillrect(xo-1, yo, 5 * AUDIO_MAX_VOICES+1, 1);
	wchip.fillrect(xo-1, yo+40, 5 * AUDIO_MAX_VOICES+1, 4);
	for(int s = 0; s <= AUDIO_MAX_VOICES; ++s)
		wchip.fillrect(xo + s*5 - 1, yo+1, 1, 39);
	for(int s = 0; s < AUDIO_MAX_VOICES; ++s)
	{
		int vu, vu2, vumin;
		if(VS_STOPPED != voicetab[s].state)
		{
			wchip.foreground(wchip.map_rgb(0x009900));
			wchip.fillrect(xo + 1, yo + 41, 2, 2);

			vu = labs((voicetab[s].ic[VIC_LVOL].v>>1) +
					(voicetab[s].ic[VIC_RVOL].v>>1)) >>
					RAMP_BITS;
#ifdef AUDIO_USE_VU
			vu2 = voicetab[s].vu;
			voicetab[s].vu = 0;
#else
			vu2 = 0;
#endif
			vu2 = (vu2>>4) * (vu>>4) >> 8;
			vu2 >>= 11;
			if(vu2 > 40)
				vu2 = 40;
			vu >>= 11;
			if(vu > 40)
				vu = 40;
			vumin = vu < vu2 ? vu : vu2;
		}
		else
			vu = vu2 = vumin = 0;
		wchip.foreground(wchip.map_rgb(0x006600));
		wchip.fillrect(xo, yo + 40 - vu, 4, vu - vu2);
		wchip.foreground(wchip.map_rgb(0xffcc00));
		wchip.fillrect(xo, yo + 40 - vu2, 4, vu2);
		xo += 5;
	}
}
#endif


void skobo_gfxengine_t::post_render()
{
	gsm.post_render(&wchip);

	if(!prefs.cmd_noframe)
		wchip.sprite(0, 0, B_FRAME, 0, 0);

#ifdef DEBUG
	if(prefs.cmd_debug)
	{
		char buf[20];
		snprintf(buf, sizeof(buf), "Obj: %d",
				gengine.objects_in_use());
		wchip.font(B_NORMAL_FONT);
		wchip.string(160, 5, buf);
	}

	switch(audio_vismode % 5)
	{
	  case 0:
		break;
	  case 1:
		draw_osc(0);
		break;
	  case 2:
		draw_vu();
		break;
	  case 3:
		draw_osc(1);
		break;
	  case 4:
		draw_osc(2);
		break;
	}
#endif

	int nt = (int)SDL_GetTicks();
	int tt = nt - fps_starttime;
	if((tt > 1000) && fps_count)
	{
		float fps = fps_count * 1000.0 / tt;
		::screen.fps(fps);
		fps_count = 0;
		fps_starttime = nt;
		if(prefs.cmd_fps)
		{
			char buf[20];
			snprintf(buf, sizeof(buf), "%.1f", fps);
			dfps.text(buf);
			if(!fps_results)
				fps_results = (float *)
						calloc(MAX_FPS_RESULTS,
						sizeof(float));
			if(fps_results)
			{
				fps_results[fps_nextresult++] = fps;
				if(fps_nextresult >= MAX_FPS_RESULTS)
					fps_nextresult = 0;
				if(fps_nextresult > fps_lastresult)
					fps_lastresult = fps_nextresult;
			}
		}
	}
	++fps_count;
}


void kobo_render_highlight(ct_widget_t *wg)
{
	static float ypos = -50;
	static int ot = 0;
	int t = (int)SDL_GetTicks();
	int y;
	int x = -((t>>3) % TUBE_LENGTH);
	int dt = t - ot;
	ot = t;
	if(dt > 1000)
		dt = 100;

	//Spring + friction style velocity component
	if(dt < 500)
		ypos += ((float)wg->y() - ypos) * (float)dt * 0.005;
	else
		ypos = (float)wg->y();

	//Constant velocity component
	float v = (float)dt * 0.1;
	if((float)wg->y() - ypos > v)
		ypos += v;
	else if((float)wg->y() - ypos < -v)
		ypos -= v;
	else
		ypos = (float)wg->y();

	y = (int)(ypos + 0.5) - wchip.y();
	while(x < wg->width())
	{
		if(wg->height() < 12)
			wchip.sprite(x, y, B_SMALL_TUBE, 0);
		else
			wchip.sprite(x, y, B_BIG_TUBE, 0);
		x += TUBE_LENGTH;
	}
}


extern "C" void close_all(void)
{
	close_js();
	gengine.close();
	close_audio();
	SDL_Quit();
}


extern "C" RETSIGTYPE breakhandler(int dummy)
{
	/* For platforms that drop the handlers on the first signal... */
	signal(SIGTERM, breakhandler);
	signal(SIGINT, breakhandler);
	brutal_quit();
#if (RETSIGTYPE != void)
	return 0;
#endif
}


int main(int argc, char *argv[])
{
	int cmd_exit = 0;
	atexit(close_all);
	signal(SIGTERM, breakhandler);
	signal(SIGINT, breakhandler);

	DBG(for(int a = 0; a < argc; ++a)
			printf("argv[%d] = \"%s\"\n", a, argv[a]);)

	setup_dirs(argv[0]);
	--argc;
	++argv;

	if((argc < 1) || (strcmp("-override", argv[0]) != 0))
		load_config(&prefs);

	if(prefs.parse(argc, argv) < 0)
	{
		put_usage();
		return 1;
	}
#if 0
	int k = -1;
	while((k=prefs.find_next(k)) >= 0)
	{
		printf("key %d: \"%s\"\ttype=%d\t\"%s\"\n",
				k,
				prefs.name(k),
				prefs.type(k),
				prefs.description(k)
			);
	}
#endif
	add_dirs(&prefs);

	if(prefs.cmd_hiscores)
	{
		scorefile.gather_high_scores();
		scorefile.print_high_scores();
		cmd_exit = 1;
	}
	if(prefs.cmd_showcfg)
	{
		printf("Configuration:\n");
		printf("----------------------------------------\n");
		prefs.write(stdout);
		printf("\nPaths:\n");
		printf("----------------------------------------\n");
		fmap.print(stdout, "*");
		printf("----------------------------------------\n");
		cmd_exit = 1;
	}
	if(cmd_exit)
		return 0;

	if(prefs.cmd_noparachute)
		SDL_Init(SDL_INIT_NOPARACHUTE);
	else
		SDL_Init(0);

	if(init_display(&prefs) < 0)
		return 2;

	//The idea is that it should be possible to do this before
	//the engine is initialized, so let's try it... :-)
	if(load_sounds(&prefs) < 0)
		return 4;

	init_audio(&prefs);

	if(load_graphics(&prefs) < 0)
		return 3;

	ct_engine.render_highlight = kobo_render_highlight;
	build_screen();
	radar.prepare(0);
	pubrand.init();
	init_js(&prefs);
	manage.init();

	gsm.push(&st_intro_1);

	while(1)
	{
		gengine.run();
		if(exit_game_fast)
			break;

		if(exit_game)
		{
			nibble();
			break;
		}

		if(global_status & OS_RESTART_AUDIO)
		{
			if(prefs.use_sound)
			{
				DBG(printf("--- Restarting audio...\n"));
				audio_stop();
				if(load_sounds(&prefs) < 0)
					return 5;
				init_audio(&prefs);
				DBG(printf("--- Audio restarted.\n"));
				build_screen();
				radar.prepare(-1);
			}
			else
			{
				DBG(printf("--- Stopping audio...\n"));
				close_audio();
				DBG(printf("--- Audio stopped.\n"));
			}
		}
		if(global_status & OS_RESTART_VIDEO)
		{
			DBG(printf("--- Restarting video...\n"));
			nibble();
			gengine.hide();
			if(init_display(&prefs) < 0)
				return 6;
			gengine.show();
			gamecontrol.init();
			DBG(printf("--- Video restarted.\n"));
		}
		if(global_status & (OS_RELOAD_GRAPHICS |
					OS_RESTART_VIDEO))
		{
			DBG(printf("--- Reloading graphics...\n"));
			if(!(global_status & OS_RESTART_VIDEO))
				nibble();
			gengine.unload();
			if(load_graphics(&prefs) < 0)
				return 7;
			build_screen();
			radar.prepare(-1);
			DBG(printf("--- Graphics reloaded.\n"));
		}
		if(global_status & OS_RESTART_ENGINE)
		{
			DBG(printf("--- Restarting engine...\n"));
			gengine.close();
			gengine.scroll_ratio(LAYER_OVERLAY, 0.0, 0.0);
			gengine.scroll_ratio(LAYER_PLAYER, 1.0, 1.0);
			gengine.scroll_ratio(LAYER_ENEMIES, 1.0, 1.0);
			gengine.scroll_ratio(LAYER_BASES, 1.0, 1.0);
			gengine.scroll_ratio(LAYER_GROUND, 1.0, 1.0);
			gengine.scroll_ratio(LAYER_STARS, 0.5, 0.5);
			gengine.wrap(MAP_SIZEX * CHIP_SIZEX,
					MAP_SIZEY * CHIP_SIZEY);
			if(gengine.open(2048) < 0)
				return 7;
			gamecontrol.init();
			DBG(printf("--- Engine restarted.\n"));
		}
		if(global_status & OS_RESTART_INPUT)
		{
			close_js();
			init_js(&prefs);
		}

		global_status = 0;
		manage.reenter();
	}

	close_all();

	/* 
	 * Seems like we got all the way here without crashing,
	 * so let's save the current configuration! :-)
	 */
	if(prefs.changed)
	{
		save_config(&prefs);
		prefs.changed = 0;
	}

	if(prefs.cmd_fps)
		_print_fps_results();

	return 0;
}

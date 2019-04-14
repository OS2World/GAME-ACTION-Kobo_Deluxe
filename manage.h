/*(GPL)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 1995, 1996, Akira Higuchi
 * Copyright (C) 2001, 2002, David Olofson
 * Copyright (C) 2002, Jeremy Sheeley
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

#ifndef _KOBO_MANAGE_H_
#define _KOBO_MANAGE_H_

#include "glSDL.h"
#include "score.h"

class _manage
{
	static int blank;
	static int next_state_out;
	static int next_state_next;
	static int game_seed;
	static int bonus_next;
	static int scroll_jump;
	static int delay_count;
	static int introtime;
	static int rest_cores;
	static int exit_manage;
	static int playing;
	static int _get_ready;
	static int _game_over;
	static s_hiscore_t hi;

	static void next_scene();
	static void retry();

	static int scene_num;
	static int score;
	static int health;
	static float disp_health;
	static int flush_score_count;
	static int flush_ships_count;
	static int score_changed;
	static int ships_changed;
	static void put_health(int force = 0);
	static void put_info();
	static void put_score();
	static void put_ships();
	static void flush_score();
	static void flush_ships();
	static void game_stop();
  public:
	static int ships;
	static int count;
	static void init();
	static void init_resources_title();
	static void init_resources_to_play();
	static void update();
	static void run_demo();
	static void run_pause();
	static void run_game();
	static void lost_myship();
	static void destroyed_a_core();
	static void add_score(int sc);
	static void set_health(int h)	{ health = h;	}
	static void key_down(SDLKey sym);
	static void key_up(SDLKey sym);
	static int title_blank()	{ return blank; }
	static void select_next(int redraw_map = 1);
	static void select_prev(int redraw_map = 1);
	static void regenerate();
	static void select_scene(int scene, int redraw_map = 1);
	static int scene()		{ return scene_num; }
	static void pause_music(int p);
	static void abort();
	static int aborted()		{ return exit_manage; }
	static void reenter();
	static int game_stopped()	{ return !playing; }
	static int get_ready();
	static void game_start();
	static int game_over();
};

extern _manage manage;

#endif	//_KOBO_MANAGE_H_

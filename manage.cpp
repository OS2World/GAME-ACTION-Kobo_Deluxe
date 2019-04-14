/*
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

#include "config.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
}
#include "kobo.h"
#include "screen.h"
#include "manage.h"
#include "options.h"
#include "scenes.h"
#include "score.h"
#include "enemies.h"
#include "myship.h"
#include "radar.h"
#include "gamectl.h"
#include "states.h"
#include "audio.h"
#include "random.h"

#define GIGA             1000000000

int _manage::blank = 0;
int _manage::next_state_out;
int _manage::next_state_next;
int _manage::game_seed;
int _manage::scene_num;
int _manage::ships;
int _manage::count;
int _manage::score;
int _manage::health;
float _manage::disp_health;
int _manage::score_changed = 1;
int _manage::ships_changed = 1;
int _manage::bonus_next;
int _manage::flush_score_count = 0;
int _manage::flush_ships_count = 0;
int _manage::scroll_jump = 0;
int _manage::delay_count;
int _manage::rest_cores;
int _manage::introtime = 0;
int _manage::exit_manage = 0;
int _manage::playing = 0;
int _manage::_get_ready = 0;
int _manage::_game_over = 0;
s_hiscore_t _manage::hi;

static int sound_fine_playing = 0;


void _manage::game_start()
{
	_game_over = 0;

	hi.clear();

	ships = game.lives;
	disp_health = 0;
	health = game.health;
	score = 0;
	flush_score_count = 0;
	bonus_next = BONUS_FIRSTTIME;
	screen.init_scene(scene_num);
	init_resources_to_play();

	gamecontrol.clear();
	gamecontrol.repeat(0, 0);

	playing = 1;
	_get_ready = 1;

	hi.skill = scorefile.profile()->skill;
	hi.playtime = 0;
	hi.gametype = game.type;
	hi.saves = 0;
	hi.loads = 0;
	hi.start_scene = scene_num;
	hi.end_lives = ships;
	if(prefs.use_music)
		music_play(0, SOUND_BGM);
}


void _manage::game_stop()
{
	if(!prefs.cmd_cheat)
	{
		hi.score = score;
		hi.end_scene = scene_num;
		hi.end_health = health;

		scorefile.record(&hi);
	}
	ships = 0;
	ships_changed = 1;
	audio_channel_stop(0, -1);
	playing = 0;
}


void _manage::pause_music(int p)
{
	if(p)
		audio_channel_stop(0, -1);
	else if(prefs.use_music)
		music_play(0, SOUND_BGM);
}


void _manage::next_scene()
{
	scene_num++;
	if(scene_num >= GIGA - 1)	//Yeah, right!!! :-D
		scene_num = GIGA - 2;
	screen.init_scene(scene_num);
	scroll_jump = 1;
	_get_ready = 1;
}


void _manage::retry()
{
	if(!prefs.cmd_cheat)
	{
		ships--;
		ships_changed = 1;
	}
	if(ships <= 0)
	{
		if(!_game_over)
		{
			game_stop();
			_game_over = 1;
		}
	}
	else
		_get_ready = 1;
}


void _manage::init_resources_title()
{
	screen.init_scene(INTRO_SCENE);
	put_info();
	put_score();
	put_ships();
	run_demo();
	gengine.force_scroll();
	gamecontrol.repeat(KOBO_KEY_DELAY, KOBO_KEY_REPEAT);
}


void _manage::init_resources_to_play()
{
	count = 0;
	delay_count = 0;
	flush_score_count = (flush_score_count) ? -1 : 0;
	flush_ships_count = 0;
	score_changed = 0;
	next_state_out = 0;
	next_state_next = 0;

	gamerand.init();
	game_seed = gamerand.get_seed();
	enemies.init();
	myship.init();
	rest_cores = screen.prepare();
	scroll_jump = 1;
	screen.generate_fixed_enemies();
	put_info();
	put_score();
	put_ships();
	myship.put();
	gengine.scroll(PIXEL2CS(myship.get_virtx()),
			PIXEL2CS(myship.get_virty()));
	gengine.force_scroll();
}


void _manage::put_health(int force)
{
	if(!force)
	{
		if(health > disp_health)
		{
			disp_health += (float)game.health * .03;
			if(disp_health > health)
				disp_health = health;
		}
		else if(health < disp_health)
		{
			disp_health -= (float)game.health * .03;
			if(disp_health < health)
				disp_health = health;
		}
		else
			return;
	}

	int y = (int)(whealth.height() *
			(game.health - disp_health) / game.health);
	int red = 255 - (int)(disp_health * 255.0 / game.health);
	int green = (int)(disp_health * 300.0 / game.health);
	if(green > 180)
		green = 180;

	whealth.foreground(whealth.map_rgb(0x34434e));
	whealth.fillrect(0, 0, whealth.width(), y);
	whealth.foreground(whealth.map_rgb(red, green, 50));
	whealth.fillrect(0, y, whealth.width(), whealth.height());
	whealth.invalidate();
}


void _manage::put_info()
{
	static char s[16];

	snprintf(s, 16, "%09d", scorefile.highscore());
	dhigh.text(s);
	dhigh.on();

	snprintf(s, 16, "%03d", scene_num + 1);
	dstage.text(s);
	dstage.on();

	score_changed = 1;
	ships_changed = 1;
}


void _manage::put_score()
{
	if(score_changed)
	{
		static char s[32];
		snprintf(s, 16, "%09d", score);
		dscore.text(s);
		dscore.on();
		if(score > scorefile.highscore())
		{
			dhigh.text(s);
			dhigh.on();
		}
		score_changed = 0;
	}
	if(flush_score_count > 0)
		flush_score();
}


void _manage::put_ships()
{
	if(ships_changed)
	{
		static char s[32];
		if(!prefs.cmd_cheat)
			snprintf(s, 16, "%03d", ships);
		else
			snprintf(s, 16, "999");
		dships.text(s);
		dships.on();
		ships_changed = 0;
	}
	if(flush_ships_count > 0)
		flush_ships();
}


void _manage::flush_score()
{
	flush_score_count--;
	if(flush_score_count & 1)
		return;

	if(flush_score_count & 2)
		dscore.off();
	else
		dscore.on();
	if(flush_score_count == 0)
		flush_score_count = -1;
}


void _manage::flush_ships()
{
	flush_ships_count--;
	if(flush_ships_count & 1)
		return;

	if(flush_ships_count & 2)
		dships.off();
	else
		dships.on();
	if(flush_ships_count == 0)
		flush_ships_count = -1;
}


/****************************************************************************/
void _manage::init()
{
	scorefile.init();
	count = 0;
	ships = 0;
	exit_manage = 0;
	scene_num = -1;
	flush_ships_count = 0;
	flush_score_count = 0;
	delay_count = 0;
	screen.init();
	init_resources_title();
}


void _manage::run_demo()
{
	static int demo_x = CHIP_SIZEX * (64-18);
	static int demo_y = CHIP_SIZEY * (64-7);

	gengine.scroll(PIXEL2CS(demo_x), PIXEL2CS(demo_y));
	demo_y -= 3;
	demo_x &= MAP_SIZEX*CHIP_SIZEX-1;
	demo_y &= MAP_SIZEY*CHIP_SIZEY-1;

	put_health();
	put_score();
	put_ships();
}


void _manage::update()
{
	myship.put();
	enemies.put();
	put_score();
	put_ships();
	gengine.scroll(PIXEL2CS(myship.get_virtx()),
			PIXEL2CS(myship.get_virty()));
	if(scroll_jump)
	{
		gengine.force_scroll();
		scroll_jump = 0;
	}
}


void _manage::run_pause()
{
	put_health();
	update();
}


void _manage::run_game()
{
	put_health();

	if(delay_count)
		delay_count--;

	if(delay_count == 1)
	{
		if(enemies.exist_pipe())
			delay_count = 2;
		else
		{
			put_info();
			if(next_state_out)
			{
				retry();
				if(ships <= 0)
					return;
			}
			if(next_state_next)
				next_scene();
			init_resources_to_play();
			return;
		}
	}

	myship.move();
	enemies.move();
	myship.hit_structure();
	update();
	++hi.playtime;

	if(sound_fine_playing)
		if(!audio_channel_playing(1))
		{
			sound_fine_playing = 0;
			audio_channel_control(0, -1, ACC_VOLUME, 65536);
		}
}


void _manage::lost_myship()
{
	if(delay_count == 0)
		delay_count = 20;
	next_state_out = 1;
}


void _manage::destroyed_a_core()
{
	audio_channel_control(0, -1, ACC_VOLUME, 32768);
	music_play(1, SOUND_FINE);
	sound_fine_playing = 1;

	rest_cores--;
	if(rest_cores == 0)
	{
		next_state_next = 1;
		delay_count = 50;
	}
	screen.generate_fixed_enemies();
}


void _manage::add_score(int sc)
{
	score += sc;
	if(score >= GIGA)	//This *could* happen... Or maybe not. :-)
		score = GIGA - 1;
	else if(!prefs.cmd_cheat)
	{
		if(score >= bonus_next)
		{
			bonus_next += BONUS_EVERY;
			ships++;
			ships_changed = 1;
			flush_ships_count = 50;

			audio_channel_control(0, -1, ACC_VOLUME, 32768);
			music_play(1, SOUND_ONEUP);
			sound_fine_playing = 1;
		}
		if(score >= scorefile.highscore())
		{
			if(flush_score_count == 0)
				flush_score_count = 50;
		}
	}
	score_changed = 1;
}


void _manage::select_next(int redraw_map)
{
	if((scene_num < scorefile.last_scene()) || prefs.cmd_cheat)
	{
		sound_play0(SOUND_METALLIC);
		scene_num++;
	}
	else
		sound_play0(SOUND_SHOT);
	select_scene(scene_num);
}


void _manage::select_prev(int redraw_map)
{
	scene_num--;
	if(scene_num < 0)
	{
		sound_play0(SOUND_SHOT);
		scene_num = 0;
	}
	else
		sound_play0(SOUND_METALLIC);
	select_scene(scene_num);
}


void _manage::regenerate()
{
	sound_play0(SOUND_SHOT);
	select_scene(scene_num, 1);
}


void _manage::select_scene(int scene, int redraw_map)
{
        scene_num = scene;
	put_info();
	if(redraw_map)
		screen.init_scene(-scene_num - 1);
}


void _manage::abort()
{
	if(!exit_manage)
	{
		game_stop();
		exit_manage = 1;
	}
}


void _manage::reenter()
{
	exit_manage = 0;
	put_health(1);
	put_info();
	put_score();
	put_ships();
}


int _manage::get_ready()
{
	if(_get_ready)
	{
		_get_ready = 0;
		return 1;
	}
	return 0;
}


int _manage::game_over()
{
	return _game_over;
}

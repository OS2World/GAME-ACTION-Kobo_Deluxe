/*
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
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

#include "glSDL.h"
#include <math.h>

#include "config.h"
#include "states.h"
#include "kobo.h"
#include "screen.h"
#include "manage.h"
#include "options.h"
#include "audio.h"
#include "radar.h"


/* Intro loop timing */
#define	INTRO_BLANK_TIME	1000
#define	INTRO_TITLE_TIME	4000
#define	INTRO_HIGHSCORE_TIME	13000
#define	INTRO_TITLE2_TIME	4000
#define	INTRO_CREDITS_TIME	12700


gamestatemanager_t gsm;

int run_demo = 0;


kobo_basestate_t::kobo_basestate_t()
{
	name = "<unnamed>";
}


void kobo_basestate_t::frame()
{
}


void kobo_basestate_t::pre_render(window_t *win)
{
	screen.render_background(win);
}


void kobo_basestate_t::post_render(window_t *win)
{
	DBG(if(prefs.cmd_debug)
	{
		win->font(B_NORMAL_FONT);
		win->string(30, 5, name);
	})
}


/*----------------------------------------------------------
	st_introbase
----------------------------------------------------------*/

int	st_introbase_t::song, st_introbase_t::playcnt;

st_introbase_t::st_introbase_t()
{
	name = "intro";
	inext = NULL;
	duration = 0;
	timer = 0;
	song = SOUND_DEMOSONG;
	playcnt = 100;
}


void st_introbase_t::enter()
{
	if(!run_demo)
	{
		manage.init_resources_title();
		if(prefs.use_music)
		{
			song = SOUND_DEMOSONG;
// KLUDGE UNTIL MIDI SONGS AND FX CONTROL IS FIXED!
			audio_bus_control(0, 1, ABC_FX_TYPE, AFX_NONE);
			audio_bus_controlf(0, 0, ABC_SEND_MASTER, 1.0);
			audio_bus_controlf(0, 0, ABC_SEND_BUS_7, 0.5);
// KLUDGE UNTIL MIDI SONGS AND FX CONTROL IS FIXED!
			music_play(0, song);
		}
		playcnt = 100;
		run_demo = 1;
	}
	start_time = (int)SDL_GetTicks();
	timer = 0;
}


void st_introbase_t::reenter()
{
	if(!run_demo)
	{
		manage.init_resources_title();
		if(prefs.use_music)
		{
			song = SOUND_DEMOSONG;
// KLUDGE UNTIL MIDI SONGS AND FX CONTROL IS FIXED!
			audio_bus_control(0, 1, ABC_FX_TYPE, AFX_NONE);
			audio_bus_controlf(0, 0, ABC_SEND_MASTER, 1.0);
			audio_bus_controlf(0, 0, ABC_SEND_BUS_7, 0.5);
// KLUDGE UNTIL MIDI SONGS AND FX CONTROL IS FIXED!
			music_play(0, song);
		}
		playcnt = 100;
		run_demo = 1;
	}
	gsm.change(&st_intro_1);
}


void st_introbase_t::press(int button)
{
	switch (button)
	{
	  case BTN_EXIT:
	  case BTN_CLOSE:
		gsm.push(&st_ask_exit);
		break;
	  case BTN_START:
	  case BTN_FIRE:
	  case BTN_SELECT:
		run_demo = 0;
	        if (scorefile.numProfiles > 0)
			gsm.push(&st_main_menu);
		else
			gsm.push(&st_new_player);
		break;
	  case BTN_BACK:
	  case BTN_UP:
	  case BTN_LEFT:
		gsm.change(&st_intro_1);
		break;
	  case BTN_DOWN:
	  case BTN_RIGHT:
		if(inext)
			gsm.change(inext);
		break;
#ifdef PROFILE_AUDIO
	  case BTN_F9:
		run_demo = 0;
		gsm.push(&st_profile_audio);
		break;
#endif
	  default:
		break;
	}
}


void st_introbase_t::frame()
{
	manage.run_demo();

	if(!prefs.use_music)
		return;

	if(playcnt)
	{
		--playcnt;
		return;
	}
	if(!audio_channel_playing(0))
	{
		++song;
		if(song > SOUND_LAST_DEMOSONG)
			song = SOUND_DEMOSONG;
		music_play(0, song);
		playcnt = 100;
	}
}


void st_introbase_t::pre_render(window_t *win)
{
	kobo_basestate_t::pre_render(win);
	timer = (int)SDL_GetTicks() - start_time;
}


void st_introbase_t::post_render(window_t *win)
{
	kobo_basestate_t::post_render(win);
	screen.scroller();
//	screen.usage();

	//This should be in frame(), but that makes
	//change() freak out sometimes, it seems...
	if((timer > duration) && inext)
		gsm.change(inext);
}




/*----------------------------------------------------------
	st_intro_1
----------------------------------------------------------*/

st_intro_1_t::st_intro_1_t()
{
	name = "intro_1 (title)";
}

void st_intro_1_t::enter()
{
	st_introbase_t::enter();
	if(!duration)
		duration = INTRO_TITLE_TIME;
	if(!inext)
		inext = &st_intro_2;
}

void st_intro_1_t::post_render(window_t *win)
{
	if(exit_game)
		return;
	st_introbase_t::post_render(win);
	if(timer < duration - INTRO_BLANK_TIME)
		screen.title();
}

st_intro_1_t st_intro_1;




/*----------------------------------------------------------
	st_intro_2
----------------------------------------------------------*/

st_intro_2_t::st_intro_2_t()
{
	name = "intro_2 (highscores)";
}

void st_intro_2_t::enter()
{
	scorefile.gather_high_scores(1);
	screen.init_highscores();
	st_introbase_t::enter();
	duration = INTRO_HIGHSCORE_TIME;
	inext = &st_intro_1;
	st_intro_1.inext = &st_intro_3;
	st_intro_1.duration = INTRO_TITLE2_TIME;
}

void st_intro_2_t::post_render(window_t *win)
{
	if(exit_game)
		return;
	st_introbase_t::post_render(win);
	if(timer < duration - INTRO_BLANK_TIME)
		screen.highscores(timer);
}

st_intro_2_t st_intro_2;




/*----------------------------------------------------------
	st_intro_3
----------------------------------------------------------*/

st_intro_3_t::st_intro_3_t()
{
	name = "intro_3 (credits)";
}

void st_intro_3_t::enter()
{
	st_introbase_t::enter();
	duration = INTRO_CREDITS_TIME;
	inext = &st_intro_1;
	st_intro_1.inext = &st_intro_2;
	st_intro_1.duration = INTRO_TITLE_TIME;
}

void st_intro_3_t::post_render(window_t *win)
{
	if(exit_game)
		return;
	st_introbase_t::post_render(win);
	if(timer < duration - INTRO_BLANK_TIME)
		screen.credits(timer);
}

st_intro_3_t st_intro_3;




/*----------------------------------------------------------
	st_ask_exit
----------------------------------------------------------*/

st_ask_exit_t::st_ask_exit_t()
{
	name = "ask_exit";
}


void st_ask_exit_t::enter()
{
	sound_play0(SOUND_EXPL);
}


void st_ask_exit_t::press(int button)
{
	switch (button)
	{
	  case BTN_EXIT:
	  case BTN_NO:
		sound_play0(SOUND_SHOT);
		pop();
		break;
	  case BTN_CLOSE:
	  case BTN_YES:
		audio_channel_stop(0, -1);	//Stop any music
		sound_play0(SOUND_EXPL);
		exit_game = 1;
		pop();
		break;
	}
}


void st_ask_exit_t::frame()
{
	if(run_demo)
		manage.run_demo();
}


void st_ask_exit_t::post_render(window_t *win)
{
	kobo_basestate_t::post_render(win);

	float ft = SDL_GetTicks() * 0.001;
	win->font(B_BIG_FONT);
	int y = PIXEL2CS(100) + (int)floor(PIXEL2CS(15)*sin(ft * 6));
	win->center_fxp(y, "Quit Kobo Deluxe?\n");
}

st_ask_exit_t st_ask_exit;




/*----------------------------------------------------------
	st_game
----------------------------------------------------------*/

st_game_t::st_game_t()
{
	name = "game";
}


void st_game_t::enter()
{
	audio_channel_stop(0, -1);	//Stop any music
	run_demo = 0;
	manage.game_start();
}


void st_game_t::leave()
{
}


void st_game_t::yield()
{
	manage.pause_music(1);
}


void st_game_t::reenter()
{
	manage.pause_music(0);
}


void st_game_t::press(int button)
{
	switch (button)
	{
	  case BTN_EXIT:
		gsm.push(&st_ask_abort_game);
		break;
	  case BTN_CLOSE:
		gsm.push(&st_ask_exit);
		break;
	  case BTN_SELECT:
	  case BTN_START:
	  case BTN_PAUSE:
		gsm.push(&st_pause_game);
		break;
	}
}


void st_game_t::frame()
{
	if(manage.get_ready())
	{
		gsm.push(&st_get_ready);
		return;
	}
	manage.run_game();
	if(manage.game_over())
		gsm.change(&st_game_over);
	else if(exit_game || manage.game_stopped())
		pop();
}

void st_game_t::post_render(window_t *win)
{
	kobo_basestate_t::post_render(win);
	radar.trace_myship();
}

st_game_t st_game;



/*----------------------------------------------------------
	st_ask_abort_game
----------------------------------------------------------*/

st_ask_abort_game_t::st_ask_abort_game_t()
{
	name = "ask_abort_game";
}


void st_ask_abort_game_t::enter()
{
	sound_play0(SOUND_EXPL);
}


void st_ask_abort_game_t::press(int button)
{
	switch (button)
	{
	  case BTN_EXIT:
	  case BTN_NO:
		gsm.change(&st_get_ready);
		break;
	  case BTN_CLOSE:
	  case BTN_YES:
		sound_play0(SOUND_EXPL);
		manage.abort();
		pop();
		break;
	}
}


void st_ask_abort_game_t::frame()
{
	manage.run_pause();
}


void st_ask_abort_game_t::post_render(window_t *win)
{
	kobo_basestate_t::post_render(win);

	float ft = SDL_GetTicks() * 0.001;
	win->font(B_BIG_FONT);
	int y = PIXEL2CS(80) + (int)floor(PIXEL2CS(15)*sin(ft * 6));
	win->center_fxp(y, "Abort Game?\n");

	radar.trace_myship();
}

st_ask_abort_game_t st_ask_abort_game;



/*----------------------------------------------------------
	st_pause_game
----------------------------------------------------------*/

st_pause_game_t::st_pause_game_t()
{
	name = "pause_game";
}


void st_pause_game_t::enter()
{
	sound_play0(SOUND_EXPL);
}


void st_pause_game_t::press(int button)
{
	switch (button)
	{
	  case BTN_EXIT:
		gsm.change(&st_ask_abort_game);
		break;
	  case BTN_SELECT:
	  case BTN_START:
	  case BTN_YES:
	  case BTN_NO:
	  case BTN_PAUSE:
		gsm.change(&st_get_ready);
		break;
	}
}


void st_pause_game_t::frame()
{
	manage.run_pause();
}


void st_pause_game_t::post_render(window_t *win)
{
	kobo_basestate_t::post_render(win);

	float ft = SDL_GetTicks() * 0.001;
	win->font(B_BIG_FONT);
	int y = PIXEL2CS(75) + (int)floor(PIXEL2CS(15)*sin(ft * 6));
	win->center_fxp(y, "PAUSED");

	radar.trace_myship();
}

st_pause_game_t st_pause_game;



/*----------------------------------------------------------
	st_get_ready
----------------------------------------------------------*/

st_get_ready_t::st_get_ready_t()
{
	name = "get_ready";
}


void st_get_ready_t::enter()
{
	manage.update();
	sound_play0(SOUND_EXPL2);
	start_time = (int)SDL_GetTicks();
	frame_time = 0;
	countdown = 9;
}


#if 0
//Note: Now, this code won't be called, as all "pause style"
//	states now *change()* to "get ready" rather than
//	right into the game. 
void st_get_ready_t::reenter()
{
	//Countdown should pause while in some sub state.
	start_time = (int)SDL_GetTicks() - frame_time;
	frame_time = 0;
}
#endif


void st_get_ready_t::press(int button)
{
	if(frame_time < 500)
		return;

	switch (button)
	{
	  case BTN_EXIT:
	  case BTN_NO:
		gsm.change(&st_ask_abort_game);
		break;
	  case BTN_LEFT:
	  case BTN_RIGHT:
	  case BTN_UP:
	  case BTN_DOWN:
	  case BTN_UL:
	  case BTN_UR:
	  case BTN_DL:
	  case BTN_DR:
	  case BTN_FIRE:
	  case BTN_YES:
		sound_play0(SOUND_EXPL);
		pop();
		break;
	  case BTN_SELECT:
	  case BTN_START:
	  case BTN_PAUSE:
		gsm.change(&st_pause_game);
		break;
	}
}


void st_get_ready_t::frame()
{
	manage.run_pause();

	if(exit_game || manage.game_stopped())
	{
		pop();
		return;
	}

	frame_time = (int)SDL_GetTicks() - start_time;

	int prevcount = countdown;
	countdown = 9 - frame_time/1000;
	if(prevcount != countdown)
		sound_play0(SOUND_METALLIC);

	if(countdown < 1)
		gsm.change(&st_pause_game);
}


void st_get_ready_t::post_render(window_t *win)
{
	kobo_basestate_t::post_render(win);

	float ft = SDL_GetTicks() * 0.001;
	char counter[2] = "0";
	win->font(B_BIG_FONT);
	int y = PIXEL2CS(75) + (int)floor(PIXEL2CS(15)*sin(ft * 6));
	win->center_fxp(y, "GET READY!");
	counter[0] = countdown + '0';
	win->font(B_COUNTER_FONT);
	win->center_fxp(y + PIXEL2CS(65), counter);

	radar.trace_myship();
}

st_get_ready_t st_get_ready;


/*----------------------------------------------------------
	st_game_over
----------------------------------------------------------*/

st_game_over_t::st_game_over_t()
{
	name = "game_over";
}


void st_game_over_t::enter()
{
	sound_play0(SOUND_EXPL);
	manage.update();
	start_time = (int)SDL_GetTicks();
}


void st_game_over_t::press(int button)
{
	if(frame_time < 500)
		return;

	switch (button)
	{
	  case BTN_EXIT:
	  case BTN_NO:
	  case BTN_LEFT:
	  case BTN_RIGHT:
	  case BTN_UP:
	  case BTN_DOWN:
	  case BTN_UL:
	  case BTN_UR:
	  case BTN_DL:
	  case BTN_DR:
	  case BTN_FIRE:
	  case BTN_START:
	  case BTN_SELECT:
	  case BTN_YES:
		sound_play0(SOUND_EXPL);
		pop();
		break;
	}
}


void st_game_over_t::frame()
{
	manage.run_pause();

	frame_time = (int)SDL_GetTicks() - start_time;
	if(frame_time > 5000)
		pop();
}


void st_game_over_t::post_render(window_t *win)
{
	kobo_basestate_t::post_render(win);

	float ft = SDL_GetTicks() * 0.001;
	win->font(B_BIG_FONT);
	int y = PIXEL2CS(100) + (int)floor(PIXEL2CS(15)*sin(ft * 6));
	win->center_fxp(y, "GAME OVER");

	radar.trace_myship();
}

st_game_over_t st_game_over;



/*----------------------------------------------------------
	Menu Base
----------------------------------------------------------*/

/*
 * menu_base_t
 */
void menu_base_t::open()
{
	init(&gengine);
	place(wchip.x(), wchip.y(), wchip.width(), wchip.height());
	font(B_NORMAL_FONT);
	foreground(wbase.map_rgb(0xffffff));
	background(wbase.map_rgb(0x000000));
	build_all();
}

void menu_base_t::close()
{
	clean();
}


/*
 * st_menu_base_t
 */

st_menu_base_t::st_menu_base_t()
{
	name = "(menu_base derivate)";
	sounds = 1;
	form = NULL;
}


st_menu_base_t::~st_menu_base_t()
{
	delete form;
}


void st_menu_base_t::enter()
{
	form = open();
	run_demo = 1;
	if(sounds)
		sound_play0(SOUND_EXPL);
}

// Because we may get back here after changing the configuration!
void st_menu_base_t::reenter()
{
	if(global_status & OS_RESTART_VIDEO)
		pop();
}

void st_menu_base_t::leave()
{
	close();
	form = NULL;
}

void st_menu_base_t::frame()
{
	manage.run_demo();
}

void st_menu_base_t::post_render(window_t *win)
{
	kobo_basestate_t::post_render(win);
	if(form)
		form->render();
}

int st_menu_base_t::translate(int tag, int button)
{
	switch(button)
	{
	  case BTN_INC:
	  case BTN_RIGHT:
	  case BTN_DEC:
	  case BTN_LEFT:
		return -1;
	  default:
		return tag;
	}
}

void st_menu_base_t::press(int button)
{
	int selection;
	if(!form)
		return;

	do_default_action = 1;

	// Translate
	switch(button)
	{
	  case BTN_EXIT:
	  case BTN_CLOSE:
		selection = 0;
		break;
	  case BTN_UP:
	  case BTN_DOWN:
		selection = -1;
		break;
	  case BTN_INC:
	  case BTN_RIGHT:
	  case BTN_DEC:
	  case BTN_LEFT:
	  case BTN_FIRE:
	  case BTN_START:
	  case BTN_SELECT:
		if(form->selected())
			selection = translate(form->selected()->tag,
					button);
		else
			selection = -2;
		break;
	  default:
		selection = -1;
		break;
	}

	// Default action
	if(do_default_action)
		switch(button)
		{
		  case BTN_EXIT:
			escape();
			break;
		  case BTN_INC:
		  case BTN_RIGHT:
			form->change(1);
			break;
		  case BTN_DEC:
		  case BTN_LEFT:
			form->change(-1);
			break;
		  case BTN_FIRE:
		  case BTN_START:
		  case BTN_SELECT:
			form->change(0);
			break;
		  case BTN_UP:
			form->prev();
			break;
		  case BTN_DOWN:
			form->next();
			break;
#ifdef PROFILE_AUDIO
		  case BTN_F9:
			gsm.push(&st_profile_audio);
			break;
#endif
		}

	switch(selection)
	{
	  case -1:
		break;
	  case 0:
		if(sounds)
			sound_play0(SOUND_SHOT);
		select(0);
		pop();
		break;
	  default:
		select(selection);
		break;
	}
}



/*----------------------------------------------------------
	st_new_player
----------------------------------------------------------*/

void new_player_t::open()
{
	init(&gengine);
	place(wchip.x(), wchip.y(), wchip.width(), wchip.height());
	font(B_NORMAL_FONT);
	foreground(wbase.map_rgb(65535, 65535, 65535));
	background(wbase.map_rgb(0, 0, 0));
	memset(name, 0, sizeof(name));
	if(strlen(name) == 0)
		name[0] = 'A';
	currentIndex = strlen(name) - 1;
	editing = 1;
	build_all();
	SDL_EnableUNICODE(1);
}

void new_player_t::close()
{
	SDL_EnableUNICODE(0);
	clean();
}

void new_player_t::change(int delta)
{
	kobo_form_t::change(delta);

	if(!selected())
		return;

	selection = selected()->tag;
}

void new_player_t::build()
{
	small();
	space(6);
		label("Use arrows, joystick or keyboard");
		label("to enter name");
	big();
		button(name, 1);
	space();
		button("Ok", 99);
		button("Cancel", 100);
}

void new_player_t::rebuild()
{
	int sel = selected_index();
	build_all();
	select(sel);
}


st_new_player_t::st_new_player_t()
{
	name = "new_player";
}

void st_new_player_t::frame()
{
	manage.run_demo();
}

void st_new_player_t::enter()
{
	menu.open();
	run_demo = 0;
	sound_play0(SOUND_EXPL);
}

void st_new_player_t::leave()
{
	menu.close();
}

void st_new_player_t::post_render(window_t *win)
{
	kobo_basestate_t::post_render(win);
	menu.render();
}

void st_new_player_t::press(int button)
{
	if(menu.editing)
	{
		switch(button)
		{
		  case BTN_EXIT:
			sound_play0(SOUND_EXPL);
			menu.editing = 0;
			menu.next();	// Select the CANCEL option.
			menu.next();
			break;

		  case BTN_FIRE:
			if(!prefs.use_joystick)
				break;
		  case BTN_START:
		  case BTN_SELECT:
			sound_play0(SOUND_EXPL);
			menu.editing = 0;
			menu.next();	// Select the OK option.
			break;

		  case BTN_UP:
		  case BTN_INC:
			if(menu.name[menu.currentIndex] == 'Z')
				menu.name[menu.currentIndex] = 'a';
			else if(menu.name[menu.currentIndex] == 'z')
				menu.name[menu.currentIndex] = 'A';
			else
				menu.name[menu.currentIndex]++;
			sound_play0(SOUND_SHOT);
			break;

		  case BTN_DEC:
		  case BTN_DOWN:
			if(menu.name[menu.currentIndex] == 'A')
				menu.name[menu.currentIndex] = 'z';
			else if(menu.name[menu.currentIndex] == 'a')
				menu.name[menu.currentIndex] = 'Z';
			else
				menu.name[menu.currentIndex]--;
			sound_play0(SOUND_SHOT);
			break;

		  case BTN_RIGHT:
			if(menu.currentIndex < sizeof(menu.name))
				menu.currentIndex++;
			if(menu.name[menu.currentIndex] == '\0')
				menu.name[menu.currentIndex] = 'A';
			sound_play0(SOUND_EXPL);
			break;

		  case BTN_LEFT:
		  case BTN_BACK:
			if(menu.currentIndex > 0)
			{
				if(menu.name[menu.currentIndex] != '\0')
					menu.name[menu.currentIndex] =
							'\0';
				menu.currentIndex--;
				sound_play0(SOUND_EXPL);
			}
			else
				sound_play0(SOUND_METALLIC);
			break;

		  default:
			if((unicode >= 'a') && (unicode <= 'z') ||
				(unicode >= 'A') && (unicode <= 'Z'))
			{
				menu.name[menu.currentIndex] = (char)unicode;
				if(menu.currentIndex < sizeof(menu.name))
					menu.currentIndex++;
				sound_play0(SOUND_SHOT);
			}
			break;
		}
		menu.rebuild();
	}
	else
	{
		menu.selection = -1;

		switch(button)
		{
		  case BTN_EXIT:
			menu.selection = 100;
			break;

		  case BTN_CLOSE:
			menu.selection = 99;
			break;

		  case BTN_FIRE:
		  case BTN_START:
		  case BTN_SELECT:
			menu.change(0);
			break;

		  case BTN_INC:
		  case BTN_UP:
			menu.prev();
			break;

		  case BTN_DEC:
		  case BTN_DOWN:
			menu.next();
			break;
		}

		switch(menu.selection)
		{
		  case 1:
			if(button == BTN_START
					|| button == BTN_SELECT
					|| button == BTN_FIRE)
			{
				sound_play0(SOUND_EXPL);
				menu.editing = 1;
			}
			break;

		  case 99:
			sound_play0(SOUND_EXPL);
			scorefile.addPlayer(menu.name);
			prefs.last_profile = scorefile.current_profile();
			prefs.changed = 1;
			pop();
			break;

		  case 100:
			sound_play0(SOUND_SHOT);
			strcpy(menu.name, "");
			pop();
			break;
		}
	}
}

st_new_player_t st_new_player;



/*----------------------------------------------------------
	st_main_menu
----------------------------------------------------------*/

void main_menu_t::buildStartLevel(int profNum)
{
	char buf[50];
	int MaxStartLevel = scorefile.last_scene(profNum);
	startLevel = manage.scene();
	if(startLevel > MaxStartLevel)
		startLevel = MaxStartLevel;
	list("Start Level", &startLevel, 5);
	for(int i = 0; i <= MaxStartLevel; ++i)
	{
		snprintf(buf, sizeof(buf), "%d", i + 1);
		item(buf, i);
	}
}

void main_menu_t::build()
{
	prefs.last_profile = scorefile.current_profile();
	manage.select_scene(scorefile.last_scene());

	halign = ALIGN_CENTER;
	xoffs = 0.5;
	big();
	space(1);
		button("Start Game!", 1);
		list("Player", &prefs.last_profile, 4);
		for(int i = 0; i < scorefile.numProfiles; ++i)
			item(scorefile.name(i), i);
		small();
		buildStartLevel(prefs.last_profile);
		big();
		button("New Player...", 3);
	space();
		button("Options", 2);
	space();
		button("Return to Intro", 0);
	space();
		button("Quit Kobo Deluxe", 100);
}

void main_menu_t::rebuild()
{
	int sel = selected_index();
	build_all();
	select(sel);
}


kobo_form_t *st_main_menu_t::open()
{
	// Moved here, as we want to do it as late as
	// possible, but *not* as a result of rebuild().
	if(prefs.last_profile >= scorefile.numProfiles)
	{
		prefs.last_profile = 0;
		prefs.changed = 1;
	}
	scorefile.select_profile(prefs.last_profile);

	menu = new main_menu_t;
	menu->open();
	return menu;
}


void st_main_menu_t::reenter()
{
	menu->rebuild();
	st_menu_base_t::reenter();
}


// Custom translator to map inc/dec on certain widgets
int st_main_menu_t::translate(int tag, int button)
{
	switch(tag)
	{
	  case 4:
		// The default translate() filters out the
		// inc/dec events, and performs the default
		// action for fire/start/select...
		switch(button)
		{
		  case BTN_FIRE:
		  case BTN_START:
		  case BTN_SELECT:
			do_default_action = 0;
			return tag + 10;
		  default:
			return tag;
		}
	  case 5:
		return tag;
	  default:
		return st_menu_base_t::translate(tag, button);
	}
}

void st_main_menu_t::select(int tag)
{
	switch(tag)
	{
	  case 1:
		gsm.change(&st_game);
		break;
	  case 2:
		gsm.push(&st_options_main);
		break;
	  case 3:
		gsm.push(&st_new_player);
		break;
	  case 4:	// Player: Inc/Dec
		sound_play0(SOUND_SHOT);
		prefs.changed = 1;
		scorefile.select_profile(prefs.last_profile);
		menu->rebuild();
		break;
	  case 14:	// Player: Select
		// Edit player profile!
//		menu->rebuild();
		break;
	  case 5:	// Start level: Inc/Dec
		sound_play0(SOUND_SHOT);
		manage.select_scene(menu->startLevel);
		break;
	  case 100:
		gsm.change(&st_ask_exit);
		break;
	}
}

st_main_menu_t st_main_menu;


/*----------------------------------------------------------
	st_options_main
----------------------------------------------------------*/

void options_main_t::build()
{
	big();
	space(2);
		button("Game", 4);
		button("Controls", 3);
		button("Video", 1);
		button("Audio", 2);
	space();
		button("DONE!", 0);
	space();
}

kobo_form_t *st_options_main_t::open()
{
	options_main_t *m = new options_main_t;
	m->open();
	return m;
}

void st_options_main_t::select(int tag)
{
	switch(tag)
	{
	  case 1:
		gsm.push(&st_options_video);
		break;
	  case 2:
		gsm.push(&st_options_audio);
		break;
	  case 3:
		gsm.push(&st_options_control);
		break;
	  case 4:
		gsm.push(&st_options_game);
		break;
	}
}

st_options_main_t st_options_main;


/*----------------------------------------------------------
	st_options_base
----------------------------------------------------------*/

kobo_form_t *st_options_base_t::open()
{
	sounds = 0;
	cfg_form = oopen();
	cfg_form->open(&prefs);
	return cfg_form;
}

void st_options_base_t::close()
{
	cfg_form->close();
}

void st_options_base_t::enter()
{
	sound_play0(SOUND_EXPL);
	st_menu_base_t::enter();
}

void st_options_base_t::select(int tag)
{
	if(cfg_form->status() & OS_CANCEL)
		cfg_form->undo();
	else if(cfg_form->status() & OS_CLOSE)
	{
		if(cfg_form->status() & (OS_RESTART | OS_RELOAD))
		{
			exit_game = 0;
			manage.abort();
		}
	}

	/* 
	 * Handle changes that require only an update...
	 */
	if(cfg_form->status() & OS_UPDATE_AUDIO)
	{
		audio_master_volume((float)prefs.volume/100.0);
		audio_group_controlf(SOUND_GROUP_INTRO, ACC_VOLUME,
				(float)prefs.intro_vol/100.0);
		audio_group_controlf(SOUND_GROUP_SFX, ACC_VOLUME,
				(float)prefs.sfx_vol/100.0);
		audio_group_controlf(SOUND_GROUP_MUSIC, ACC_VOLUME,
				(float)prefs.music_vol/100.0);

		audio_master_reverb((float)prefs.reverb/100.0);
		audio_set_limiter((float)prefs.threshold / 100.0,
				(float)prefs.release / 100.0);
		audio_quality((audio_quality_t)prefs.mixquality);
	}
	if(cfg_form->status() & OS_UPDATE_ENGINE)
	{
		gengine.period(prefs.wait_msec);
		gengine.interpolation(prefs.filter);
	}
	cfg_form->clearstatus(OS_UPDATE);

	if(cfg_form->status() & (OS_CANCEL | OS_CLOSE))
		pop();
}

void st_options_base_t::escape()
{
	sound_play0(SOUND_SHOT);
	cfg_form->undo();
}



/*----------------------------------------------------------
	Options...
----------------------------------------------------------*/
st_options_video_t st_options_video;
st_options_audio_t st_options_audio;
st_options_control_t st_options_control;
st_options_game_t st_options_game;



/*----------------------------------------------------------
	Debug: Audio Engine Profiling
----------------------------------------------------------*/
#ifdef PROFILE_AUDIO

#include "a_midicon.h"

st_profile_audio_t::st_profile_audio_t()
{
	name = "profile_audio";
	pan = 0;
	pitch = 60;
	shift = 0;
}


void st_profile_audio_t::enter()
{
	audio_group_control(SOUND_GROUP_SFX, ACC_PAN, pan);
	audio_group_control(SOUND_GROUP_SFX, ACC_PITCH, pitch << 16);
}


void st_profile_audio_t::press(int button)
{
	switch (button)
	{
	  case BTN_EXIT:
		pop();
		break;
	  case BTN_LEFT:
		pan -= 8192;
		if(pan < -65536)
			pan = -65536;
		audio_group_control(SOUND_GROUP_SFX, ACC_PAN, pan);
		break;
	  case BTN_RIGHT:
		pan += 8192;
		if(pan > 65536)
			pan = 65536;
		audio_group_control(SOUND_GROUP_SFX, ACC_PAN, pan);
		break;
	  case BTN_UP:
		++pitch;
		if(pitch > 127)
			pitch = 127;
		audio_group_control(SOUND_GROUP_SFX, ACC_PITCH, pitch << 16);
		break;
	  case BTN_DOWN:
		--pitch;
		if(pitch < 0)
			pitch = 0;
		audio_group_control(SOUND_GROUP_SFX, ACC_PITCH, pitch << 16);
		break;
	  case BTN_NO:
	  case BTN_UL:
	  case BTN_UR:
	  case BTN_DL:
	  case BTN_DR:
		break;
	  case BTN_INC:
		shift += 8;
		if(shift > AUDIO_MAX_WAVES-8)
			shift = 0;
		break;
	  case BTN_DEC:
		shift -= 8;
		if(shift < 0)
			shift = AUDIO_MAX_WAVES-8;
		break;
	  case BTN_FIRE:
	  {
		audio_channel_stop(-1, -1);
		int startt = SDL_GetTicks();
		audio_wave_load(0, "edit.agw", 0);
		printf("(Loading + processing time: %d ms)\n",
				SDL_GetTicks() - startt);
		break;
	  }
	  case BTN_START:
	  case BTN_SELECT:
		audio_channel_stop(-1, -1);
		break;
	  case BTN_YES:
		break;
	  case BTN_F1:
		sound_play0(0 + shift);
		midicon_midisock.program_change(0, 0 + shift);
		break;
	  case BTN_F2:
		sound_play0(1 + shift);
		midicon_midisock.program_change(0, 1 + shift);
		break;
	  case BTN_F3:
		sound_play0(2 + shift);
		midicon_midisock.program_change(0, 2 + shift);
		break;
	  case BTN_F4:
		sound_play0(3 + shift);
		midicon_midisock.program_change(0, 3 + shift);
		break;
	  case BTN_F5:
		sound_play0(4 + shift);
		midicon_midisock.program_change(0, 4 + shift);
		break;
	  case BTN_F6:
		sound_play0(5 + shift);
		midicon_midisock.program_change(0, 5 + shift);
		break;
	  case BTN_F7:
		sound_play0(6 + shift);
		midicon_midisock.program_change(0, 6 + shift);
		break;
	  case BTN_F8:
		sound_play0(7 + shift);
		midicon_midisock.program_change(0, 7 + shift);
		break;
	  case BTN_F11:
		switch(audio_cpu_ticks)
		{
		  case 50:
			audio_cpu_ticks = 100;
			break;
		  case 100:
			audio_cpu_ticks = 250;
			break;
		  case 250:
			audio_cpu_ticks = 500;
			break;
		  case 500:
			audio_cpu_ticks = 1000;
			break;
		  default:
			audio_cpu_ticks = 50;
			break;
		}
		break;
	}
}

void st_profile_audio_t::pre_render(window_t *win)
{
	/*
	 * Heeelp! I just *can't* stay away from chances
	 * like this to play around... :-D
	 */
	static int dither = 0;
	int y = 0;
	int y2 = win->height();
	float t = SDL_GetTicks()/1000.0;
	while(y < y2)
	{
		float c1 = sin(y*0.11 + t*1.5)*30.0 + 30;
		float c2 = sin(y*0.07 + t*2.5)*25.0 + 25;
		float c3 = sin(y*0.03 - t)*40.0 + 40;
		//Wideband color dither - improves 15/16 bit modes.
		float c4 = (dither + y) & 1 ? 3.0 : 0.0;
		int r = (int)(c1 + c2 + c4);
		int g = (int)(c2 + 3.0 - c4);
		int b = (int)(c1 + c2 + c3 + c4);
		win->foreground(win->map_rgb(r, g, b));
		win->fillrect(0, y, win->width(), 1);
		++y;
	}
	dither = 1 - dither;
}

void st_profile_audio_t::post_render(window_t *win)
{
	kobo_basestate_t::post_render(win);

	win->font(B_BIG_FONT);
	win->center(20, "Audio CPU Load");

	win->font(B_NORMAL_FONT);
	Uint32 fgc = win->map_rgb(0xffcc00);
	Uint32 bgc = win->map_rgb(0x006600);
	char buf[40];
	for(int i = 0; i < AUDIO_CPU_FUNCTIONS; ++i)
	{
		int perc = (int)(audio_cpu_function[i] / audio_cpu_total * 100.0);
		win->foreground(fgc);
		win->fillrect(103, 50+i*12+9, (int)audio_cpu_function[i]/2, 2);
		win->fillrect(128+32, 50+i*12+9, (int)perc/2, 2);
		win->foreground(bgc);
		win->fillrect(103 + (int)audio_cpu_function[i]/2, 50+i*12+9,
				50 - (int)audio_cpu_function[i]/2, 2);
		win->fillrect(128+32 + (int)perc/2, 50+i*12+9, 50 - (int)perc/2, 2);
		snprintf(buf, sizeof(buf), "%s:%5.2f%% (%1.0f%%)",
				audio_cpu_funcname[i],
				audio_cpu_function[i],
				audio_cpu_function[i] / audio_cpu_total * 100.0);
		win->center_token(120, 50+i*12, buf, ':');
	}

	win->foreground(fgc);
	win->fillrect(80, 178, (int)audio_cpu_total, 4);
	win->foreground(bgc);
	win->fillrect(80 + (int)audio_cpu_total, 178, 100 - (int)audio_cpu_total, 4);
	win->font(B_BIG_FONT);
	snprintf(buf, sizeof(buf), "Total:%5.2f%%", audio_cpu_total);
	win->center_token(120, 180, buf, ':');

	win->font(B_NORMAL_FONT);
	snprintf(buf, sizeof(buf), "Pan [L/R]:%5.2f  "
			"Pitch [U/D]: %d  ",
			(float)pan/65536.0,
			pitch);
	win->center(200, buf);
	snprintf(buf, sizeof(buf), "F1..F8 [+/-]: %d..%d",
			shift, shift + 7);
	win->center(215, buf);
}

st_profile_audio_t st_profile_audio;

#endif /*PROFILE_AUDIO*/


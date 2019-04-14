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

#ifndef _SKOBO_H_
#define _SKOBO_H_

#include "gfxengine.h"
#include "window.h"
#include "display.h"
#include "filemap.h"
#include "prefs.h"

extern window_t wchip;
extern window_t wbase;
extern window_t wradar;
extern window_t whealth;

extern display_t dhigh;
extern display_t dscore;
extern display_t dstage;
extern display_t dships;

extern int mouse_x, mouse_y;
extern int mouse_left, mouse_middle, mouse_right;

extern int exit_game;

class skobo_gfxengine_t : public gfxengine_t
{
	void frame();
	void pre_render();
	void post_render();
};

/* Sprite priority levels */
#define	LAYER_OVERLAY	0	//Mouse crosshair
#define	LAYER_PLAYER	1	//Player and beams
#define	LAYER_ENEMIES	2	//Enemies and their bullets
#define	LAYER_BASES	3	//Not yet in use
#define	LAYER_GROUND	4	//Not yet in use
#define	LAYER_STARS	5	//Not yet in use

/* Graphics banks */
#define	B_TILES		0
#define	B_SPRITES	1
#define	B_BULLETS	2
#define	B_BIGSHIP	3

#define	B_SCREEN	16
#define	B_FRAME		17
#define	B_LOGO		18
#define	B_BRUSHES	19
#define	B_SMALL_TUBE	20
#define	B_BIG_TUBE	21

#define B_HIGH_BACK	22
#define B_SCORE_BACK	23
#define B_RADAR_BACK	24
#define B_SHIPS_BACK	25
#define B_STAGE_BACK	26

#define	B_TEMP		31

#define	B_LOADING	32
#define	B_NORMAL_FONT	33
#define	B_MEDIUM_FONT	34
#define	B_BIG_FONT	35
#define	B_COUNTER_FONT	36

#define	INTRO_SCENE	-100000

extern skobo_gfxengine_t gengine;
extern filemapper_t fmap;
extern prefs_t prefs;

/* Joystick support */
extern SDL_Joystick *joystick;
extern int js_lr;
extern int js_ud;
extern int js_fire;
#define DEFAULT_JOY_LR		0	// Joystick axis left-right default
#define DEFAULT_JOY_UD		1	// Joystick axis up-down default
#define DEFAULT_JOY_FIRE	0	// Default fire button on joystick
#define DEFAULT_JOY_START	1

#include "kobosfx.h"

/* Sound groups */
#define	SOUND_GROUP_INTRO	0
#define	SOUND_GROUP_SFX		1
#define	SOUND_GROUP_MUSIC	2

#endif // _SKOBO_H_

/*
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 1995, 1996,  Akira Higuchi
 * Copyright (C) 2001, David Olofson
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

#ifndef XKOBO_H_MYSHIP
#define XKOBO_H_MYSHIP

#include "config.h"
#include "gfxengine.h"
#include "game.h"

#define ABS(x)   (((x)>=0) ? (x) : (-(x)))
#define MAX(x,y) (((x)>(y)) ? (x) : (y))

//---------------------------------------------------------------------------//
enum _myship_state
{
	normal,
	dead
};

class _myship
{
	static _myship_state _state;
	static int di;	/* direction */
	static int virtx, virty;	/* scroll position */
	static int x, y;
	static int _health;
	static int explo_counter;
	static int shot_counter;
	static int lapx, lapy;
	static int beamx[MAX_MISSILES], beamy[MAX_MISSILES];
	static int beamdi[MAX_MISSILES], beamst[MAX_MISSILES];
	/* For the gfxengine connection */
	static cs_obj_t *object;
	static cs_obj_t *beam_objects[MAX_MISSILES];
	static cs_obj_t *crosshair;
	static void state(_myship_state s);
	static void shot_single(int i, int dir);
	static void apply_position();
  public:
	 _myship();
	static inline int get_x()
	{
		return x;
	}
	static inline int get_y()
	{
		return y;
	}
	static inline int get_virtx()
	{
		return virtx;
	}
	static inline int get_virty()
	{
		return virty;
	}
	static int init();
	static void off();
	static int move();
	static int put();
	static void put_crosshair();
	static int shot();
	static int hit_structure();
	static int hit_beam(int ex, int ey, int hitsize, int health);
	static void hit(int dmg);
	static int health()	{ return _health;	}
	static void set_position(int px, int py);
};

extern _myship    myship;

#endif // XKOBO_H_MYSHIP

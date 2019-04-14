/*
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 1995, 1996,  Akira Higuchi
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

#ifndef KOBO_RADAR_H
#define KOBO_RADAR_H

enum  radar_modes_t
{
	RM_OFF,		//Gray, empty
	RM_FREE,	//Doing nothing
	RM_SWEEP,	//Slow band by band refresh
	RM_SCROLL	//Fast full window scrolling
};

class _radar
{
	static radar_modes_t mode;
	static int xpos, ypos;		//Player position
	static int xoffs, yoffs;	//Scroll offset (16:16 fixp)
	static int xspeed, yspeed;	//Scroll speed (16:16 fixp)
	static int target_xoffs, target_yoffs;	//For scroll mode
	static int time;		//for delta time calc
	static int refresh_pos;		//Sweeping refresh posn
	static int width, height;
	static int pixel_player;
	static int pixel_core;
	static int pixel_launcher;
	static int pixel_hard;
	static int pixel_bg;
	static void plot(int x, int y);
	static void sweep(int reset);
	static void scroll(int reset);
  public:
	static void prepare(int clear);
	static void update(int mx, int my);
	static void trace_myship();
};

extern _radar radar;

#endif // KOBO_RADAR_H

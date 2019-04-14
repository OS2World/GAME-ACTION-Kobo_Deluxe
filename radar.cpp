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

extern "C"
{
#include <stdlib.h>
#include <math.h>
#ifndef M_PI
# define M_PI 3.14159265358979323846	/* pi */
#endif
}
#include "kobo.h"
#include "screen.h"
#include "myship.h"
#include "radar.h"
#include "prefs.h"

radar_modes_t _radar::mode = RM_OFF;
int _radar::width = MAP_SIZEX;
int _radar::height = MAP_SIZEY;
int _radar::xpos = -1;
int _radar::ypos = -1;
int _radar::xoffs = 0;
int _radar::yoffs = 0;
int _radar::xspeed = 0;
int _radar::yspeed = 0;
int _radar::target_xoffs = 0;
int _radar::target_yoffs = 0;
int _radar::pixel_player = -1;
int _radar::pixel_core = -1;
int _radar::pixel_launcher = -1;
int _radar::pixel_hard = -1;
int _radar::pixel_bg = -1;
int _radar::time = 0;
int _radar::refresh_pos = 0;


void _radar::plot(int x, int y)
{
	int mx = (x + (xoffs>>16)) & ((WORLD_SIZEX - 1)>>4);
	int my = (y + (yoffs>>16)) & ((WORLD_SIZEY - 1)>>4);
	int a = screen.get_chip_number(mx, my);
	if(IS_SPACE(a))
		return;

	if(a == CORE)
		wradar.foreground(pixel_core);
	else if((a == U_MASK) || (a == R_MASK) ||
			(a == D_MASK) || (a == L_MASK))
		wradar.foreground(pixel_launcher);
	else if((a & HIT_MASK))
		wradar.foreground(pixel_hard);
	wradar.point(x, y);
}


void _radar::update(int mx, int my)
{
	int x = (mx - (xoffs>>16)) & ((WORLD_SIZEX - 1)>>4);
	int y = (my - (yoffs>>16)) & ((WORLD_SIZEY - 1)>>4);
	int a = screen.get_chip_number(mx, my);
	if(IS_SPACE(a))
	{
		SDL_Rect r;
		r.x = x;
		r.y = y;
		r.w = r.h = 1;
		wradar.clear(&r);
	}
	else
	{
		if(a == CORE)
			wradar.foreground(pixel_core);
		else if((a == U_MASK) || (a == R_MASK) ||
				(a == D_MASK) || (a == L_MASK))
			wradar.foreground(pixel_launcher);
		else if((a & HIT_MASK))
			wradar.foreground(pixel_hard);
		wradar.point(x, y);
	}
	SDL_Rect r;
	r.x = x;
	r.y = y;
	r.w = r.h = 1;
	wradar.invalidate(&r);
}


void _radar::prepare(int clear)
{
	pixel_player = wbase.map_rgb(128, 240, 240);
	pixel_core = wbase.map_rgb(255, 255, 128);
	pixel_hard = wbase.map_rgb(64, 128, 128);
	pixel_launcher = wbase.map_rgb(64, 200, 240);
	pixel_bg = wbase.map_rgb(32, 48, 64);

	wradar.clear();

	if(clear == -1)
		switch(mode)
		{
		  case RM_OFF:
			clear = 1;
			break;
		  case RM_FREE:
		  case RM_SCROLL:
		  case RM_SWEEP:
			clear = 0;
			break;
		}

	if(clear)
		mode = RM_OFF;
	else
	{
		int i, j;
		xpos = ypos = -1;
		xoffs = yoffs = 0;
		for(i = 0; i < width; i++)
			for(j = 0; j < height; j++)
				plot(i, j);
		mode = RM_FREE;
	}
	wradar.invalidate();
}


void _radar::scroll(int reset)
{
	int y, x, dt, nt, dx, dy;

	/* ms since last frame */
	nt = SDL_GetTicks();
	dt = nt - time;
	time = nt;

	if(reset)
	{
		dx = (target_xoffs - (xoffs>>16)) & ((WORLD_SIZEX-1)>>4);
		dy = (target_yoffs - (yoffs>>16)) & ((WORLD_SIZEY-1)>>4);

		// Take shortest path!
		if(dx > (WORLD_SIZEX>>5))
			dx -= (WORLD_SIZEX>>4);
		if(dy > (WORLD_SIZEY>>5))
			dy -= (WORLD_SIZEY>>4);

		// Speed for 1000 ms scroll duration
		xspeed = (dx<<16) / 1000;
		yspeed = (dy<<16) / 1000;

		// Add 20 pixels/second
		float sp = 20 * 65536.0 / 1000;
		float dist = sqrt(dx*dx + dy*dy);
		if(dist > 1)
		{
			xspeed += (int)(sp * (float)dx / dist);
			yspeed += (int)(sp * (float)dy / dist);
		}
		else
		{
			xspeed = yspeed = 0;
			xoffs = target_xoffs<<16;
			yoffs = target_yoffs<<16;
		}
		return;
	}

	/* Scale to frame rate independent speed */
	dx = xspeed * dt;
	dy = yspeed * dt;

	/* Scroll */
	if(labs(dx) > labs((target_xoffs<<16) - xoffs))
		xoffs = target_xoffs<<16;
	else
		xoffs += dx;
	xoffs &= (WORLD_SIZEX<<(16-4))-1;
	if(labs(dy) > labs((target_yoffs<<16) - yoffs))
		yoffs = target_yoffs<<16;
	else
		yoffs += dy;
	yoffs &= (WORLD_SIZEY<<(16-4))-1;

	/* Repaint the whole radar window */
	wradar.clear();
	for(y = 0; y < height; y++)
		for(x = 0; x < width; x++)
			plot(x, y);
	wradar.invalidate();

	/* Stop fast scrolling when done */
	if(((xoffs>>16) == target_xoffs) && ((yoffs>>16) == target_yoffs))
		mode = RM_FREE;
}


void _radar::sweep(int reset)
{
	int y, x, start_y, end_y, dt, nt;

	/* ms since last frame */
	nt = SDL_GetTicks();
	dt = nt - time;
	time = nt;

	if(reset || (dt > 1000))
	{
		start_y = 0;
		end_y = height >> 4;
		refresh_pos = 0;
	}
	else
	{
		start_y = refresh_pos;
		refresh_pos += dt * 2;
		if(refresh_pos >= height)
		{
			end_y = height;
			refresh_pos = 0;
			mode = RM_FREE;
		}
		else
			end_y = refresh_pos;
	}

	SDL_Rect r;
	r.x = 0;
	r.y = start_y;
	r.w = width;
	r.h = end_y - start_y;
	wradar.clear(&r);
#if 0
	wradar.foreground(wradar.map_rgb(32, 72, 64));
	wradar.fillrect(0, end_y, width, 1);
	wradar.foreground(wradar.map_rgb(32, 112, 64));
	wradar.fillrect(0, end_y + 1, width, 1);
	wradar.foreground(wradar.map_rgb(32, 144, 64));
	wradar.fillrect(0, end_y + 2, width, 1);
#endif
	for(y = start_y; y < end_y; y++)
		for(x = 0; x < width; x++)
			plot(x, y);
	wradar.invalidate(&r);
}


void _radar::trace_myship()
{
	if(RM_OFF == mode)
		return;

	SDL_Rect r;
	int xpos_new = (myship.get_x() & (WORLD_SIZEX - 1)) >> 4;
	int ypos_new = (myship.get_y() & (WORLD_SIZEY - 1)) >> 4;

	if(!prefs.scrollradar)
		if((xpos_new == xpos) && (ypos_new == ypos))
			return;

	/* Remove old player marker */
	if(mode != RM_SCROLL)
		update(xpos, ypos);

	/* Set new position */
	xpos = xpos_new;
 	ypos = ypos_new;

	if(prefs.scrollradar)
	{
		switch(mode)
		{
		  case RM_OFF:
		  case RM_FREE:
			/* Calculate new target scroll position */
			target_xoffs = (WORLD_SIZEX >> 5) + xpos;
			target_yoffs = (WORLD_SIZEY >> 5) + ypos;
			target_xoffs &= ((WORLD_SIZEX-1) >> 4);
			target_yoffs &= ((WORLD_SIZEY-1) >> 4);
			break;
		  case RM_SCROLL:
		  case RM_SWEEP:
			break;
		}
		switch(mode)
		{
		  case RM_OFF:
		  case RM_FREE:
		  {
			int delta = labs(target_xoffs - (xoffs>>16)) &
					((WORLD_SIZEX-1) >> 4);
			if(delta <= 1)
				delta = labs(target_yoffs - (yoffs>>16)) &
						((WORLD_SIZEY-1) >> 4);
			if(delta > 5)
			{
				mode = RM_SCROLL;
				scroll(1);
			}
			else if(delta)
			{
				xoffs = target_xoffs<<16;
				yoffs = target_yoffs<<16;
				mode = RM_SWEEP;
				sweep(1);
			}
			break;
		  }
		  case RM_SWEEP:
			sweep(0);
			break;
		  case RM_SCROLL:
			scroll(0);
			break;
		}
	}

	/* Plot player marker */
	if(prefs.scrollradar && (RM_SCROLL != mode))
	{
		//Cheat some to keep the ship marker from jumping around...
		r.x = WORLD_SIZEX >> 5;
		r.y = WORLD_SIZEY >> 5;
	}
	else
	{
		r.x = (xpos - (xoffs>>16)) & ((WORLD_SIZEX-1)>>4);
		r.y = (ypos - (yoffs>>16)) & ((WORLD_SIZEY-1)>>4);
	}
	r.w = r.h = 1;
	wradar.foreground(pixel_player);
	wradar.point(r.x, r.y);
	if(mode != RM_SCROLL)
		wradar.invalidate(&r);
}

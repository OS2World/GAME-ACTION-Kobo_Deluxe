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

#include "kobo.h"
#include "screen.h"
#include "myship.h"
#include "enemies.h"
#include "gamectl.h"
#include "manage.h"
#include "random.h"
#include "audio.h"

_myship_state _myship::_state;
int _myship::di;
int _myship::virtx;
int _myship::virty;
int _myship::x;
int _myship::y;
int _myship::_health;
int _myship::explo_counter;
int _myship::shot_counter;
int _myship::lapx;
int _myship::lapy;
int _myship::beamx[MAX_MISSILES];
int _myship::beamy[MAX_MISSILES];
int _myship::beamdi[MAX_MISSILES];
int _myship::beamst[MAX_MISSILES];
cs_obj_t *_myship::object;
cs_obj_t *_myship::beam_objects[MAX_MISSILES];
cs_obj_t *_myship::crosshair;


_myship::_myship()
{
	object = NULL;
	memset(beam_objects, 0, sizeof(beam_objects));
	crosshair = NULL;
}


void _myship::state(_myship_state s)
{
	switch (s)
	{
	  case dead:
		_health = 0;
		if(object)
			gengine.free_obj(object);
		object = NULL;
		if(crosshair)
			gengine.free_obj(crosshair);
		crosshair = NULL;
		break;
	  case normal:
		_health = game.health;
		if(!object)
			object = gengine.get_obj(LAYER_PLAYER);
		if(object)
			cs_obj_show(object);
		break;
	}
	_state = s;
	manage.set_health(_health);
}


void _myship::off()
{
	state(dead);
	int i;
	for(i = 0; i < MAX_MISSILES; i++)
		if(beam_objects[i])
		{
			gengine.free_obj(beam_objects[i]);
			beam_objects[i] = NULL;
		}
}


int _myship::init()
{
	x = WORLD_SIZEX >> 1;
	y = (WORLD_SIZEY >> 2) * 3;
	virtx = x - (WSIZE >> 1);
	virty = y - (WSIZE >> 1);
	lapx = 0;
	lapy = 0;
	di = 1;
	state(normal);

	apply_position();

	int i;
	for(i = 0; i < MAX_MISSILES; i++)
	{
		beamx[i] = 0;
		beamy[i] = 0;
		beamdi[i] = 0;
		beamst[i] = 0;
		if(beam_objects[i])
			gengine.free_obj(beam_objects[i]);
		beam_objects[i] = NULL;
	}
	return 0;
}


#define BEAMV1           12
#define BEAMV2           (BEAMV1*2/3)

int _myship::move()
{
	int i;
	di = gamecontrol.dir();

	virtx = x - (WSIZE >> 1);
	virty = y - (WSIZE >> 1);
	if(_state == normal)
	{
		int vd, vo;
		if(!prefs.cmd_pushmove)
		{
			vd = 2;
			vo = 3;
		}
		else if(gamecontrol.movekey_pressed)
			{
				vd = 1;
				vo = 1;
			}
			else
			{
				vd = 0;
				vo = 0;
			}
		switch (di)
		{
		  case 1:
			virty -= vo;
			break;
		  case 2:
			virty -= vd;
			virtx += vd;
			break;
		  case 3:
			virtx += vo;
			break;
		  case 4:
			virtx += vd;
			virty += vd;
			break;
		  case 5:
			virty += vo;
			break;
		  case 6:
			virty += vd;
			virtx -= vd;
			break;
		  case 7:
			virtx -= vo;
			break;
		  case 8:
			virtx -= vd;
			virty -= vd;
			break;
		}
		explo_counter = 0;
	}
	else if(_state == dead)
	{
		enemies.make(&explosion, x + pubrand.get(6) - 32,
				y + pubrand.get(6) - 32);
		if(--explo_counter < 0)
		{
			explo_counter = pubrand.get(2) + 3;
			if(pubrand.get(8) > 100)
				sound_play0(SOUND_EXPL);
			else
				sound_play0(SOUND_BOMB);
		}
	}

	lapx = 0;
	lapy = 0;
	if(virtx < 0)
	{
		virtx += WORLD_SIZEX;
		lapx = WORLD_SIZEX;
	}
	if(virtx >= WORLD_SIZEX)
	{
		virtx -= WORLD_SIZEX;
		lapx = -WORLD_SIZEX;
	}
	if(virty < 0)
	{
		virty += WORLD_SIZEY;
		lapy = WORLD_SIZEY;
	}
	if(virty >= WORLD_SIZEY)
	{
		virty -= WORLD_SIZEY;
		lapy = -WORLD_SIZEY;
	}
	x = virtx + (WSIZE >> 1);
	y = virty + (WSIZE >> 1);

	if((_state == normal) && gamecontrol.get_shot())
	{
		if(shot_counter > 0)
			--shot_counter;
		else
		{
			_myship::shot();
			shot_counter = game.loadtime - 1;
		}
	}
	else
		if(shot_counter > 0)
			--shot_counter;

	for(i = 0; i < MAX_MISSILES; i++)
	{
		if(!beamst[i])
			continue;
		beamx[i] += lapx;
		beamy[i] += lapy;
		switch (beamdi[i])
		{
		  case 1:
			beamy[i] -= BEAMV1;
			break;
		  case 2:
			beamy[i] -= BEAMV2;
			beamx[i] += BEAMV2;
			break;
		  case 3:
			beamx[i] += BEAMV1;
			break;
		  case 4:
			beamx[i] += BEAMV2;
			beamy[i] += BEAMV2;
			break;
		  case 5:
			beamy[i] += BEAMV1;
			break;
		  case 6:
			beamy[i] += BEAMV2;
			beamx[i] -= BEAMV2;
			break;
		  case 7:
			beamx[i] -= BEAMV1;
			break;
		  case 8:
			beamx[i] -= BEAMV2;
			beamy[i] -= BEAMV2;
			break;
		}
		if((ABS(beamx[i] - x) >= (VIEWLIMIT >> 1) + 16) ||
				(ABS(beamy[i] - y) >= (VIEWLIMIT >> 1) + 16))
		{
			beamst[i] = 0;
			if(beam_objects[i])
				gengine.free_obj(beam_objects[i]);
			beam_objects[i] = NULL;
		}
	}
	return 0;
}


void _myship::hit(int dmg)
{
	if(_state != normal)
		return;

	if(!dmg)
		dmg = 1000;

	_health -= dmg;
	if(_health > 0)
	{
		manage.set_health(_health);
		sound_play0(SOUND_METALLIC);
	}
	else
	{
		manage.lost_myship();
		state(dead);
		sound_play0(SOUND_EXPL);
		sound_play0(SOUND_BOMB);
	}
}


int _myship::hit_structure()
{
	int x1, y1;
	int i, ch;
	for(i = 0; i < MAX_MISSILES; i++)
	{
		if(!beamst[i])
			continue;
		x1 = (beamx[i] & (WORLD_SIZEX - 1)) >> 4;
		y1 = (beamy[i] & (WORLD_SIZEY - 1)) >> 4;
		ch = screen.get_chip_number(x1, y1);
		if(!IS_SPACE(ch) && (ch & HIT_MASK))
		{
			sound_play(SOUND_METALLIC, x1<<4, y1<<4);
			beamst[i] = 0;
			if(beam_objects[i])
				gengine.free_obj(beam_objects[i]);
			beam_objects[i] = NULL;
		}
	}
	x1 = (x & (WORLD_SIZEX - 1)) >> 4;
	y1 = (y & (WORLD_SIZEY - 1)) >> 4;
	ch = screen.get_chip_number(x1, y1);
	if(!IS_SPACE(ch) && (ch & HIT_MASK))
	{
		if(prefs.cmd_indicator)
			sound_play(SOUND_METALLIC, x1<<4, y1<<4);
		else
			_myship::hit(1000);
	}
	return 0;
}


int _myship::hit_beam(int ex, int ey, int hitsize, int health)
{
	int dmg = 0;
	int i;
	for(i = 0; i < MAX_MISSILES; i++)
	{
		if(beamst[i] == 0)
			continue;
		if(ABS(ex - beamx[i]) >= hitsize)
			continue;
		if(ABS(ey - beamy[i]) >= hitsize)
			continue;
		if(!prefs.cmd_cheat)
		{
			beamst[i] = 0;
			if(beam_objects[i])
				gengine.free_obj(beam_objects[i]);
			beam_objects[i] = NULL;
		}
		dmg += game.missile_damage;
		if(dmg >= health)
			break;
	}
	return dmg;
}


void _myship::put_crosshair()
{
	if(_state != normal)
		return;
	if(!crosshair)
	{
		crosshair = gengine.get_obj(LAYER_OVERLAY);
		if(crosshair)
			cs_obj_show(crosshair);
	}
	if(crosshair)
	{
		cs_obj_image(crosshair, B_SPRITES, 5 * 16 + 15);
		crosshair->point.v.x = PIXEL2CS(mouse_x - 8 - MARGIN - (CHIP_SIZEX>>1));
		crosshair->point.v.y = PIXEL2CS(mouse_y - MARGIN - (CHIP_SIZEX>>1));
	}
}


int _myship::put()
{
	/* Player */
	apply_position();
	if(object)
		cs_obj_image(object, B_SPRITES, 3 * 16 + di - 1);

	/* Bullets */
	int i;
	for(i = 0; i < MAX_MISSILES; i++)
	{
		if(!beam_objects[i])
			continue;
		if(beamst[i])
		{
			beam_objects[i]->point.v.x =
					PIXEL2CS(beamx[i] - (CHIP_SIZEX>>1));
			beam_objects[i]->point.v.y =
					PIXEL2CS(beamy[i] - (CHIP_SIZEY>>1));
		}
	}
	return 0;
}


void _myship::shot_single(int i, int dir)
{
	beamdi[i] = dir;
	beamx[i] = x;
	beamy[i] = y;
	beamst[i] = 1;
	if(!beam_objects[i])
		beam_objects[i] = gengine.get_obj(LAYER_PLAYER);
	if(beam_objects[i])
	{
		beam_objects[i]->point.v.x =
				PIXEL2CS(beamx[i] - (CHIP_SIZEX>>1));
		beam_objects[i]->point.v.y =
				PIXEL2CS(beamy[i] - (CHIP_SIZEY>>1));
		cs_obj_image(beam_objects[i], B_SPRITES, 2 * 16 + beamdi[i] - 1);
		cs_obj_show(beam_objects[i]);
	}
}


int _myship::shot()
{
	int i, j;
	for(i = 0; i < game.missiles && beamst[i]; i++)
		;
	for(j = i + 1; j < game.missiles && beamst[j]; j++)
		;
	if(j >= game.missiles)
		return 1;

	shot_single(i, di);
	shot_single(j, (di > 4) ? (di - 4) : (di + 4) );

	sound_play0(SOUND_SHOT);
	return 0;
}


void _myship::set_position(int px, int py)
{
	x = px;
	y = py;
	virtx = x - (WSIZE >> 1);
	virty = y - (WSIZE >> 1);

	if(object)
	{
		apply_position();
		cs_point_force(&object->point);
	}
}


void _myship::apply_position()
{
	if(object)
	{
		object->point.v.x = PIXEL2CS(x - (CHIP_SIZEX>>1));
		object->point.v.y = PIXEL2CS(y - (CHIP_SIZEY>>1));
	}
	sound_position(x, y);
}

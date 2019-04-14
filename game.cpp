/*(GPL)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 2002, David Olofson
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
#include "game.h"

game_t game;


game_t::game_t()
{
	reset();
}


void game_t::reset()
{
	set(GAME_SINGLE, SKILL_NORMAL);
}


void game_t::set(game_types_t tp, skill_levels_t sk)
{
	type = tp;
	skill = sk;

	switch(type)
	{
	  case GAME_SINGLE:
		speed = 30;
		lives = 5;
		health = 1;
		damage = 0;
		missiles = 10;
		loadtime = 1;
		missile_damage = 20;
		break;
	  case GAME_SINGLE_NEW:
		speed = 30;
		lives = 3;
		health = 100;
		damage = 100;
		missiles = 10;
		loadtime = 2;
		missile_damage = 20;
		break;
	  case GAME_COOPERATIVE:
		speed = 30;
		lives = 3;
		health = 100;
		damage = 100;
		missiles = 10;
		loadtime = 2;
		missile_damage = 20;
		break;
	  case GAME_DEATHMATCH:
		speed = 30;
		lives = 20;
		health = 100;
		damage = 100;
		missiles = 20;
		loadtime = 10;
		missile_damage = 34;
		break;
	  default:
		break;
	}

	switch(skill)
	{
	  case SKILL_BEGINNER:
		speed += 10;
		if(type != GAME_DEATHMATCH)
		{
			missiles += 10;
			loadtime = 1;
		}
		break;
	  case SKILL_LOW:
		speed += 5;
		if(type != GAME_DEATHMATCH)
			missiles += 5;
		break;
	  case SKILL_NORMAL:
		break;
	  case SKILL_HIGH:
		speed -= 5;
		if(type != GAME_DEATHMATCH)
			loadtime += 1;
		break;
	  case SKILL_GODLIKE:
		speed -= 10;
		if(type != GAME_DEATHMATCH)
		{
			missiles -= 3;
			loadtime += 1;
		}
		break;
	  default:
		break;
	}

	if(missiles > MAX_MISSILES)
		missiles = MAX_MISSILES;
}

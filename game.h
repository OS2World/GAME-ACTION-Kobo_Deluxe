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

#ifndef	_KOBO_GAME_H_
#define	_KOBO_GAME_H_

#define BONUS_FIRSTTIME  	2000
#define BONUS_EVERY      	3000

#define MAX_MISSILES		30

enum game_types_t
{
	GAME_UNKNOWN = -1,
	GAME_SINGLE,
	GAME_SINGLE_NEW,
	GAME_COOPERATIVE,
	GAME_DEATHMATCH
};


enum skill_levels_t
{
	SKILL_UNKNOWN = -1,
	SKILL_BEGINNER,
	SKILL_LOW,
	SKILL_NORMAL,
	SKILL_HIGH,
	SKILL_GODLIKE
};

class game_t
{
  public:
	int	type;
	int	skill;
	int	speed;		// ms per logic frame
	int	lives;		// When starting new games of certain types
	int	health;		// Initial health
	int	damage;		// Damage player inflicts when colliding with
				// another object
	int	missiles;	// maximum active at a time
	int	loadtime;	// frames/shot
	int	missile_damage;	// Damage inflicted by missiles.
	game_t();
	void reset();
	void set(game_types_t tp, skill_levels_t sk);
};

extern game_t game;

#endif /*_KOBO_GAME_H_*/

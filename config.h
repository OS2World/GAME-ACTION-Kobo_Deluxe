/*(GPL)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 1995, 1996, Akira Higuchi
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

#ifndef _KOBO_CONFIG_H_
#define _KOBO_CONFIG_H_

#include <aconfig.h>

#ifndef DEBUG
#	undef	DBG
#	undef	DBG2
#	undef	DBG3
#	undef	DBG4
#	define	DBG(x)
#	define	DBG2(x)
#	define	DBG3(x)
#	define	DBG4(x)
#endif

#ifndef HAVE_SNPRINTF
#define snprintf _snprintf
#endif

/* Key/button repeat timing */
#define	KOBO_KEY_DELAY	250
#define	KOBO_KEY_REPEAT	40

#define ENEMY_MAX	1024

/*
 * Game display size. (Used to be 224x224.)
 *
 * Note that Kobo Deluxe adds 8 "partially"
 * visible pixels around the display, which
 * could mean that some objects can be created
 * or deleted in view. Finding out whether or
 * not it can really happen calls for closer
 * analysis of the code. (Most things still
 * seem to be activated when just out of
 * view.)
 */
#define WSIZE	230
#define MARGIN	5

/*
 * (In XKobo, WSIZE was used where this is
 * used now; in the game logic code.)
 *
 * NOTE:  I *DON'T* want to change the view
 *        range as the XKobo engine knows it,
 *        as that would make the game play
 *        slightly differently. (Probably not
 *        so that anyone would notice, but
 *        let's not take chances... This is
 *        NOT "Kobo II".)
 */
#define VIEWLIMIT	224

#define HIT_MYSHIP	5
#define HIT_BEAM	5

/* Actually, this is not the *full* size in windowed mode any more. */
#define	SCREEN_WIDTH	320
#define	SCREEN_HEIGHT	240

/* Text scroller speed (pixels/second) */
#define	SCROLLER_SPEED	180

/* Various size info (DO NOT EDIT!) */
#define CHIP_SIZEX_LOG2   4
#define CHIP_SIZEY_LOG2   4
#define MAP_SIZEX_LOG2    6
#define MAP_SIZEY_LOG2    7
#define WORLD_SIZEX_LOG2 (MAP_SIZEX_LOG2+CHIP_SIZEX_LOG2)
#define WORLD_SIZEY_LOG2 (MAP_SIZEY_LOG2+CHIP_SIZEY_LOG2)

#define CHIP_SIZEX        (1<<CHIP_SIZEX_LOG2)
#define CHIP_SIZEY        (1<<CHIP_SIZEY_LOG2)
#define MAP_SIZEX         (1<<MAP_SIZEX_LOG2)
#define MAP_SIZEY         (1<<MAP_SIZEY_LOG2)
#define WORLD_SIZEX      (1<<WORLD_SIZEX_LOG2)
#define WORLD_SIZEY      (1<<WORLD_SIZEY_LOG2)

#endif	/*_KOBO_CONFIG_H_*/

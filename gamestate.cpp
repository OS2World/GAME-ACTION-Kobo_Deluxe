/*(GPL)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
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

#define	DBG(x)

#include <stdio.h>
#include "config.h"
#include "kobo.h"
#include "gamestate.h"

/*----------------------------------------------------------
	gamestate_t
----------------------------------------------------------*/

gamestate_t::gamestate_t()
{
	next = 0;
	manager = 0;
	unicode = 0;
}

gamestate_t::~gamestate_t()			{}

void gamestate_t::pop()
{
	if(!manager)
		return;
	//This is far from fool-proof, but it should
	//handle the most common cases of "trying to
	//pop 'this' more than once"...
	if(manager->current() == this)
		manager->pop();
	else
		fprintf(stderr, "WARNING: Tried to pop state more than once!\n");
}

void gamestate_t::enter()			{}
void gamestate_t::leave()			{}
void gamestate_t::yield()			{}
void gamestate_t::reenter()			{}
void gamestate_t::press(int button)		{}
void gamestate_t::release(int button)		{}
void gamestate_t::pos(int x, int y)		{}
void gamestate_t::delta(int dx, int dy)		{}
void gamestate_t::frame()			{}
void gamestate_t::pre_render(window_t *win)	{}
void gamestate_t::post_render(window_t *win)	{}



/*----------------------------------------------------------
	gamestatemanager_t
----------------------------------------------------------*/

gamestatemanager_t::gamestatemanager_t()
{
	top = 0;
}


gamestatemanager_t::~gamestatemanager_t()
{
}


void gamestatemanager_t::change(gamestate_t *gs)
{
	gs->manager = this;

	gamestate_t *oldtop = top;
	if(top)
		top = top->next;
	gs->next = top;
	top = gs;

	if(oldtop)
	{
		oldtop->leave();
		oldtop->manager = 0;
	}
	if(top)
		top->enter();

	DBG(if(prefs.cmd_debug)
		printf("switched to '%s'\n", gs->name);)
}


void gamestatemanager_t::push(gamestate_t *gs)
{
	gs->manager = this;

	gamestate_t *oldtop = top;
	gs->next = top;
	top = gs;

	if(oldtop)
		oldtop->yield();
	if(top)
		top->enter();

	DBG(if(prefs.cmd_debug)
		printf("pushed state '%s'\n", gs->name);)
}


void gamestatemanager_t::pop()
{
	gamestate_t *oldtop = top;
	if(top)
		top = top->next;
	if(oldtop)
	{
		oldtop->leave();
		oldtop->manager = 0;
		DBG(if(prefs.cmd_debug)
			printf("popped state '%s'\n", oldtop->name);)
	}
	if(top)
		top->reenter();
}


gamestate_t *gamestatemanager_t::current()
{
	return top;
}


gamestate_t *gamestatemanager_t::previous()
{
	if(top)
		return top->next;
	else
		return NULL;
}


void gamestatemanager_t::press(int button, int unicode)
{
	if(top)
	{
		top->unicode = unicode;
		top->press(button);
	}
}


void gamestatemanager_t::release(int button, int unicode)
{
	if(top)
	{
		top->unicode = unicode;
		top->release(button);
	}
}


void gamestatemanager_t::pos(int x, int y)
{
	if(top)
		top->pos(x, y);
}


void gamestatemanager_t::delta(int dx, int dy)
{
	if(top)
		top->delta(dx, dy);
}


void gamestatemanager_t::frame()
{
	if(top)
		top->frame();
}


void gamestatemanager_t::pre_render(window_t *win)
{
	if(top)
		top->pre_render(win);
}


void gamestatemanager_t::post_render(window_t *win)
{
	if(top)
		top->post_render(win);
}

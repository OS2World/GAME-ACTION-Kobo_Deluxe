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

#ifndef	_KOBO_GAMESTATE_H_
#define	_KOBO_GAMESTATE_H_

#include "window.h"

class gamestate_t
{
	friend class gamestatemanager_t;
  private:
	gamestate_t		*next;		//LIFO stack
	gamestatemanager_t	*manager;
  protected:
	const char		*name;
	int			unicode;	//for last press()/release()
	void pop();

	virtual void press(int button);
	virtual void release(int button);
	virtual void pos(int x, int y);
	virtual void delta(int dx, int dy);

	virtual void enter();
	virtual void leave();
	virtual void yield();
	virtual void reenter();

	virtual void frame();			//Control system stuff
	virtual void pre_render(window_t *win);	//Background rendering
	virtual void post_render(window_t *win);//Foreground rendering
  public:
	gamestate_t();
	virtual ~gamestate_t();
};


class gamestatemanager_t
{
  private:
	gamestate_t	*top;		//Stack top
  public:
	gamestatemanager_t();
	~gamestatemanager_t();

	/* Event router interface */
	void press(int button, int unicode = 0);
	void release(int button, int unicode = 0);
	void pos(int x, int y);
	void delta(int dx, int dy);

	/* CS frame and rendering callbacks */
	void frame();
	void pre_render(window_t *win);
	void post_render(window_t *win);

	/* State management */
	void change(gamestate_t *gs);
	void push(gamestate_t *gs);
	void pop();
	gamestate_t *current();
	gamestate_t *previous();
};

#endif

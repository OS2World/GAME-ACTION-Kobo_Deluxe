/*(LGPL)
---------------------------------------------------------------------------
	window.cpp - Generic Rendering Window
---------------------------------------------------------------------------
 * Copyright (C) 2001, 2002, David Olofson
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "window.h"
#include "gfxengine.h"
#include "sofont.h"

#define	SELECT	if(selected != this) _select();

window_t *window_t::selected = NULL;

window_t::window_t()
{
	engine = NULL;
	rect.x = rect.y = 0;
	rect.w = 320;
	rect.h = 240;
	vx = vy = 0;
	gvx = gvy = 0;
	__gvx = __gvy = 0;
	vw = vh = 512;
	fgcolor = bgcolor = 0;
	selected = 0;
	xsc = ysc = 1;
	bg_bank = -1;
	bg_frame = -1;
}


window_t::~window_t()
{
	if(selected == this)
		selected = NULL;
}


void window_t::init(gfxengine_t *e)
{
	engine = e;
	xsc = e->xsc;
	ysc = e->ysc;
}


void window_t::place(int left, int top,
		int sizex, int sizey,
		int vsizex, int vsizey)
{
	rect.x = left * xsc;
	rect.y = top * ysc;
	rect.w = sizex * xsc;
	rect.h = sizey * ysc;
	vx = 0;
	vy = 0;
	gvx = 0;
	gvy = 0;
	__gvx = 0;
	__gvy = 0;

	if(vsizex < 0)
		vw = rect.w;
	else
		vw = vsizex * xsc;

	if(vsizey < 0)
		vh = rect.h;
	else
		vh = vsizey * ysc;
}


void window_t::select()
{
	if(engine)
		_select();
}


void window_t::_select()
{
	SDL_Rect r = rect;
	selected = this;
	if(engine->surface())
		SDL_SetClipRect(engine->surface(), &r);
}


void window_t::invalidate(SDL_Rect * r)
{
	if(!engine)
		return;
	if(!engine->screen)
		return;

	SELECT
	if(!r)
		engine->invalidate(&rect);
	else
	{
		/* Translate to screen coordinates */
		SDL_Rect dr = *r;
		dr.x *= xsc;
		dr.y *= ysc;
		dr.w *= xsc;
		dr.h *= ysc;
		dr.x += rect.x;
		dr.y += rect.y;

		/* Clip to window (stolen from SDL_surface.c) */
		int Amin, Amax, Bmin, Bmax;

		/* Horizontal intersection */
		Amin = dr.x;
		Amax = Amin + dr.w;
		Bmin = rect.x;
		Bmax = Bmin + rect.w;
		if(Bmin > Amin)
			Amin = Bmin;
		dr.x = Amin;
		if(Bmax < Amax)
			Amax = Bmax;
		dr.w = Amax - Amin > 0 ? Amax - Amin : 0;

		/* Vertical intersection */
		Amin = dr.y;
		Amax = Amin + dr.h;
		Bmin = rect.y;
		Bmax = Bmin + rect.h;
		if(Bmin > Amin)
			Amin = Bmin;
		dr.y = Amin;
		if(Bmax < Amax)
			Amax = Bmax;
		dr.h = Amax - Amin > 0 ? Amax - Amin : 0;

		if(dr.w && dr.h)
			engine->invalidate(&dr);
	}
}


#if 0
void window_t::set_position(int vposx, int vposy)
{
	vx = vposx;
	vy = vposy;
	if(engine->output() != this)
	{
		gvx = vx;
		gvy = vy;
	}
}


void window_t::force_position(int vposx, int vposy)
{
	set_position(vposx, vposy);
	engine->force_scroll();
}
#endif


/*---------------------------------------------------------------
	Rendering API
---------------------------------------------------------------*/

Uint32 window_t::map_rgb(Uint8 r, Uint8 g, Uint8 b)
{
	if(!engine)
		return 0;

	if(engine->surface())
		return SDL_MapRGB(engine->surface()->format, r, g, b);
	else
		return 0xffffff;
}

Uint32 window_t::map_rgb(Uint32 rgb)
{
	if(!engine)
		return 0;

	Uint8 r = (rgb >> 16) & 0xff;
	Uint8 g = (rgb >> 8) & 0xff;
	Uint8 b = rgb & 0xff;

	if(engine->surface())
		return SDL_MapRGB(engine->surface()->format, r, g, b);
	else
		return 0xffffff;
}

void window_t::foreground(Uint32 color)
{
	fgcolor = color;
}

void window_t::background(Uint32 color)
{
	bgcolor = color;
}

void window_t::bgimage(int bank, int frame)
{
	bg_bank = bank;
	bg_frame = frame;
}


void window_t::font(int fnt)
{
	_font = fnt;
}


void window_t::string(int _x, int _y, const char *txt)
{
	string_fxp(PIXEL2CS(_x), PIXEL2CS(_y), txt);
}


void window_t::center(int _y, const char *txt)
{
	center_fxp(PIXEL2CS(_y), txt);
}


void window_t::center_token(int _x, int _y, const char *txt, char token)
{
	center_token_fxp(PIXEL2CS(_x), PIXEL2CS(_y), txt, token);
}


void window_t::string_fxp(int _x, int _y, const char *txt)
{
	if(!engine)
		return;

	_x = CS2PIXEL(_x*xsc);
	_y = CS2PIXEL(_y*ysc);
	SoFont *f = engine->get_font(_font);
	if(!f)
		return;
	SELECT
	_x += rect.x - gvx;
	_y += rect.y - gvy;
	if(engine->surface())
		f->PutString(engine->surface(), _x, _y, txt);
}


void window_t::center_fxp(int _y, const char *txt)
{
	_y = CS2PIXEL(_y*ysc);

	if(!engine)
		return;

	SoFont *f = engine->get_font(_font);
	if(!f)
		return;
	SELECT
	int _x = (rect.w - f->TextWidth(txt)) / 2;
	_x += rect.x - gvx;
	_y += rect.y - gvy;
	if(engine->surface())
		f->PutString(engine->surface(), _x, _y, txt);
}


void window_t::center_token_fxp(int _x, int _y, const char *txt, char token)
{
	_x = CS2PIXEL(_x*xsc);
	_y = CS2PIXEL(_y*ysc);

	if(!engine)
		return;

	SoFont *f = engine->get_font(_font);
	if(!f)
		return;
	SELECT
	int _cx;
	if(-1 == token)
		_cx = _x - f->TextWidth(txt)/2;
	else
	{
		int tokpos;
		/*
		 * My docs won't say if strchr(???, 0) is legal
		 * or even defined, so I'm not taking any chances...
		 */
		if(token)
		{
			char *tok = strchr(txt, token);
			if(tok)
				tokpos = tok-txt;
			else
				tokpos = 255;
		}
		else
			tokpos = 255;
		_cx = _x - f->TextWidth(txt, 0, tokpos);
	}
	_cx += rect.x - gvx;
	_y += rect.y - gvy;
	if(engine->surface())
		f->PutString(engine->surface(), _cx, _y, txt);
}


int window_t::textwidth(const char *txt, int min, int max)
{
	if(!engine)
		return strlen(txt);

	SoFont *f = engine->get_font(_font);
	if(!f)
		return strlen(txt);

	return f->TextWidth(txt, min, max) / xsc;
}


int window_t::fontheight()
{
	if(!engine)
		return 1;

	SoFont *f = engine->get_font(_font);
	if(!f)
		return 1;

	return f->FontHeight() / ysc;
}


void window_t::clear(SDL_Rect *r)
{
	SDL_Rect sr, dr;
	if(!engine)
		return;
	if(!engine->surface())
		return;
	SELECT
	if(!r)
	{
		sr = rect;
		sr.x = 0;
		sr.y = 0;
		dr = rect;
	}
	else
	{
		sr.x = r->x * xsc;
		sr.w = r->w * xsc;
		sr.y = r->y * ysc;
		sr.h = r->h * ysc;
		dr = sr;
		dr.x += rect.x;
		dr.y += rect.y;
	}
	if((-1 == bg_bank) && (-1 == bg_frame))
		SDL_FillRect(engine->surface(), &dr, bgcolor);
	else
	{
		SDL_Surface *img;
		img = engine->get_sprite(bg_bank, bg_frame);
		if(!img)
		{
			SDL_FillRect(engine->surface(), &dr, bgcolor);
			return;
		}

		SDL_BlitSurface(img, &sr, engine->surface(), &dr);
	}
}


void window_t::point(int _x, int _y)
{
	_x *= xsc;
	_y *= ysc;

	if(!engine)
		return;
	SELECT
	/* Quick hack; slow */
	SDL_Rect r;
	r.x = rect.x + _x - gvx;
	r.y = rect.y + _y - gvy;
	r.w = xsc;
	r.h = ysc;
	if(engine->surface())
		SDL_FillRect(engine->surface(), &r, fgcolor);
}


void window_t::rectangle(int _x, int _y, int w, int h)
{
	_x *= xsc;
	_y *= ysc;
	w *= xsc;
	h *= ysc;

	if(!engine)
		return;
	if(!engine->surface())
		return;

	SELECT
	SDL_Rect r;
	r.x = rect.x + _x - gvx;
	r.y = rect.y + _y - gvy;
	r.w = w;
	r.h = ysc;
	SDL_FillRect(engine->surface(), &r, fgcolor);

	r.y = rect.y + _y+h-ysc - gvy;
	SDL_FillRect(engine->surface(), &r, fgcolor);

	r.x = rect.x + _x - gvx;
	r.y = rect.y + _y+ysc - gvy;
	r.w = xsc;
	r.h = h-2*ysc;
	SDL_FillRect(engine->surface(), &r, fgcolor);

	r.x = rect.x + _x+w-xsc - gvx;
	SDL_FillRect(engine->surface(), &r, fgcolor);
}


void window_t::fillrect(int _x, int _y, int w, int h)
{
	_x *= xsc;
	_y *= ysc;
	w *= xsc;
	h *= ysc;

	if(!engine)
		return;
	SELECT
	SDL_Rect r;
	r.x = rect.x + _x - gvx;
	r.y = rect.y + _y - gvy;
	r.w = w;
	r.h = h;
	if(engine->surface())
		SDL_FillRect(engine->surface(), &r, fgcolor);
}


void window_t::fillrect_fxp(int _x, int _y, int w, int h)
{
	_x = CS2PIXEL(_x*xsc);
	_y = CS2PIXEL(_y*ysc);
	w = CS2PIXEL(w*xsc);
	h = CS2PIXEL(h*ysc);

	if(!engine)
		return;
	SELECT
	SDL_Rect r;
	r.x = rect.x + _x - gvx;
	r.y = rect.y + _y - gvy;
	r.w = w;
	r.h = h;
	if(engine->surface())
		SDL_FillRect(engine->surface(), &r, fgcolor);
}


void window_t::sprite(int _x, int _y, int bank, int frame, int inval)
{
	sprite_fxp(PIXEL2CS(_x), PIXEL2CS(_y), bank, frame, inval);
}


void window_t::sprite_fxp(int _x, int _y, int bank, int frame, int inval)
{
	if(!engine)
		return;
	_x = CS2PIXEL(_x*xsc);
	_y = CS2PIXEL(_y*ysc);
	SDL_Rect dest_rect;
	SDL_Surface *img;
	img = engine->get_sprite(bank, frame);
	if(!img)
		return;

	SELECT
	dest_rect.x = rect.x + _x - gvx;
	dest_rect.y = rect.y + _y - gvy;
	if(engine->surface())
		SDL_BlitSurface(img, NULL, engine->surface(), &dest_rect);

	if(inval && !engine->autoinvalidate)
	{
		dest_rect.w = img->w;
		dest_rect.h = img->h;
		engine->invalidate(&dest_rect);
	}
}

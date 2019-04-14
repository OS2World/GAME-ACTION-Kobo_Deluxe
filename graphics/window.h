/*(LGPL)
---------------------------------------------------------------------------
	window.h - Generic Rendering Window
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

/*
 * Programmer's Documentation:
 *	The window_t class represents a simple window of the
 *	screen surface. It provides some high level rendering
 *	functions that operate inside the window's bounding
 *	rectangle. All operations are clipped. However,
 *	window occlusion is *not* handled, so you can't use
 *	this class for "real" windows.
 *
 *	virtual void init(gfxengine_t *e);
 *		Call this to connect a window_t instance to
 *		the gfxengine_t instance that manages your
 *		screen. You *must* call this for pretty much
 *		any other calls to work.
 *
 *	void place(int left, int top,
 *			int sizex, int sizey,
 *			int vsizex = -1, int vsizey = -1);
 *		Position the window, and set the virtual
 *		size for scrolling. (Note that scrolling is
 *		currently not controllable by any public API.
 *		However, gfxengine_t *does* use it for the
 *		window_t that serves as the engine's output
 *		window.)
 *
 *	void select();
 *		Makes this window active, and sets up SDL's
 *		clipping appropriately. You should not use
 *		this call normally, but it's provided to
 *		allow this engine to interface with other
 *		SDL code.
 *
 *	void invalidate(SDL_Rect *r = NULL);
 *		Invalidate the specified area, or the whole
 *		window if r is NULL. This will ensure that
 *		any changes in the invalidated area are made
 *		visible on the screen.
 *
 *	Uint32 map_rgb(Uint8 r, Uint8 g, Uint8 b);
 *	Uint32 map_rgb(Uint32 rgb);
 *		Two different ways of translating a color in
 *		standard RGB format (separate components or
 *		HTML style hex code, respectively) into a
 *		pixel value compatible with the screen.
 *
 *	void foreground(Uint32 color);
 *	void background(Uint32 color);
 *		Change the foreground and background colors
 *		used by subsequent rendering operations.
 *		Note that the arguments have to be in screen
 *		pixel format - the easiest way is to use the
 *		map_rgb() calls to generate the values.
 *
 *	void bgimage(int bank, int frame);
 *		Select a background image, to use instead of
 *		a background color. Setting bank and frame to
 *		-1 (default) disables the image, so that the
 *		background color is used instead.
 *
 *	void clear(SDL_Rect *r = NULL);
 *		Fill the specified rectangle with the current
 *		background color or background image. If r is
 *		NULL (default), clear the whole window.
 *
 *	void font(int fnt);
 *		Select the font to be used for subsequent
 *		text rendering operations. The argument is a
 *		gfxengine_t bank index, and must refer to a
 *		bank that has been loaded with
 *		gfxengine_t::loadfont().
 *
 *	void string(int _x, int _y, const char *txt);
 *		Put text string 'txt' starting at (_x, _y).
 *
 *	void center(int _y, const char *txt);
 *		Put text string 'txt' centered horizontally
 *		inside the window, using '_y' as the y coord.
 *
 *	void center_token(int _x, int _y, const char *txt, char token = 0);
 *		Print string 'txt', positioning it so that
 *		the first occurrence of character 'token'
 *		is located at (_x, _y). Omitting the 'token'
 *		argument, or passing 0 aligns the end of the
 *		string with _x. Setting 'token' to -1 results
 *		in the graphical center of the string being
 *		located at (_x, _y) - basically an advanced
 *		version of center().
 *
 *	void string_fxp(int _x, int _y, const char *txt);
 *	void center_fxp(int _y, const char *txt);
 *	void center_token_fxp(int _x, int _y, const char *txt, char token = 0);
 *		gfxengine_t fixed point format versions of
 *		the corresponding non-fxp calls. (For sub-
 *		pixel accurate rendering and/or to make use
 *		of the higher resolutions in scaled modes.)
 *
 *	int textwidth(const char *txt, int min = 0, int max = 255);
 *		Calculates the graphical width of 'txt' if it
 *		was to be printed with the current font.
 *		Calculation starts at position 'min' in the
 *		string, and ends at 'max', which makes it
 *		possible to use this call for various
 *		advanced alignment calculations.
 *
 *	int fontheight();
 *		Returns the height of the currently selected
 *		font.
 *
 *	void point(int _x, int _y);
 *		Plot a pixel with the current foreground
 *		color at (_x, _y). Note that the pixel will
 *		become a quadratic block if the screen is
 *		scaled to a higher resolution.
 *
 *	void rectangle(int _x, int _y, int w, int h);
 *		Draw a hollow rectangle of w x h pixels at
 *		(_x, _y). The line thickness is scaled along
 *		with screen resolution, as with point().
 *
 *	void fillrect(int _x, int _y, int w, int h);
 *		Draw a solid rectangle of w x h pixels at
 *		(_x, _y).
 *
 *	void fillrect_fxp(int _x, int _y, int w, int h);
 *		As fillrect(), but with pixel or sub-pixel
 *		accuracy, depending on scaling and video
 *		driver.
 *
 *	void sprite(int _x, int _y, int bank, int frame, int inval = 1);
 *		Render sprite 'bank':'frame' at (_x, _y). If
 *		inval is passed and set to 0, the affected
 *		area will *not* be invalidated automatically.
 *
 *	void sprite_fxp(int _x, int _y, int bank, int frame, int inval = 1);
 *		As sprite(), but with pixel or sub-pixel
 *		accuracy, depending on scaling and video
 *		driver.
 *
 *	int x()		{ return rect.x / xsc; }
 *	int y()		{ return rect.y / ysc; }
 *	int width()	{ return rect.w / xsc; }
 *	int height()	{ return rect.h / ysc; }
 *		Get the current screen position of the
 *		window's top-left corden, and it's size.
 *
 *	int x2()	{ return (rect.x + rect.w) / xsc; }
 *	int y2()	{ return (rect.y + rect.h) / ysc; }
 *		Get the screen position of the bottom-right
 *		corner of the window.
 */

#ifndef _WINDOW_H_
#define _WINDOW_H_

#include "glSDL.h"

class gfxengine_t;

class window_t
{
	friend class gfxengine_t;
  public:
	window_t();
	virtual ~window_t();

	virtual void init(gfxengine_t *e);
	void place(int left, int top ,
			int sizex, int sizey,
			int vsizex = -1, int vsizey = -1);
	void select();
	void invalidate(SDL_Rect *r = NULL);

	/* Rendering */
	Uint32 map_rgb(Uint8 r, Uint8 g, Uint8 b);
	Uint32 map_rgb(Uint32 rgb);
	void foreground(Uint32 color);
	void background(Uint32 color);
	void bgimage(int bank = -1, int frame = -1);

	void clear(SDL_Rect *r = NULL);
	void font(int fnt);
	void string(int _x, int _y, const char *txt);
	void center(int _y, const char *txt);
	void center_token(int _x, int _y, const char *txt, char token = 0);
	void string_fxp(int _x, int _y, const char *txt);
	void center_fxp(int _y, const char *txt);
	void center_token_fxp(int _x, int _y, const char *txt, char token = 0);
	int textwidth(const char *txt, int min = 0, int max = 255);
	int fontheight();

	void point(int _x, int _y);
	void rectangle(int _x, int _y, int w, int h);
	void fillrect(int _x, int _y, int w, int h);
	void fillrect_fxp(int _x, int _y, int w, int h);

	void sprite(int _x, int _y, int bank, int frame, int inval = 1);
	void sprite_fxp(int _x, int _y, int bank, int frame, int inval = 1);

	int x()		{ return rect.x / xsc; }
	int y()		{ return rect.y / ysc; }
	int width()	{ return rect.w / xsc; }
	int height()	{ return rect.h / ysc; }
	int x2()	{ return (rect.x + rect.w) / xsc; }
	int y2()	{ return (rect.y + rect.h) / ysc; }

  protected:
	/*
	 * Kludge: This is for window selection to work.
	 * the problem is that this will break (along
	 * with some other things) if more than one
	 * gfxengine_t instance is used.
	 */
	static window_t	*selected;	//Currently selected window

	gfxengine_t	*engine;
	int		xsc, ysc;
	SDL_Rect	rect;
	int		vx, vy;
	int		gvx, gvy;
	int		__gvx, __gvy;
	int		vw, vh;
	Uint32		fgcolor, bgcolor;
	int		bg_bank, bg_frame;
	int		_font;

	void _select();		//Internal version
};

#endif

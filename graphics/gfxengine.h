/*(LGPL)
----------------------------------------------------------------------
	gfxengine.h - Graphics Engine
----------------------------------------------------------------------
 * Copyright (C) David Olofson, 2001, 2002
 * This code is released under the terms of the GNU LGPL.
 */

#ifndef	_GFXENGINE_H_
#define	_GFXENGINE_H_

#define GFX_BANKS	256
#define	MAX_DIRTYRECTS	1024

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}
#include "glSDL.h"

#include "sprite.h"
#include "cs.h"

enum gfx_drivers_t
{
	GFX_DRIVER_SDL2D =	0,
	GFX_DRIVER_OPENGLBUF =	1,
	GFX_DRIVER_OPENGL_INT =	2,
	GFX_DRIVER_OPENGL =	3
};

enum gfx_scalemodes_t
{
	GFX_SCALE_NEAREST =	0,
	GFX_SCALE_BILINEAR =	1,
	GFX_SCALE_BILIN_OVER =	2,
	GFX_SCALE_SCALE2X =	3,
	GFX_SCALE_DIAMOND =	4
};

enum gfx_buffermodes_t
{
	GFX_BUFFER_SINGLE =	0,
	GFX_BUFFER_DOUBLE =	1,
	GFX_BUFFER_SEMITRIPLE =	2,
	GFX_BUFFER_HALF =	3
};

enum gfx_syncmodes_t
{
	GFX_SYNC_FLIP =		0,
	GFX_SYNC_TIMER =	1,
	GFX_SYNC_RETRACE =	2
};

class window_t;
class SoFont;

class gfxengine_t
{
	friend class window_t;
  public:
	gfxengine_t();
	virtual ~gfxengine_t();

	void output(window_t *outwin);
	window_t *output()	{ return window; }

	/* Initialization */
	void size(int w, int h);
	void scale(int x, int y);
	void scalemode(gfx_scalemodes_t sm, int clamping = 0);
	void driver(gfx_drivers_t drv);
	void mode(int bits, int fullscreen);
	void buffering(gfx_buffermodes_t bm, gfx_syncmodes_t sm, int ainv = 1);
	void interpolation(int inter);
	void period(float frameduration); // 0 to reset internal timer
	void wrap(int x, int y);

	/* Info */
	gfx_buffermodes_t buffering()	{ return buffermode; }

	/* Engine open/close */
	int open(int objects = 1024, int extraflags = 0);
	void close();

	/* Data management (use while engine is open) */
	void colorkey(Uint8 r, Uint8 g, Uint8 b);
	void clampcolor(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
	void dither(int on = 1, int _broken_rgba8 = 0);
	int loadimage(int bank, const char *name);
	int loadsprites(int bank, int w, int h, const char *name);
	int loadtiles(int bank, int w, int h, const char *name);
	int loadfont(int bank, const char *name);
	int loadrect(int bank, int sbank, int sframe, SDL_Rect *r);
	void reload();
	void unload(int bank = -1);

	/* Settings (use while engine is open) */
	void title(const char *win, const char *icon);

	/* Display show/hide */
	int show();
	void hide();

	/* Main loop take-over */
	void run();

	/*
	 * Override these;
	 *	frame() is called once per control system frame,
	 *		after the control system has executed.
	 *	pre_render() is called after the engine has advanced
	 *		to the state for the current video frame
	 *		(interpolated state is calculated), before
	 *		the engine renders all graphics.
	 *	post_render() is called after the engine have
	 *		rendered all sprites, but before video the
	 *		sync/flip/update operation.
	 */
	virtual void frame();
	virtual void pre_render();
	virtual void post_render();

	/*
	---------------------------------------------
	 * The members below this line can safely be
	 * called from within the frame() handler.
	 */

	/* Rendering */
	void invalidate(SDL_Rect *rect = NULL);

	/* Screenshots */
	void screenshot();

	/* Control */
	cs_engine_t *cs()	{ return csengine; }
	SDL_Surface *surface();
	void flip();
	void stop();
	cs_obj_t *get_obj(int layer);
	void free_obj(cs_obj_t *obj);
	void cursor(int csr);

	SDL_Surface *get_sprite(int bank, int _frame);
	SoFont *get_font(unsigned int f);

	void scroll_ratio(int layer, float xr, float yr);
	void scroll(int xs, int ys);
	void force_scroll();

	int xoffs(int layer);
	int yoffs(int layer);

	/* Info */
	int objects_in_use();

  protected:
	gfx_drivers_t		_driver;
	gfx_buffermodes_t	buffermode;
	gfx_syncmodes_t		syncmode;
	gfx_scalemodes_t	_scalemode;
	SDL_Surface	*screen;
	SDL_Surface	*softbuf;
	int		backpage;
	int		frontpage;
	int		dirtyrects[2];
	SDL_Rect	dirtytable[2][MAX_DIRTYRECTS];
	window_t	*window;
	int		wx, wy;
	int		xsc, ysc;
	s_filter_t	*sf1, *sf2;	// Scaling filter plugins
	s_filter_t	*df;		// Dither filter plugin
	int		xscroll, yscroll;
	float		xratio[CS_LAYERS];
	float		yratio[CS_LAYERS];
	s_container_t	*gfx;
	SoFont		*fonts[GFX_BANKS];	// Kludge.
	cs_engine_t	*csengine;
	int		xflags;
	int		_fullscreen;
	int		use_interpolation;
	int		_width, _height;
	int		_depth;
	const char	*_title;
	const char	*_icontitle;
	int		_cursor;
	int		autoinvalidate;
	int		_dither;
	int		broken_rgba8;	//Klugde for OpenGL (if RGBA8 ==> RGBA4)

	int		start_tick;
	float		ticks_per_frame;

	int		is_showing;
	int		is_running;
	int		is_open;

	int		screenshot_count;

	void __invalidate(int page, SDL_Rect *rect = NULL);

	static void on_frame(cs_engine_t *e);
	void __frame();

	void sync_half();
	void sync();
	void start_engine();
	void stop_engine();
	static void render_sprite(cs_obj_t *o);
};


extern gfxengine_t *gfxengine;

#endif

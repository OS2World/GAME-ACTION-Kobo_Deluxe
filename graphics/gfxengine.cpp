/*(LGPL)
----------------------------------------------------------------------
	gfxengine.cpp - Graphics Engine
----------------------------------------------------------------------
 * Copyright (C) David Olofson, 2001, 2002
 * This code is released under the terms of the GNU LGPL.
 */

#define	DBG(x)	x

#include "config.h"

#include <string.h>
#include <math.h>

#include "gfxengine.h"
#include "SDL_image.h"
#include "glSDL.h"
#include "sofont.h"
#include "window.h"

#ifdef	USE_VIDSYNC
#include "vidsync.h"
#endif

gfxengine_t *gfxengine;


gfxengine_t::gfxengine_t()
{
	gfxengine = this;	/* Uurgh! Kludge. */

	screen = NULL;
	softbuf = NULL;
	window = NULL;
	wx = wy = 0;
	xsc = ysc = 1;
	sf1 = sf2 = df = NULL;
	gfx = NULL;
	csengine = NULL;
	_driver = GFX_DRIVER_SDL2D;
	buffermode = GFX_BUFFER_SEMITRIPLE;
	syncmode = GFX_SYNC_FLIP;
	_fullscreen = 0;
	use_interpolation = 1;
	_depth = 0;
	_title = "GfxEngine v0.2";
	_icontitle = "GfxEngine";
	_cursor = 1;
	_width = 320;
	_height = 240;
	autoinvalidate = 1;
	xflags = 0;

	_dither = 0;
	broken_rgba8 = 0;

	start_tick = 0;
	ticks_per_frame = 1000.0/60.0;

	is_running = 0;
	is_showing = 0;
	is_open = 0;

	memset(fonts, 0, sizeof(fonts));

	xscroll = yscroll = 0;
	for(int i = 0; i < CS_LAYERS ; ++i)
		xratio[i] = yratio[i] = 0.0;

	dirtyrects[0] = 0;
	dirtyrects[1] = 0;
	frontpage = 0;
	backpage = 1;
	screenshot_count = 0;
}


gfxengine_t::~gfxengine_t()
{
	stop();
	hide();
	close();
}


void gfxengine_t::output(window_t *outwin)
{
	window = outwin;
}


/*----------------------------------------------------------
	Initialization
----------------------------------------------------------*/

void gfxengine_t::size(int w, int h)
{
	int was_showing = is_showing;
	hide();

	_width = w;
	_height = h;
	if(csengine)
		cs_engine_set_size(csengine, w, h);

	if(was_showing)
		show();
}

void gfxengine_t::scale(int x, int y)
{
	xsc = x;
	ysc = y;
}

void gfxengine_t::scalemode(gfx_scalemodes_t sm, int clamping)
{
	_scalemode = sm;
	if(!sf1 || !sf2 || !df)
		return;

	//Set up dither filter
	switch(clamping)
	{
	  case 0:
	  case 1:
		df->args.y = 0;
		break;
	  case 2:
		df->args.y = SF_CLAMP_SFONT;
		break;
	}

	// Filter 2 is off in most cases
	sf2->args.x = 0;
	sf2->args.y = 0;
	sf2->args.fx = 0.0;
	sf2->args.fy = 0.0;

	if((xsc == 1) && (ysc == 1))
	{
		sf1->args.x = SF_SCALE_NEAREST;
		sf1->args.y = 0;
		sf1->args.fx = 1.0;
		sf1->args.fy = 1.0;
		return;
	}

	switch(clamping)
	{
	  case 0:
		sf1->args.y = 0;
		sf2->args.y = 0;
		break;
	  case 1:
		sf1->args.y = SF_CLAMP_EXTEND;
		sf2->args.y = SF_CLAMP_EXTEND;
		break;
	  case 2:
		sf1->args.y = SF_CLAMP_SFONT;
		sf2->args.y = SF_CLAMP_SFONT;
		break;
	}

	sf1->args.fx = xsc;
	sf1->args.fy = ysc;

	switch(_scalemode)
	{
	  case GFX_SCALE_NEAREST:	//Nearest
		sf1->args.x = SF_SCALE_NEAREST;
		break;
	  case GFX_SCALE_BILINEAR:	//Bilinear
	  case GFX_SCALE_BILIN_OVER:	//Bilinear + Oversampling
		sf1->args.x = SF_SCALE_BILINEAR;
		sf2->args.x = SF_SCALE_BILINEAR;
		sf1->args.fx = xsc * 1.5;
		sf1->args.fy = ysc * 1.5;
		sf2->args.fx = 1.0/1.5;
		sf2->args.fy = 1.0/1.5;
		break;
		/* Hack for better bilinear [+ oversampling] scaling... */
		switch(clamping)
		{
		  case 2:
			sf1->args.x = SF_SCALE_NEAREST;
			sf2->args.x = SF_SCALE_BILINEAR;
			switch(xsc)
			{
			  case 2:
				sf1->args.fx = 4.0;
				sf1->args.fy = 4.0;
				sf2->args.fx = 0.5;
				sf2->args.fy = 0.5;
				break;
			  case 3:
				sf1->args.fx = 2.0;
				sf1->args.fy = 2.0;
				sf2->args.fx = 1.5;
				sf2->args.fy = 1.5;
				break;
			  case 4:
				sf1->args.fx = 2.0;
				sf1->args.fy = 2.0;
				sf2->args.fx = 2.0;
				sf2->args.fy = 2.0;
				break;
			  default:
				sf1->args.fx = xsc * 2.0;
				sf1->args.fy = ysc * 2.0;
				sf2->args.fx = 0.5;
				sf2->args.fy = 0.5;
				break;
			}
			break;
		  default:
			sf1->args.x = SF_SCALE_BILINEAR;
			break;
		}
		break;
	  case GFX_SCALE_SCALE2X:	//Scale2x
		sf1->args.x = SF_SCALE_SCALE2X;
		break;
	  case GFX_SCALE_DIAMOND:	//Diamond2x
		sf1->args.x = SF_SCALE_DIAMOND;
		break;
	}
}

void gfxengine_t::mode(int bits, int fullscreen)
{
	int was_showing = is_showing;
	hide();

	int olddepth = _depth;
	_depth = bits;
	if(_depth != olddepth)
		reload();
	_fullscreen = fullscreen;

	if(was_showing)
		show();
}

void gfxengine_t::driver(gfx_drivers_t drv)
{
	int was_showing = is_showing;
	hide();

	_driver = drv;

	if(was_showing)
		show();
}

void gfxengine_t::buffering(gfx_buffermodes_t bm, gfx_syncmodes_t sm, int ainv)
{
	int was_showing = is_showing;
	hide();

	buffermode = bm;
	syncmode = sm;
	autoinvalidate = ainv;

	if(was_showing)
		show();
}

void gfxengine_t::period(float frameduration)
{
	if(frameduration > 0)
	{
		DBG(printf("Setting period to %f ms.\n", frameduration);)
		ticks_per_frame = frameduration;
	}
	else
		DBG(printf("Time line reset.\n");)	
	start_tick = SDL_GetTicks();
	if(csengine)
		cs_engine_advance(csengine, 0);
}

void gfxengine_t::wrap(int x, int y)
{
	wx = x;
	wy = y;
	if(csengine)
		cs_engine_set_wrap(csengine, x, y);
}


/*----------------------------------------------------------
	Data management
----------------------------------------------------------*/

void gfxengine_t::colorkey(Uint8 r, Uint8 g, Uint8 b)
{
	s_colorkey.r = r;
	s_colorkey.g = g;
	s_colorkey.b = b;
}


void gfxengine_t::clampcolor(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	s_clampcolor.r = r;
	s_clampcolor.g = g;
	s_clampcolor.b = b;
	s_clampcolor.a = b;
}


void gfxengine_t::dither(int on, int _broken_rgba8)
{
	_dither = on;
	broken_rgba8 = _broken_rgba8;

	if(!df)
		return;
	if(!screen)
		return;

	if(_dither)
	{
		if(_driver >= GFX_DRIVER_OPENGLBUF)
		{
			//Klugde for glSDL, which doesn't give us a faked
			//screen surface - while we're interested only in
			//the *texture* depth; not the display depth!
			df->args.r = df->args.g = df->args.b = 0;
			//Another kludge, because some cards support RGB8
			//(24 bit) textures, but not RGBA8 (32 bit).
			df->args.x = broken_rgba8;
		}
		else
		{
			df->args.x = 0;
			df->args.r = 1<<(screen->format->Rloss-1);
			df->args.g = 1<<(screen->format->Gloss-1);
			df->args.b = 1<<(screen->format->Bloss-1);
		}
	}
	else
		df->args.x = df->args.r = df->args.g = df->args.b = 0;
}


int gfxengine_t::loadimage(int bank, const char *name)
{
	if(!csengine)
	{
		fprintf(stderr, "loadimage: Engine must be open!\n");
		return -10;
	}
	s_blitmode = S_BLITMODE_AUTO;
	scalemode(_scalemode, 0);
	DBG(printf("Loading image %s (bank %d)... ", name, bank);)
	if(s_load_image(gfx, bank, name))
	{
		fprintf(stderr, "Failed to load %s!\n", name);
		return -1;
	}

	s_bank_t *b = s_get_bank(gfx, bank);
	if(!b)
	{ 
		fprintf(stderr, "gfxengine: Internal error 1!\n");
		return -15;
	}
	cs_engine_set_image_size(csengine, bank, b->w, b->h);

	DBG(printf("Ok.\n");)
	return 0;
}


int gfxengine_t::loadsprites(int bank, int w, int h, const char *name)
{
	if(!csengine)
	{
		fprintf(stderr, "loadsprites: Engine must be open!\n");
		return -10;
	}
	s_blitmode = S_BLITMODE_AUTO;
	scalemode(_scalemode, 0);
	DBG(printf("Loading sprites %s (bank %d; %dx%d)... ",
			name, bank, w, h);)
	if(s_load_bank(gfx, bank, w, h, name))
	{
		fprintf(stderr, "Failed to load %s!\n", name);
		return -2;
	}

	cs_engine_set_image_size(csengine, bank, w, h);

	DBG(printf("Ok. (%d frames)\n", s_get_bank(gfx, bank)->max+1);)
	return 0;
}


int gfxengine_t::loadtiles(int bank, int w, int h, const char *name)
{
	if(!csengine)
	{
		fprintf(stderr, "loadtiles: Engine must be open!\n");
		return -10;
	}
	s_blitmode = S_BLITMODE_AUTO;
	scalemode(_scalemode, 1);
	DBG(printf("Loading tiles %s (bank %d; %dx%d)... ",
			name, bank, w, h);)
	if(s_load_bank(gfx, bank, w, h, name))
	{
		fprintf(stderr, "Failed to load %s!\n", name);
		return -2;
	}

	cs_engine_set_image_size(csengine, bank, w, h);

	DBG(printf("Ok. (%d frames)\n", s_get_bank(gfx, bank)->max+1);)
	return 0;
}


int gfxengine_t::loadfont(int bank, const char *name)
{
	if(!csengine)
	{
		fprintf(stderr, "loadfont: Engine must be open!\n");
		return -10;
	}
	s_blitmode = S_BLITMODE_AUTO;
	scalemode(_scalemode, 2);
	DBG(printf("Loading font %s (bank %d)... ", name, bank);)
	if(s_load_image(gfx, bank, name))
	{
		fprintf(stderr, "Failed to load %s!\n", name);
		return -2;
	}
	if(bank >= GFX_BANKS)
	{
		fprintf(stderr, "Too high bank #!\n");
		return -3;
	}

	if(!fonts[bank])
		fonts[bank] = new SoFont;
	if(!fonts[bank])
	{
		fprintf(stderr, "Failed to instantiate SoFont!\n");
		return -4;
	}

	if(fonts[bank]->load(s_get_sprite(gfx, bank, 0)->surface))
	{
		DBG(printf("Ok.\n");)
		return 0;
	}
	else
	{
		fprintf(stderr, "SoFont::load() failed!\n");
		return -5;
	}
	return 0;
}


int gfxengine_t::loadrect(int bank, int sbank, int sframe, SDL_Rect *r)
{
	if(!csengine)
	{
		fprintf(stderr, "loadfont: Engine must be open!\n");
		return -10;
	}

	DBG(printf("Copying rect from %d:%d (bank %d)... ",
			sbank, sframe, bank);)

	if(s_load_rect(gfx, bank, sbank, sframe, r) < 0)
	{
		fprintf(stderr, "s_load_rect() failed!\n");
		return -1;
	}
	DBG(printf("Ok.\n");)
	return 0;
}


void gfxengine_t::reload()
{
	DBG(printf("Reloading all banks. (Not implemented!)\n");)
//	s_reload_all_banks(gfx);
}


void gfxengine_t::unload(int bank)
{
	if(bank < 0)
	{
		DBG(printf("Unloading all banks.\n");)
		for(int i = 0; i < GFX_BANKS; ++i)
		{
			delete fonts[i];
			fonts[i] = NULL;
		}
		if(gfx)
			s_delete_all_banks(gfx);
	}
	else
	{
		DBG(printf("Unloading bank %d.\n", bank);)
		if(bank < GFX_BANKS)
		{
			delete fonts[bank];
			fonts[bank] = NULL;
		}
		if(gfx)
			s_delete_bank(gfx, bank);
	}
}


/*----------------------------------------------------------
	Engine open/close
----------------------------------------------------------*/

void gfxengine_t::on_frame(cs_engine_t *e)
{
	gfxengine->__frame();
	gfxengine->frame();
}


int gfxengine_t::open(int objects, int extraflags)
{
	xflags = extraflags;

	if(is_open)
		return 0;

	DBG(printf("Opening engine...\n");)
	csengine = cs_engine_create(_width, _height, objects);
	if(!csengine)
	{
		fprintf(stderr, "Failed to set up control system engine!\n");
		return -1;
	}

	csengine->on_frame = on_frame;
	cs_engine_set_wrap(csengine, wx, wy);

	gfx = s_new_container(GFX_BANKS);
	if(!gfx)
	{
		fprintf(stderr, "Failed to set up graphics container!\n");
		cs_engine_delete(csengine);
		return -2;
	}

	s_filter_t *fi;
	s_remove_filter(NULL);

	s_add_filter(s_filter_rgba8);

	fi = s_add_filter(s_filter_key2alpha);
	fi->args.max = 1;

	sf1 = s_add_filter(s_filter_scale);
	sf2 = s_add_filter(s_filter_scale);
	scalemode(_scalemode);

	fi = s_add_filter(s_filter_cleanalpha);
	fi->args.min = 16;
	fi->args.max = 255-16;
	fi->args.fx = 1.25;
	fi->args.x = -16;

	df = s_add_filter(s_filter_dither);
	df->args.x = 0;
	df->args.y = 0;
	df->args.r = 0;
	df->args.g = 0;
	df->args.b = 0;

	s_add_filter(s_filter_displayformat);

	is_open = 1;
	return show();
}


void gfxengine_t::close()
{
	if(!is_open)
		return;

	DBG(printf("Closing engine...\n");)
	stop();
	hide();
	unload();
	cs_engine_delete(csengine);
	csengine = NULL;
	s_remove_filter(NULL);
	sf1 = sf2 = df = NULL;
	s_delete_container(gfx);
	gfx = NULL;
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	is_open = 0;
}


/*----------------------------------------------------------
	Settings
----------------------------------------------------------*/

void gfxengine_t::title(const char *win, const char *icon)
{
	_title = win;
	_icontitle = icon;
	if(screen)
		SDL_WM_SetCaption(_title, _icontitle);
}


/*----------------------------------------------------------
	Display show/hide
----------------------------------------------------------*/

int gfxengine_t::show()
{
	int flags = 0;

	if(is_showing)
		return 0;

	DBG(printf("Opening screen...\n");)
	if(SDL_InitSubSystem(SDL_INIT_VIDEO) == -1)
	{
		fprintf(stderr, "Failed to initialize SDL!\n");
		return -2;
	}

	switch(_driver)
	{
	  case GFX_DRIVER_SDL2D:
		/* Nothing extra */
		break;
	  case GFX_DRIVER_OPENGLBUF:
	  	flags |= SDL_GLSDL | SDL_DOUBLEBUF;
		buffermode = GFX_BUFFER_HALF;
		break;
	  case GFX_DRIVER_OPENGL_INT:
	  	flags |= SDL_GLSDL | SDL_DOUBLEBUF;
		buffermode = GFX_BUFFER_DOUBLE;
		break;
	  case GFX_DRIVER_OPENGL:
		fprintf(stderr, "GFX_DRIVER_OPENGL not yet implemented!\n");
		buffermode = GFX_BUFFER_DOUBLE;
		return -10;
	}

	switch(buffermode)
	{
	  case GFX_BUFFER_SINGLE:
	  	flags |= SDL_HWSURFACE;
		break;
	  case GFX_BUFFER_DOUBLE:
		flags |= SDL_HWSURFACE | SDL_DOUBLEBUF;
		break;
	  case GFX_BUFFER_SEMITRIPLE:
		flags |= SDL_HWSURFACE | SDL_DOUBLEBUF;
		break;
	  case GFX_BUFFER_HALF:
		flags |= SDL_SWSURFACE;
		break;
	}

	if(_fullscreen)
		flags |= SDL_FULLSCREEN;

	flags |= xflags;

	screen = SDL_SetVideoMode(_width, _height, _depth, flags);
	if(!screen)
	{
		fprintf(stderr, "Failed to open display!\n");
		return -3;
	}

	switch(buffermode)
	{
	  case GFX_BUFFER_SINGLE:
		break;
	  case GFX_BUFFER_DOUBLE:
	  case GFX_BUFFER_SEMITRIPLE:
		switch(_driver)
		{
		  case GFX_DRIVER_SDL2D:
			if(!(screen->flags & SDL_HWSURFACE))
			{
				buffermode = GFX_BUFFER_SINGLE;
				printf("WARNING: Failed to get hardware screen surface."
						"Switching to SINGLE buffer mode.\n");
			}
			break;
		  case GFX_DRIVER_OPENGLBUF:
		  case GFX_DRIVER_OPENGL_INT:
		  case GFX_DRIVER_OPENGL:
			buffermode = GFX_BUFFER_DOUBLE;
			break;
		}
		break;
	  case GFX_BUFFER_HALF:
		switch(_driver)
		{
		  case GFX_DRIVER_SDL2D:
		  case GFX_DRIVER_OPENGLBUF:
			break;
		  case GFX_DRIVER_OPENGL_INT:
		  case GFX_DRIVER_OPENGL:
			buffermode = GFX_BUFFER_SINGLE;
			printf("WARNING: Half buffering only supported in SDL 2D"
					" and OpenGL Buffer modes."
					"Switching to SINGLE buffer mode.\n");
			break;
		}
		break;
	}

	if(GFX_BUFFER_SEMITRIPLE == buffermode)
	{
		softbuf = SDL_CreateRGBSurface(SDL_SWSURFACE,
				_width, _height,
				screen->format->BitsPerPixel,
				screen->format->Rmask,
				screen->format->Gmask,
				screen->format->Bmask,
				screen->format->Amask);
		if(!softbuf)
		{
			fprintf(stderr, "Failed to open soft back buffer!\n");
			return -4;
		}
	}

	SDL_WM_SetCaption(_title, _icontitle);
	SDL_ShowCursor(_cursor);
	cs_engine_set_size(csengine, _width, _height);
	csengine->filter = use_interpolation;

	dither(_dither);

	is_showing = 1;
	return 0;
}


void gfxengine_t::hide(void)
{
	if(!is_showing)
		return;

	DBG(printf("Closing screen...\n");)
	stop();
	if(softbuf)
	{
		SDL_FreeSurface(softbuf);
		softbuf = NULL;
	}
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	screen = NULL;

	is_showing = 0;
}


void gfxengine_t::invalidate(SDL_Rect *rect)
{
	switch(buffermode)
	{
	  case GFX_BUFFER_SEMITRIPLE:
		__invalidate(1, rect);
	  case GFX_BUFFER_SINGLE:
		__invalidate(0, rect);
		break;
	  case GFX_BUFFER_DOUBLE:
	  case GFX_BUFFER_HALF:
		return;
	}
}


void gfxengine_t::__invalidate(int page, SDL_Rect *rect)
{
	if(!screen)
		return;

	if(!rect)
	{
		dirtyrects[page] = 1;
		dirtytable[page][0].x = 0;
		dirtytable[page][0].y = 0;
		dirtytable[page][0].w = screen->w;
		dirtytable[page][0].h = screen->h;
		return;
	}

	/* Clip to screen (stolen from SDL_surface.c) */
	SDL_Rect dr = *rect;
	int Amin, Amax, Bmin, Bmax;

	/* Horizontal intersection */
	Amin = dr.x;
	Amax = Amin + dr.w;
	Bmin = screen->clip_rect.x;
	Bmax = Bmin + screen->clip_rect.w;
	if(Bmin > Amin)
	        Amin = Bmin;
	dr.x = Amin;
	if(Bmax < Amax)
	        Amax = Bmax;
	dr.w = Amax - Amin > 0 ? Amax - Amin : 0;

	/* Vertical intersection */
	Amin = dr.y;
	Amax = Amin + dr.h;
	Bmin = screen->clip_rect.y;
	Bmax = Bmin + screen->clip_rect.h;
	if(Bmin > Amin)
	        Amin = Bmin;
	dr.y = Amin;
	if(Bmax < Amax)
	        Amax = Bmax;
	dr.h = Amax - Amin > 0 ? Amax - Amin : 0;

	if(!dr.w || !dr.h)
		return;

	for(int i = 0; i < dirtyrects[page]; ++i)
		if(memcmp(&dirtytable[page][i], &dr, sizeof(dr)) == 0)
			return;

	if(dirtyrects[page] < MAX_DIRTYRECTS - 1)
		dirtytable[page][dirtyrects[page]++] = dr;
	else
		fprintf(stderr, "gfxengine: Page %d out of dirtyrects!\n",
				page);
}


/*----------------------------------------------------------
	Engine start/stop
----------------------------------------------------------*/

void gfxengine_t::start_engine()
{
#ifdef	USE_VIDSYNC
	if(syncmode == GFX_SYNC_RETRACE)
		vidsync_init(0.3);
#endif
	start_tick = SDL_GetTicks();
}


void gfxengine_t::stop_engine()
{
#ifdef	USE_VIDSYNC
	if(syncmode == GFX_SYNC_RETRACE)
		vidsync_close();
#endif
}


/*----------------------------------------------------------
	Control
----------------------------------------------------------*/

/*
 * Run engine until stop() is called.
 *
 * The virtual member frame() will be called once for
 * each control system frame. pre_render() and
 * post_render() will be called before/after the engine
 * renders each video frame.
 */
void gfxengine_t::run()
{
	open();
	show();
	start_engine();
	is_running = 1;
	while(is_running)
	{
		int tick = SDL_GetTicks() - start_tick;
		float toframe = (float)tick / ticks_per_frame;
		cs_engine_advance(csengine, toframe);
		pre_render();
		window->select();
		cs_engine_render(csengine);
		post_render();
		if(autoinvalidate)
			window->invalidate();
		flip();
	}
	stop_engine();
}


void gfxengine_t::stop()
{
	if(!is_running)
		return;

	DBG(printf("Stopping engine...\n");)
	is_running = 0;
}


void gfxengine_t::cursor(int csr)
{
	_cursor = csr;
	if(screen)
		SDL_ShowCursor(csr);
}


void gfxengine_t::interpolation(int inter)
{
	use_interpolation = inter;
	if(!csengine)
		return;

	csengine->filter = use_interpolation;
}


void gfxengine_t::scroll_ratio(int layer, float xr, float yr)
{
	if(layer < 0)
		return;
	if(layer >= CS_LAYERS)
		return;

	xratio[layer] = xr;
	yratio[layer] = yr;
}


void gfxengine_t::scroll(int xs, int ys)
{
	xscroll = xs;
	yscroll = ys;

	/* Apply current scroll pos to layers */
	for(int i = 0; i < CS_LAYERS ; ++i)
	{
		csengine->offsets[i].v.x = (int)floor(xscroll * xratio[i]);
		csengine->offsets[i].v.y = (int)floor(yscroll * yratio[i]);
	}
}


void gfxengine_t::force_scroll()
{
	if(csengine)
		for(int i = 0; i < CS_LAYERS ; ++i)
			cs_point_force(&csengine->offsets[i]);
}


int gfxengine_t::xoffs(int layer)
{
	if(layer < 0)
		return 0;
	if(layer >= CS_LAYERS)
		return 0;
	return csengine->offsets[layer].gx;
}


int gfxengine_t::yoffs(int layer)
{
	if(layer < 0)
		return 0;
	if(layer >= CS_LAYERS)
		return 0;
	return csengine->offsets[layer].gy;
}


SDL_Surface *gfxengine_t::surface()
{
	if(softbuf)
		return softbuf;
	else
		return screen;
}


void gfxengine_t::screenshot()
{
	char filename[1024];
	snprintf(filename, sizeof(filename), "screen%d.bmp", screenshot_count++);
	switch(_driver)
	{
	  case GFX_DRIVER_SDL2D:
		SDL_SaveBMP(screen, filename);
		break;
	  case GFX_DRIVER_OPENGLBUF:
	  case GFX_DRIVER_OPENGL_INT:
	  case GFX_DRIVER_OPENGL:
		fprintf(stderr, "Screenshots in OpenGL modes not yet implemented!\n");
		break;
	}
}


/*----------------------------------------------------------
	Internal stuff
----------------------------------------------------------*/

/* Default frame handler */
void gfxengine_t::frame()
{
	SDL_Event ev;
	while(SDL_PollEvent(&ev))
	{
		if(ev.type == SDL_KEYDOWN)
			if(ev.key.keysym.sym == SDLK_ESCAPE)
				stop();
	}
}


/* Internal frame handler */
void gfxengine_t::__frame()
{
}


void gfxengine_t::pre_render()
{
}


void gfxengine_t::post_render()
{
}


void gfxengine_t::sync_half()
{
	switch(syncmode)
	{
	  case GFX_SYNC_FLIP:
		/* Not applicable. */
		break;
	  case GFX_SYNC_TIMER:
		/* Not yet implemented. */
		break;
	  case GFX_SYNC_RETRACE:
#ifdef	USE_VIDSYNC
		vidsync_wait(0.47);
#endif
		break;
	}
}

void gfxengine_t::sync()
{
	switch(syncmode)
	{
	  case GFX_SYNC_FLIP:
		/* Do nothing. */
		break;
	  case GFX_SYNC_TIMER:
		/* Not yet implemented. */
		break;
	  case GFX_SYNC_RETRACE:
#ifdef	USE_VIDSYNC
		vidsync_wait(0);
#endif
		break;
	}
}

void gfxengine_t::flip()
{
	if(!screen)
		return;
	int i;
	switch(buffermode)
	{
	  case GFX_BUFFER_SINGLE:
		sync();
		SDL_UpdateRects(screen, dirtyrects[0], dirtytable[0]);
		dirtyrects[0] = 0;
		break;
	  case GFX_BUFFER_DOUBLE:
		sync();
		SDL_Flip(screen);
		i = backpage;
		backpage = frontpage;
		frontpage = i;
		dirtyrects[backpage] = 0;	//These are not used in this mode...
		dirtyrects[frontpage] = 0;
		break;
	  case GFX_BUFFER_SEMITRIPLE:
		for(i = 0; i < dirtyrects[backpage]; ++i)
			SDL_BlitSurface(softbuf, &dirtytable[backpage][i],
					screen, &dirtytable[backpage][i]);
		dirtyrects[backpage] = 0;
		i = backpage;
		backpage = frontpage;
		frontpage = i;
		sync();
		SDL_Flip(screen);
		break;
	  case GFX_BUFFER_HALF:
		//This isn't correct... It should probably
		//use softbuf and SDL_BlitSurface() instead.
		//(And besides, it makes sense only with vidsync!)
		sync_half();
		SDL_UpdateRect(screen, 0,0, _width,_height/2);
		sync();
		SDL_UpdateRect(screen, 0,_height/2, _width,_height/2);
		dirtyrects[backpage] = 0;	//These are not used in this mode...
		dirtyrects[frontpage] = 0;
		break;
	}
}


/*
 * Generic render() callback for sprites and tiles.
 */
void gfxengine_t::render_sprite(cs_obj_t *o)
{
	SDL_Rect dest_rect;
	SDL_Surface *img;
	img = gfxengine->get_sprite(o->anim.bank, o->anim.frame);
	if(!img)
		return;

	dest_rect.x = CS2PIXEL(o->point.gx * gfxengine->xsc);
	dest_rect.y = CS2PIXEL(o->point.gy * gfxengine->ysc);
	dest_rect.x += gfxengine->window->x() * gfxengine->xsc;
	dest_rect.y += gfxengine->window->y() * gfxengine->xsc;
	SDL_BlitSurface(img, NULL, gfxengine->surface(), &dest_rect);

	if(!gfxengine->autoinvalidate)
	{
		dest_rect.w = img->w;
		dest_rect.h = img->h;
		gfxengine->invalidate(&dest_rect);
	}
}


cs_obj_t *gfxengine_t::get_obj(int layer)
{
	cs_obj_t *o = cs_engine_get_obj(csengine);
	if(o)
	{
		o->render = render_sprite;
		cs_obj_layer(o, layer);
		cs_obj_activate(o);
	}
	return o;
}


void gfxengine_t::free_obj(cs_obj_t *obj)
{
	cs_obj_free(obj);
}


int gfxengine_t::objects_in_use()
{
	if(!csengine)
		return 0;

	return csengine->pool_total - csengine->pool_free;
}


SDL_Surface *gfxengine_t::get_sprite(int bank, int _frame)
{
	s_sprite_t *s = s_get_sprite(gfx, bank, _frame);
	if(s)
		return s->surface;
	else
		return NULL;
}


SoFont *gfxengine_t::get_font(unsigned int f)
{
	if(f < GFX_BANKS)
		return fonts[f];
	else
		return NULL;
}

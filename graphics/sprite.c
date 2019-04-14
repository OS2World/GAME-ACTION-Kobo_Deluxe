/*(LGPL)
 * Project Spitfire/SDL
----------------------------------------------------------------------
	sprite.c - Sprite engine for use with cs.h
----------------------------------------------------------------------
 * © David Olofson, 2001
 */

/*
TODO: Automatic extension of the bank and sprite
TODO: tables as needed when loading banks.
*/

#define	DBG(x)

#include "sprite.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glSDL.h"
#include "SDL_image.h"


s_blitmodes_t	s_blitmode = S_BLITMODE_AUTO;
pix_t s_colorkey = {0, 0, 0, 0};
pix_t s_clampcolor = {0, 0, 0, 0};
unsigned char	s_alpha = SDL_ALPHA_OPAQUE;


s_filter_t *filters = NULL;


/*
----------------------------------------------------------------------
	Filter Plugin Interface
----------------------------------------------------------------------
 */

s_filter_t *s_insert_filter(s_filter_cb_t callback)
{
	s_filter_t *nf = calloc(1, sizeof(s_filter_t));
	if(!nf)
		return NULL;

	nf->callback = callback;
	nf->next = filters;
	filters = nf;
	return nf;
}


s_filter_t *s_add_filter(s_filter_cb_t callback)
{
	s_filter_t *nf = calloc(1, sizeof(s_filter_t));
	if(!nf)
		return NULL;

	nf->callback = callback;
	if(!filters)
		filters = nf;
	else
	{
		s_filter_t *f = filters;
		while(f->next)
			f = f->next;
		f->next = nf;
	}
	return nf;
}


/* filter == NULL means "remove all plugins" */
void s_remove_filter(s_filter_t *filter)
{
	s_filter_t *f;
	if(!filters)
		return;

	f = filters;
	while(f->next)
	{
		if(f->next == filter || !filter)
		{
			s_filter_t *df = f->next;
			f->next = f->next->next;
			free(df);
		}
		if(f->next)
			f = f->next;
	}

	if(filters == filter || !filter)
	{
		s_filter_t *df = filters;
		filters = filters->next;
		free(df);
	}
}


static void __run_plugins(s_bank_t *b, unsigned first, unsigned frames)
{
	s_filter_t *f = filters;
	while(f)
	{
		f->callback(b, first, frames, &f->args);
		f = f->next;
	}	
}


/*
----------------------------------------------------------------------
	Some handy built-in plugins
----------------------------------------------------------------------
 */

/*
 * Pixel access operations for 32 bit RGBA
 */
static pix_t getpix32_empty = {0x7f, 0x7f, 0x7f, 0x7f};

static inline pix_t getpix32(SDL_Surface *s, int x, int y)
{
	pix_t	*p;
	if(x < 0 || x >= s->w || y < 0 || y >= s->h)
		return getpix32_empty;

	p = (pix_t *)((char *)s->pixels + y * s->pitch);
	return p[x];
}


/* Clamping version */
static inline pix_t getpix32c(SDL_Surface *s, int x, int y)
{
	pix_t	*p;
	if(x < 0)
		x = 0;
	else if(x > s->w-1)
		x = s->w-1;
	if(y < 0)
		y = 0;
	else if(y > s->h-1)
		y = s->h-1;
	p = (pix_t *)((char *)s->pixels + y * s->pitch);
	return p[x];
}

/* Interpolated; 28:4 fixed point coords. */
static inline pix_t getpix32i(SDL_Surface *s, int x, int y)
{
	int c0x, c0y, c1x, c1y;
	int c[4];
	pix_t e = {0, 0, 0, 0};
	pix_t p[4], r;
	getpix32_empty = e;

	/* Calculate filter core */
	x -= 8;
	y -= 8;
	c1x = x & 0xf;
	c1y = y & 0xf;
	c0x = 16 - c1x;
	c0y = 16 - c1y;
	c[0] = c0x * c0y;
	c[1] = c1x * c0y;
	c[2] = c0x * c1y;
	c[3] = c1x * c1y;

	/* Grab input pixels */
	x >>= 4;
	y >>= 4;
	p[0] = getpix32(s, x, y);
	p[1] = getpix32(s, x+1, y);
	p[2] = getpix32(s, x, y+1);
	p[3] = getpix32(s, x+1, y+1);

	/* Interpolate... */
	r.r = (p[0].r*c[0] + p[1].r*c[1] + p[2].r*c[2] + p[3].r*c[3])>>8;
	r.g = (p[0].g*c[0] + p[1].g*c[1] + p[2].g*c[2] + p[3].g*c[3])>>8;
	r.b = (p[0].b*c[0] + p[1].b*c[1] + p[2].b*c[2] + p[3].b*c[3])>>8;
	r.a = (p[0].a*c[0] + p[1].a*c[1] + p[2].a*c[2] + p[3].a*c[3])>>8;
	return r;
}


/* Clamiping version */
static inline pix_t getpix32ic(SDL_Surface *s, int x, int y)
{
	int c0x, c0y, c1x, c1y;
	int c[4];
	pix_t p[4], r;

	/* Calculate filter core */
	x -= 8;
	y -= 8;
	c1x = x & 0xf;
	c1y = y & 0xf;
	c0x = 16 - c1x;
	c0y = 16 - c1y;
	c[0] = c0x * c0y;
	c[1] = c1x * c0y;
	c[2] = c0x * c1y;
	c[3] = c1x * c1y;

	/* Grab input pixels */
	x >>= 4;
	y >>= 4;
	p[0] = getpix32c(s, x, y);
	p[1] = getpix32c(s, x+1, y);
	p[2] = getpix32c(s, x, y+1);
	p[3] = getpix32c(s, x+1, y+1);

	/* Interpolate... */
	r.r = (p[0].r*c[0] + p[1].r*c[1] + p[2].r*c[2] + p[3].r*c[3])>>8;
	r.g = (p[0].g*c[0] + p[1].g*c[1] + p[2].g*c[2] + p[3].g*c[3])>>8;
	r.b = (p[0].b*c[0] + p[1].b*c[1] + p[2].b*c[2] + p[3].b*c[3])>>8;
	r.a = (p[0].a*c[0] + p[1].a*c[1] + p[2].a*c[2] + p[3].a*c[3])>>8;
	return r;
}


static inline void setpix32(SDL_Surface *s, int x, int y, pix_t pix)
{
	pix_t *p;
	if(x < 0 || x >= s->w || y < 0 || y >= s->h)
		return;
	p = (pix_t *)((char *)s->pixels + y * s->pitch);
	p[x] = pix;
}


static inline pix_t *pix32(SDL_Surface *s, int x, int y)
{
	static pix_t dummy = {0x7f, 0x7f, 0x7f, 0x7f};
	pix_t *p;
	if(x < 0 || x >= s->w || y < 0 || y >= s->h)
		return &dummy;
	p = (pix_t *)((char *)s->pixels + y * s->pitch);
	return p + x;
}


/*
 * Surface formate conversion plugins
 */
#if 0
int s_filter_rgb8(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args)
{
	unsigned i;
	SDL_PixelFormat fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.BitsPerPixel = 24;
	fmt.BytesPerPixel = 3;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	fmt.Rmask = 0xff000000;
	fmt.Gmask = 0x00ff0000;
	fmt.Bmask = 0x0000ff00;
#else
	fmt.Rmask = 0x000000ff;
	fmt.Gmask = 0x0000ff00;
	fmt.Bmask = 0x00ff0000;
#endif
	fmt.Amask = 0;
	for(i = 0; i < frames; ++i)
	{
		SDL_Surface *tmp;
		s_sprite_t *s = s_get_sprite_b(b, first+i);
		if(!s)
			continue;
		tmp = SDL_ConvertSurface(s->surface, &fmt,
				SDL_SWSURFACE);		
		if(!tmp)
			return -1;

		SDL_FreeSurface(s->surface);
		s->surface = tmp;
	}
	return 0;
}
#endif

int s_filter_rgba8(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args)
{
	unsigned i;
	SDL_PixelFormat fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.BitsPerPixel = 32;
	fmt.BytesPerPixel = 4;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	fmt.Rmask = 0xff000000;
	fmt.Gmask = 0x00ff0000;
	fmt.Bmask = 0x0000ff00;
	fmt.Amask = 0x000000ff;
#else
	fmt.Rmask = 0x000000ff;
	fmt.Gmask = 0x0000ff00;
	fmt.Bmask = 0x00ff0000;
	fmt.Amask = 0xff000000;
#endif
	for(i = 0; i < frames; ++i)
	{
		SDL_Surface *tmp;
		s_sprite_t *s = s_get_sprite_b(b, first+i);
		if(!s)
			continue;
		tmp = SDL_ConvertSurface(s->surface, &fmt,
				SDL_SWSURFACE);
		if(!tmp)
			return -1;

		SDL_FreeSurface(s->surface);
		s->surface = tmp;
	}
	return 0;
}


/*
 * args.x = 1 activates 16 bit RGBA fix (assume +/-16 dither depth for RGBA)
 * args.y = clamping flags;
 *	SF_CLAMP_SFONT		SFont mode; first row is ignored.
 * args.r = red dither depth
 * args.g = green dither depth
 * args.b = blue dither depth
 *
 * Note: This plugin does *not* dither the alpha channel!
 */
int s_filter_dither(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args)
{
	int x, y;
	unsigned i;
	int ar, ag, ab;
	for(i = 0; i < frames; ++i)
	{
		s_sprite_t *s = s_get_sprite_b(b, first+i);
		if(!s)
			continue;
		ar = args->r >> 1;
		ag = args->g >> 1;
		ab = args->b >> 1;
		if(args->x)
			switch (s_blitmode)
			{
			  case S_BLITMODE_AUTO:
				if(s->surface->format->Amask)
					ar = ag = ab = 4;
				break;
			  case S_BLITMODE_OPAQUE:
				break;
			  case S_BLITMODE_COLORKEY:
			  case S_BLITMODE_ALPHA:
				ar = ag = ab = 4;
				break;
			}
		for(y = (args->y & SF_CLAMP_SFONT) ? 1 : 0; y < b->h; ++y)
			for(x = 0; x < b->w; ++x)
			{
				int r, g, b;
				pix_t *pix = pix32(s->surface, x, y);
				if((x^y)&1)
				{
					r = pix->r + ar;
					g = pix->g + ag;
					b = pix->b + ab;
					if(r>255)
						r = 255;
					if(g>255)
						g = 255;
					if(b>255)
						b = 255;
				}
				else
				{
					r = pix->r - ar;
					g = pix->g - ag;
					b = pix->b - ab;
					if(r<0)
						r = 0;
					if(g<0)
						g = 0;
					if(b<0)
						b = 0;
				}
				pix->r = r;
				pix->g = g;
				pix->b = b;
			}
	}
	return 0;
}


int s_filter_displayformat(s_bank_t * b, unsigned first, unsigned frames,
		s_filter_args_t * args)
{
	unsigned i;
	for(i = 0; i < frames; ++i)
	{
		SDL_Surface *tmp;
		s_sprite_t *s = s_get_sprite_b(b, first + i);
		if(!s)
			continue;

		switch (s_blitmode)
		{
		  case S_BLITMODE_AUTO:
			if(s->surface->format->Amask)
				SDL_SetAlpha(s->surface,
						SDL_SRCALPHA |
						SDL_RLEACCEL,
						SDL_ALPHA_OPAQUE);
			else
				SDL_SetColorKey(s->surface, SDL_RLEACCEL, 0);
			break;
		  case S_BLITMODE_OPAQUE:
			SDL_SetColorKey(s->surface, SDL_RLEACCEL, 0);
			break;
		  case S_BLITMODE_COLORKEY:
			SDL_SetColorKey(s->surface,
					SDL_SRCCOLORKEY | SDL_RLEACCEL,
					SDL_MapRGB(s->surface->format,
						s_colorkey.r,
						s_colorkey.g,
						s_colorkey.b));
			break;
		  case S_BLITMODE_ALPHA:
			SDL_SetAlpha(s->surface,
					SDL_SRCALPHA | SDL_RLEACCEL,
					s_alpha);
			break;
		}

		if(s->surface->format->Amask)
			tmp = SDL_DisplayFormatAlpha(s->surface);
		else
			tmp = SDL_DisplayFormat(s->surface);
		if(!tmp)
			return -1;

		SDL_FreeSurface(s->surface);
		s->surface = tmp;
	}
	return 0;
}


/*
 * Image processing plugins
 */

/*
 * args.x = scaling mode;
 *	SF_SCALE_NEAREST	Nearest
 *	SF_SCALE_BILINEAR	Bilinear interpolation
 *	SF_SCALE_SCALE2X	Scale2x (Based on an algo from AdvanceMAME)
 *	SF_SCALE_DIAMOND	Diamond (Weighted diamond shaped core)
 *
 * args.y = clamping flags;
 *	SF_CLAMP_EXTEND		Clamp at edges; image edge is extended outwards
 *	SF_CLAMP_SFONT		SFont mode; first row is treated specially.
 *
 * args.fx = horizontal scale factor
 *
 * args.fy = vertical scale factor
 */
int s_filter_scale(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args)
{
	int nw, nh, scx, scy;
	int x, y, sx, sy;
	int start_y, start_sy, fmode;
	unsigned i;
	int start_sx = 0;
	getpix32_empty = s_clampcolor;

	if((args->fx == 0.0) && (args->fy == 0.0))
		return 0;

	nw = (int)(b->w * args->fx);
	nh = (int)(b->h * args->fy);
	scx = (int)(65536.0/args->fx);
	scy = (int)(65536.0/args->fy);

	/* Check for special SFont mode */
	if(args->y & SF_CLAMP_SFONT)
	{
		start_y = 1;
		start_sy = 65536+32768;
		nh -= (int)args->fy - 1;
	}
	else
	{
		start_y = 0;
		start_sy = 0+32768;
	}

	/* Decode filter type + SF_CLAMP_EXTEND flag */
	fmode = (args->y & SF_CLAMP_EXTEND) ? 1 : 0;
	switch(args->x)
	{
	  case SF_SCALE_NEAREST:
		fmode += 0;
		break;
	  case SF_SCALE_BILINEAR:
		fmode += 2;
		break;
	  case SF_SCALE_SCALE2X:
		fmode += 4;
		break;
	  case SF_SCALE_DIAMOND:
		fmode += 6;
		break;
	}

	for(i = 0; i < frames; ++i)
	{
		pix_t clear = {0, 0, 0, 0};
		SDL_Surface *tmp;
		s_sprite_t *s = s_get_sprite_b(b, first+i);
		if(!s)
			continue;

		tmp = SDL_CreateRGBSurface(SDL_SWSURFACE, nw, nh, 32,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
				0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#else
				0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#endif
				);
		if(!tmp)
			return -1;

		/* Deal with SFont marker row */
		if(args->y & SF_CLAMP_SFONT)
		{
			/* Scale marker row with "nearest" filter */
			for(x = 0, sx = start_sx; x < nw; ++x, sx += scx)
				setpix32(tmp, x, 0, getpix32(s->surface,
						sx >> 16, 0));
			/* Remove marker row to avoid garbage */
			for(x = 0; x < b->w; ++x)
				setpix32(s->surface, x, 0, clear);
		}

		/* Do the scaling! */
		switch(fmode)
		{
		  case 0:	/*Nearest*/
		  case 1:	/*Nearest + extend clamping*/
			for(y = start_y, sy = start_sy; y < nh; ++y, sy += scy)
				for(x = 0, sx = start_sx; x < nw; ++x, sx += scx)
					setpix32(tmp, x, y, getpix32(s->surface,
							sx >> 16, sy >> 16));
			break;

		  case 2:	/*Bilinear*/
			for(y = start_y, sy = start_sy; y < nh; ++y, sy += scy)
				for(x = 0, sx = start_sx; x < nw; ++x, sx += scx)
					setpix32(tmp, x, y, getpix32i(s->surface,
							sx >> 12, sy >> 12));
			break;
		  case 3:	/*Bilinear + extend clamping*/
			for(y = start_y, sy = start_sy; y < nh; ++y, sy += scy)
				for(x = 0, sx = start_sx; x < nw; ++x, sx += scx)
					setpix32(tmp, x, y, getpix32ic(s->surface,
							sx >> 12, sy >> 12));
			break;

#define	LOOP2X	for(y = start_y, sy = start_y; y < nh; y += 2, ++sy)	\
			for(x = 0, sx = 0; x < nw; x += 2, ++sx)

		  /*
		   * This one's cool for 16 color graphics and the like, but
		   * even with the fuzziness factor, it doesn't work well
		   * with 24 bit data, or even 256 color graphics.
		   */
#define	CDIFF(x,y) (((x).r^(y).r) | ((x).g^(y).g) | ((x).b^(y).b) | ((x).a^(y).a))
		  case 4:	/*Scale2x*/
			LOOP2X
			{
				int DB, BF, DH, FH;
				pix_t	   B   ;
				pix_t	D, E, F;
				pix_t	   H   ;
				pix_t	E0, E1;
				pix_t	E2, E3;
				B = getpix32(s->surface, sx, sy-1);
				D = getpix32(s->surface, sx-1, sy);
				E = getpix32(s->surface, sx, sy);
				F = getpix32(s->surface, sx+1, sy);
				H = getpix32(s->surface, sx, sy+1);

				DB = CDIFF(D, B) & 0xc0;
				BF = CDIFF(B, F) & 0xc0;
				DH = CDIFF(D, H) & 0xc0;
				FH = CDIFF(F, H) & 0xc0;

				E0 = (!DB && BF && DH) ? D : E;
				E1 = (!BF && DB && FH) ? F : E;
				E2 = (!DH && DB && FH) ? D : E;
				E3 = (!FH && DH && BF) ? F : E;

				setpix32(tmp, x, y, E0);
				setpix32(tmp, x+1, y, E1);
				setpix32(tmp, x, y+1, E2);
				setpix32(tmp, x+1, y+1, E3);
			}
			break;
		  case 5:	/*Scale2x + extend clamping*/
			LOOP2X
			{
				int DB, BF, DH, FH;
				pix_t	   B   ;
				pix_t	D, E, F;
				pix_t	   H   ;
				pix_t	E0, E1;
				pix_t	E2, E3;
				B = getpix32c(s->surface, sx, sy-1);
				D = getpix32c(s->surface, sx-1, sy);
				E = getpix32c(s->surface, sx, sy);
				F = getpix32c(s->surface, sx+1, sy);
				H = getpix32c(s->surface, sx, sy+1);

				DB = CDIFF(D, B) & 0xc0;
				BF = CDIFF(B, F) & 0xc0;
				DH = CDIFF(D, H) & 0xc0;
				FH = CDIFF(F, H) & 0xc0;

				E0 = (!DB && BF && DH) ? D : E;
				E1 = (!BF && DB && FH) ? F : E;
				E2 = (!DH && DB && FH) ? D : E;
				E3 = (!FH && DH && BF) ? F : E;

				setpix32(tmp, x, y, E0);
				setpix32(tmp, x+1, y, E1);
				setpix32(tmp, x, y+1, E2);
				setpix32(tmp, x+1, y+1, E3);
			}
			break;
#undef	CDIFF

#define	DIAMOND(__getpix, __setpix)					\
			LOOP2X						\
			{						\
				pix_t	   B;				\
				pix_t	D, E, F;			\
				pix_t	   H;				\
				pix_t	E0, E1;				\
				pix_t	E2, E3;				\
				B = __getpix(s->surface, sx, sy-1);	\
				D = __getpix(s->surface, sx-1, sy);	\
				E = __getpix(s->surface, sx, sy);	\
				F = __getpix(s->surface, sx+1, sy);	\
				H = __getpix(s->surface, sx, sy+1);	\
				MIX(E0, E, B, D);			\
				MIX(E1, E, B, F);			\
				MIX(E2, E, H, D);			\
				MIX(E3, E, H, F);			\
				__setpix(tmp, x, y, E0);		\
				__setpix(tmp, x+1, y, E1);		\
				__setpix(tmp, x, y+1, E2);		\
				__setpix(tmp, x+1, y+1, E3);		\
			}
		  /*
		   * This works *great* though; kicks the butt of
		   * Linear/Oversampled real hard - and at an order
		   * of magnitude higher speed! :-)
		   */
#define MIX(t,x,y,z)	(t).r = ((x).r+(x).r+(y).r+(z).r)>>2;	\
			(t).g = ((x).g+(x).g+(y).g+(z).g)>>2;	\
			(t).b = ((x).b+(x).b+(y).b+(z).b)>>2;	\
			(t).a = ((x).a+(x).a+(y).a+(z).a)>>2;
		  case 6:	/*Diamond2x*/
			DIAMOND(getpix32, setpix32);
			break;
		  case 7:	/*Diamond2x + extend clamping*/
			DIAMOND(getpix32c, setpix32);
			break;
#undef	MIX

#undef	DIAMOND
		}

		SDL_FreeSurface(s->surface);
		s->surface = tmp;
	}
#undef	LOOPALL
	b->w = nw;
	b->h = nh;
	return 0;
}


/*
 * args.fx = alpha contrast (1.0 = unchanged; 2.0 = double; scales around 128)
 * args.x = alpha offset (0 = unchanged; -255 = all transp.; 255 = all opaque)
 * args.min = min alpha passed (lower ==> 0)
 * args.max = max alpha passed (higher ==> 255)
 */
int s_filter_cleanalpha(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args)
{
	int min = args->min;
	int max = args->max;
	int x, y;
	unsigned i;
	int contrast = (int)(args->fx * 256);
	int offset = args->x;
	if(!contrast)
		contrast = 256;	/*Default = 1.0*/
	for(i = 0; i < frames; ++i)
	{
		s_sprite_t *s = s_get_sprite_b(b, first+i);
		if(!s)
			continue;
		for(y = 0; y < b->h; ++y)
			for(x = 0; x < b->w; ++x)
			{
				pix_t *pix = pix32(s->surface, x, y);
				int alpha = pix->a;
				alpha = (alpha-128) * contrast >> 8;
				alpha += 128;
				alpha += offset;
				if(alpha < min)
					alpha = 0;
				else if(alpha > max)
					alpha = 255;
				pix->a = alpha;
			}
	}
	return 0;
}


static void construct_limits(int *min, int *max, int maxdev)
{
	*min -= maxdev;
	if(*min < 0)
		*min = 0;
	*max += maxdev;
	if(*max > 255)
		*max = 255;
}


static inline int check_limits(int min, int max, int value)
{
	if(value < min)
		return 0;
	if(value > max)
		return 0;
	return 1;
}


/*
 * args.max = colorkey detection fuzziness factor
 */
int s_filter_key2alpha(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args)
{
	int x, y, krmin, krmax, kgmin, kgmax, kbmin, kbmax;
	unsigned i;
	krmin = krmax = s_colorkey.r;
	kgmin = kgmax = s_colorkey.g;
	kbmin = kbmax = s_colorkey.b;
	construct_limits(&krmin, &krmax, args->max);
	construct_limits(&kgmin, &kgmax, args->max);
	construct_limits(&kbmin, &kbmax, args->max);
	for(i = 0; i < frames; ++i)
	{
		s_sprite_t *s = s_get_sprite_b(b, first+i);
		if(!s)
			continue;
		for(y = 0; y < b->h; ++y)
			for(x = 0; x < b->h; ++x)
			{
				pix_t *pix = pix32(s->surface, x, y);
				if(check_limits(krmin, krmax, pix->r)
						&& check_limits(kgmin, kgmax, pix->g)
						&& check_limits(kbmin, kbmax, pix->b))
					pix->r = pix->g = pix->b = pix->a = 0;
			}
	}
	return 0;
}



/*
----------------------------------------------------------------------
	Basic Container Management
----------------------------------------------------------------------
 */

static int __alloc_bank_table(s_container_t *c, unsigned int count)
{
	c->max = count - 1;
	c->banks = (s_bank_t **)calloc(sizeof(s_bank_t *), (unsigned)count);
	return -(c->banks == NULL);
}

s_container_t *s_new_container(unsigned banks)
{
	s_container_t *ret = (s_container_t *)calloc(sizeof(s_container_t), 1);
	if(!ret)
		return NULL;
	if(__alloc_bank_table(ret, banks) < 0)
		return NULL;
	return ret;
}

void s_delete_container(s_container_t *c)
{
	s_delete_all_banks(c);
	free(c->banks);
	free(c);
}


/*
 * Allocates a new sprite.
 * If the sprite exists already, it's surface will be removed, so
 * that one can safely expect to get an *empty* sprite.
 */
s_sprite_t *s_new_sprite_b(s_bank_t *b, unsigned frame)
{
	s_sprite_t	*s = NULL;
	if(frame > b->max)
		return NULL;
	if(!b->sprites[frame])
	{
		s = calloc(sizeof(s_sprite_t), 1);
		if(!s)
			return NULL;
		b->sprites[frame] = s;
	}
	else
	{
		if(s->surface)
			SDL_FreeSurface(s->surface);
		s->surface = NULL;
	}
	return s;
}


/*
 * Allocates a new sprite.
 * If the sprite exists already, it's surface will be removed, so
 * that one can safely expect to get an *empty* sprite.
 *
 * If the bank does not exists, or if the operation failed for
 * other reasons, this call returns NULL.
 */
s_sprite_t *s_new_sprite(s_container_t *c, unsigned bank, unsigned frame)
{
	if(bank > c->max)
		return NULL;
	if(!c->banks[bank])
		return NULL;

	return s_new_sprite_b(c->banks[bank], frame);
}


void s_delete_sprite_b(s_bank_t *b, unsigned frame)
{
	if(!b->sprites)	/* This can happen when failing to create a new bank */
		return;
	if(frame > b->max)
		return;
	if(!b->sprites[frame])
		return;
	if(b->sprites[frame]->surface)
		SDL_FreeSurface(b->sprites[frame]->surface);
	free(b->sprites[frame]);
	b->sprites[frame] = NULL;
}


void s_delete_sprite(s_container_t *c, unsigned bank, unsigned frame)
{
	s_bank_t *b = s_get_bank(c, bank);
	if(b)
		s_delete_sprite_b(b, frame);
}


static int __alloc_sprite_table(s_bank_t *b, unsigned frames)
{
	b->max = frames - 1;
	b->sprites = (s_sprite_t **)calloc(sizeof(s_sprite_t *), frames);
	if(!b->sprites)
		return -1;
	return 0;
}


s_bank_t *s_new_bank(s_container_t *c, unsigned bank, unsigned frames,
				unsigned w, unsigned h)
{
	s_bank_t *b;
	DBG(printf("s_new_bank(%p, %d, %d, %d, %d)\n", c, bank, frames, w, h));
	if(bank > c->max)
		return NULL;
	if(c->banks[bank])
		s_delete_bank(c, bank);

	c->banks[bank] = (s_bank_t *)calloc(sizeof(s_bank_t), 1);
	if(!c->banks[bank])
		return NULL;
	if(__alloc_sprite_table(c->banks[bank], frames) < 0)
	{
		s_delete_bank(c, bank);
		return NULL;
	}
	b = c->banks[bank];
	b->max = frames - 1;
	b->w = w;
	b->h = h;
	return b;
}


void s_delete_bank(s_container_t *c, unsigned bank)
{
	unsigned i;
	s_bank_t *b;
	if(!c->banks)
		return;
	if(bank > c->max)
		return;
	b = c->banks[bank];
	if(b)
	{
		for(i = 0; i < b->max; ++i)
			s_delete_sprite_b(b, i);
		free(b->sprites);
		free(b);
		c->banks[bank] = NULL;
	}
}


void s_delete_all_banks(s_container_t *c)
{
	unsigned i;
	for(i = 0; i < c->max; ++i)
		s_delete_bank(c, i);
}



/*
----------------------------------------------------------------------
	Getting data
----------------------------------------------------------------------
 */

s_bank_t *s_get_bank(s_container_t *c, unsigned bank)
{
	if(bank > c->max)
		return NULL;
	if(!c->banks[bank])
		return NULL;
	return c->banks[bank];
}

s_sprite_t *s_get_sprite(s_container_t *c, unsigned bank, unsigned frame)
{
	s_bank_t *b = s_get_bank(c, bank);
	if(!b)
		return NULL;
	if(frame > b->max)
		return NULL;
	return b->sprites[frame];
}

s_sprite_t *s_get_sprite_b(s_bank_t *b, unsigned frame)
{
	if(!b)
		return NULL;
	if(frame > b->max)
		return NULL;
	return b->sprites[frame];
}


/*
----------------------------------------------------------------------
	File tools
----------------------------------------------------------------------
 */

static int extract_sprite(s_bank_t *bank, unsigned frame,
				SDL_Surface *src, SDL_Rect *from)
{
	SDL_Surface	*tmp;
	SDL_Rect	dr;
	if(frame > bank->max)
	{
		fprintf(stderr, "sprite: Too many frames!\n");
		return -1;
	}
	if(!s_new_sprite_b(bank, frame))
		return -2;

	tmp = SDL_CreateRGBSurface(SDL_SWSURFACE,
			from->w, from->h,
			src->format->BitsPerPixel,
			src->format->Rmask,
			src->format->Gmask,
			src->format->Bmask,
			src->format->Amask	);
	if(!tmp)
		return -3;

	if(src->format->palette)
		SDL_SetColors(tmp, src->format->palette->colors, 0,
				src->format->palette->ncolors);

	dr.x = 0;
	dr.y = 0;
	switch(s_blitmode)
	{
	  case S_BLITMODE_AUTO:
		if(src->format->Amask)
			SDL_SetAlpha(src, 0, s_alpha);
		SDL_BlitSurface(src, from, tmp, &dr);
		break;
	  case S_BLITMODE_OPAQUE:
	  case S_BLITMODE_COLORKEY:
	  case S_BLITMODE_ALPHA:
		SDL_BlitSurface(src, from, tmp, &dr);
		break;
	}
	bank->sprites[frame]->surface = tmp;

	DBG(printf("image %d: (%d,%d)/%dx%d @ %p\n", frame,
			from->x, from->y, from->w, from->h,
			bank->sprites[frame]->surface));
	return 0;
}


int s_load_rect(s_container_t *c, unsigned bank,
		unsigned frombank, unsigned fromframe, SDL_Rect *from)
{
	SDL_Surface	*src;
	s_sprite_t	*s;
	s_bank_t	*b;

	s = s_get_sprite(c, frombank, fromframe);
	if(!s)
	{
		fprintf(stderr, "sprite: Couldn't get source sprite %d:%d!\n",
				frombank, fromframe);
		return -1;
	}
	src = s->surface;
	if(!src)
	{
		fprintf(stderr, "sprite: Sprite %d:%d"
				" does not have a surface!\n",
				frombank, fromframe);
		return -2;
	}

	b = s_new_bank(c, bank, 1, from->w, from->h);
	if(!b)
	{
		fprintf(stderr, "sprite: Failed to allocate bank %d!\n", bank);
		return -3;
	}

	if(extract_sprite(b, 0, src, from) < 0)
	{
		fprintf(stderr, "sprite: Something went wrong while "
				"copying from sprite %d:%d.\n",
				frombank, fromframe);
		return -4;
	}

	s_filter_displayformat(b, 0, 1, NULL);
	return 0;
}


int s_load_image(s_container_t *c, unsigned bank, const char *name)
{
	SDL_Surface	*src;
	s_bank_t	*b;

	src = IMG_Load(name);
	if(!src)
	{
		fprintf(stderr, "sprite: Failed to load sprite \"%s\"!\n", name);
		return -1;
	}

	b = s_new_bank(c, bank, 1, src->clip_rect.w, src->clip_rect.h);
	if(!b)
	{
		fprintf(stderr, "sprite: Failed to allocate bank for \"%s\"!\n", name);
		return -2;
	}

	if(extract_sprite(b, 0, src, &src->clip_rect) < 0)
	{
		fprintf(stderr, "sprite: Something went wrong while"
				" extracting sprite \"%s\".\n", name);
		return -3;
	}

	SDL_FreeSurface(src);
	__run_plugins(b, 0, 1);
	return 0;
}


int s_load_sprite(s_container_t *c, unsigned bank, unsigned frame,
					const char *name)
{
	SDL_Surface	*src;
	SDL_Rect	from;
	s_bank_t	*b = s_get_bank(c, bank);
	if(!b)
	{
		fprintf(stderr, "sprite: While loading \"%s\":"
				" Bank %d does not exist!\n", name, bank);
		return -2;
	}

	src = IMG_Load(name);
	if(!src)
	{
		fprintf(stderr, "sprite: Failed to load sprite \"%s\"!\n", name);
		return -1;
	}

	from = src->clip_rect;
	if( (from.w != b->w) || (from.h != b->h) )
	{
		fprintf(stderr, "sprite: Warning: Sprite \"%s\" cropped"
				" to fit in bank %d.\n", name, bank);
		from.w = b->w;
		from.h = b->h;
	}

	if(extract_sprite(b, frame, src, &from) < 0)
	{
		fprintf(stderr, "sprite: Something went wrong while"
				" extracting sprite \"%s\".\n", name);
		return -3;
	}

	SDL_FreeSurface(src);
	__run_plugins(b, frame, 1);
	return 0;
}


int s_load_bank(s_container_t *c, unsigned bank, unsigned w, unsigned h,
					const char *name)
{
	SDL_Surface	*src;
	s_bank_t	*b;
	int		x, y;
	unsigned	frame = 0;
	unsigned	frames;

	DBG(printf("s_load_bank(%p, %d, %d, %d, %s)\n", c, bank, w, h, name));

	src = IMG_Load(name);
	if(!src)
	{
		fprintf(stderr, "sprite: Failed to load sprite palette %s!\n", name);
		return -1;
	}

	if(w > src->w)
	{
		fprintf(stderr, "sprite: Source image %s not wide enough!\n", name);
		return -4;
	}

	if(h > src->h)
	{
		fprintf(stderr, "sprite: Source image %s not high enough!\n", name);
		return -5;
	}

	frames = (src->w / w) * (src->h / h);
	b = s_new_bank(c, bank, frames, w, h);
	if(!b)
	{
		fprintf(stderr, "sprite: Failed to allocate bank for \"%s\"!\n", name);
		return -2;
	}

	for(y = 0; y <= src->h - h; y += h)
		for(x = 0; x <= src->w - w; x += w)
		{
			SDL_Rect r;
			r.x = x;
			r.y = y;
			r.w = w;
			r.h = h;
			if(extract_sprite(b, frame, src, &r) < 0)
			{
				fprintf(stderr, "sprite: Something went wrong while"
						" extracting sprites.\n");
				return -3;
			}
			++frame;
		}

	SDL_FreeSurface(src);
	__run_plugins(b, 0, frames);
	return 0;
}

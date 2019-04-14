/*(LGPL)
 * Project Spitfire/SDL, Kobo Deluxe
----------------------------------------------------------------------
	sprite.h - Sprite engine for use with cs.h
----------------------------------------------------------------------
 * Copyright 2001 David Olofson
 * This code is released under the terms of the GNU LGPL.
 */

#ifndef _SPRITE_H_
#define _SPRITE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "glSDL.h"

typedef enum
{
	S_BLITMODE_AUTO = 0,	/* Use source alpha if present */
	S_BLITMODE_OPAQUE,	/* Alpha: blend over black during conv. */
	S_BLITMODE_COLORKEY,	/* Like OPAQUE, but apply colorkey to the result */
	S_BLITMODE_ALPHA	/* Apply source alpha during conv,
				 * then use full surface alpha */
} s_blitmodes_t;

struct s_container_t;

/* sprite image */
typedef struct
{
/* TODO TODO TODO TODO TODO TODO */
	int		x, y;		/* kern or hot-spot*/
	struct
	{
		int	x, y, w, h;	/* spacing or collision rect */
	} logic;
/* /TODO TODO TODO TODO TODO TODO */
	SDL_Surface	*surface;
} s_sprite_t;

/* Bank of sprite images */
typedef struct
{
/* TODO TODO TODO TODO TODO TODO */
	struct s_container_t *container;
	int		index;	/* Index of this bank */
/* /TODO TODO TODO TODO TODO TODO */
	unsigned	w, h;	/* Same size for all sprites in bank! */
	unsigned	last;
	unsigned	max;
	s_sprite_t	**sprites;
} s_bank_t;

/* Two dimensionally indexed sprite container */
typedef struct s_container_t
{
	unsigned	max;
	s_bank_t	**banks;
} s_container_t;


/*
 * Basic Container Management
 */
s_container_t *s_new_container(unsigned banks);
void s_delete_container(s_container_t *c);

s_bank_t *s_new_bank(s_container_t *c, unsigned bank, unsigned frames,
				unsigned w, unsigned h);
void s_delete_bank(s_container_t *c, unsigned bank);
void s_delete_all_banks(s_container_t *c);

s_sprite_t *s_new_sprite(s_container_t *c, unsigned bank, unsigned frame);
void s_delete_sprite(s_container_t *c, unsigned bank, unsigned frame);

/*
 * Getting data
 */
s_sprite_t *s_get_sprite(s_container_t *c, unsigned bank, unsigned frame);
s_sprite_t *s_get_sprite_b(s_bank_t *b, unsigned frame);

/*
 * File Import Tools
 * (Will run plugins as files are loaded!)
 */
int s_load_image(s_container_t *c, unsigned bank, const char *name);
int s_load_bank(s_container_t *c, unsigned bank, unsigned w, unsigned h,
						const char *name);
int s_load_sprite(s_container_t *c, unsigned bank, unsigned frame,
						const char *name);

/*
 * Internal data manipulation API
 * (Will not automatically run plugins.)
 */
int s_load_rect(s_container_t *c, unsigned bank,
		unsigned frombank, unsigned fromframe, SDL_Rect *from);

/*
 * Lower level interfaces
 */
s_bank_t *s_get_bank(s_container_t *c, unsigned bank);
s_sprite_t *s_new_sprite_b(s_bank_t *b, unsigned frame);
void s_delete_sprite_b(s_bank_t *b, unsigned frame);


/*
 * Filter Plugin Interface
 *
 * Rules:
 *	* Plugins are called to process one bank at a time,
 *	  after new frames have been loaded.
 *
 *	* Plugins are called in the order they were
 *	  registered.
 *
 *	* Plugins may replace the surfaces of sprites. If
 *	  they do so, they're responsible for disposing of
 *	  the old surfaces.
 *
 *	* It is recommended that plugins operate only on
 *	  the specified range of frames in a bank.
 *
 *	* The surfaces size of all sprites in a bank should
 *	  preferably match that of the bank. A chain of
 *	  plugins may break this rule temporarily, as long
 *	  as "order is restored" (ie surfaces are resized, or
 *	  bank size values is adjusted) before the chain ends.
 *
 * Cool plugin ideas:
 *	* Colorizer:
 *	  Converts images to HSV and then replaces hue with
 *	  a fixed color.
 *
 *	* Map Colorizer:
 *	  Like Colorizer, but uses a (wrapping) source image
 *	  as a map to determine the new hue for each pixel.
 *
 *	* Tiled Stretch:
 *	  Given a list of tile interdependencies, generate
 *	  all tiles required for seamless rendering of the
 *	  map for which the interdependency list was generated.
 */
typedef struct s_filter_args_t
{
	int		x, y;
	float		fx, fy;
	int		min, max;
	short		r, g, b;
	unsigned	bank;
	void		*data;
} s_filter_args_t;

typedef int (*s_filter_cb_t)(s_bank_t *b, unsigned first, unsigned frames,
				s_filter_args_t *args);

typedef struct s_filter_t
{
	struct s_filter_t	*next;
	s_filter_cb_t		callback;
	s_filter_args_t		args;
} s_filter_t;

s_filter_t *s_insert_filter(s_filter_cb_t callback);
s_filter_t *s_add_filter(s_filter_cb_t callback);
/* callback == NULL means "remove all plugins" */
void s_remove_filter(s_filter_t *filter);

/*
 * Some handy built-in plugins
 */
#if 0
int s_filter_rgb8(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args);
#endif
int s_filter_rgba8(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args);
int s_filter_dither(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args);
int s_filter_displayformat(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args);

int s_filter_key2alpha(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args);
int s_filter_cleanalpha(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args);
int s_filter_scale(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args);

/* RGBA pixel type used by most plugins */
typedef struct
{
	Uint8	r;
	Uint8	g;
	Uint8	b;
	Uint8	a;
} pix_t;


/*
 * Flags for the "scale" plugin.
 */
/* args.x; mode: */
#define	SF_SCALE_NEAREST	0
#define	SF_SCALE_BILINEAR	1
#define	SF_SCALE_SCALE2X	2
#define	SF_SCALE_DIAMOND	3

/* args.y; flags: */
#define	SF_CLAMP_EXTEND		0x00000001
#define	SF_CLAMP_SFONT		0x00000002


/*
 * UURGH!!!
 */
extern s_blitmodes_t	s_blitmode;
extern pix_t		s_colorkey;	/* NOTE: Alpha is ignored! */
extern pix_t		s_clampcolor;
extern unsigned char	s_alpha;

#ifdef __cplusplus
};
#endif

#endif /* _SPRITE_H_ */

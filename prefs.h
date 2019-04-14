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

#ifndef	_KOBO_PREFS_H_
#define	_KOBO_PREFS_H_

#include "aconfig.h"
#include "cfgparse.h"

class prefs_t : public config_parser_t
{
  public:
	//Input options
	int	use_joystick;
	int	use_mouse;
	int	mousemode;
	int	broken_numdia;
	int	dia_emphasis;

	//Game options
	int	filter;		//Use motion filtering
	int	wait_msec;	//ms per control system frame
	int	scrollradar;	//Use scrolling radar display

	//Sound: System
	int	use_sound;	//Enable sound
	int	use_music;	//Enable "real" music
	int	use_oss;	//Use OSS audio driver
	int	samplerate;
	int	latency;	//Audio latency in ms
	int	mixquality;	//Mixer quality control
	int	volume;		//Digital master volume
	//Sound: Mixer
	int	intro_vol;	//Intro music volume
	int	sfx_vol;	//Sound effects volume
	int	music_vol;	//In-game music volume
	//Sound: Master Effects
	int	reverb;		//Master reverb level
	int	threshold;	//Limiter threshold
	int	release;	//Limiter release rate

	//Video settings
	int	fullscreen;	//Use fullscreen mode
	int	videodriver;	//Internal video driver
	int	width;		//Screen/window width
	int	height;		//Screen/window height
	int	depth;		//Bits per pixel
	int	buffermode;	//Internal buffering mode
	int	internalres;	//Internal resolution for OpenGL modes
	int	scalemode;	//Scaling filter mode
	int	use_dither;
	int	broken_rgba8;	//For some OpenGL setups

	//File paths
	cfg_string_t	dir;		//Path to kobo-deluxe/
	cfg_string_t	gfxdir;		//Path to gfx/
	cfg_string_t	sfxdir;		//Path to sfx/
	cfg_string_t	scoredir;	//Path to scores/

	//Obsolete stuff (compatibility)
	int		__size;			//Screen scale
//	int		__use_ext_music;	//Use external playlist + data
	cfg_string_t	__bgm_indexfile;	//Ext playlist path

	//"Hidden" stuff ("to remember until next startup")
	int	last_profile;	//Last used player profile

	void init();
	void postload();

	/* "Commands" - never written to config files */
	int cmd_showcfg;
	int cmd_hiscores;
	int cmd_override;
	int cmd_debug;
	int cmd_fps;
	int cmd_noframe;
	int cmd_midi;
	int cmd_cheat;		//Unlimited lives; select any starting stage
	int cmd_indicator;	//Enable collision testing mode
	int cmd_pushmove;	//Stop when not holding any direction down
	int cmd_noparachute;	//Disable SDL parachute
	int cmd_pollaudio;	//Use polling based audio instead of thread
};

#endif	//_KOBO_PREFS_H_

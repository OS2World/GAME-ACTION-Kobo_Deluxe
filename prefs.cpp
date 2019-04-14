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

#include "prefs.h"
#include "gfxengine.h"
#include "gamectl.h"
#include "a_types.h"
#include "game.h"

void prefs_t::init()
{
	comment("--------------------------------------------");
	comment(" Kobo Deluxe "VERSION" Configuration File");
	comment("--------------------------------------------");
	comment(" Switches - [no]<switch>");
	comment(" Values - <key> [<value>|\"<string>\"]");
	comment("--------------------------------------------");
	comment("--- Input options --------------------------");
	yesno("joystick", use_joystick, 0); desc("Use Joystick");
	yesno("mouse", use_mouse, 0); desc("Use Mouse");
	key("mousemode", mousemode, MMD_CROSSHAIR); desc("Mouse Control Mode");
	yesno("broken_numdia", broken_numdia, 0); desc("Broken NumPad Diagonals");
	key("dia_emphasis", dia_emphasis, 0); desc("Diagonals Emphasis Filter");

	comment("--- Game options ---------------------------");
	yesno("scrollradar", scrollradar, 1); desc("Scrolling Radar");
	key("wait", wait_msec, 30); desc("Game Speed");
	yesno("filter", filter, 1); desc("Motion Interpolation");

	comment("--- Sound settings -------------------------");
	yesno("sound", use_sound, 1); desc("Enable Sound");
	yesno("music", use_music, 1); desc("Enable Music");
#ifdef HAVE_OSS
	yesno("oss", use_oss, 0);
#else
	yesno("oss", use_oss, 0, 0);	//Don't write!
#endif
			 desc("Use OSS Sound Driver");
	key("samplerate", samplerate, 44100); desc("Sample Rate");
	key("latency", latency, 50); desc("Sound Latency");
	key("mixquality", mixquality, AQ_HIGH); desc("Mixing Quality");
	key("vol", volume, 100); desc("Master Volume");
	key("intro_vol", intro_vol, 100); desc("Intro Music Volume");
	key("sfx_vol", sfx_vol, 100); desc("Sound Effects Volume");
	key("music_vol", music_vol, 50); desc("In-Game Music Volume");
	key("reverb", reverb, 100); desc("Reverb Level");
	key("threshold", threshold, 200); desc("Limiter Gain");
	key("release", release, 50); desc("Limiter Speed");

	comment("--- Video settings -------------------------");
	yesno("fullscreen", fullscreen, 0); desc("Fullscreen Display");
	key("videodriver", videodriver, GFX_DRIVER_SDL2D);
			desc("Display Driver");
	key("width", width, 320); desc("Horizontal Resolution");
	key("height", height, 240); desc("Vertical Resolution");
	key("depth", depth, 0); desc("Display Depth");
	key("buffer", buffermode, GFX_BUFFER_SEMITRIPLE);
			desc("Display Buffer Mode");
	key("internalres", internalres, 1); desc("Texture Resolution");
	key("scalemode", scalemode, 2); desc("Scaling Filter Mode");
	yesno("dither", use_dither, 1); desc("Use Dithering");
	yesno("broken_rgba8", broken_rgba8, 0); desc("Broken RGBA (OpenGL)");

	comment("--- File paths -----------------------------");
	key("files", dir, ""); desc("Game Root Path");
	key("gfx", gfxdir, ""); desc("Graphics Data Path");
	key("sfx", sfxdir, ""); desc("Sound Data Path");
	key("scores", scoredir, ""); desc("Score File Path");

	// Obsolete stuff (not written into new files)
	key("size", __size, 0, 0);
	key("bgm", __bgm_indexfile, "", 0);

	comment("--- Temporary variables --------------------");
	key("last_profile", last_profile, 0);
			desc("Last used player profile");

	// "Commands" - never written to config files
	command("showcfg", cmd_showcfg);
	command("hiscores", cmd_hiscores);
	command("highscores", cmd_hiscores);
	command("override", cmd_override);
	command("debug", cmd_debug);
	command("fps", cmd_fps);
	command("noframe", cmd_noframe);
	command("midi", cmd_midi);
	command("cheat", cmd_cheat);
	command("indicator", cmd_indicator);
	command("pushmove", cmd_pushmove);
	command("noparachute", cmd_noparachute);
	command("pollaudio", cmd_pollaudio);
}


void prefs_t::postload()
{
	if(redefined(__size))
	{
		//For pre 20011007 compatibility
		width = 320 * __size;
		height = 240 * __size;
		__size = 0;
		accept(__size);
	}

	if((wait_msec != 30) && !cmd_cheat)
	{
		fprintf(stderr, "'wait' is only avaliable in cheat mode!\n");
		wait_msec = 30;
	}
}

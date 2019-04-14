/*
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


#include "config.h"
#include "options.h"
#include "gfxengine.h"
#include "gamectl.h"
#include "a_types.h"


void video_options_t::build()
{
	big(); label("Video Options"); small();
	xoffs = 0.6;
	space(2);
		yesno("Fullscreen Display", &prf->fullscreen,
				OS_RESTART_VIDEO);
		list("Display Depth", &prf->depth, OS_RESTART_VIDEO);
			item("Default", 0);
			item("8 bits", 8);
			item("15 bits", 15);
			item("16 bits", 16);
			item("24 bits", 24);
			item("32 bits", 32);
#if defined(HAVE_OPENGL) && !defined(GLSDL_OFF)
		list("Display Driver", &prf->videodriver,
				OS_RESTART_VIDEO | OS_REBUILD);
			item("SDL 2D", GFX_DRIVER_SDL2D);
//			item("OpenGL/Buffer", GFX_DRIVER_OPENGLBUF);
			item("OpenGL/glSDL", GFX_DRIVER_OPENGL_INT);
//			item("OpenGL/Native", GFX_DRIVER_OPENGL);
#else
		prf->videodriver = GFX_DRIVER_SDL2D;
#endif
		list("Display Buffer Mode", &prf->buffermode,
				OS_RESTART_VIDEO);
			item("Single", GFX_BUFFER_SINGLE);
			item("Double", GFX_BUFFER_DOUBLE);
		switch(prf->videodriver)
		{
		  case GFX_DRIVER_SDL2D:
			item("SemiTriple", GFX_BUFFER_SEMITRIPLE);
			item("Half", GFX_BUFFER_HALF);
			break;
		  case GFX_DRIVER_OPENGLBUF:
		  case GFX_DRIVER_OPENGL_INT:
		  case GFX_DRIVER_OPENGL:
			break;
		}
		/*
		 * For now, we don't support OpenGL stretching.
		 */
		int scale = prf->width / 320;
		switch(prf->videodriver)
		{
		  case GFX_DRIVER_SDL2D:
#if 0
			scale = prf->width / 320;
#endif
			break;
		  case GFX_DRIVER_OPENGLBUF:
		  case GFX_DRIVER_OPENGL_INT:
		  case GFX_DRIVER_OPENGL:
#if 0
			list("Texture Resolution", &prf->internalres,
					OS_RESTART_VIDEO | OS_REBUILD);
				item("320x240", 1);
				item("640x480", 2);
				item("960x720", 3);
				item("1280x1024", 4);
			scale = prf->width / (320*prf->internalres);
#endif
			yesno("Broken RGBA8 (OpenGL)", &prf->broken_rgba8,
					OS_RELOAD_GRAPHICS);
			break;
		}
	space(1);
	xoffs = 0.4;
//		spin("Scale Factor", &prf->size, 1, 4, TAG_RESTART_VIDEO);
		list("Display Size", &prf->width,
				OS_RESTART_VIDEO | OS_REBUILD);
			item("320x240", 320);
			item("400x300 (OpenGL)", 400);
			item("512x384 (OpenGL)", 512);
			item("640x480", 640);
			item("800x600 (OpenGL)", 800);
			item("960x720", 960);
			item("1024x768 (OpenGL)", 1024);
			item("1152x864 (OpenGL)", 1152);
			item("1280x1024", 1280);
		if(scale > 1)
		{
			list("Scale Mode", &prf->scalemode,
					OS_RELOAD_GRAPHICS);
				item("Nearest", GFX_SCALE_NEAREST);
				item("Bilinear", GFX_SCALE_BILINEAR);
				item("Bilinear+Oversampling", GFX_SCALE_BILIN_OVER);
			if(2 == scale)
			{
				item("Scale2x", GFX_SCALE_SCALE2X);
				item("Diamond2x", GFX_SCALE_DIAMOND);
			}
			else if(prf->scalemode >= GFX_SCALE_SCALE2X)
				prf->scalemode = GFX_SCALE_BILINEAR;
		}
	space(1);
	xoffs = 0.55;
		yesno("Dithering", &prf->use_dither, OS_RELOAD_GRAPHICS);
	space(2);
	big();
		button("ACCEPT", OS_CLOSE);
		button("CANCEL", OS_CANCEL);
}


void audio_options_t::build()
{
	big(); label("Audio Options"); small();
	xoffs = 0.57;
	space(1);
		yesno("Enable Sound", &prf->use_sound, OS_RESTART_AUDIO | OS_REBUILD);
		if(prf->use_sound)
		{
			//System
			list("Sample Rate", &prf->samplerate, OS_RESTART_AUDIO);
				item("8 kHz", 8000);
				item("11025 Hz", 11025);
				item("16 kHz", 16000);
				item("22050", 22050);
				item("32 kHz", 32000);
				item("44.1 kHz", 44100);
				item("48 kHz", 48000);
			spin("Sound Latency", &prf->latency, 1, 200, "ms",
					OS_RESTART_AUDIO);
#ifdef HAVE_OSS
			yesno("Use OSS Sound Driver", &prf->use_oss,
					OS_RESTART_AUDIO);
#endif
			list("Mixing Quality", &prf->mixquality, OS_UPDATE_AUDIO);
				item("Very Low", AQ_VERY_LOW);
				item("Low", AQ_LOW);
				item("Normal", AQ_NORMAL);
				item("High", AQ_HIGH);
				item("Very High", AQ_VERY_HIGH);

		space();
			//Mixer
/*			list("Master Volume", &prf->volume, OS_UPDATE_AUDIO);
				item("OFF", 0);
				perc_list(10, 100, 10);*/
			list("Sound Effects", &prf->sfx_vol, OS_UPDATE_AUDIO);
				item("OFF", 0);
				perc_list(10, 90, 10);
				perc_list(100, 200, 25);
			yesno("Enable Music", &prf->use_music, OS_RESTART_AUDIO | OS_REBUILD);
			if(prf->use_music)
			{
				list("Intro Music", &prf->intro_vol, OS_UPDATE_AUDIO);
					item("OFF", 0);
					perc_list(10, 90, 10);
					perc_list(100, 200, 25);
				list("In-Game Music", &prf->music_vol, OS_UPDATE_AUDIO);
					item("OFF", 0);
					perc_list(10, 90, 10);
					perc_list(100, 200, 25);
			}

		space();
			//Master effects
			list("Reverb Level", &prf->reverb, OS_UPDATE_AUDIO);
				item("OFF", 0);
				perc_list(10, 90, 10);
				perc_list(100, 200, 25);
			list("Limiter Speed", &prf->release, OS_UPDATE_AUDIO);
				item("Slow", 40);
				item("Normal", 100);
				item("Fast", 250);
				item("Aggressive", 500);
			list("Limiter Gain", &prf->threshold, OS_UPDATE_AUDIO);
				item("None", 100);
				item("Low", 90);
				item("Normal", 80);
				item("High", 60);
				item("Pumping", 30);
				item("Extreme", 10);
		}
	space(1);
	big();
		button("ACCEPT", OS_CLOSE);
		button("CANCEL", OS_CANCEL | OS_UPDATE_AUDIO);
}

void audio_options_t::undo_hook()
{
	clearstatus(OS_RELOAD | OS_RESTART | OS_UPDATE);
	setstatus(OS_UPDATE_AUDIO);
}


void control_options_t::build()
{
	big(); label("Control Options"); small();
	xoffs = 0.67;
	space(4);
		yesno("Use Joystick", &prf->use_joystick,
				OS_RESTART_INPUT);
		yesno("Use Mouse", &prf->use_mouse,
				OS_RESTART_INPUT | OS_REBUILD);
		if(prf->use_mouse)
		{
			list("Mouse Control Mode", &prf->mousemode,
					OS_RESTART_INPUT);
				item("Disabled", MMD_OFF);
				item("Crosshair", MMD_CROSSHAIR);
//				item("Relative", MMD_RELATIVE);
		}
		yesno("Broken NumPad Diagonals", &prf->broken_numdia, 0);
		list("Diagonals Emphasis Filter", &prf->dia_emphasis, 0);
			item("OFF", 0);
			item("1 frame", 1);
			item("2 frames", 2);
			item("3 frames", 3);
			item("4 frames", 4);
			item("5 frames", 5);
	space(2);
	big();
		button("ACCEPT", OS_CLOSE);
		button("CANCEL", OS_CANCEL);
}


void game_options_t::build()
{
	big(); label("Game Options"); small();
	xoffs = 0.6;
	space(4);
//		spin("Game Speed", &prf->wait_msec, 10, 100, "ms/frame",
//				OS_UPDATE_ENGINE);
		onoff("Scrolling Radar", &prf->scrollradar, 0);
		onoff("Motion Interpolation", &prf->filter,
				OS_UPDATE_ENGINE);
	space(2);
	big();
		button("ACCEPT", OS_CLOSE);
		button("CANCEL", OS_CANCEL | OS_UPDATE_ENGINE);
}

void game_options_t::undo_hook()
{
	clearstatus(OS_RELOAD | OS_RESTART | OS_UPDATE);
	setstatus(OS_UPDATE_ENGINE);
}

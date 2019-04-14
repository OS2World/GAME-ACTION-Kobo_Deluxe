/*(LGPL)
---------------------------------------------------------------------------
	a_commands.c - Asynchronous Command Interface for the audio engine
---------------------------------------------------------------------------
 * Copyright (C) 2001. 2002, David Olofson
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

#include <stdlib.h>

#include "a_globals.h"

/*
 * These really shouldn't be here, but are
 * required for some shortcuts around here...
 */
#include "a_struct.h"
#include "a_sequencer.h"
#include "a_control.h"

sfifo_t commands;

static inline void __push_command(command_t *cmd)
{
	if(sfifo_space(&commands) < sizeof(command_t))
	{
		if(_audio_running)
			fprintf(stderr, "WARNING: Audio command FIFO overflow!\n");
		return;
	}
	sfifo_write(&commands, cmd, (unsigned)sizeof(command_t));
}


/*----------------------------------------------------------
	Mixer Control
----------------------------------------------------------*/
void audio_bus_controlf(unsigned bus, unsigned slot, unsigned ctl, float arg)
{
	audio_bus_control(bus, slot, ctl, (int)(arg * 65536.0));
}

void audio_bus_control(unsigned bus, unsigned slot, unsigned ctl, int arg)
{
	command_t cmd;
#ifdef AUDIO_SAFE
	if(bus >= AUDIO_MAX_BUSSES)
	{
		fprintf(stderr, "audio_bus_control(): Bus out of range!\n");
		return;
	}
	if(ctl >= ABC_COUNT)
	{
		fprintf(stderr, "audio_bus_control(): Control out of range!\n");
		return;
	}
#endif
	cmd.action = CMD_MCONTROL;
	cmd.cid = (signed char)bus;
	cmd.index = ctl;
	cmd.arg1 = (int)slot;
	cmd.arg2 = arg;
	__push_command(&cmd);
}


/*----------------------------------------------------------
	Group Control
----------------------------------------------------------*/

void audio_group_stop(unsigned gid)
{
	int i;
#ifdef AUDIO_SAFE
	if(gid >= AUDIO_MAX_GROUPS)
	{
		fprintf(stderr, "audio_group_stop(): Group out of range!\n");
		return;
	}
#endif
/*
FIXME: This isn't exactly thread safe...
*/
	for(i = 0; i < AUDIO_MAX_VOICES; i++)
	{
		audio_channel_t *c;
		audio_voice_t *v = voicetab + i;
		if(VS_STOPPED == v->state)
			continue;
		c = v->channel;
		if((unsigned)c->ctl[ACC_GROUP] == gid)
			audio_channel_stop(i, -1);
	}
}

void audio_group_controlf(unsigned gid, unsigned ctl, float arg)
{
	audio_group_control(gid, ctl, (int)(arg * 65536.0));
}

void audio_group_control(unsigned gid, unsigned ctl, int arg)
{
	command_t cmd;
#ifdef AUDIO_SAFE
	if(gid >= AUDIO_MAX_GROUPS)
	{
		fprintf(stderr, "audio_group_control(): Group out of range!\n");
		return;
	}
	if(ctl >= ACC_COUNT)
	{
		fprintf(stderr, "audio_group_control(): Control out of range!\n");
		return;
	}
#endif
	cmd.action = CMD_GCONTROL;
	cmd.cid = (char)gid;
	cmd.index = ctl;
	cmd.arg1 = arg;
	__push_command(&cmd);
}


/*----------------------------------------------------------
	Channel Control
----------------------------------------------------------*/

void audio_channel_play(int cid, int tag, int pitch, int velocity)
{
	command_t cmd;
#ifdef AUDIO_SAFE
	if(cid < 0 || cid >= AUDIO_MAX_CHANNELS)
	{
		fprintf(stderr, "audio_play(): Channel out of range!\n");
		return;
	}
#endif
	cmd.action = CMD_PLAY;
	cmd.cid = cid;
	cmd.tag = tag;
	cmd.arg1 = pitch;
	cmd.arg2 = velocity;
	__push_command(&cmd);
}


void audio_channel_controlf(int cid, int tag, int ctl, float arg)
{
	if(ACC_IS_FIXEDPOINT(ctl))
		arg *= 65536.0;
	audio_channel_control(cid, tag, ctl, (int)arg);
}

void audio_channel_control(int cid, int tag, int ctl, int arg)
{
	command_t cmd;
#ifdef AUDIO_SAFE
	if(cid < 0 || cid >= AUDIO_MAX_CHANNELS)
	{
		fprintf(stderr, "audio_channel_control(): Channel out of range!\n");
		return;
	}
	if(ctl < 0 || ctl >= ACC_COUNT)
	{
		fprintf(stderr, "audio_channel_control(): Control out of range!\n");
		return;
	}
#endif
	cmd.action = CMD_CCONTROL;
	cmd.cid = cid;
	cmd.tag = tag;
	cmd.index = (unsigned char)ctl;
	cmd.arg1 = arg;
	__push_command(&cmd);
}


void audio_channel_stop(int cid, int tag)
{
	command_t cmd;
#ifdef AUDIO_SAFE
	if(cid >= AUDIO_MAX_CHANNELS)
	{
		fprintf(stderr, "audio_stop(): Channel out of range!\n");
		return;
	}
#endif
	if(cid >= 0)
		cmd.action = CMD_STOP;
	else
		cmd.action = CMD_STOP_ALL;
	cmd.cid = cid;
	cmd.tag = tag;
	__push_command(&cmd);
}


int audio_channel_playing(int cid)
{
	return channeltab[cid].playing;
}



/*----------------------------------------------------------
	Simple Wrappers for Music & Sound Effects
----------------------------------------------------------*/

void play_init(void)
{
	audio_channel_control(0, AVT_ALL, ACC_PRIORITY, 0);
	audio_channel_control(0, AVT_ALL, ACC_PITCH, 60<<16);

	audio_channel_control(1, AVT_ALL, ACC_PRIORITY, 0);
	audio_channel_control(1, AVT_ALL, ACC_PITCH, 60<<16);

	audio_channel_control(2, AVT_ALL, ACC_PRIORITY, 10);
	audio_channel_control(2, AVT_ALL, ACC_PITCH, 60<<16);
}


void music_play(int channel, int wid)
{
//printf("music_play(%d)\n", wid);
	audio_channel_control(channel, AVT_FUTURE, ACC_PATCH, wid);
	audio_channel_play(channel, 0, 60<<16, 65536);
}


/*----------------------------------------------------------
	Primitive 2D Positional SFX System
----------------------------------------------------------*/

static int sfx2d_tag = 0;

static int listener_x = 0;
static int listener_y = 0;
static int wrap_x = 0;
static int wrap_y = 0;
static int scale = 65536 / 1000;
static int panscale = 65536 / 700;

void sound_position(int x, int y)
{
	listener_x = x;
	listener_y = y;
}


void sound_wrap(int w, int h)
{
	wrap_x = w;
	wrap_y = h;
}


void sound_scale(int maxrange, int pan_maxrange)
{
	scale = 65536 / maxrange;
	panscale = 65536 / pan_maxrange;
}


void sound_play(int wid, int x, int y)
{
	int volume, vx, vy, pan;

	/* Calculate volume */
	x -= listener_x;
	y -= listener_y;
	if(wrap_x)
	{
		while(x < 0)
			x += wrap_x;
		x %= wrap_x;
		if(x > wrap_x / 2)
			x -= wrap_x;
	}
	if(wrap_y)
	{
		while(y < 0)
			y += wrap_y;
		y %= wrap_y;
		if(y > wrap_y / 2)
			y -= wrap_y;
	}

	/* Approximation of distance attenuation */
	vx = abs(x * scale);
	vy = abs(y * scale);
	if((vx | vy) & 0xffff0000)
		return;

	vx = (65536 - vx) >> 1;
	vy = (65536 - vy) >> 1;
	volume = vx * vy >> 14;
	volume = (volume>>1) * (volume>>1) >> 14;

	pan = x * panscale;
	if(pan < -65536)
		pan = -65536;
	else if(pan > 65536)
		pan = 65536;

	audio_channel_control(2, AVT_FUTURE, ACC_PATCH, wid);
	audio_channel_control(2, AVT_FUTURE, ACC_PAN, pan);
	audio_channel_control(2, AVT_FUTURE, ACC_VOLUME, volume);
	audio_channel_play(2, sfx2d_tag, 60<<16, 65536);
	sfx2d_tag = (sfx2d_tag + 1) & 0xffff;
}


void sound_play0(int wid)
{
/*printf("sound_play0(%d)\n", wid);*/
	audio_channel_control(2, AVT_FUTURE, ACC_PATCH, wid);
	audio_channel_control(2, AVT_FUTURE, ACC_PAN, 0);
	audio_channel_control(2, AVT_FUTURE, ACC_VOLUME, 65536);
	audio_channel_play(2, sfx2d_tag, 60<<16, 65536);
	sfx2d_tag = (sfx2d_tag + 1) & 0xffff;
}

/*(LGPL)
---------------------------------------------------------------------------
	a_wave.c - Wava Data Manager
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "a_globals.h"
#include "a_wave.h"
#include "a_struct.h"
#include "a_math.h"
#include "a_tools.h"
#include "a_agw.h"
#include "eel.h"

static int _was_init = 0;

void audio_wave_open(void)
{
	if(_was_init)
		return;

	memset(wavetab, 0, sizeof(wavetab));
	_was_init = 1;
}

void audio_wave_close(void)
{
	if(!_was_init)
		return;

	audio_wave_free(-1);

	_was_init = 0;
}

#define	CHECKINIT	if(!_was_init) audio_wave_open();


/*----------------------------------------------------------
	Internal Tools
----------------------------------------------------------*/

/*
 * Fill in the loop/interpolation extension zone with the right
 * data, depending on whether the waveform is looped or not.
 *
 * (This should probably be replaced with smarter mixers. Cheap
 * sample players like the EMU8000 chips, have the same problem
 * with interpolation over loop wraps, which causes various
 * problems with anything but plain "loop forever" samples.)
 */
static void _render_extension(int wid)
{
	Sint8 *eos;
	if(AF_MIDI == wavetab[wid].format)
		return;

	eos = wavetab[wid].data.si8 + wavetab[wid].size;
	if(wavetab[wid].looped)
	{
		unsigned s, from = 0;
		for(s = 0; s < wavetab[wid].xsize; ++s)
		{
			eos[s] = wavetab[wid].data.si8[from++];
			if(from >= wavetab[wid].size)
				from = 0;
		}
	}
	else
		memset(eos, 0, wavetab[wid].xsize);
}


static void _calc_info(int wid)
{
	wavetab[wid].speed = (unsigned)((double)wavetab[wid].rate * 65536.0 /
			(double)a_settings.samplerate);
	switch(wavetab[wid].format)
	{
	  case AF_MONO8:
		wavetab[wid].samples = wavetab[wid].size;
		break;
	  case AF_STEREO8:
	  case AF_MONO16:
		wavetab[wid].samples = wavetab[wid].size / 2;
		break;
	  case AF_STEREO16:
	  case AF_MONO32:
		wavetab[wid].samples = wavetab[wid].size / 4;
		break;
	  case AF_STEREO32:
		wavetab[wid].samples = wavetab[wid].size / 8;
		break;
	  case AF_MIDI:
		wavetab[wid].samples = 1;
		break;
	}
}


void audio_wave_prepare(int wid)
{
	int w, first, last;
	if(wid < 0)
	{
		first = 0;
		last = AUDIO_MAX_WAVES - 1;
	}
	else
		first = last = wid;
	for(w = first; w <= last; ++w)
	{
		if(!wavetab[w].allocated)
			continue;
		_calc_info(w);
		_render_extension(w);
	}
}


static int _get_free_wid(void)
{
	int w;
	for(w = 0; w < AUDIO_MAX_WAVES; ++w)
		if(!wavetab[w].allocated)
			return w;
	return -1;
}


static int LoadRAW_S8(const char *name, Uint8 **data, Uint32 *size)
{
	FILE *f = fopen(name, "rb");
	if(!f)
		return -1;

	if(fseek(f, 0, SEEK_END) == 0)
	{
		int s = (int)ftell(f);
		if(s < 0)
		{
			fclose(f);
			return -2;
		}
		*size = (Uint32)s;
		if(fseek(f, 0, SEEK_SET) == 0)
		{
			*data = malloc(*size);
			if(*data)
			{
				if(fread(*data, *size, 1, f) == 1)
				{
					fclose(f);
					return 0;
				}
				free(*data);
				*data = NULL;
			}
		}
	}
	fclose(f);
	return -3;
}


/*
 * Calculate waveform memory needed, including the extra bytes
 * needed for proper end-of-waveform handling. Will set the
 * 'xsize' and 'play_samples' field.
 *
 * Must know sample format and original size!
 *
 * Also note that this is heavily dependent on the voice
 * mixer - that's where the extra samples are needed.
FIXME: ...which means that this code probably belongs there,
FIXME: or that the voice mixer should set the parameters.
 *
 * Returns the total size required in bytes.
 */
static unsigned _calc_xsize(int wid)
{
	unsigned samples, fsamples;
	unsigned bytes_sample = 1;
	unsigned ssize = wavetab[wid].size;
	switch(wavetab[wid].format)
	{
	  case AF_MONO8:
		bytes_sample = 1;
		break;
	  case AF_STEREO8:
	  case AF_MONO16:
		bytes_sample = 2;
		ssize >>= 1;
		break;
	  case AF_STEREO16:
	  case AF_MONO32:
		bytes_sample = 4;
		ssize >>= 2;
		break;
	  case AF_STEREO32:
		bytes_sample = 8;
		ssize >>= 3;
		break;
	  case AF_MIDI:
		wavetab[wid].xsize = 0;
		return 0;
	}

	/* Fixed part: */
	/*	Looping. */
	samples = ssize;
	while(samples < MIN_LOOP)
		samples <<= 1;
	wavetab[wid].play_samples = samples;
	samples -= ssize;

	/*	Interpolation. */
	samples += 3;

	/* Freq. ratio dependent part: oversampling and looping */
	fsamples = AUDIO_MAX_MIX_RATE / AUDIO_MIN_OUTPUT_RATE;
	fsamples *= AUDIO_MAX_OVERSAMPLING;
	++fsamples;

	wavetab[wid].xsize = bytes_sample * (samples + fsamples);
	return wavetab[wid].size + wavetab[wid].xsize;
}


/*----------------------------------------------------------
	Basic Wave API
----------------------------------------------------------*/

int audio_wave_alloc(int wid)
{
	CHECKINIT
	if(wid >= AUDIO_MAX_WAVES)
		return -1;
	if(wid < 0)
		wid = _get_free_wid();
	if(wid < 0)
		return -2;

	audio_wave_free(wid);
	wavetab[wid].allocated = 1;
	return wid;
}


int audio_wave_alloc_range(int first_wid, int last_wid)
{
	int w, res, last_done;
	if(last_wid < first_wid)
		return -1;
	last_done = -1;
	res = 0;
	for(w = first_wid; w <= last_wid; ++w)
	{
		res = audio_wave_alloc(w);
		if(res < 0)
			break;
		last_done = w;
	}

	if(res < 0)
	{
		if(last_done >= 0)
			for(w = first_wid; w <= last_done; ++w)
				audio_wave_free(w);
		return -2;
	}
	else
		return first_wid;
}


audio_wave_t *audio_wave_get(int wid)
{
	if(wid < 0)
		return NULL;
	if(wid >= AUDIO_MAX_WAVES)
		return NULL;
	return &wavetab[wid];
}


int audio_wave_format(int wid, audio_formats_t fmt, int fs)
{
	unsigned old_xsize, new_size;
	wid = audio_wave_alloc(wid);
	if(wid < 0)
		return wid;

	old_xsize = wavetab[wid].xsize;
	wavetab[wid].format = fmt;
	wavetab[wid].rate = fs;
	if(wavetab[wid].data.si8)
	{
		new_size = _calc_xsize(wid);
		if(wavetab[wid].xsize != old_xsize)
		{
			void *ndata = realloc(wavetab[wid].data.si8, new_size);
			if(!ndata)
				return -3;
			wavetab[wid].data.si8 = ndata;
			_calc_info(wid);
		}
	}
	return wid;
}


int audio_wave_load_mem(int wid, void *data, unsigned size, int looped)
{
	wid = audio_wave_alloc(wid);
	if(wid < 0)
		return wid;
	
	wavetab[wid].size = size;
	wavetab[wid].looped = looped;
	wavetab[wid].data.si8 = (Sint8 *)malloc(_calc_xsize(wid));
	if(!wavetab[wid].data.si8)
	{
		audio_wave_free(wid);
		return -1;
	}

	if(data)
		memcpy(wavetab[wid].data.si8, data, size);
	else
		memset(wavetab[wid].data.si8, 0, size);

#ifdef xDEBUG
	{
		int i;
		int peak = 0;
		double avg = 0;
		double power = 0;
		int iavg, ipower;
		for(i = 0; i < size; ++i)
		{
			int s;
			s = wavetab[wid].data.si8[i];
			avg += s;
			power += labs(s);
			if(labs(s) > peak)
				peak = labs(s);
		}
		avg /= size;
		power /= size;
		iavg = (int)avg;
		ipower = (int)power;
		printf("audio_wave_load_mem(id=%d): size=%d  peak=%d"
				"  average=%d  power=%d\n",
				wid, size, peak, iavg, ipower);
	}
#endif
	_calc_info(wid);
	return wid;
}


int audio_wave_blank(int wid, unsigned samples, int looped)
{
	int bps = 0;

	wid = audio_wave_alloc(wid);
	if(wid < 0)
		return wid;

	switch(wavetab[wid].format)
	{
	  case AF_MONO8:
		bps = 1;
		break;
	  case AF_STEREO8:
	  case AF_MONO16:
		bps = 2;
		break;
	  case AF_STEREO16:
	  case AF_MONO32:
		bps = 4;
		break;
	  case AF_STEREO32:
		bps = 8;
		break;
	  case AF_MIDI:
		return wid;
	}
	return audio_wave_load_mem(wid, NULL, samples * bps, looped);
}


int audio_wave_convert(int wid, int new_wid, audio_formats_t fmt,
		int fs, audio_resample_t resamp)
{
	int inplace, private_pool = 0, i;
	audio_voice_t resampler;
	audio_quality_t	old_quality;
	int *bus;
	Sint8 *out8;
	Sint16 *out16;
	float *out32;
	unsigned newlen, j;

	/* We need this to run the voice mixer... */
	if(ptab_init(65536) < 0)
	{
		fprintf(stderr, "audio_wave_convert(): ptab_init() failed!\n");
		return -20;
	}

	if(AF_MIDI == fmt)
	{
		fprintf(stderr, "audio_wave_convert(): Cannot convert to MIDI!\n");
		return -10;
	}

	if(wid >= AUDIO_MAX_WAVES)
		return -1;
	if(wid < 0)
		return -2;

	if(AF_MIDI == wavetab[wid].format)
	{
		fprintf(stderr, "audio_wave_convert(): Cannot convert from MIDI!\n");
		return -11;
	}

	if(new_wid == wid)
	{
		inplace = 1;
		new_wid = audio_wave_alloc(-1);
		if(new_wid < 0)
			return new_wid;
	}
	else
	{
		inplace = 0;
		new_wid = audio_wave_alloc(new_wid);
		if(new_wid < 0)
			return new_wid;
	}
	audio_wave_format(new_wid, fmt, fs);
	newlen = (unsigned)ceil((float)wavetab[wid].samples * (float)fs /
			(float)wavetab[wid].rate);
	audio_wave_blank(new_wid, newlen, wavetab[wid].looped);

	/* Must prepare, as we're gonna use the wave mixer! */
	audio_wave_prepare(wid);

	memset(&resampler, 0, sizeof(resampler));

	/* We need to tweak the 'speed' to get the output rate right! */
	wavetab[wid].speed = (unsigned)((double)wavetab[wid].rate * 65536.0 /
			(double)wavetab[new_wid].rate);

	old_quality = a_settings.quality;
	a_settings.quality = AQ_VERY_HIGH;

	if(!aev_event_pool)
	{
		private_pool = 1;
		aev_open(20);
	}

	switch(resamp)
	{
	  case AR_WORST:
		resamp = AR_NEAREST;
		break;
	  case AR_MEDIUM:
		resamp = AR_LINEAR_2X_R;
		break;
	  case AR_BEST:
		resamp = AR_CUBIC_R;
		break;
	  default:
		break;
	}

	aev_timer = 0;
	(void)aev_send1(&resampler.port, 0, VE_START, wid);
	(void)aev_sendi1(&resampler.port, 0, VE_SET, VC_PITCH, 60<<16);
	(void)aev_sendi1(&resampler.port, 0, VE_SET, VC_RESAMPLE, resamp);
	(void)aev_sendi1(&resampler.port, 0, VE_SET, VC_SEND_BUS, -1);
	(void)aev_sendi1(&resampler.port, 0, VE_ISET, VIC_LVOL,
			65536 >> (16-VOL_BITS));
	(void)aev_sendi1(&resampler.port, 0, VE_ISET, VIC_RVOL,
			65536 >> (16-VOL_BITS));
	bus = malloc(256 * sizeof(int) * 2);
	out8 = wavetab[new_wid].data.si8;
	out16 = wavetab[new_wid].data.si16;
	out32 = wavetab[new_wid].data.f32;
	for(i = (int)wavetab[new_wid].samples; i > 0; i -= 256)
	{
		unsigned frames;
		if(i > 256)
			frames = 256;
		else
			frames = (unsigned)i;
		s32clear(bus, frames);
		voice_process_mix(&resampler, &bus, frames);
		switch(wavetab[new_wid].format)
		{
		  case AF_MONO8:
			for(j = 0; j < frames; ++j)
				*out8++ = (bus[j<<1] + bus[(j<<1)+1]) >> 9;
			break;
		  case AF_STEREO8:
			for(j = 0; j < frames; ++j)
			{
				*out8++ = bus[j<<1] >> 8;
				*out8++ = bus[(j<<1)+1] >> 8;
			}
			break;
		  case AF_MONO16:
			for(j = 0; j < frames; ++j)
				*out16++ = (Sint16)(bus[j<<1] +
						bus[(j<<1)+1]) >> 1;
			break;
		  case AF_STEREO16:
			for(j = 0; j < frames; ++j)
			{
				*out16++ = (Sint16)(bus[j<<1]);
				*out16++ = (Sint16)(bus[(j<<1)+1]);
			}
			break;
		  case AF_MONO32:
			for(j = 0; j < frames; ++j)
				*out32++ = (float)(bus[j<<1] +
						bus[(j<<1)+1]) * 0.5;
			break;
		  case AF_STEREO32:
			for(j = 0; j < frames; ++j)
			{
				*out32++ = (float)bus[j<<1];
				*out32++ = (float)bus[(j<<1)+1];
			}
			break;
		  case AF_MIDI:	/* whinestopper... */
			break;
		}
	}
	free(bus);
	_calc_info(new_wid);

	voice_kill(&resampler);
	a_settings.quality = old_quality;

	if(private_pool)
		aev_close();

	if(inplace)
	{
		audio_wave_free(wid);
		memcpy(wavetab + wid, wavetab + new_wid, sizeof(audio_wave_t));
		memset(wavetab + new_wid, 0, sizeof(audio_wave_t));
		return wid;
	}
	else
	{
		_calc_info(wid);	/* Restore after our tweaking */
		return new_wid;
	}
}


int audio_wave_clone(int wid, int new_wid)
{
	if(wid >= AUDIO_MAX_WAVES)
		return -1;
	if(wid < 0)
		return -2;
	new_wid = audio_wave_format(new_wid, wavetab[wid].format,
			wavetab[wid].rate);
	if(new_wid < 0)
		return new_wid;
	return audio_wave_load_mem(new_wid, wavetab[wid].data.si8,
			wavetab[wid].size, wavetab[wid].looped);
}


/* We're simply using the same path for everything. */
void audio_set_path(const char *path)
{
	eel_set_path(path);
}


const char *audio_path(void)
{
	return eel_path();
}


static int load_midi(int wid, const char *name)
{
	midi_file_t *mf;

	wid = audio_wave_alloc(wid);
	if(wid < 0)
		return wid;

	mf = mf_open(name);
	if(!mf)
	{
		fprintf(stderr, "load_midi(): Failed to load file"
				" \"%s\"! (Path = \"%s\")\n", name,
				eel_path());
		audio_wave_free(wid);
		return -1;
	}

	wavetab[wid].data.midi = mf;

	wavetab[wid].size = 1;	/* Duration in ms or something? */
	wavetab[wid].xsize = 0;	/* N/A */
	wavetab[wid].howtofree = HTF_FREE;

	wavetab[wid].format = AF_MIDI;
	wavetab[wid].rate = 120;	/* PPQN? */
	wavetab[wid].looped = 0;	/* Not yet implemented */

	wavetab[wid].speed = 120;	/* ? */
	wavetab[wid].samples = 1;	/* Number of events? */

	printf(".------------------------------------------------------\n");
	printf("| MIDI File: %s\n", name);
	printf("|    Format: %u\n", mf->format);
	printf("|     Title: %s\n", mf->title);
	printf("|    Author: %s\n", mf->author);
	printf("|   Remarks: %s\n", mf->remarks);
	printf("'------------------------------------------------------\n");

	return wid;
}


int audio_wave_load(int wid, const char *name, int looped)
{
	char buf[1024];
	SDL_AudioSpec spec;
	Uint8 *data = NULL;
	Uint32 size;
	int res;
	int using_loadwav = 0;

	/* Prepend path */
	strncpy(buf, eel_path(), sizeof(buf));
#ifdef WIN32
	strncat(buf, "\\", sizeof(buf));
#elif defined MACOS
	strncat(buf, ":", sizeof(buf));
#else
	strncat(buf, "/", sizeof(buf));
#endif
	strncat(buf, name, sizeof(buf));

	/* Check extension */
	if(strstr(name, ".raw") || strstr(name, ".RAW"))
		res = LoadRAW_S8(buf, &data, &size);
	else if(strstr(name, ".agw") || strstr(name, ".AGW"))
		return agw_load(wid, name);	/* No full path here! */
	else if(strstr(name, ".mid") || strstr(name, ".MID"))
		return load_midi(wid, buf);
	else
	{
		using_loadwav = 1;
		res = SDL_LoadWAV(buf, &spec, &data, &size) ? 0 : -1;
	}

	wid = audio_wave_alloc(wid);
	if(wid < 0)
		return wid;

	if(using_loadwav)
	{
		switch (spec.format)
		{
		  case AUDIO_S8:
			wavetab[wid].format = AF_MONO8;
			break;
		  case AUDIO_S16SYS:
			wavetab[wid].format = AF_MONO16;
			break;
		  default:
			fprintf(stderr, "sound_load(): Unsupported wave format!\n");
			SDL_FreeWAV((Uint8 *)(wavetab[wid].data.si8));
			res = -1;
			break;
		}
		if(spec.channels == 2)
			++wavetab[wid].format;
		wavetab[wid].rate = spec.freq;
	}
	if(res < 0)
	{
		fprintf(stderr, "audio_wave_load(): Failed to load file"
				" \"%s\"! (Path = \"%s\")\n", name,
				eel_path());
		audio_wave_free(wid);
		return -3;
	}

	if(data)
		audio_wave_load_mem(wid, data, size, looped);

	if(using_loadwav)
		SDL_FreeWAV(data);
	else
		free(data);

	return wid;
}


void audio_wave_free(int wid)
{
	int w, first, last;
	CHECKINIT
	if(wid < 0)
	{
		first = 0;
		last = AUDIO_MAX_WAVES - 1;
	}
	else
		first = last = wid;
	for(w = first; w <= last; ++w)
	{
		if(!wavetab[w].data.si8)
			continue;
		if(HTF_FREE == wavetab[w].howtofree)
			switch(wavetab[w].format)
			{
			  case AF_MONO8:
			  case AF_STEREO8:
			  case AF_MONO16:
			  case AF_STEREO16:
			  case AF_MONO32:
			  case AF_STEREO32:
				free(wavetab[w].data.si8);
				break;
			  case AF_MIDI:
				mf_close(wavetab[w].data.midi);
				break;
			}
		wavetab[w].data.si8 = NULL;
		wavetab[w].size = 0;
		wavetab[w].xsize = 0;
		wavetab[w].allocated = 0;
	}
}

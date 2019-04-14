/*
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 2001. 2002, David Olofson
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

#define	DBG(x)

#include "config.h"
#include "cfgform.h"
#include "kobo.h"
#include "audio.h"

int global_status = 0;


config_form_t::config_form_t()
{
	prf = NULL;
	stat = 0;
}


config_form_t::~config_form_t()
{
}


void config_form_t::open(prefs_t *p)
{
	DBG(printf("config_form_t::open()\n");)
	init(&gengine);
	place(wchip.x(), wchip.y(), wchip.width(), wchip.height());
	font(B_NORMAL_FONT);
	foreground(wbase.map_rgb(0xffffff));
	background(wbase.map_rgb(0x000000));
	prf = p;
	prfbak = *p;
	stat = global_status & (OS_RELOAD | OS_RESTART | OS_UPDATE);
	build_all();
}


void config_form_t::close()
{
	clean();
}


void config_form_t::undo()
{
	*prf = prfbak;
	undo_hook();
}


int config_form_t::status()
{
	return stat;
}


void config_form_t::clearstatus(int mask)
{
	stat &= ~mask;
	global_status = stat & (OS_RELOAD | OS_RESTART | OS_UPDATE);
}


void config_form_t::setstatus(int mask)
{
	stat |= mask;
	global_status = stat & (OS_RELOAD | OS_RESTART | OS_UPDATE);
}

/*virtual*/ void config_form_t::change(int delta)
{
	kobo_form_t::change(delta);

	if(!selected())
		return;

	if(selected()->tag & OS_CLOSE)
	{
		if(delta == 0)
		{
			sound_play0(SOUND_EXPL);
			setstatus(OS_CLOSE);
		}
	}
	else if(selected()->tag & OS_CANCEL)
	{
		if(delta == 0)
		{
			sound_play0(SOUND_SHOT);
			undo();
			setstatus(OS_CLOSE);
		}
	}
	else
		sound_play0(SOUND_METALLIC);

	if(selected()->user)
		prf->changed = 1;

	setstatus(selected()->tag & (OS_RELOAD | OS_RESTART | OS_UPDATE));

	switch(prf->width)
	{
	  case 320: prf->height = 240; break;
	  case 400: prf->height = 300; break;
	  case 512: prf->height = 384; break;
	  case 640: prf->height = 480; break;
	  case 800: prf->height = 600; break;
	  case 960: prf->height = 720; break;
	  case 1024: prf->height = 768; break;
	  case 1152: prf->height = 864; break;
	  case 1280: prf->height = 1024; break;
	}

	if(selected()->tag & OS_REBUILD)
	{
		int sel = selected_index();
		build_all();
		select(sel);
	}
}

/* virtual */void config_form_t::build()
{
}

/* virtual */ void config_form_t::undo_hook()
{
	stat = 0;
	global_status = 0;
}

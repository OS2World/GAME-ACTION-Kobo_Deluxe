/*(LGPL)
 * Simple display for score, lives etc.
 * © David Olofson, 2001, 2002
 */

#define	D_LINE_HEIGHT	9

#define	D_LINE1_POS	0
#define	D_LINE1_TXOFFS	0

#define	D_LINE2_POS	9
#define	D_LINE2_TXOFFS	0

#include <string.h>

#include "window.h"
#include "display.h"

display_t::display_t()
{
	visible = 0;
	caption("CAPTION");
	text("TEXT");
	on();
}


void display_t::color(Uint32 _cl)
{
	_color = _cl;
	if(visible)
	{
		render_caption();
		render_text();
	}
}


void display_t::caption(const char *cap)
{
	strncpy(_caption, cap, sizeof(_caption));
	if(visible)
		render_caption();
}


void display_t::text(const char *txt)
{
	strncpy(_text, txt, sizeof(_text));
	if(visible)
		render_text();
}


void display_t::on()
{
	if(visible)
		return;

	visible = 1;
	render_caption();
	render_text();
}


void display_t::off()
{
	if(!visible)
		return;

	visible = 0;
	background(_color);
	SDL_Rect r;
	r.x = 0;
	r.y = D_LINE1_POS;
	r.w = width();
	r.h = D_LINE_HEIGHT * 2;
	clear(&r);
//	fillrect(0, D_LINE1_POS, width(), D_LINE_HEIGHT * 2);
	invalidate();
}


void display_t::render_caption()
{
	SDL_Rect r;
	r.x = 0;
	r.y = D_LINE1_POS;
	r.w = width();
	r.h = D_LINE_HEIGHT;
	background(_color);
	clear(&r);
//	fillrect(0, D_LINE1_POS, width(), D_LINE_HEIGHT);
	center(D_LINE1_POS + D_LINE1_TXOFFS, _caption);

	invalidate(&r);
}


void display_t::render_text()
{
	SDL_Rect r;
	r.x = 0;
	r.y = D_LINE2_POS;
	r.w = width();
	r.h = D_LINE_HEIGHT;
	background(_color);
	clear(&r);
////	fillrect(0, D_LINE2_POS, width(), D_LINE_HEIGHT);
	center(D_LINE2_POS + D_LINE2_TXOFFS, _text);

	invalidate(&r);
}

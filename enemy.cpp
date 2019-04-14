/*(GPL)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 1995, 1996, Akira Higuchi
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

#include "screen.h"
#include "manage.h"
#include "enemies.h"
#include "myship.h"
#include "random.h"
#include "audio.h"

/*
 * ===========================================================================
 *                                (template)
 * ===========================================================================
 */

inline void _enemy::move_enemy_template(int quick, int maxspeed)
{
	if(diffx > 0)
	{
		if(h > -maxspeed)
			h -= quick;
	}
	else if(diffx < 0)
	{
		if(h < maxspeed)
			h += quick;
	}
	if(diffy > 0)
	{
		if(v > -maxspeed)
			v -= quick;
	}
	else if(diffy < 0)
	{
		if(v < maxspeed)
			v += quick;
	}

	int m = MAX(MAX(h, -(h)), MAX(v, -(v)));
	if(m == h)
	{
		if((m >> 1) < v)
			di = 4;
		else if((m >> 1) < -(v))
			di = 2;
		else
			di = 3;
	}
	else if(m == -(h))
	{
		if((m >> 1) < v)
			di = 6;
		else if((m >> 1) < -(v))
			di = 8;
		else
			di = 7;
	}
	else if(m == v)
	{
		if((m >> 1) < h)
			di = 4;
		else if((m >> 1) < -(h))
			di = 6;
		else
			di = 5;
	}
	else
	{
		if((m >> 1) < h)
			di = 2;
		else if((m >> 1) < -(h))
			di = 8;
		else
			di = 1;
	}
}
inline void _enemy::move_enemy_template_2(int quick, int maxspeed)
{
	h = -PIXEL2CS(diffy) / (1<<quick);
	v = PIXEL2CS(diffx) / (1<<quick);

	if(diffx > 0)
	{
		if(h > -maxspeed)
			h -= quick;
	}
	else if(diffx < 0)
	{
		if(h < maxspeed)
			h += quick;
	}
	if(diffy > 0)
	{
		if(v > -maxspeed)
			v -= quick;
	}
	else if(diffy < 0)
	{
		if(v < maxspeed)
			v += quick;
	}

	int m = MAX(MAX(h, -(h)), MAX(v, -(v)));
	if(m == h)
	{
		if((m >> 1) < v)
			di = 4;
		else if((m >> 1) < -(v))
			di = 2;
		else
			di = 3;
	}
	else if(m == -(h))
	{
		if((m >> 1) < v)
			di = 6;
		else if((m >> 1) < -(v))
			di = 8;
		else
			di = 7;
	}
	else if(m == v)
	{
		if((m >> 1) < h)
			di = 4;
		else if((m >> 1) < -(h))
			di = 6;
		else
			di = 5;
	}
	else
	{
		if((m >> 1) < h)
			di = 2;
		else if((m >> 1) < -(h))
			di = 8;
		else
			di = 1;
	}
}
inline void _enemy::move_enemy_template_3(int quick, int maxspeed)
{
	h = PIXEL2CS(diffy) / (1<<quick);
	v = -PIXEL2CS(diffx) / (1<<quick);

	if(diffx > 0)
	{
		if(h > -maxspeed)
			h -= quick;
	}
	else if(diffx < 0)
	{
		if(h < maxspeed)
			h += quick;
	}
	if(diffy > 0)
	{
		if(v > -maxspeed)
			v -= quick;
	}
	else if(diffy < 0)
	{
		if(v < maxspeed)
			v += quick;
	}

	int m = MAX(MAX(h, -(h)), MAX(v, -(v)));
	if(m == h)
	{
		if((m >> 1) < v)
			di = 4;
		else if((m >> 1) < -(v))
			di = 2;
		else
			di = 3;
	}
	else if(m == -(h))
	{
		if((m >> 1) < v)
			di = 6;
		else if((m >> 1) < -(v))
			di = 8;
		else
			di = 7;
	}
	else if(m == v)
	{
		if((m >> 1) < h)
			di = 4;
		else if((m >> 1) < -(h))
			di = 6;
		else
			di = 5;
	}
	else
	{
		if((m >> 1) < h)
			di = 2;
		else if((m >> 1) < -(h))
			di = 8;
		else
			di = 1;
	}
}
inline void _enemy::shot_template(const enemy_kind * ekp,
		int shift, int rnd, int maxspeed)
{
	int vx = -diffx;
	int vy = -diffy;
	if(rnd)
	{
		vx += gamerand.get() & (rnd - 1) - (rnd >> 1);
		vy += gamerand.get() & (rnd - 1) - (rnd >> 1);
	}
	vx = PIXEL2CS(vx) / (1<<shift);
	vy = PIXEL2CS(vy) / (1<<shift);
	if(maxspeed > 0)
	{
		if(vx > maxspeed)
			vx = maxspeed;
		else if(vx < -maxspeed)
			vx = -maxspeed;
		if(vy > maxspeed)
			vy = maxspeed;
		else if(vy < -maxspeed)
			vy = -maxspeed;
	}
	enemies.make(ekp, CS2PIXEL(x + vx), CS2PIXEL(y + vy), vx,
			vy);
	if(&ring == ekp)
		sound_play(SOUND_RING, CS2PIXEL(x), CS2PIXEL(y));
	else if(&beam == ekp)
		sound_play(SOUND_BEAM, CS2PIXEL(x), CS2PIXEL(y));
	else if(&bomb1 == ekp || &bomb2 == ekp)
		sound_play(SOUND_LAUNCH2, CS2PIXEL(x), CS2PIXEL(y));
	else
		sound_play(SOUND_LAUNCH, CS2PIXEL(x), CS2PIXEL(y));
}

void _enemy::shot_template_8_dir(const enemy_kind * ekp)
{
	static int vx[] = { 0, 200, 300, 200, 0, -200, -300, -200 };
	static int vy[] = { -300, -200, 0, 200, 300, 200, 0, -200 };
	int i;
	for(i = 0; i < 8; i++)
		enemies.make(ekp, CS2PIXEL(x), CS2PIXEL(y), vx[i],
				vy[i]);
	if(&ring == ekp)
		sound_play(SOUND_RING, CS2PIXEL(x), CS2PIXEL(y));
	else if(&beam == ekp)
		sound_play(SOUND_BEAM, CS2PIXEL(x), CS2PIXEL(y));
	else if(&bomb1 == ekp || &bomb2 == ekp)
		sound_play(SOUND_LAUNCH2, CS2PIXEL(x), CS2PIXEL(y));
	else
		sound_play(SOUND_LAUNCH, CS2PIXEL(x), CS2PIXEL(y));
	sound_play(SOUND_ENEMYM, CS2PIXEL(x), CS2PIXEL(y));
}


/*
 * ===========================================================================
 *                                beam
 * ===========================================================================
 */
void _enemy::make_beam()
{
	di = 1;
//	shield = -1;
	health = 1;
	shootable = 0;
	damage = 20;
}

void _enemy::move_beam()
{
	if(norm >= ((VIEWLIMIT >> 1) + 32))
		release();
	if(++di > 8)
		di = 1;
}
const enemy_kind beam = {
	0,
	&_enemy::make_beam,
	&_enemy::move_beam,
	2,
	B_BULLETS, 0,
	8,
};

/*
 * ===========================================================================
 *                                rock
 * ===========================================================================
 */
void _enemy::make_rock()
{
	count = 500;
//	shield = 255;
	health = 255 * 20;
	damage = 1000;
	di = (gamerand.get() % 3) + 1;
}

void _enemy::move_rock()
{
	;
}
const enemy_kind rock = {
	10,
	&_enemy::make_rock,
	&_enemy::move_rock,
	4,
	B_SPRITES, 11+4*16,
	16,
};

/*
 * ===========================================================================
 *                                ring
 * ===========================================================================
 */
void _enemy::make_ring()
{
	count = 500;
//	shield = 1;
	health = 20;
	damage = 30;
	di = 1;
}

void _enemy::move_ring()
{
	;
}
const enemy_kind ring = {
	1,
	&_enemy::make_ring,
	&_enemy::move_ring,
	4,
	B_SPRITES, 15+4*16,
	16,
};

/*
 * ===========================================================================
 *                                bomb
 * ===========================================================================
 */
void _enemy::make_bomb()
{
	count = 500;
//	shield = 1;
	health = 20;
	damage = 70;
	di = 1;
}

void _enemy::move_bomb1()
{
	int h1 = ABS(diffx);
	int v1 = ABS(diffy);
	if(((h1 < 100) && (v1 < 30)) || (h1 < 30) && (v1 < 100))
	{
		int vx1 = PIXEL2CS(-diffx) / (3*8);
		int vy1 = PIXEL2CS(-diffy) / (3*8);
		int vx2 = vx1, vx3 = vx1;
		int vy2 = vy1, vy3 = vy1;
		int i;
		for(i = 0; i < 4; i++)
		{
			int tmp = vx2;
			vx2 += (vy2 >> 4);
			vy2 -= (tmp >> 4);
			tmp = vx3;
			vx3 -= (vy3 >> 4);
			vy3 += (tmp >> 4);
		}
		enemies.make(&beam, CS2PIXEL(x), CS2PIXEL(y), vx2, vy2);
		enemies.make(&beam, CS2PIXEL(x), CS2PIXEL(y), vx3, vy3);
		sound_play(SOUND_BUBBLE, CS2PIXEL(x), CS2PIXEL(y));
		release();
	}
}
const enemy_kind bomb1 = {
	5,
	&_enemy::make_bomb,
	&_enemy::move_bomb1,
	5,
	B_SPRITES, 14+4*16,
	16,
};

/*
 * ===========================================================================
 *                                bomb2
 * ===========================================================================
 */
void _enemy::move_bomb2()
{
	int h1 = ABS(diffx);
	int v1 = ABS(diffy);
	if(((h1 < 100) && (v1 < 20)) || (h1 < 20) && (v1 < 100))
	{
		int vx1 = PIXEL2CS(-diffx) / (3*8);
		int vy1 = PIXEL2CS(-diffy) / (3*8);
		int vx2 = vx1, vx3 = vx1;
		int vy2 = vy1, vy3 = vy1;
		int i;
		for(i = 0; i < 6; i++)
		{
			int tmp = vx2;
			vx2 += (vy2 >> 4);
			vy2 -= (tmp >> 4);
			tmp = vx3;
			vx3 -= (vy3 >> 4);
			vy3 += (tmp >> 4);
		}
		int vx4 = vx2, vx5 = vx3;
		int vy4 = vy2, vy5 = vy3;
		for(i = 0; i < 6; i++)
		{
			int tmp = vx2;
			vx2 += (vy2 >> 4);
			vy2 -= (tmp >> 4);
			tmp = vx3;
			vx3 -= (vy3 >> 4);
			vy3 += (tmp >> 4);
		}
		enemies.make(&beam, CS2PIXEL(x), CS2PIXEL(y), vx1, vy1);
		enemies.make(&beam, CS2PIXEL(x), CS2PIXEL(y), vx2, vy2);
		enemies.make(&beam, CS2PIXEL(x), CS2PIXEL(y), vx3, vy3);
		enemies.make(&beam, CS2PIXEL(x), CS2PIXEL(y), vx4, vy4);
		enemies.make(&beam, CS2PIXEL(x), CS2PIXEL(y), vx5, vy5);
		sound_play(SOUND_BUBBLE, CS2PIXEL(x), CS2PIXEL(y));
		release();
	}
}
const enemy_kind bomb2 = {
	20,
	&_enemy::make_bomb,
	&_enemy::move_bomb2,
	5,
	B_SPRITES, 14+4*16,
	16,
};

/*
 * ===========================================================================
 *                                explosion
 * ===========================================================================
 */
void _enemy::make_expl()
{
	di = 0;
//	shield = -1;
	health = 1;
	damage = 0;
	shootable = 0;
}

void _enemy::move_expl()
{
	if(++di > 8)
		release();
}

const enemy_kind explosion = {
	0,
	&_enemy::make_expl,
	&_enemy::move_expl,
	-1,
	B_SPRITES, 0+1*16,
	16,
};

/*
 * ===========================================================================
 *                                cannon
 * ===========================================================================
 */
void _enemy::make_cannon()
{
	count = 0;
//	shield = 1;
	health = 20;
	damage = 75;
	b = enemies.eint1() - 1;
	a = gamerand.get() & b;
}

void _enemy::move_cannon()
{
	(count)++;
	(count) &= (b);
	if(count == a && norm < ((VIEWLIMIT >> 1) + 8))
	{
		int shift = (enemies.ek1() == &beam) ? 6 : 5;
		this->shot_template(enemies.ek1(), shift, 32, 0);
	}
}
const enemy_kind cannon = {
	10,
	&_enemy::make_cannon,
	&_enemy::move_cannon,
	4,
	0, 0,
	0,
};

/*
 * ===========================================================================
 *                                core
 * ===========================================================================
 */
void _enemy::make_core()
{
	count = 0;
//	shield = 1;
	health = 20;
	damage = 150;
	b = enemies.eint2() - 1;
	a = gamerand.get() & b;
}

void _enemy::move_core()
{
	(count)++;
	(count) &= (b);
	if(count == a && norm < ((VIEWLIMIT >> 1) + 8))
	{
		int shift = (enemies.ek2() == &beam) ? 6 : 5;
		this->shot_template(enemies.ek2(), shift, 0, 0);
	}
}
const enemy_kind core = {
	200,
	&_enemy::make_core,
	&_enemy::move_core,
	4,
	0, 0,
	0,
};

/*
 * ===========================================================================
 *                                pipe1
 * ===========================================================================
 */
void _enemy::make_pipe1()
{
//	shield = -1;
	health = 1;
	damage = 0;
	shootable = 0;
	count = 4;
	a = 0;
	b = 0;
}

void _enemy::move_pipe1()
{
	if(--b < 0)
	{
		sound_play(SOUND_EXPL, CS2PIXEL(x), CS2PIXEL(y));
		b = pubrand.get(3) + 2;
	}

	if((norm < ((VIEWLIMIT >> 1) + 32)) && (count == 1))
		enemies.make(&explosion,
				CS2PIXEL(x) + pubrand.get(4) - 8,
				CS2PIXEL(y) + pubrand.get(4) - 8);
	if(++count < 4)
		return;
	count = 0;

	int x1 = (CS2PIXEL(x) & (WORLD_SIZEX - 1)) >> 4;
	int y1 = (CS2PIXEL(y) & (WORLD_SIZEY - 1)) >> 4;
	int a_next = 0;
	int x_next = 0;
	int y_next = 0;
	int p = screen.get_chip_number(x1, y1);
	if(IS_SPACE(p))
	{
		release();
		return;
	}
	if(norm < ((VIEWLIMIT >> 1) + 32))
		enemies.make(&explosion, CS2PIXEL(x), CS2PIXEL(y));
	if((p ^ a) == U_MASK)
	{
		a_next = D_MASK;
		y_next = -PIXEL2CS(16);
	}
	if((p ^ a) == R_MASK)
	{
		a_next = L_MASK;
		x_next = PIXEL2CS(16);
	}
	if((p ^ a) == D_MASK)
	{
		a_next = U_MASK;
		y_next = PIXEL2CS(16);
	}
	if((p ^ a) == L_MASK)
	{
		a_next = R_MASK;
		x_next = -PIXEL2CS(16);
	}
	if(a_next)
	{
		screen.set_chip_number(x1, y1, 0);
		x += x_next;
		y += y_next;
		a = a_next;
		return;
	}
	if(p != CORE)
	{
		p ^= a;
		screen.set_chip_number(x1, y1, p);
	}
	release();
}
const enemy_kind pipe1 = {
	0,
	&_enemy::make_pipe1,
	&_enemy::move_pipe1,
	-1,
	0, 0,
	0,
};

/*
 * ===========================================================================
 *                                pipe2
 * ===========================================================================
 */
void _enemy::make_pipe2()
{
	int x1 = (CS2PIXEL(x) & (WORLD_SIZEX - 1)) >> 4;
	int y1 = (CS2PIXEL(y) & (WORLD_SIZEY - 1)) >> 4;

	screen.set_chip_number(x1, y1, 0);
//	shield = -1;
	health = 1;
	damage = 0;
	shootable = 0;
	count = 4;
	switch (di)
	{
	  case 1:
		a = D_MASK;
		y -= PIXEL2CS(16);
		break;
	  case 3:
		a = L_MASK;
		x += PIXEL2CS(16);
		break;
	  case 5:
		a = U_MASK;
		y += PIXEL2CS(16);
		break;
	  case 7:
		a = R_MASK;
		x -= PIXEL2CS(16);
		break;
	}
	b = 0;
}


void _enemy::move_pipe2()
{
	if(--b < 0)
	{
		sound_play(SOUND_EXPL, CS2PIXEL(x), CS2PIXEL(y));
		b = pubrand.get(4) + 3;
	}

	if((norm < ((VIEWLIMIT >> 1) + 32)) && (count == 1))
		enemies.make(&explosion,
				CS2PIXEL(x) + pubrand.get(4) - 8,
				CS2PIXEL(y) + pubrand.get(4) - 8);
	if(++count < 4)
		return;
	count = 0;

	int x1 = (CS2PIXEL(x) & (WORLD_SIZEX - 1)) >> 4;
	int y1 = (CS2PIXEL(y) & (WORLD_SIZEY - 1)) >> 4;
	int a_next = 0;
	int x_next = 0;
	int y_next = 0;
	int p = screen.get_chip_number(x1, y1);
	if(IS_SPACE(p))
	{
		release();
		return;
	}
	if(norm < ((VIEWLIMIT >> 1) + 32))
		enemies.make(&explosion, CS2PIXEL(x), CS2PIXEL(y));
	if((p ^ a) == 0)
	{
		manage.add_score(30);
		release();
		enemies.erase_cannon(x1, y1);
		screen.set_chip_number(x1, y1, 0);
		return;
	}
	if((p ^ a) == HARD)
	{
		release();
		screen.set_chip_number(x1, y1, 0);
		return;
	}
	if((p ^ a) == U_MASK)
	{
		a_next = D_MASK;
		y_next = -PIXEL2CS(16);
	}
	if((p ^ a) == R_MASK)
	{
		a_next = L_MASK;
		x_next = PIXEL2CS(16);
	}
	if((p ^ a) == D_MASK)
	{
		a_next = U_MASK;
		y_next = PIXEL2CS(16);
	}
	if((p ^ a) == L_MASK)
	{
		a_next = R_MASK;
		x_next = -PIXEL2CS(16);
	}
	screen.set_chip_number(x1, y1, 0);
	if(a_next)
	{
		x += x_next;
		y += y_next;
		a = a_next;
		return;
	}
	p ^= a;
	if(p & U_MASK)
		enemies.make(&pipe2, CS2PIXEL(x), CS2PIXEL(y), 0, 0, 1);
	if(p & R_MASK)
		enemies.make(&pipe2, CS2PIXEL(x), CS2PIXEL(y), 0, 0, 3);
	if(p & D_MASK)
		enemies.make(&pipe2, CS2PIXEL(x), CS2PIXEL(y), 0, 0, 5);
	if(p & L_MASK)
		enemies.make(&pipe2, CS2PIXEL(x), CS2PIXEL(y), 0, 0, 7);
	manage.add_score(10);
	release();
}


const enemy_kind pipe2 = {
	0,
	&_enemy::make_pipe2,
	&_enemy::move_pipe2,
	-1,
	0, 0,
	0,
};

/*
 * ===========================================================================
 *                                enemy1
 * ===========================================================================
 */
void _enemy::make_enemy1()
{
	di = 1;
//	shield = 1;
	health = 20;
}

void _enemy::move_enemy1()
{
	this->move_enemy_template(2, 256);
}
const enemy_kind enemy1 = {
	2,
	&_enemy::make_enemy1,
	&_enemy::move_enemy1,
	6,
	B_SPRITES, 8+0*16,
	16,
};

/*
 * ===========================================================================
 *                                enemy2
 * ===========================================================================
 */
void _enemy::make_enemy2()
{
	di = 1;
//	shield = 1;
	health = 20;
	count = gamerand.get() & 63;
}

void _enemy::move_enemy2()
{
	this->move_enemy_template(4, 192);
	if(--(count) <= 0)
	{
		if(norm < ((VIEWLIMIT >> 1) + 8))
		{
			this->shot_template(&beam, 5, 0, 0);
		}
		count = 32;
	}
}
const enemy_kind enemy2 = {
	10,
	&_enemy::make_enemy2,
	&_enemy::move_enemy2,
	6,
	B_SPRITES, 8+1*16,
	16,
};

/*
 * ===========================================================================
 *                                enemy3
 * ===========================================================================
 */
void _enemy::make_enemy3()
{
	di = 1;
//	shield = 1;
	health = 20;
}

void _enemy::move_enemy3()
{
	this->move_enemy_template(32, 96);
}
const enemy_kind enemy3 = {
	1,
	&_enemy::make_enemy3,
	&_enemy::move_enemy3,
	6,
	B_SPRITES, 8+2*16,
	16,
};

/*
 * ===========================================================================
 *                                enemy4
 * ===========================================================================
 */
void _enemy::make_enemy4()
{
	di = 1;
//	shield = 1;
	health = 20;
}

void _enemy::move_enemy4()
{
	this->move_enemy_template(4, 96);
}
const enemy_kind enemy4 = {
	1,
	&_enemy::make_enemy4,
	&_enemy::move_enemy4,
	6,
	B_SPRITES, 8+3*16,
	16,
};

/*
 * ===========================================================================
 *                                enemy5
 * ===========================================================================
 */
void _enemy::make_enemy5()
{
	count = gamerand.get() & 127;
	di = 1;
//	shield = 1;
	health = 20;
	a = 0;
}

void _enemy::move_enemy5()
{
	if(a == 0)
	{
		if(norm > ((VIEWLIMIT >> 1) - 32))
			this->move_enemy_template(6, 192);
		else
			a = 1;
	}
	else
	{
		if(norm < VIEWLIMIT)
			this->move_enemy_template_2(4, 192);
		else
			a = 0;
	}
	if((--count) <= 0)
	{
		count = 8;
		if(norm > ((VIEWLIMIT >> 1) - 32))
			this->shot_template(&beam, 6, 0, 0);
	}
}
const enemy_kind enemy5 = {
	5,
	&_enemy::make_enemy5,
	&_enemy::move_enemy5,
	6,
	B_SPRITES, 0+4*16,
	16,
};

/*
 * ===========================================================================
 *                                enemy6
 * ===========================================================================
 */
void _enemy::move_enemy6()
{
	if(a == 0)
	{
		if(norm > ((VIEWLIMIT >> 1) - 0))
			this->move_enemy_template(6, 192);
		else
			a = 1;
	}
	else
	{
		if(norm < VIEWLIMIT)
			this->move_enemy_template_2(5, 192);
		else
			a = 0;
	}
	if((--count) <= 0)
	{
		count = 128;
		if(norm > ((VIEWLIMIT >> 1) - 32))
			this->shot_template(&beam, 6, 0, 0);
	}
}
const enemy_kind enemy6 = {
	2,
	&_enemy::make_enemy5,
	&_enemy::move_enemy6,
	6,
	B_SPRITES, 0+5*16,
	16,
};

/*
 * ===========================================================================
 *                                enemy7
 * ===========================================================================
 */
void _enemy::move_enemy7()
{
	if(a == 0)
	{
		if(norm > ((VIEWLIMIT >> 1) - 32))
			this->move_enemy_template(6, 192);
		else
			a = 1;
	}
	else
	{
		if(norm < VIEWLIMIT)
			this->move_enemy_template_3(4, 192);
		else
			a = 0;
	}
	if((--count) <= 0)
	{
		count = 8;
		if(norm > ((VIEWLIMIT >> 1) - 32))
			this->shot_template(&beam, 6, 0, 0);
	}
}
const enemy_kind enemy7 = {
	5,
	&_enemy::make_enemy5,
	&_enemy::move_enemy7,
	6,
	B_SPRITES, 0+6*16,
	16,
};
/*
 * ===========================================================================
 *                                enemy_m1
 * ===========================================================================
 */
void _enemy::make_enemy_m1()
{
	di = 1;
//	shield = 26;
	health = 20 * 26;
	count = gamerand.get() & 15;
}

void _enemy::move_enemy_m1()
{
	this->move_enemy_template(3, 128);
	di = 1;
	if((count--) <= 0)
	{
		count = 4;
		if(norm < ((VIEWLIMIT >> 1) - 16))
		{
			this->shot_template(&enemy1, 4, 0, 0);
		}
	}
	if(health < 200)
	{
		this->shot_template_8_dir(&enemy2);
		release();
	}
}
const enemy_kind enemy_m1 = {
	50,
	&_enemy::make_enemy_m1,
	&_enemy::move_enemy_m1,
	12,
	B_BIGSHIP, 0,
	32,
};

/*
 * ===========================================================================
 *                                enemy_m2
 * ===========================================================================
 */
void _enemy::make_enemy_m2()
{
	di = 1;
//	shield = 26;
	health = 20 * 26;
	count = gamerand.get() & 15;
}

void _enemy::move_enemy_m2()
{
	this->move_enemy_template(3, 128);
	di = 1;
	if((count--) <= 0)
	{
		count = 8;
		if(norm < ((VIEWLIMIT >> 1) + 8))
		{
			this->shot_template(&enemy2, 4, 128, 192);
		}
	}
	if(health < 200)
	{
		this->shot_template_8_dir(&bomb2);
		release();
	}
}
const enemy_kind enemy_m2 = {
	50,
	&_enemy::make_enemy_m2,
	&_enemy::move_enemy_m2,
	12,
	B_BIGSHIP, 0,
	32,
};

/*
 * ===========================================================================
 *                                enemy_m3
 * ===========================================================================
 */
void _enemy::make_enemy_m3()
{
	di = 1;
//	shield = 26;
	health = 20 * 26;
	count = gamerand.get() & 15;
}

void _enemy::move_enemy_m3()
{
	this->move_enemy_template(3, 128);
	di = 1;
	if((count--) <= 0)
	{
		count = 64;
		if(norm < ((VIEWLIMIT >> 1) + 8))
		{
			this->shot_template_8_dir(&bomb2);
		}
	}
	if(health < 200)
	{
		this->shot_template_8_dir(&rock);
		release();
	}
}
const enemy_kind enemy_m3 = {
	50,
	&_enemy::make_enemy_m3,
	&_enemy::move_enemy_m3,
	12,
	B_BIGSHIP, 0,
	32,
};

/*
 * ===========================================================================
 *                                enemy_m4
 * ===========================================================================
 */
void _enemy::make_enemy_m4()
{
	di = 1;
//	shield = 26;
	health = 20 * 26;
	count = gamerand.get() & 15;
}

void _enemy::move_enemy_m4()
{
	this->move_enemy_template(2, 96);
	di = 1;
	static const enemy_kind *shot[8] = {
		&enemy1, &enemy2, &bomb2, &ring, &enemy1, &enemy2, &ring,
		&enemy1
	};
	if((count--) <= 0)
	{
		count = 64;
		if(norm < ((VIEWLIMIT >> 1) + 8))
		{
			this->shot_template_8_dir(shot[gamerand.get() & 7]);
		}
	}
	if(health < 200)
	{
		this->shot_template_8_dir(&rock);
		release();
	}
}
const enemy_kind enemy_m4 = {
	100,
	&_enemy::make_enemy_m4,
	&_enemy::move_enemy_m4,
	12,
	B_BIGSHIP, 0,
	32,
};


/*---------------------------------------------------------------------------*/
/*  void _enemy::make_xxxx(){}
 *  void _enemy::move_xxxx(){}
 *  enemy_kind xxxxx = { score, make_xxxx, move_xxxx, hitsize, cpx, cpy, size};
 */

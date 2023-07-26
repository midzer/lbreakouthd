/*
 * sprite.cpp
 * (C) 2018 by Michael Speck
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "tools.h"
#include "sdl.h"
#include "mixer.h"
#include "clientgame.h"
#include "theme.h"
#include "sprite.h"

extern SDL_Renderer *mrc;

Particle::Particle(GridImage &simg, int _gx, int _gy,
					int _sx, int _sy, int _sw, int _sh,
					double px, double py, double vx, double vy,
					double vpms, uint lifetime)
	: img(simg), gx(_gx), gy(_gy), sx(_sx), sy(_sy), sw(_sw), sh(_sh),
	  pos(px,py), vel(vx,vy)
{
	/* adjust velocity vector */
	vel.setLength(vpms);

	/* use lifetime to count down alpha from 255 to 0 */
	sc.init(SCT_ONCE, 255, 0, lifetime/255.0);
}

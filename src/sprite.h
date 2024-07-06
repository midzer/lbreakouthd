/*
 * sprite.h
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

#ifndef SRC_SPRITE_H_
#define SRC_SPRITE_H_

/* Graphical object (hail to the C64 :) that can be rendered and updated. */
class Sprite {
public:
	virtual ~Sprite() {};
	virtual int update(uint ms) = 0; /* return 1 if to be removed, 0 otherwise */
	virtual void render() = 0;
};

class Animation : public Sprite {
	GridImage& img;
	uint id;
	int x, y; /* position on screen */
	FrameCounter fc;
public:
	Animation(GridImage &_img, uint _id, uint delay, int _x, int _y)
				: img(_img), id(_id), x(_x), y(_y) {
		fc.init(img.getGridSizeX(), delay);
	}
	int update(uint ms) {
		if (fc.update(ms))
			return 1; /* die */
		return 0;
	}
	void render() {
		img.copy(fc.get(), id, x, y);
	}
};

class Particle : public Sprite {
	GridImage &img;
	int gx, gy, sx, sy, sw, sh;
	Vec pos, vel;
	SmoothCounter sc;
public:
	Particle(GridImage &simg, int gx, int gy, int sx, int sy, int sw, int sh,
			double px, double py, double vx, double vy, double vpms, uint lifetime);
	int update(uint ms) {
		pos.add(ms, vel);
		return sc.update(ms);
	}
	void render() {
		img.setAlpha(sc.get());
		img.copy(gx,gy,sx,sy,sw,sh, pos.getX(), pos.getY());
		img.clearAlpha();
	}
};

#endif /* SRC_SPRITE_H_ */

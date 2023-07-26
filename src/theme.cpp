/*
 * theme.cpp
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

extern SDL_Renderer *mrc;

/** Load resources and scale if necessary using bricks screen height.
 * Whatever is missing: Fall back to Standard theme. */
void Theme::load(string name, uint screenWidth, uint screenHeight,
				uint brickScreenWidth, uint brickScreenHeight,
				int antialiasing)
{
	string path, fpath;
	uint iw, ih;

	if (name[0] == '~')
		path = getHomeDir() + "/" + CONFIGDIR + "/themes/" + name.substr(1);
	else
		path = string(DATADIR) + "/themes/" + name;

#ifndef WIN32
	if (!fileExists(path))
		_logerr("CRITICAL ERROR: theme %s not found. I will continue but most likely crash...\n",path.c_str());
#endif
	_loginfo("Loading theme %s\n",path.c_str());

	/* set default config values for old themes */
	oldTheme = true;
	title = "unknown";
	author = "unknown";
	version= "v?.??";
	brickFileWidth = 40;
	brickFileHeight = 20;
	shadowOffset = 10;
	fontColorNormal = {255,255,255,255};
	fontColorHighlight = {255,220,0,255};
	shotFrameNum = 4;
	shotAnimDelay = 200;
	weaponFrameNum = 4;
	weaponAnimDelay = 200;
	explFrameNum = 9;
	explAnimDelay = 50;
	shineFrameNum = 6;
	shineAnimDelay = 50;
	fontSmallName = "fsmall.otf";
	fontSmallSize = 14;
	fontNormalName = "fnormal.otf";
	fontNormalSize = 18;
	menuX = 180;
	menuY = 350;
	menuItemWidth = 200;
	menuItemHeight = 18;
	menuFontNormalName = "fsmall.otf";
	menuFontNormalSize = 18;
	menuFontFocusName = "fnormal.otf";
	menuFontFocusSize = 21;
	menuFontColorNormal = {255,255,255,255};
	menuFontColorFocus  = {255,220,0,255};

	/* load theme values */
	if (fileExists(path + "/theme.ini")) {
		oldTheme = false;
		FileParser fp(path + "/theme.ini");
		fp.get("title",title);
		fp.get("author",author);
		fp.get("version",version);
		fp.get("brickWidth",brickFileWidth);
		fp.get("brickHeight",brickFileHeight);
		fp.get("shadowOffset",shadowOffset);
		fp.get("fontSmall.name",fontSmallName);
		fp.get("fontSmall.size",fontSmallSize);
		fp.get("fontNormal.name",fontNormalName);
		fp.get("fontNormal.size",fontNormalSize);
		fp.get("fontColorNormal.r",fontColorNormal.r);
		fp.get("fontColorNormal.g",fontColorNormal.g);
		fp.get("fontColorNormal.b",fontColorNormal.b);
		fp.get("fontColorNormal.a",fontColorNormal.a);
		fp.get("fontColorHighlight.r",fontColorHighlight.r);
		fp.get("fontColorHighlight.g",fontColorHighlight.g);
		fp.get("fontColorHighlight.b",fontColorHighlight.b);
		fp.get("fontColorHighlight.a",fontColorHighlight.a);
		fp.get("shotAnim.frames",shotFrameNum);
		fp.get("shotAnim.delay",shotAnimDelay);
		fp.get("weaponAnim.frames",weaponFrameNum);
		fp.get("weaponAnim.delay",weaponAnimDelay);
		fp.get("explAnim.frames",explFrameNum);
		fp.get("explAnim.delay",explAnimDelay);
		fp.get("shineAnim.frames",shineFrameNum);
		fp.get("shineAnim.delay",shineAnimDelay);
		fp.get("menu.centerX",menuX);
		fp.get("menu.centerY",menuY);
		fp.get("menu.itemWidth",menuItemWidth);
		fp.get("menu.itemHeight",menuItemHeight);
		fp.get("menu.fontNormal.name",menuFontNormalName);
		fp.get("menu.fontNormal.size",menuFontNormalSize);
		fp.get("menu.fontFocus.name",menuFontFocusName);
		fp.get("menu.fontFocus.size",menuFontFocusSize);
		fp.get("menu.fontNormal.color.r",menuFontColorNormal.r);
		fp.get("menu.fontNormal.color.g",menuFontColorNormal.g);
		fp.get("menu.fontNormal.color.b",menuFontColorNormal.b);
		fp.get("menu.fontNormal.color.a",menuFontColorNormal.a);
		fp.get("menu.fontFocus.color.r",menuFontColorFocus.r);
		fp.get("menu.fontFocus.color.g",menuFontColorFocus.g);
		fp.get("menu.fontFocus.color.b",menuFontColorFocus.b);
		fp.get("menu.fontFocus.color.a",menuFontColorFocus.a);
	}

	/* standard board geometry, will be shifted if composed frame is used */
	boardX = MAPWIDTH * brickScreenWidth;
	boardWidth = 4.33 * brickScreenWidth;

	/* load standard values for fallback */
	FileParser stdSettings(stdPath + "/theme.ini");
	int sbfw, sbfh; /* standard brick file width/height */
	stdSettings.get("brickWidth",sbfw);
	stdSettings.get("brickHeight",sbfh);

	if (oldTheme) {
		Image::setRenderScaleQuality(antialiasing);
		Image::useColorKeyBlack = true;
	} else {
		Image::setRenderScaleQuality(1);
		Image::useColorKeyBlack = false;
	}

	/* adjust shadow offset to screen geometry */
	shadowOffset = shadowOffset * brickScreenHeight / brickFileHeight;

	/* load bricks */
	if (fileExists(path + "/bricks.png"))
		bricks.load(path + "/bricks.png",brickFileWidth,brickFileHeight);
	else
		bricks.load(stdPath + "/bricks.png",sbfw,sbfh);
	bricks.scale(brickScreenWidth,brickScreenHeight);

	/* load extras */
	if (fileExists(path + "/extras.png")) {
		if (oldTheme) {
			SDL_Surface *surf = IMG_Load(string(path + "/extras.png").c_str());
			/* last extra column is for color key, none otherwise */
			if (Image::getWidth(path + "/extras.png") & 1) {
				Uint32 ckey = Image::getSurfacePixel(surf,surf->w-1,0);
				SDL_SetColorKey(surf,SDL_TRUE,ckey);
			}
			extras.load(surf, brickFileWidth, brickFileHeight);
			SDL_FreeSurface(surf);
		} else
			extras.load(path + "/extras.png",brickFileWidth,brickFileHeight);
	} else
		extras.load(stdPath + "/extras.png",sbfw,sbfh);
	extras.scale(brickScreenWidth,brickScreenHeight);

	/* either load frame.png
	 * or create frame from left,top,right part
	 * or create standard frame with wall bricks */
	if (fileExists(path + "/frame.png")) {
		frame.load(path + "/frame.png");
		if (brickFileHeight != brickScreenHeight) {
			int nw = frame.getWidth() * brickScreenWidth / brickFileWidth;
			int nh = frame.getHeight() * brickScreenHeight / brickFileHeight;
			frame.scale(nw, nh);
		}
	} else if (fileExists(path + "/fr_left.png")) {
		frame.create(screenWidth,screenHeight);
		frame.fill(0,0,0,0);

		/* we need to set fr_right:w-1,0 as colorkey so we have to use
		 * surfaces here... */
		SDL_Surface *sfleft = IMG_Load(string(path + "/fr_left.png").c_str());
		SDL_Surface *sfright = IMG_Load(string(path + "/fr_right.png").c_str());
		SDL_Surface *sftop = IMG_Load(string(path + "/fr_top.png").c_str());
		if (sfleft == NULL || sfright == NULL || sftop == NULL)
			_logsdlerr();
		else {
			/* convert all surfaces to same pixel format so ckey matches */
			SDL_Surface *oldsurf = sfleft;
			sfleft = SDL_ConvertSurface(sfleft,sfright->format,0);
			SDL_FreeSurface(oldsurf);
			oldsurf = sftop;
			sftop = SDL_ConvertSurface(sftop,sfright->format,0);
			SDL_FreeSurface(oldsurf);

			Uint32 ckey = Image::getSurfacePixel(sfright, sfright->w - 1, 0 );
			SDL_SetColorKey( sfleft, SDL_TRUE, ckey );
			SDL_SetColorKey( sftop, SDL_TRUE, ckey );
			SDL_SetColorKey( sfright, SDL_TRUE, ckey );
			Image fleft, ftop, fright;
			fleft.load(sfleft);
			fright.load(sfright);
			ftop.load(sftop);

			SDL_Texture *oldTex = SDL_GetRenderTarget(mrc);
			SDL_SetRenderTarget(mrc,frame.getTex());

			fleft.copy(0,0,brickScreenWidth,screenHeight);
			ftop.copy(brickScreenWidth,0,(MAPWIDTH-2)*brickScreenWidth,brickScreenHeight);
			fright.copy((MAPWIDTH-1)*brickScreenWidth,0,brickScreenWidth,screenHeight);

			/* adjust board position to have some distance from frame */
			boardX += brickScreenWidth/2;

			addBox(frame, boardX, brickScreenHeight, boardWidth,
							12*brickScreenHeight);
			addBox(frame, boardX, brickScreenHeight*14, boardWidth,
							3*brickScreenHeight);
			addBox(frame, boardX, brickScreenHeight*18, boardWidth,
							5*brickScreenHeight);

			SDL_SetRenderTarget(mrc,oldTex);
		}
		if (sfleft)
			SDL_FreeSurface(sfleft);
		if (sfright)
			SDL_FreeSurface(sfright);
		if (sftop)
			SDL_FreeSurface(sftop);
	} else {
		/* do it straight in screen resolution */
		frame.create(screenWidth,screenHeight);
		frame.fill(0,0,0,0);

		SDL_Texture *oldTex = SDL_GetRenderTarget(mrc);
		SDL_SetRenderTarget(mrc,frame.getTex());

		/* darken board for better reading text */
		SDL_SetRenderDrawColor(mrc,0,0,0,160);
		SDL_Rect boardRect = {
				(int)(MAPWIDTH*brickScreenWidth),
				0,
				(int)(screenWidth - MAPWIDTH*brickScreenWidth),
				(int)(screenHeight) };
		SDL_RenderFillRect(mrc,&boardRect);

		/* use bricks for frame */
		for (int i = 0; i < MAPWIDTH; i++)
			for (int j = 0; j < MAPHEIGHT; j++)
				if (j == 0 || i == 0 || i == MAPWIDTH-1)
					bricks.copy(0, 0, i*brickScreenWidth, j*brickScreenHeight);
		for (int i = MAPWIDTH; i < MAPWIDTH + 6; i++) {
			bricks.copy(0, 0, i*brickScreenWidth, 0);
			bricks.copy(0, 0, i*brickScreenWidth, MAPHEIGHT*brickScreenHeight-brickScreenHeight);
			bricks.copy(0, 0, i*brickScreenWidth, (MAPHEIGHT-7)*brickScreenHeight);
			bricks.copy(0, 0, i*brickScreenWidth, (MAPHEIGHT-11)*brickScreenHeight);
		}
		for (int j = 0; j < MAPHEIGHT; j++) {
			bricks.copy(0, 0, screenWidth - brickScreenWidth, j*brickScreenHeight);
		}

		SDL_SetRenderTarget(mrc,oldTex);
	}

	/* paddle is 90% of brick height */
	if (fileExists(path + "/paddle.png"))
		fpath = path + "/paddle.png";
	else
		fpath = stdPath + "/paddle.png";
	ih = Image::getHeight(fpath) / 4;
	if (oldTheme && fileExists(path + "/paddle.png")) {
		/* color key wasn't quite 0x0... */
		SDL_Surface *surf = IMG_Load(string(path + "/paddle.png").c_str());
		if (surf) {
			Uint32 ckey = Image::getSurfacePixel(surf,0,0);
			SDL_SetColorKey(surf,SDL_TRUE,ckey);
			paddles.load(surf, ih, ih);
			SDL_FreeSurface(surf);
		}
	} else
		paddles.load(fpath, ih, ih);
	paddles.scale(9*brickScreenHeight/10, 9*brickScreenHeight/10);

	/* balls are 60% of brick height */
	if (fileExists(path + "/ball.png"))
		fpath = path + "/ball.png";
	else
		fpath = stdPath + "/ball.png";
	ih = Image::getHeight(fpath);
	if (oldTheme && fileExists(path + "/ball.png")) {
		/* color key wasn't quite 0x0... */
		SDL_Surface *surf = IMG_Load(string(path + "/ball.png").c_str());
		if (surf) {
			Uint32 ckey = Image::getSurfacePixel(surf,0,0);
			SDL_SetColorKey(surf,SDL_TRUE,ckey);
			balls.load(surf, ih, ih);
			SDL_FreeSurface(surf);
		}
	} else
		balls.load(fpath,ih,ih);
	balls.scale(6*brickScreenHeight/10,6*brickScreenHeight/10);

	/* shots are 50% of brick height */
	if (fileExists(path + "/shot.png"))
		fpath = path + "/shot.png";
	else
		fpath = stdPath + "/shot.png";
	ih = Image::getHeight(fpath);
	if (oldTheme && fileExists(path + "/shot.png")) {
		/* color key wasn't quite 0x0... */
		SDL_Surface *surf = IMG_Load(string(path + "/shot.png").c_str());
		if (surf) {
			Uint32 ckey = Image::getSurfacePixel(surf,0,0);
			SDL_SetColorKey(surf,SDL_TRUE,ckey);
			shot.load(surf, ih, ih);
			SDL_FreeSurface(surf);
		}
	} else
		shot.load(fpath,ih,ih);
	shot.scale(5*brickScreenHeight/10,5*brickScreenHeight/10);

	/* weapon is 90% brick height (old weapons get scaled as width was 70%) */
	if (fileExists(path + "/weapon.png"))
		fpath = path + "/weapon.png";
	else {
		fpath = stdPath + "/weapon.png";
		stdSettings.get("weaponAnim.frames",weaponFrameNum);
	}
	iw = Image::getWidth(fpath) / weaponFrameNum;
	ih = Image::getHeight(fpath);
	weapon.load(fpath,iw,ih);
	weapon.scale(9*brickScreenHeight/10,9*brickScreenHeight/10);

	/* life symbol is brick size, vertically arranged,
	 * first is off, second is on */
	if (fileExists(path + "/life.png"))
		life.load(path + "/life.png",brickFileWidth,brickFileHeight);
	else
		life.load(stdPath+ "/life.png",sbfw,sbfh);
	life.scale(brickScreenWidth,brickScreenHeight);

	/* shine animation is one row of brick size frames */
	if (fileExists(path + "/shine.png"))
		shine.load(path + "/shine.png",brickFileWidth,brickFileHeight);
	else
		shine.load(stdPath+ "/shine.png",sbfw,sbfh);
	shine.scale(brickScreenWidth,brickScreenHeight);

	/* warp symbol is brick size */
	/* ignore old warp icon as it's geometry sucks big time */
	if (!oldTheme)
		warpIcon.load(testRc(path,"warp.png"));
	else
		warpIcon.load(stdPath + "/warp.png");
	warpIcon.scale(brickScreenWidth,brickScreenHeight);

	/* explosions are square, scaled according to brick ratio */
	if (fileExists(path + "/explosions.png"))
		fpath = path + "/explosions.png";
	else {
		fpath = stdPath + "/explosions.png";
		stdSettings.get("explAnim.frames",explFrameNum);
	}
	iw = Image::getWidth(fpath) / explFrameNum;
	explosions.load(fpath,iw,iw);
	if (fileExists(path + "/explosions.png"))
		explosions.scale(explosions.getGridWidth() * brickScreenWidth / brickFileWidth,
				explosions.getGridHeight() * brickScreenHeight / brickFileHeight);
	else
		explosions.scale(explosions.getGridWidth() * brickScreenWidth / sbfw,
				explosions.getGridHeight() * brickScreenHeight / sbfh);

	/* load backgrounds always without color key workaround */
	Image::useColorKeyBlack = false;

	/* load and scale up to 10 wallpapers */
	string wpath = stdPath;
	uint wbfh = sbfh, wbfw = sbfw; /* for scaling */
	if (fileExists(path + "/back0.png") || fileExists(path + "/back0.jpg")) {
		wpath = path;
		wbfh = brickFileHeight;
		wbfw = brickFileWidth;
	}
	numWallpapers = 0;
	for (int i = 0; i < MAXWALLPAPERS; i++) {
		string wfname = wpath + "/back" + to_string(i);
		if (fileExists(wfname + ".png"))
			wfname += ".png";
		else if (fileExists(wfname + ".jpg"))
			wfname += ".jpg";
		else
			break;
		wallpapers[i].load(wfname);
		if (wbfh != brickScreenHeight) {
			int nw, nh;
			nw = wallpapers[i].getWidth() * brickScreenWidth / wbfw;
			nh = wallpapers[i].getHeight() * brickScreenHeight / wbfh;
			wallpapers[i].scale(nw, nh);
		}
		wallpapers[i].setBlendMode(0);
		numWallpapers++;
	}

	/* create shadow images */
	frameShadow.createShadow(frame);
	bricksShadow.createShadow(bricks);
	paddlesShadow.createShadow(paddles);
	ballsShadow.createShadow(balls);
	extrasShadow.createShadow(extras);
	shotShadow.createShadow(shot);

	/* fonts */
	if (fileExists(path + "/" + fontSmallName))
		fSmall.load(path + "/" + fontSmallName,
			fontSmallSize * brickScreenHeight / brickFileHeight);
	else {
		stdSettings.get("fontSmall.name",fontSmallName);
		stdSettings.get("fontSmall.size",fontSmallSize);
		fSmall.load(stdPath + "/" + fontSmallName,
			fontSmallSize * brickScreenHeight / sbfh);
	}
	if (fileExists(path + "/" + fontNormalName))
		fNormal.load(path + "/" + fontNormalName,
			fontNormalSize * brickScreenHeight / brickFileHeight);
	else {
		stdSettings.get("fontNormal.name",fontNormalName);
		stdSettings.get("fontNormal.size",fontNormalSize);
		fNormal.load(stdPath + "/" + fontNormalName,
			fontNormalSize * brickScreenHeight / sbfh);
	}
	fNormal.setColor(fontColorNormal);
	fSmall.setColor(fontColorNormal);

	/* menu stuff */
	if (fileExists(path + "/menuback.png"))
		menuBackground.load(path + "/menuback.png");
	else if (fileExists(path + "/menuback.jpg"))
		menuBackground.load(path + "/menuback.jpg");
	else {
		menuBackground.load(stdPath + "/menuback.jpg");
		stdSettings.get("menu.centerX",menuX);
		stdSettings.get("menu.centerY",menuY);
		stdSettings.get("menu.itemWidth",menuItemWidth);
		stdSettings.get("menu.itemHeight",menuItemHeight);
	}
	menuBackground.setBlendMode(0);
	menuX = menuX * screenWidth / menuBackground.getWidth();
	menuY = menuY * screenHeight / menuBackground.getHeight();
	menuItemWidth = menuItemWidth * screenWidth / menuBackground.getWidth();
	menuItemHeight = menuItemHeight * screenHeight / menuBackground.getHeight();
	menuBackground.scale(screenWidth, screenHeight);

	if (fileExists(path + "/" + menuFontNormalName))
		fMenuNormal.load(path + "/" + menuFontNormalName,
			menuFontNormalSize * brickScreenHeight / brickFileHeight);
	else {
		stdSettings.get("menu.fontNormal.name",menuFontNormalName);
		stdSettings.get("menu.fontNormal.size",menuFontNormalSize);
		fMenuNormal.load(stdPath + "/" + menuFontNormalName,
			menuFontNormalSize * brickScreenHeight / sbfh);
	}
	if (fileExists(path + "/" + menuFontFocusName))
		fMenuFocus.load(path + "/" + menuFontFocusName,
			menuFontFocusSize * brickScreenHeight / brickFileHeight);
	else {
		stdSettings.get("menu.fontFocus.name",menuFontFocusName);
		stdSettings.get("menu.fontFocus.size",menuFontFocusSize);
		fMenuFocus.load(stdPath + "/" + menuFontFocusName,
			menuFontFocusSize * brickScreenHeight / sbfh);
	}
	fMenuNormal.setColor(menuFontColorNormal);
	fMenuFocus.setColor(menuFontColorFocus);

	/* sounds */
	sReflectBrick.load(testRc(path,"reflectbrick.wav"));
	sReflectPaddle.load(testRc(path,"reflectpaddle.wav"));
	sBrickHit.load(testRc(path,"brickhit.wav"));
	sExplosion.load(testRc(path,"explosion.wav"));
	sEnergyHit.load(testRc(path,"energyhit.wav"));
	sShot.load(testRc(path,"shot.wav"));
	sAttach.load(testRc(path,"attach.wav"));
	sClick.load(testRc(path,"click.wav"));
	sDamn.load(testRc(path,"damn.wav"));
	sDammit.load(testRc(path,"dammit.wav"));
	sExcellent.load(testRc(path,"excellent.wav"));
	sVeryGood.load(testRc(path,"verygood.wav"));
	sMenuClick.load(testRc(path,"menuclick.wav"));
	sMenuMotion.load(testRc(path,"menumotion.wav"));
	sExtras[EX_SCORE200].load(testRc(path,"score.wav"));
	sExtras[EX_SCORE500].load(testRc(path,"score.wav"));
	sExtras[EX_SCORE1000].load(testRc(path,"score.wav"));
	sExtras[EX_SCORE2000].load(testRc(path,"score.wav"));
	sExtras[EX_SCORE5000].load(testRc(path,"score.wav"));
	sExtras[EX_SCORE10000].load(testRc(path,"score.wav"));
	sExtras[EX_GOLDSHOWER].load(testRc(path,"score.wav"));
	sExtras[EX_SHORTEN].load(testRc(path,"shrink.wav"));
	sExtras[EX_LENGTHEN].load(testRc(path,"expand.wav"));
	sExtras[EX_LIFE].load(testRc(path,"gainlife.wav"));
	sExtras[EX_SLIME].load(testRc(path,"attach.wav"));
	sExtras[EX_METAL].load(testRc(path,"energyhit.wav"));
	sExtras[EX_BALL].load(testRc(path,"extraball.wav"));
	sExtras[EX_WALL].load(testRc(path,"wall.wav"));
	sExtras[EX_FROZEN].load(testRc(path,"freeze.wav"));
	sExtras[EX_WEAPON].load(testRc(path,"standard.wav"));
	sExtras[EX_RANDOM].load(testRc(path,"standard.wav"));
	sExtras[EX_FAST].load(testRc(path,"speedup.wav"));
	sExtras[EX_SLOW].load(testRc(path,"speeddown.wav"));
	sExtras[EX_JOKER].load(testRc(path,"joker.wav"));
	sExtras[EX_DARKNESS].load(testRc(path,"darkness.wav"));
	sExtras[EX_CHAOS].load(testRc(path,"chaos.wav"));
	sExtras[EX_GHOST_PADDLE].load(testRc(path,"ghost.wav"));
	sExtras[EX_DISABLE].load(testRc(path,"disable.wav"));
	sExtras[EX_TIME_ADD].load(testRc(path,"timeadd.wav"));
	sExtras[EX_EXPL_BALL].load(testRc(path,"explball.wav"));
	sExtras[EX_BONUS_MAGNET].load(testRc(path,"bonusmagnet.wav"));
	sExtras[EX_MALUS_MAGNET].load(testRc(path,"malusmagnet.wav"));
	sExtras[EX_WEAK_BALL].load(testRc(path,"weakball.wav"));
	sLooseLife.load(testRc(path,"looselife.wav"));
}

void Theme::addBox(Image &img, int x, int y, int w, int h)
{
	SDL_Texture *oldTex = SDL_GetRenderTarget(mrc);

	SDL_SetRenderTarget(mrc,img.getTex());

	SDL_SetRenderDrawColor(mrc,0,0,0,160);
	SDL_Rect drect = {x,y,w,h};
	SDL_RenderFillRect(mrc,&drect);

	SDL_SetRenderDrawColor(mrc,fontColorNormal.r,fontColorNormal.g,
					fontColorNormal.b,fontColorNormal.a);
	SDL_Point pts[5] = { {x,y} , {x+w-1,y}, {x+w-1,y+h-1}, {x,y+h-1}, {x,y} };
	SDL_RenderDrawLines(mrc,pts,5);

	SDL_SetRenderTarget(mrc,oldTex);
}

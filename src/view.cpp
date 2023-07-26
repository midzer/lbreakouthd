/*
 * view.cpp
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
#include "clientgame.h"
#include "mixer.h"
#include "theme.h"
#include "sprite.h"
#include "menu.h"
#include "selectdlg.h"
#include "editor.h"
#include "view.h"

extern SDL_Renderer *mrc;
extern int last_ball_brick_reflect_x;

View::View(Config &cfg, ClientGame &_cg)
	: config(cfg), mw(NULL), editor(theme,mixer),
	  curMenu(NULL), graphicsMenu(NULL), resumeMenuItem(NULL),
	  selectDlg(theme, mixer), lblCredits1(true), lblCredits2(true),
	  cgame(_cg), quitReceived(false),
	  showWarpIcon(false), warpIconX(0), warpIconY(0),
	  fpsCycles(0), fpsStart(0), fps(0)
{
	_loginfo("Initializing SDL\n");
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
			SDL_Log("SDL_Init failed: %s\n", SDL_GetError());
	if (TTF_Init() < 0)
	 		SDL_Log("TTF_Init failed: %s\n", SDL_GetError());
	mixer.open(cfg.channels, cfg.audio_buffer_size);
	mixer.setVolume(cfg.volume);
	mixer.setMute(!config.sound);

	/* load theme names */
	readDir(string(DATADIR)+"/themes", RD_FOLDERS, themeNames);
	if (string(CONFIGDIR) != ".") {
		vector<string> homeThemes;
		readDir(getHomeDir() + "/" + CONFIGDIR + "/themes",
						RD_FOLDERS, homeThemes);
		for (auto &e : homeThemes)
			themeNames.push_back(string("~")+e);
	}
	if ((uint)config.theme_id >= themeNames.size())
		config.theme_id = 0;
	config.theme_count = themeNames.size();

	/* name for saved game */
	if (string(CONFIGDIR) != ".")
		saveFileName = getHomeDir() + "/" + CONFIGDIR + "/lbreakouthd.sav";
	else
		saveFileName = "./lbreakouthd.sav";

	MainWindow::getModeNames(modeNames);
	if ((uint)config.mode >= modeNames.size())
		config.mode = 0;
	viewport.x = viewport.y = viewport.w = viewport.h = 0;

	gamepad.open();
	if (gamepad.opened()) {
		printf("  NOTE: Gamepad cannot be configured via menu yet but you\n");
		printf("  can edit the gp_ entries in config file %s .\n",config.path.c_str());
		printf("  In case connection to gamepad gets lost, you can press F5 do reconnect.\n");
	}

	shineX = -1;
	shineY = -1;

	init(themeNames[config.theme_id], MainWindow::getModeResolution(config.mode));
}

/** (Re)Initialize window, theme and menu.
 * t is theme name, r=0 means fullscreen, otherwise vertical resolution. */
void View::init(string t, uint r)
{
	int cpdix = 0; /* default to display 0 */

	_loginfo("Initializing View (Theme=%s, Resolution=%d)\n",t.c_str(),r);

	/* get current display index or default to 0 */
	if (mw) {
		cpdix = mw->getDisplayIndex();
	} else if (SDL_GetNumVideoDisplays() > 1) {
		/* XXX first time we open the window, so to properly determine the
		 * current display we open a small window for multiple monitors */
		SDL_Window *testwindow;
		_loginfo("Multiple displays detected\n");
		if((testwindow = SDL_CreateWindow("LBreakoutHD",
					SDL_WINDOWPOS_CENTERED,
					SDL_WINDOWPOS_CENTERED,
					320, 200, 0)) == NULL) {
			_logerr("Could not open display test window: %s\n",
								SDL_GetError());
			_loginfo("Falling back to settings of display 0\n");
		} else {
			cpdix = SDL_GetWindowDisplayIndex(testwindow);
			SDL_DestroyWindow(testwindow);
		}
	}
	_loginfo("Using display %d\n",cpdix);

	/* determine resolution and scale factor */
	int sw, sh;
	if (r == 0) {
		/* fullscreen is tricky... might also be 16:10,4:3,...
		 * use width to get 16:9 leaving space at bottom */
		SDL_DisplayMode mode;
		SDL_GetCurrentDisplayMode(cpdix,&mode);
		/* TEST  mode.w = 1600; mode.h = 1200; */
		sw = mode.w;
		sh = mode.w/16*9;
		if (sh != mode.h) {
			if (sh < mode.h) { /* e.g. 4:3 */
				viewport.x = 0;
				viewport.y = (mode.h - sh)/2;
			} else { /* e.g. 21:9 */
				sh = mode.h;
				sw = sh / 9 * 16;
				viewport.x = (mode.w - sw)/2;
				viewport.y = 0;
			}
			viewport.w = sw;
			viewport.h = sh;
			_loginfo("Fullscreen resolution not 16:9! Using %dx%d\n",sw,sh);
		} else
			_loginfo("Using fullscreen resolution %dx%d\n",mode.w,mode.h);
	} else {
		sh = r;
		sw = sh * 16 / 9;
		_loginfo("Using window resolution %dx%d\n",sw,sh);
	}

	/* depending on screen ratio we don't fit all of the screen with bricks */
	brickScreenHeight = sh / MAPHEIGHT;
	scaleFactor = brickScreenHeight * 100 / VG_BRICKHEIGHT;
	brickScreenWidth = v2s(VG_BRICKWIDTH);
	brickAreaWidth = brickScreenWidth * MAPHEIGHT;
	brickAreaHeight = brickScreenHeight * MAPHEIGHT;
	_loginfo("Scale factor x100: %d\n",scaleFactor);
	_loginfo("Brick screen size: %dx%d\n",brickScreenWidth,brickScreenHeight);

	/* (re)create main window */
	if (mw)
		delete mw;
	mw = new MainWindow("LBreakoutHD", sw, sh, (r==0) );

	/* load theme (scaled if necessary) */
	theme.load(t, sw, sh, brickScreenWidth, brickScreenHeight, config.antialiasing);
	weaponFrameCounter.init(theme.weaponFrameNum, theme.weaponAnimDelay);
	shotFrameCounter.init(theme.shotFrameNum, theme.shotAnimDelay);
	shineFrameCounter.init(theme.shineFrameNum, theme.shineAnimDelay);
	shineDelay.set(1000);
	warpIconAlphaCounter.init(SCT_UPDOWN, 64, 255, 3);
	showWarpIcon = false;
	energyBallAlphaCounter.init(SCT_UPDOWN, 32, 255, 2);

	/* create menu structure */
	createMenus();

	/* set label stuff*/
	lblCredits1.setText(theme.fSmall, "http://lgames.sf.net");
	lblCredits2.setText(theme.fSmall, string("v")+PACKAGE_VERSION);
	lblInfo.setBorder(2);

	/* create render images and positions */
	imgBackground.create(sw,sh);
	imgBackground.setBlendMode(0);
	curWallpaperId = rand() % theme.numWallpapers;
	imgScore.create(brickScreenWidth*3, brickScreenHeight);
	imgScoreX = theme.boardX + (theme.boardWidth - imgScore.getWidth())/2;
	imgScoreY = brickScreenHeight * 15 + brickScreenHeight/2;
	/* EDITHEIGHT+1 enough for everything except barrier so make it bigger */
	imgBricks.create(EDITWIDTH*brickScreenWidth,
				(EDITHEIGHT+4)*brickScreenHeight);
	imgBricksX = brickScreenWidth;
	imgBricksY = brickScreenHeight;
	imgExtras.create(theme.boardWidth, 4*brickScreenHeight);
	imgExtrasX = theme.boardX;
	imgExtrasY = 19*brickScreenHeight;
	imgFloor.create(EDITWIDTH*brickScreenWidth,brickScreenHeight);
	imgFloorX = brickScreenWidth;
	imgFloorY = (MAPHEIGHT-1)*brickScreenHeight;
	SDL_SetRenderTarget(mrc, imgFloor.getTex());
	for (int i = 0; i < EDITWIDTH; i++)
		theme.bricks.copy(1, 0, i*brickScreenWidth, 0);
	SDL_SetRenderTarget(mrc, NULL);
	warpIconX = (MAPWIDTH - 2)*brickScreenWidth;
	warpIconY = (MAPHEIGHT - 1)*brickScreenHeight;
}

View::~View()
{
	delete mw;
	mixer.close();
	gamepad.close();

	/* XXX fonts need to be killed before SDL/TTF_Quit otherwise they
	 * segfault but attribute's dtors are called after ~View is finished */
	theme.fSmall.free();
	theme.fNormal.free();
	theme.fMenuNormal.free();
	theme.fMenuFocus.free();

#ifdef WIN32
	/* XXX after the view object clientgame and config get destructed which
	 * works in linux and saves hiscores and config. however, in windows
	 * destructing some sub object in clientgame seg faults. the hiscore's
	 * dtor is never called (and so is clientgame's dtor) so we explicitly
	 * save hiscores and config here. if anyone knows why: let me know. */
	cgame.saveHiscores();
	config.save();
#endif

	_loginfo("Finalizing SDL\n");
	TTF_Quit();
	SDL_Quit();
}

/** Main game loop. Handle events, update game and render view. */
void View::run()
{
	SDL_Event ev;
	int flags;
	PaddleInputState pis;
	Ticks ticks;
	Ticks renderTicks;
	int maxDelay, delay = 0;
	Uint32 ms;
	bool leave = false;
	bool resumeLater = false;
	double rx = 0;
	vector<string> text;

	curWallpaperId = rand() % theme.numWallpapers;
	showWarpIcon = false;

	fpsStart = SDL_GetTicks();
	fpsCycles = 0;

	if (config.fps == 1)
		maxDelay = 5;
	else if (config.fps == 2)
		maxDelay = 10;
	else
		maxDelay = 0;

	initTitleLabel();
	lblInfo.clearText();
	sprites.clear();
	renderBackgroundImage();
	renderBricksImage();
	renderScoreImage();
	render();

	grabInput(1);

	while (!leave) {
		renderTicks.reset();
		flags = 0;

		/* handle events */
		if (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT) {
				quitReceived = true;
				leave = true;
				resumeLater = true;
			}
			if (ev.type == SDL_KEYUP) {
				switch (ev.key.keysym.scancode) {
				case SDL_SCANCODE_F5:
					gamepad.close();
					gamepad.open();
					break;
				case SDL_SCANCODE_P:
					showInfo(_("Pause"),WT_PAUSE);
					ticks.reset();
					break;
				case SDL_SCANCODE_F:
					config.show_fps = !config.show_fps;
					break;
				case SDL_SCANCODE_R:
					if (cgame.getCurrentPlayer()->getLives() > 1 ) {
						text.clear();
						text.push_back(_("Restart level? y/n"));
						text.push_back(_("(Costs a life.)"));
						if (showInfo(text, WT_YESNO)) {
							flags |= CGF_RESTARTLEVEL;
							ticks.reset();
						}
					} else
						showInfo("No life left to restart.", WT_ANYKEY);
					break;
				case SDL_SCANCODE_D:
					if (!cgame.isBonusLevel()) {
						runBrickDestroyDlg();
						ticks.reset();
					}
					break;
				case SDL_SCANCODE_ESCAPE:
					text.clear();
					text.push_back(_("Quit Game? y/n"));
					text.push_back(_("(No hiscore entry yet, game can be resumed later.)"));
					if (showInfo(text,WT_YESNO)) {
						leave = true;
						resumeLater = true;
					}
					ticks.reset();
					break;
				default:
					break;
				}
			}
		}

		/* get paddle input state */
		pis.reset();
		/* mouse input */
		int mxoff = 0, mx = 0;
		Uint32 buttons = 0;
		if (config.rel_motion)
			buttons = SDL_GetRelativeMouseState( &mxoff, NULL );
		else
			buttons = SDL_GetMouseState(&mx, NULL);
		if (buttons & SDL_BUTTON(SDL_BUTTON_LEFT))
			pis.leftFire = 1;
		if (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT))
			pis.rightFire = 1;
		if (buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE))
			pis.speedUp = 1;
		if (config.rel_motion && mxoff != 0) {
			if (config.invert)
				mxoff = -mxoff;
			mxoff = mxoff * config.motion_mod / 100;
			rx = s2v((double)mxoff);
		} else if (!config.rel_motion)
			rx = s2v((double)mx);
		else
			rx = 0;
		/* key input */
		const Uint8 *keystate = SDL_GetKeyboardState(NULL);
		if (keystate[config.k_maxballspeed])
			pis.speedUp = 1;
		if (keystate[config.k_return])
			pis.recall = 1;
		if (keystate[config.k_left])
			pis.left = 1;
		if (keystate[config.k_right])
			pis.right = 1;
		if (keystate[config.k_lfire])
			pis.leftFire = 1;
		if (keystate[config.k_rfire])
			pis.rightFire = 1;
		if (keystate[config.k_turbo])
			pis.turbo = 1;
		if (keystate[config.k_warp])
			pis.warp = 1;
		/* gamepad input */
		const Uint8 *gpadstate = gamepad.update();
		if (gpadstate[GPAD_LEFT])
			pis.left = 1;
		if (gpadstate[GPAD_RIGHT])
			pis.right = 2;
		if (gpadstate[GPAD_BUTTON0 + config.gp_lfire])
			pis.leftFire = 1;
		if (gpadstate[GPAD_BUTTON0 + config.gp_rfire])
			pis.rightFire = 1;
		if (gpadstate[GPAD_BUTTON0 + config.gp_turbo])
			pis.turbo = 1;
		if (gpadstate[GPAD_BUTTON0 + config.gp_warp])
			pis.warp = 1;
		if (gpadstate[GPAD_BUTTON0 + config.gp_maxballspeed])
			pis.speedUp = 1;

		/* get passed time */
		ms = ticks.get();

		/* update animations and particles */
		energyBallAlphaCounter.update(ms);
		if (showWarpIcon)
			warpIconAlphaCounter.update(ms);
		if (lblTitleCounter.isRunning())
			lblTitleCounter.update(ms);
		shotFrameCounter.update(ms);
		weaponFrameCounter.update(ms);
		for (auto it = begin(sprites); it != end(sprites); ++it) {
			if ((*it).get()->update(ms))
				it = sprites.erase(it);
		}
		if (shineX != -1) {
			if (shineFrameCounter.update(ms)) {
				shineX = -1;
				shineDelay.reset();
			}
		} else if (shineDelay.update(ms))
			getNewShinePosition();

		/* update game context */
		if (flags & CGF_RESTARTLEVEL)
			flags = cgame.restartLevel();
		else
			flags = cgame.update(ms, rx, pis);
		if (flags & CGF_LIFELOST) {
			mixer.playx(theme.sLooseLife,0);
			if (config.speech && config.badspeech && (rand()%2))
				mixer.play((rand()%2)?theme.sDamn:theme.sDammit);
		}
		if (flags & CGF_LASTLIFELOST) {
			text.clear();
			text.push_back(cgame.getPlayerMessage());
			text.push_back(_("Buy Continue? y/n"));
			text.push_back(_("(Costs ALL of your score.)"));
			if (showInfo(text,WT_YESNO)) {
				cgame.continueGame();
				/* game is certainly not over so clear flag */
				flags &= ~CGF_GAMEOVER;
				/* continueGame reinits the level */
				flags |= CGF_NEWLEVEL;
			}
			/* and message has been handled, too */
			flags &= ~CGF_PLAYERMESSAGE;
		}
		if (flags & CGF_PLAYERMESSAGE)
			showInfo(cgame.getPlayerMessage());
		if (flags & CGF_GAMEOVER)
			break;
		if ((flags & CGF_NEWLEVEL) || (flags & CGF_RESTARTLEVEL)) {
			flags |= CGF_UPDATEBACKGROUND | CGF_UPDATEBRICKS |
					CGF_UPDATESCORE | CGF_UPDATEEXTRAS;
			curWallpaperId = rand() % theme.numWallpapers;
			if (flags & CGF_NEWLEVEL)
				if (!(flags & CGF_LIFELOST) && config.speech && (rand()%2))
					mixer.play((rand()%2)?theme.sVeryGood:theme.sExcellent);
			dim();
			ticks.reset();
			if (!(flags & CGF_LIFELOST)) {
				initTitleLabel();
				showWarpIcon = false;
				sprites.clear();
			}
			lblInfo.clearText();
		}
		if (flags & CGF_UPDATEBACKGROUND)
			renderBackgroundImage();
		if (flags & CGF_UPDATEBRICKS)
			renderBricksImage();
		if (flags & CGF_UPDATEEXTRAS)
			renderExtrasImage();
		if (flags & CGF_UPDATESCORE)
			renderScoreImage();
		if (flags & CGF_NEWANIMATIONS)
			createSprites();
		if (flags & CGF_WARPOK)
			showWarpIcon = true;
		if (flags & CGF_UPDATEINFO)
			lblInfo.setText(theme.fSmall, cgame.getBonusLevelInfo());

		/* handle sounds by accessing game->mod */
		playSounds();

		/* render */
		render();
		SDL_RenderPresent(mrc);

		/* stats */
		fpsCycles++;
		if (fpsStart < SDL_GetTicks())
			fps = 1000 * fpsCycles / (SDL_GetTicks() - fpsStart);
		if (fpsCycles > 100) {
			fpsCycles = 0;
			fpsStart = SDL_GetTicks();
		}

		/* limit frame rate */
		delay = maxDelay - renderTicks.get(true);
		if (delay > 0)
			SDL_Delay(delay);
		SDL_FlushEvent(SDL_MOUSEMOTION); /* prevent event loop from dying */
	}

	if (resumeLater)
		saveGame();
	else {
		/* check hiscores */
		cgame.updateHiscores();
		/* show final hiscore */
		if (!quitReceived)
			showFinalHiscores();
		/* delete save game */
		remove(saveFileName.c_str());
	}
	updateResumeGameTooltip();

	if (!quitReceived)
		dim();
	grabInput(0);
}

/** Render current game state. */
void View::render()
{
	Game *game = cgame.getGameContext(); /* direct lib game context */
	Paddle *paddle = game->paddles[0]; /* local paddle always at bottom */
	Extra *extra = 0;
	Shot *shot = 0;

	if (viewport.w != 0)
		SDL_RenderSetViewport(mrc,&viewport);

	if (cgame.darknessActive()) {
		SDL_SetRenderDrawColor(mrc,0,0,0,255);
		SDL_RenderClear(mrc);
		theme.paddles.setAlpha(128);
		theme.shot.setAlpha(128);
		theme.weapon.setAlpha(128);
	} else {
		imgBackground.copy(0,0);
		imgScore.copy(imgScoreX,imgScoreY);
		imgExtras.copy(imgExtrasX,imgExtrasY);
		theme.paddles.clearAlpha();
		theme.shot.clearAlpha();
		theme.weapon.clearAlpha();
	}

	/* balls - shadows */
	renderBalls(true);

	if (!cgame.darknessActive())
		imgBricks.copy(imgBricksX,imgBricksY);

	/* extras - shadows */
	list_reset(game->extras);
	while ( ( extra = (Extra*)list_next( game->extras) ) != 0 ) {
		int a = extra->alpha;
		if (cgame.darknessActive())
			a /= 2;
		theme.extrasShadow.setAlpha(a);
		theme.extrasShadow.copy(extra->type, 0,
				v2s(extra->x) + theme.shadowOffset,
				v2s(extra->y) + theme.shadowOffset);
	}

	/* shots - shadows */
	list_reset(game->shots);
	while ( ( shot = (Shot*)list_next( game->shots) ) != 0 )
		theme.shotShadow.copy(shotFrameCounter.get(),0,
					v2s(shot->x) + theme.shadowOffset,
					v2s(shot->y) + theme.shadowOffset);

	/* paddle */
	if (!paddle->invis || paddle->invis_delay > 0) {
		int pid = 0;
		int poff = theme.paddles.getGridWidth();
		int ph = theme.paddles.getGridHeight();
		double px = v2s(paddle->x);
		double py = v2s(paddle->y);
		int pmw = v2s(paddle->w) - 2*poff;
		int pxr = px + poff + pmw; /* right end part x */
		if (paddle->frozen)
			pid = 2;
		else if (paddle->slime)
			pid = 1;
		if (paddle->invis || cgame.darknessActive())
			theme.paddles.setAlpha(128);
		else
			theme.paddles.clearAlpha();
		/* FIXME for paddle we do the shadow here as it's composed and
		 * otherwise difficult and quite at the bottom so chance for
		 * graphical errors is pretty low... yes I'm lazy... */
		theme.paddlesShadow.copy(0,pid,px+theme.shadowOffset,py+theme.shadowOffset);
		theme.paddles.copy(0,pid,px,py);
		for (px += poff; px < pxr - poff; px += poff) {
			theme.paddlesShadow.copy(1,pid,px+theme.shadowOffset,py+theme.shadowOffset);
			theme.paddles.copy(1,pid,px,py);
		}
		/* copy last middle part, potentially shortened */
		theme.paddlesShadow.copy(1,pid,0,0,pxr-px,ph,px+theme.shadowOffset,py+theme.shadowOffset);
		theme.paddles.copy(1,pid,0,0,pxr-px,ph,px,py);
		theme.paddlesShadow.copy(2,pid,pxr+theme.shadowOffset,py+theme.shadowOffset);
		theme.paddles.copy(2,pid,pxr,py);
	}
	if (paddle->weapon_inst) {
		int wx = v2s(paddle->x + paddle->w/2) -
						theme.weapon.getGridWidth()/2;
		int wy = v2s(paddle->y);
		theme.weapon.copy(weaponFrameCounter.get(),0,wx,wy);
	}

	/* balls */
	if (config.ball_level == BALL_BELOW_BONUS)
		renderBalls();

	/* shots */
	list_reset(game->shots);
	while ( ( shot = (Shot*)list_next( game->shots) ) != 0 )
		theme.shot.copy(shotFrameCounter.get(),0,v2s(shot->x),v2s(shot->y));

	/* extra floor */
	if (cgame.floorActive() && !cgame.darknessActive()) {
		if (cgame.getFloorTime() <= 3000 && cgame.getFloorTime() > 0) {
			int off = ((cgame.getFloorTime()/200 % 2) == 0);
			imgFloor.setAlpha(off?128:255);
		} else
			imgFloor.setAlpha(cgame.getFloorAlpha());
		imgFloor.copy(imgFloorX,imgFloorY);
	}

	/* shine effect */
	if (shineX != -1 && cgame.isBrickAtPosition(shineX,shineY))
		theme.shine.copy(shineFrameCounter.get(),0,
			shineX*brickScreenWidth,
			shineY*brickScreenHeight);

	/* sprites */
	if (cgame.darknessActive()) {
		for (auto& s : sprites) {
			Particle *p = dynamic_cast<Particle *>(s.get());
			if (p == NULL) // skip particles
				s->render();
		}
	} else
		for (auto& s : sprites)
			s->render();

	/* extras */
	list_reset(game->extras);
	while ( ( extra = (Extra*)list_next( game->extras) ) != 0 ) {
		int a = extra->alpha;
		if (cgame.darknessActive())
			a /= 2;
		theme.extras.setAlpha(a);
		theme.extras.copy(extra->type, 0, v2s(extra->x), v2s(extra->y));
	}

	/* balls */
	if (config.ball_level == BALL_ABOVE_BONUS)
		renderBalls();

	/* copy part of frame to cover shadow */
	if (!cgame.darknessActive()) {
		int sx = (MAPWIDTH-1) * brickScreenWidth;
		int sy = brickScreenHeight;
		imgBackground.copy(sx,sy,brickScreenWidth,
					(MAPHEIGHT-1)*brickScreenHeight,sx,sy);
	}

	/* title and author */
	if (lblTitleCounter.isRunning()) {
		double a = 255;
		if (lblTitleCounter.get() < 1)
			a = lblTitleCounter.get() * 255;
		else if (lblTitleCounter.get() >= 4)
			a = (5-lblTitleCounter.get()) * 255;
		lblTitle.setAlpha(a);
		lblTitle.copy((1+MAPWIDTH/2)*brickScreenWidth, mw->getHeight()/2);
	}

	/* info for bonus levels */
	if (cgame.isBonusLevel())
		lblInfo.copy(1.1*brickScreenWidth,(MAPHEIGHT-1)*brickScreenHeight,ALIGN_X_LEFT | ALIGN_Y_TOP);

	/* warp icon */
	if (showWarpIcon) {
		theme.warpIcon.setAlpha(warpIconAlphaCounter.get());
		theme.warpIcon.copy(warpIconX,warpIconY);
	}

	/* stats */
	if (config.show_fps) {
		theme.fSmall.setAlign(ALIGN_X_LEFT | ALIGN_Y_TOP);
		theme.fSmall.write(0,0,to_string((int)fps));
		//theme.fSmall.write(0,theme.fSmall.getLineHeight(),to_string((int)(cgame.getPaddleVelocity()*1000)));
	}

	if (viewport.w)
		SDL_RenderSetViewport(mrc, NULL);
}

/** Take background image, add frame and static hiscore chart */
void View::renderBackgroundImage() {
	int bw = theme.bricks.getGridWidth();
	int bh = theme.bricks.getGridHeight();
	SDL_Texture *tex = imgBackground.getTex();

	SDL_SetRenderTarget(mrc,tex);
	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

	/* wallpaper background */
	Image& wallpaper = theme.wallpapers[curWallpaperId];
	for (int wy = 0; wy < imgBackground.getHeight(); wy += wallpaper.getHeight())
		for (int wx = 0; wx < imgBackground.getWidth(); wx += wallpaper.getWidth())
			wallpaper.copy(wx,wy);

	/* frame */
	theme.frameShadow.copy(theme.shadowOffset,theme.shadowOffset);
	theme.frame.copy(0,0);

	/* title + current level */
	int tx = theme.boardX, ty = bh;
	int tw = theme.boardWidth;
	theme.fNormal.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
	theme.fSmall.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
	theme.fNormal.write(tx+tw/2, ty + bh/2, cgame.getLevelsetName().c_str());
	theme.fSmall.write(tx + tw/2, ty + 3*bh/2,
			string(_("(Level ")) +
				to_string(cgame.getCurrentPlayer()->getLevel()+1)
				+ "/" + to_string(cgame.getLevelCount()) + ")");

	/* hiscores */
	renderHiscore(theme.fNormal, theme.fSmall, tx+bw/4, 3*bh, tw - bw/2, 10*bh,false);

	/* player name */
	theme.fNormal.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
	theme.fNormal.write(tx+tw/2, 15*bh, cgame.getCurrentPlayer()->getName());

	/* active extras */
	theme.fNormal.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
	theme.fNormal.write(tx+tw/2, 18*bh+bh/2, _("Active Extras"));

	/* lives */
	ClientPlayer *cp = cgame.getCurrentPlayer();
	for (uint i = 0; i < cp->getMaxLives(); i++)
		theme.life.copy(0, i < cp->getLives(), 0, (MAPHEIGHT-i-1)*bh);

	SDL_SetRenderTarget(mrc,NULL);
	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_NONE);
}

/* Render at given region. */
void View::renderHiscore(Font &fTitle, Font &fEntry, int x, int y, int w, int h, bool detailed)
{
	HiscoreChart *chart = cgame.getHiscoreChart();

	int cy = y + (h - fTitle.getLineHeight() - 10*fEntry.getLineHeight())/2;

	string str = _("Hiscores");
	if (detailed)
		str = cgame.getLevelsetName() + " " + str;
	fTitle.setAlign(ALIGN_X_CENTER | ALIGN_Y_TOP);
	fTitle.write(x + w/2, cy, str);
	cy += fTitle.getLineHeight();
	if (detailed)
		cy += fTitle.getLineHeight();

	int cx_num = x;
	int cx_name = x + fEntry.getCharWidth('0')*3;
	int cx_level = x + fEntry.getCharWidth('W')*10;
	int cx_score = x + w;

	for (int i = 0; i < CHARTSIZE; i++) {
		const ChartEntry *entry = chart->get(i);
		if (entry->newEntry && detailed)
			fEntry.setColor(theme.fontColorHighlight);
		else
			fEntry.setColor(theme.fontColorNormal);

		fEntry.setAlign(ALIGN_X_LEFT | ALIGN_Y_TOP);

		/* number */
		str = to_string(i+1) + ".";
		fEntry.write(cx_num, cy, str);
		/* name */
		fEntry.write(cx_name, cy, entry->name);
		/* level */
		if (detailed)
			fEntry.write(cx_level, cy, "L" + to_string(entry->level+1));
		/* score */
		fEntry.setAlign(ALIGN_X_RIGHT | ALIGN_Y_TOP);
		fEntry.write(cx_score, cy, to_string(entry->score));

		cy += fEntry.getLineHeight();
	}
	fEntry.setColor(theme.fontColorNormal);
}

void View::renderBricksImage()
{
	Game *game = cgame.getGameContext();
	int bw = theme.bricks.getGridWidth();
	int bh = theme.bricks.getGridHeight();

	imgBricks.fill(0,0,0,0);
	SDL_SetRenderTarget(mrc, imgBricks.getTex());

	for (int i = 1; i < MAPWIDTH-1; i++)
		for (int j = 1; j < MAPHEIGHT; j++) {
			Brick *b = &game->bricks[i][j];
			if (b->type != MAP_EMPTY && b->id != INVIS_BRICK_ID)
				theme.bricksShadow.copy(b->id, 0, (i-1)*bw + theme.shadowOffset,
						(j-1)*bh + theme.shadowOffset);
		}
	for (int i = 1; i < MAPWIDTH-1; i++)
		for (int j = 1; j < MAPHEIGHT; j++) {
			Brick *b = &game->bricks[i][j];
			if (b->type != MAP_EMPTY && b->id != INVIS_BRICK_ID)
				theme.bricks.copy(b->id, 0, (i-1)*bw, (j-1)*bh);
		}

	SDL_SetRenderTarget(mrc, NULL);
}
void View::renderScoreImage()
{
	imgScore.fill(0,0,0,0);
	SDL_SetRenderTarget(mrc, imgScore.getTex());
	theme.fNormal.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
	theme.fNormal.write(imgScore.getWidth()/2, imgScore.getHeight()/2,
				to_string(cgame.getCurrentPlayer()->getScore()));
	SDL_SetRenderTarget(mrc, NULL);
}
void View::renderExtrasImage()
{
	Game *game = cgame.getGameContext();
	uint bw = brickScreenWidth, bh = brickScreenHeight;
	uint xstart = (imgExtras.getWidth() - 3*bw) / 4;
	uint ystart = (imgExtras.getHeight() - 3*bh) / 4;
	uint xoff = bw + xstart;
	uint yoff = bh + ystart;
	uint x = xstart, y = ystart;

	SDL_SetRenderTarget(mrc, imgExtras.getTex());
	SDL_SetRenderDrawColor(mrc,0,0,0,0);
	SDL_RenderClear(mrc);

	theme.extras.setAlpha(192);
	/* energy/explosive/weak balls */
	if (game->extra_active[EX_METAL])
		renderActiveExtra(EX_METAL, game->extra_time[EX_METAL], x, y);
	else if (game->extra_active[EX_EXPL_BALL])
		renderActiveExtra(EX_EXPL_BALL, game->extra_time[EX_EXPL_BALL], x, y);
	else if (game->extra_active[EX_WEAK_BALL])
		renderActiveExtra(EX_WEAK_BALL, game->extra_time[EX_WEAK_BALL], x, y);
	x += xoff;
	/* slow/fast balls */
	if (game->extra_active[EX_SLOW])
		renderActiveExtra(EX_SLOW, game->extra_time[EX_SLOW], x, y);
	else if (game->extra_active[EX_FAST])
		renderActiveExtra(EX_FAST, game->extra_time[EX_FAST], x, y);
	x += xoff;
	/* chaotic */
	if (game->extra_active[EX_CHAOS])
		renderActiveExtra(EX_CHAOS, game->extra_time[EX_CHAOS], x, y);
	x = xstart;
	y += yoff;
	/* sticky paddle */
	if (game->paddles[0]->extra_active[EX_SLIME])
		renderActiveExtra(EX_SLIME, game->paddles[0]->extra_time[EX_SLIME], x, y);
	x += xoff;
	/* weapon - paddle */
	if (game->paddles[0]->extra_active[EX_WEAPON])
		renderActiveExtra(EX_WEAPON, game->paddles[0]->extra_time[EX_WEAPON], x, y);
	x += xoff;
	/* goldshower - paddle */
	if (game->paddles[0]->extra_active[EX_GOLDSHOWER])
		renderActiveExtra(EX_GOLDSHOWER, game->paddles[0]->extra_time[EX_GOLDSHOWER], x, y);
	x = xstart;
	y += yoff;
	/* wall - paddle */
	if (game->paddles[0]->extra_active[EX_WALL])
		renderActiveExtra(EX_WALL, game->paddles[0]->extra_time[EX_WALL], x, y);
	x += xoff;
	/* magnet - paddle */
	if (game->paddles[0]->extra_active[EX_BONUS_MAGNET])
		renderActiveExtra(EX_BONUS_MAGNET, game->paddles[0]->extra_time[EX_BONUS_MAGNET], x, y);
	else if (game->paddles[0]->extra_active[EX_MALUS_MAGNET])
		renderActiveExtra(EX_MALUS_MAGNET, game->paddles[0]->extra_time[EX_MALUS_MAGNET], x, y);
	x += xoff;
	/* ghost - paddle */
	if (game->paddles[0]->extra_active[EX_GHOST_PADDLE])
		renderActiveExtra(EX_GHOST_PADDLE, game->paddles[0]->extra_time[EX_GHOST_PADDLE], x, y);

	theme.extras.clearAlpha();
	SDL_SetRenderTarget(mrc, NULL);
}

/** Render to current target: extra image and time in seconds */
void View::renderActiveExtra(int id, int ms, int x, int y)
{
	SDL_Rect drect = {x,y,(int)brickScreenWidth,(int)brickScreenHeight};
	SDL_RenderFillRect(mrc,&drect);
	theme.extras.copy(id,0,x,y);
	theme.fSmall.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
	theme.fSmall.write(x + brickScreenWidth/2, y + brickScreenHeight/2,
				to_string(ms/1000+1));
}

/* Dim screen to black. As game engine is already set for new state
 * we cannot use render() but need current screen state. */
void View::dim()
{
	Image img;
	img.createFromScreen();

	for (uint8_t a = 250; a > 0; a -= 10) {
		SDL_SetRenderDrawColor(mrc,0,0,0,255);
		SDL_RenderClear(mrc);
		img.setAlpha(a);
		img.copy();
		SDL_RenderPresent(mrc);
		SDL_Delay(10);
	}

	/* make sure no input is given yet for next state */
	waitForInputRelease();
}

bool View::showInfo(const string &line, int type)
{
	vector<string> text;
	text.push_back(line);
	return showInfo(text, type);
}

/* Darken current screen content and show info text.
 * If confirm is false, wait for any key/click.
 * If confirm is true, wait for key y/n and return true false. */
bool View::showInfo(const vector<string> &text, int type)
{
	Font &font = theme.fSmall;
	bool ret = true;
	uint h = text.size() * font.getLineHeight();
	int tx = mw->getWidth()/2;
	int ty = (mw->getHeight() - h)/2;

	darkenScreen();

	font.setAlign(ALIGN_X_CENTER | ALIGN_Y_TOP);
	for (uint i = 0; i < text.size(); i++) {
		font.write(tx,ty,text[i]);
		ty += font.getLineHeight();
	}

	SDL_RenderPresent(mrc);

	grabInput(0);
	ret = waitForKey(type);
	grabInput(1);
	return ret;
}

/* Create particles for brick hit of type HT_REMOVE (already checked). */
void View::createParticles(BrickHit *hit)
{
	double vx, vy;
	Particle *p;
	int pw, ph;
	int bx = hit->x*brickScreenWidth, by = hit->y*brickScreenHeight;

	/* if brick is explosive always explode */
	if (hit->draw_explosion)
		hit->dest_type = SHR_BY_EXPL;

	switch (hit->dest_type) {
	case SHR_BY_NORMAL_BALL:
		vx = cos( 6.28 * hit->degrees / 180);
		vy = sin( 6.28 * hit->degrees / 180 );
		p = new Particle(theme.bricks, hit->brick_id, 0, 0, 0,
					brickScreenWidth, brickScreenHeight,
					bx, by, vx, vy, v2s(0.13), 500);
		sprites.push_back(unique_ptr<Particle>(p));
		break;
	case SHR_BY_ENERGY_BALL:
		pw = brickScreenWidth/2;
		ph = brickScreenHeight/2;
		for ( int i = 0, sx = 0; i < 2; i++, sx+=pw ) {
			for ( int j = 0, sy = 0; j < 2; j++, sy+=ph ) {
				vx = pw - (sx + pw/2);
				vy = ph - (sy + ph/2);
				p = new Particle(theme.bricks, hit->brick_id, 0,
						sx, sy, pw, ph,
						bx+sx, by+sy,
						vx, vy, v2s(0.02), 500);
				sprites.push_back(unique_ptr<Particle>(p));
			}
		}
		break;
	case SHR_BY_SHOT:
		/* FIXME this will cause artifacts on 768p better start
		 * creating from the middle moving outwards */
		pw = brickScreenWidth / 10;
		for ( int i = 0, sx = 0; i < 5; i++, sx += pw ) {
			p = new Particle(theme.bricks, hit->brick_id, 0,
						sx, 0, pw, brickScreenHeight,
						bx+sx, by,
						0, -1, v2s(0.006*i+0.002), 500);
			sprites.push_back(unique_ptr<Particle>(p));
			p = new Particle(theme.bricks, hit->brick_id, 0,
						brickScreenWidth - sx - pw,
						0, pw, brickScreenHeight,
						bx+brickScreenWidth - sx - pw, by,
						0, -1, v2s(0.006*i+0.002), 500);
			sprites.push_back(unique_ptr<Particle>(p));
		}
		break;
	case SHR_BY_EXPL:
		pw = brickScreenWidth/10;
		ph = brickScreenHeight/5;
		for ( int i = 0, sx=0; i < 10; i++, sx += pw )
			for ( int j = 0, sy=0; j < 5; j++, sy += ph ) {
				vx = 0.5 - 0.01*(rand()%100);
				vy = 0.5 - 0.01*(rand()%100);
				p = new Particle(theme.bricks, hit->brick_id, 0,
						sx, sy, pw, ph, bx+sx, by+sy,
						vx, vy, 0.01*(rand()%10+5), 1000);
				sprites.push_back(unique_ptr<Particle>(p));
			}
		break;
	}
}

/* Create new explosions, particles, etc according to game mods */
void View::createSprites()
{
	Game *game = cgame.getGameContext();
	BrickHit *hit;

	for (int i = 0; i < game->mod.brick_hit_count; i++) {
		hit = &game->mod.brick_hits[i];

		/* brick hit animation */
		if (hit->type == HT_REMOVE)
			createParticles(hit);

		/* explosion */
		if (hit->draw_explosion) {
			int x = hit->x * brickScreenWidth + brickScreenWidth/2 -
										theme.explosions.getGridWidth()/2;
			int y = hit->y * brickScreenHeight + brickScreenHeight/2 -
										theme.explosions.getGridHeight()/2;
			sprites.push_back(unique_ptr<Animation>(
				new Animation(theme.explosions,
					rand() % theme.explosions.getGridSizeY(),
					theme.explAnimDelay, x, y)));
		}

	}
}

void View::getBallViewInfo(Ball *ball, int *x, int *y, uint *type)
{
	Game *game = cgame.getGameContext(); /* direct lib game context */
	Paddle *paddle = game->paddles[0]; /* local paddle always at bottom */

	uint bt = 0;
	double px = ball->cur.x;
	double py = ball->cur.y;
	if (ball->attached) {
		px += paddle->x;
		py += paddle->y;
	}
	px = v2s(px);
	py = v2s(py);
	if (game->extra_active[EX_METAL])
		bt = 1;
	else if (game->extra_active[EX_EXPL_BALL])
		bt = 2;
	else if (game->extra_active[EX_WEAK_BALL])
		bt = 3;
	else if (game->extra_active[EX_CHAOS])
		bt = 4;

	*x = px;
	*y = py;
	*type = bt;
}

void View::playSounds()
{
	Game *game = cgame.getGameContext();
	bool playReflectSound = false;
	int sx = VG_BRICKAREAWIDTH/2;
	int paddlesx = game->paddles[0]->x + game->paddles[0]->w/2;

	if (game->mod.brick_reflected_ball_count > 0)
		if (game->mod.brick_hit_count == 0) {
			playReflectSound = true;
			sx = last_ball_brick_reflect_x;
		}
	for (int i = 0; i < game->mod.brick_hit_count; i++)
		if (game->mod.brick_hits[i].type == HT_HIT) {
			playReflectSound = true;
			sx = game->mod.brick_hits[i].x * VG_BRICKWIDTH;
			break;
		}
	if (playReflectSound)
		mixer.playx(theme.sReflectBrick,sx);
	if (game->mod.paddle_reflected_ball_count > 0)
		mixer.playx(theme.sReflectPaddle,paddlesx);
	if (game->mod.fired_shot_count > 0)
		mixer.playx(theme.sShot,paddlesx);
	if (game->mod.attached_ball_count > 0)
		mixer.playx(theme.sAttach,paddlesx);

	/* by default brick hit and explosion are the same sound,
	 * but playing it for an explosion twice makes it louder
	 * so this does make sense... */
	bool hitPlayed = false;
	bool explPlayed = false;
	bool energyPlayed = false;
	for (int i = 0; i < game->mod.brick_hit_count; i++) {
		if (game->mod.brick_hits[i].no_sound)
			continue;
		if (game->mod.brick_hits[i].type != HT_REMOVE)
			continue;
		sx = game->mod.brick_hits[i].x * VG_BRICKWIDTH;
		if (game->mod.brick_hits[i].dest_type == SHR_BY_ENERGY_BALL && !energyPlayed) {
			mixer.playx(theme.sEnergyHit,sx);
			energyPlayed = true;
		}
		if (game->mod.brick_hits[i].dest_type != SHR_BY_ENERGY_BALL && !hitPlayed) {
			mixer.playx(theme.sBrickHit,sx);
			hitPlayed = true;
		}
		if (game->mod.brick_hits[i].draw_explosion && !explPlayed) {
			mixer.playx(theme.sExplosion,sx);
			explPlayed = true;
		}
	}

	for (int i = 0; i < game->mod.collected_extra_count[0]; i++)
		mixer.playx(theme.sExtras[game->mod.collected_extras[0][i]],
								paddlesx);
}

void View::createMenus()
{
	Menu *mNewGame, *mOptions, *mAudio, *mGraphics, *mControls, *mAdv, *mEditor;
	const char *diffNames[] = {_("Kids"),_("Very Easy"),_("Easy"),_("Medium"),_("Hard") } ;
	const char *fpsLimitNames[] = {_("No Limit"),_("200 FPS"),_("100 FPS") } ;
	const int bufSizes[] = { 256, 512, 1024, 2048, 4096 };
	const int channelNums[] = { 8, 16, 32 };

	/* XXX too lazy to set fonts for each and every item...
	 * use static pointers instead */
	MenuItem::fNormal = &theme.fMenuNormal;
	MenuItem::fFocus = &theme.fMenuFocus;
	MenuItem::fTooltip = &theme.fSmall;
	MenuItem::tooltipWidth = 0.3 * theme.menuBackground.getWidth();

	rootMenu.reset(); /* delete any old menu ... */

	rootMenu = unique_ptr<Menu>(new Menu(theme)); /* .. or is assigning a new object doing it? */
	mNewGame = new Menu(theme);
	mOptions = new Menu(theme);
	mAudio = new Menu(theme);
	mGraphics = new Menu(theme);
	graphicsMenu = mGraphics; /* needed to return after mode/theme change */
	mControls = new Menu(theme);
	mAdv = new Menu(theme);
	mEditor = new Menu(theme);

	mNewGame->add(new MenuItem(_("Start Original Levels"),
			_("Start level set 'LBreakoutHD'."),
			AID_STARTORIGINAL));
	mNewGame->add(new MenuItem(_("Start Custom Levels"),
			_("Select and run a custom level set."),
			AID_STARTCUSTOM));
	mNewGame->add(new MenuItemSep());
	mNewGame->add(new MenuItemList(_("Difficulty"),
			_("This affects lives, paddle size and ball speed. For Kids and Easy score is reduced, for Hard a bonus is given."),
			AID_NONE,config.diff,diffNames,DIFF_COUNT));
	mNewGame->add(new MenuItemSep());
	mNewGame->add(new MenuItemRange(_("Players"),
			_("Number and names of players. Players alternate whenever a life is lost."),
			AID_NONE,config.player_count,1,MAX_PLAYERS,1));
	mNewGame->add(new MenuItemEdit(_("1st"),config.player_names[0]));
	mNewGame->add(new MenuItemEdit(_("2nd"),config.player_names[1]));
	mNewGame->add(new MenuItemEdit(_("3rd"),config.player_names[2]));
	mNewGame->add(new MenuItemEdit(_("4th"),config.player_names[3]));
	mNewGame->add(new MenuItemSep());
	mNewGame->add(new MenuItemBack(rootMenu.get()));

	mControls->add(new MenuItemKey(_("Left"),"", config.k_left));
	mControls->add(new MenuItemKey(_("Right"),"",config.k_right));
	mControls->add(new MenuItemKey(_("Left Fire"),
			_("Fire balls to the left. Ignored if 'Ball Fire Angle' in 'Advanced Settings' is 'Random'."),
			config.k_lfire));
	mControls->add(new MenuItemKey(_("Right Fire"),
			_("Fire balls to the right. Ignored if 'Ball Fire Angle' in 'Advanced Settings' is 'Random'."),
			config.k_rfire));
	mControls->add(new MenuItemKey(_("Paddle Turbo"),
			_("Speed up paddle movement."),
			config.k_turbo));
	mControls->add(new MenuItemKey(_("Ball Turbo"),
			_("Speed up balls to limit 'Acc. Ball Speed' in 'Advanced Settings'."),
			config.k_maxballspeed));
	mControls->add(new MenuItemKey(_("Idle Return"),
			_("Return all idle balls (no effective brick hits for some time) to the paddle."),
			config.k_return));
	mControls->add(new MenuItemSep());
	mControls->add(new MenuItemRange(_("Key Speed"),
			_("The higher the value the faster the paddle moves by keys."),
			AID_ADJUSTKEYSPEED,config.i_key_speed,100,1000,50));
	mControls->add(new MenuItemList(_("Mouse Input"),
			_("'Absolute' uses absolute mouse position. No fine tuning possible and you might experience delay (if moving too far to the right you'll need to come back to the paddle position first) but it will be more accurate for slow movements.\n'Relative' allows modifying paddle speed but might get inaccurate for slow movements with high resolution and high frame rates."),
			AID_NONE,config.rel_motion,_("Absolute"),_("Relative")));
	mControls->add(new MenuItemRange(_("Motion Modifier"),
			_("Adjust mouse sensitivity. (relative motion only)"),
			AID_NONE,config.motion_mod,40,160,10));
	mControls->add(new MenuItemSwitch(_("Invert Motion"),
			_("Invert mouse motion if needed. (relative motion only)"),
			AID_NONE,config.invert));
	mControls->add(new MenuItemSep());
	mControls->add(new MenuItemBack(mOptions));

	mGraphics->add(new MenuItemList(_("Theme"),
			_("Select theme. (not applied yet)"),
			AID_NONE,config.theme_id,themeNames));
	mGraphics->add(new MenuItemList(_("Mode"),
			_("Select mode. (not applied yet)"),
			AID_NONE,config.mode,modeNames));
	mGraphics->add(new MenuItemList(_("Antialiasing"),
			_("'Auto' means no antialiasing for old LBreakout2 low resolution themes as this might cause (slight) graphical errors. 'Always' means to use it even for low resolution themes."),
			AID_NONE,config.antialiasing,_("Auto"),_("Always")));
	mGraphics->add(new MenuItem(_("Apply Theme&Mode"),
			_("Apply the above settings."),AID_APPLYTHEMEMODE));
	mGraphics->add(new MenuItemSep());
	mGraphics->add(new MenuItemList(_("Frame Limit"),
			_("Maximum number of frames per second.\nBe careful: The higher the limit the more insensitive your mouse might become to slow movements (because relative motion is used and program cycles are shorter).\n200 FPS should be a good value."),
			AID_NONE,config.fps,fpsLimitNames,3));
	mGraphics->add(new MenuItemSep());
	mGraphics->add(new MenuItemBack(mOptions));

	mAudio->add(new MenuItemSwitch(_("Sound"),"",AID_SOUND,config.sound));
	mAudio->add(new MenuItemRange(_("Volume"),"",AID_VOLUME,config.volume,0,100,10));
	mAudio->add(new MenuItemSwitch(_("Speech"),"",AID_NONE,config.speech));
	mAudio->add(new MenuItemSep());
	mAudio->add(new MenuItemIntList(_("Buffer Size"),
			_("Reduce buffer size if you experience sound delays. Might have more CPU impact though. (not applied yet)"),
			config.audio_buffer_size,bufSizes,5));
	mAudio->add(new MenuItemIntList(_("Channels"),
			_("More channels gives more sound variety, less channels less (not applied yet)"),config.channels,
			channelNums,3));
	mAudio->add(new MenuItem(_("Apply Size&Channels"),
			_("Apply above settings"),AID_APPLYAUDIO));
	mAudio->add(new MenuItemSep());
	mAudio->add(new MenuItemBack(mOptions));

	mOptions->add(new MenuItemSub(_("Controls"),mControls));
	mOptions->add(new MenuItemSub(_("Graphics"),mGraphics));
	mOptions->add(new MenuItemSub(_("Audio"),mAudio));
	mOptions->add(new MenuItemSub(_("Advanced"),mAdv));
	mOptions->add(new MenuItemSep());
	mOptions->add(new MenuItemBack(rootMenu.get()));

	mAdv->add(new MenuItemList(_("Paddle Style"),
			_("Use 'Convex' for casual play: The paddle is treated as convex allowing easy aiming.\nUse 'Normal' for regular behaviour. You will have to aim by using the side hemispheres or giving balls momentum by moving at the right moment when they hit the flat part. Only for experienced players."),
			AID_NONE,config.convex,_("Normal"),_("Convex")));
	mAdv->add(new MenuItemList(_("Ball Layer"),
			_("'Below Extras' is the normal layer. It might get hard with many extras and effects though, so change to 'Above Extras' to have balls always on top of everything (looks weird but helps to keep an eye on them)."),
			AID_NONE,config.ball_level,_("Below Extras"),_("Above Extras")));
	mAdv->add(new MenuItemList(_("Ball Fire Angle"),
			_("Either 50 degrees to the left/right or random no matter what fire key has been pressed."),
			AID_NONE,config.random_angle,"50",_("Random")));
	mAdv->add(new MenuItemList(_("Ball Turbo"),
			_("'Auto' will automatically speed up your balls (the farther away from the paddle the more).\n'Manually' puts them to maximum speed while key is pressed (default key is c)."),
			AID_NONE,config.ball_auto_turbo,_("Manually"),_("Auto")));
	mAdv->add(new MenuItemList(_("Return Balls"),
			_("'Auto' returns all idle balls (no effective brick hits) after 10 seconds to the paddle.\n'Manually' requires you to press a key (default key is Backspace).\n'Auto' is more convenient but 'Manually' might be required for (badly designed) levels where balls need to bounce around a lot."),
			AID_NONE,config.return_on_click,_("Auto"),_("Manually")));
	mAdv->add(new MenuItemRange(_("Warp Limit"),
			_("If this percentage of bricks is destroyed you can warp to the next level (no penalties) by pressing 'w'. A small icon at the lower right hand side is shown when warp is ready."),
			AID_NONE,config.rel_warp_limit,0,100,10));
	mAdv->add(new MenuItemRange(_("Acc. Ball Speed"),
			_("Maximum speed of accelerated balls."),
			AID_MAXBALLSPEEDCHANGED,config.maxballspeed_int1000,700,1200,50));
	mAdv->add(new MenuItemSwitch(_("Bonus Levels"),_("Add bonus levels with a mini game every 4 regular levels. Game over will end the mini game without loosing a life."),AID_NONE,config.add_bonus_levels));
	mAdv->add(new MenuItemSep());
	mAdv->add(new MenuItemBack(mOptions));

	mEditor->add(new MenuItemEdit(_("Create New Set"),
			config.edit_setname, AID_EDITNEWSET, true));
	mEditor->add(new MenuItem(_("Edit Existing Set"),
			_("Edit an existing levelset."),
			AID_EDITCUSTOM));
	mEditor->add(new MenuItemSep());
	mEditor->add(new MenuItemBack(rootMenu.get()));

	rootMenu->add(new MenuItemSub(_("New Game"), mNewGame));
	resumeMenuItem = new MenuItem(_("Resume Game"), "", AID_RESUME);
	rootMenu->add(resumeMenuItem);
	updateResumeGameTooltip();
	rootMenu->add(new MenuItemSep());
	rootMenu->add(new MenuItemSub(_("Settings"), mOptions));
	rootMenu->add(new MenuItem(_("Help"), "", AID_HELP));
	rootMenu->add(new MenuItemSep());
	rootMenu->add(new MenuItemSub(_("Editor"), mEditor));
	rootMenu->add(new MenuItemSep());
	rootMenu->add(new MenuItem(_("Quit"), "", AID_QUIT));

	rootMenu->adjust();
}

/** Handle click on either create new set or edit existing set.
 * Run editor and create test game if single level is to be tested.
 * Since editor would need the view to run the game (not sure if a
 * second view would be ok because of the shared SDL window?), we
 * shortly leave the editor and run the test game from inside the view.
 * Messy solution, I know, but that's just how we roll...
 */
void View::handleEditor(int type)
{
	string fname;

	/* get file name from menu or select dialog */
	if (type == AID_EDITNEWSET)
		fname = config.edit_setname;
	else {
		darkenScreen();
		selectDlg.init(SDT_CUSTOMONLY);
		if (selectDlg.run())
			fname = selectDlg.get();
		else {
			if (selectDlg.quitRcvd())
				quitReceived = true;
			return;
		}
	}

	/* run editor */
	while (1) {
		editor.run(fname);
		if (editor.quitRcvd()) {
			quitReceived = true;
			return;
		}
		/* test level? */
		if (editor.testRequested()) {
			if (cgame.initTestlevel(editor.getCurrentLevel()->title,
					editor.getCurrentLevel()->author,
					editor.getCurrentLevel()->bricks,
					editor.getCurrentLevel()->extras) == 0) {
				dim();
				run();
			}
		} else
			break; /* editor normally exited */
	}
}

void View::runMenu()
{
	SDL_Event ev;
	Ticks ticks;
	MenuItemSub *subItem;
	MenuItemBack *backItem;
	bool changingKey = false, newEvent = false, changedKey = false;
	int aid = AID_NONE;

	curMenu = rootMenu.get();
	curMenu->resetSelection();
	renderMenu();

	while (!quitReceived) {
		/* handle events */
		newEvent = false;
		if (SDL_PollEvent(&ev)) {
			newEvent = true;
			changedKey = false;
			if (ev.type == SDL_QUIT)
				quitReceived = true;
			if (changingKey && ev.type == SDL_KEYDOWN) {
				MenuItemKey *keyItem = dynamic_cast<MenuItemKey*>
							(curMenu->getCurItem());
				switch (ev.key.keysym.scancode) {
				case SDL_SCANCODE_P:
					break;
				case SDL_SCANCODE_ESCAPE:
					keyItem->cancelChange();
					changingKey = false;
					break;
				default:
					keyItem->setKey(ev.key.keysym.scancode);
					changingKey = false;
					changedKey = true;
					break;
				}
			} else if (ev.type == SDL_KEYDOWN &&
					(ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE ||
					ev.key.keysym.scancode == SDL_SCANCODE_BACKSPACE)) {
				Menu *lm = curMenu->getLastMenu();
				if (lm)
					curMenu = lm;
			}

		}

		/* update current menu */
		curMenu->update(ticks.get());

		/* handle events in items */
		if (newEvent && !changingKey && !changedKey &&
					(aid = curMenu->handleEvent(ev))) {
			if (aid != AID_FOCUSCHANGED)
				mixer.play(theme.sMenuClick);
			switch (aid) {
			case AID_FOCUSCHANGED:
				mixer.play(theme.sMenuMotion);
				break;
			case AID_QUIT:
				quitReceived = true;
				break;
			case AID_ENTERMENU:
				subItem = dynamic_cast<MenuItemSub*>(curMenu->getCurItem());
				if (subItem) { /* should never fail */
					curMenu = subItem->getSubMenu();
					curMenu->resetSelection();
				} else
					_logerr("Oops, submenu not found...\n");
				break;
			case AID_LEAVEMENU:
				backItem = dynamic_cast<MenuItemBack*>(curMenu->getCurItem());
				if (backItem) { /* should never fail */
					curMenu = backItem->getLastMenu();
					curMenu->resetSelection();
				} else
					_logerr("Oops, last menu not found...\n");
				break;
			case AID_CHANGEKEY:
				changingKey = true;
				break;
			case AID_ADJUSTKEYSPEED:
				config.key_speed = 0.001 * config.i_key_speed;
				break;
			case AID_SOUND:
				mixer.setMute(!config.sound);
				break;
			case AID_VOLUME:
				mixer.setVolume(config.volume);
				break;
			case AID_APPLYAUDIO:
				mixer.close();
				mixer.open(config.channels, config.audio_buffer_size);
				break;
			case AID_APPLYTHEMEMODE:
				/* XXX workaround for SDL bug: clear event
				 * loop otherwise left mouse button event is
				 * screwed for the first click*/
				waitForInputRelease();
				init(themeNames[config.theme_id],
					MainWindow::getModeResolution(config.mode));
				curMenu = graphicsMenu;
				curMenu->resetSelection();
				break;
			case AID_RESUME:
				if (resumeGame()) {
					dim();
					run();
					ticks.reset();
				}
				break;
			case AID_STARTORIGINAL:
				cgame.init("LBreakoutHD");
				dim();
				run();
				ticks.reset();
				break;
			case AID_STARTCUSTOM:
				darkenScreen();
				selectDlg.init();
				if (selectDlg.run()) {
					/* if FREAKOUT is started again change
					 * seed to get different order of levels */
					if (selectDlg.get() == TOURNAMENT)
						config.freakout_seed = rand();
					cgame.init(selectDlg.get());
					dim();
					run();
					ticks.reset();
				} else if (selectDlg.quitRcvd())
					quitReceived = true;
				break;
			case AID_MAXBALLSPEEDCHANGED:
				config.maxballspeed_float = (float)config.maxballspeed_int1000 / 1000;
				break;
			case AID_HELP:
				showHelp();
				ticks.reset();
				break;
			case AID_EDITNEWSET:
			case AID_EDITCUSTOM:
				handleEditor(aid);
				dim();
				ticks.reset();
				break;
			}
		}

		/* render */
		renderMenu();
		SDL_RenderPresent(mrc);
		if (config.fps)
			SDL_Delay(10);
		SDL_FlushEvent(SDL_MOUSEMOTION); /* prevent event loop from dying */
	}
	dim();

	/* clear events for menu loop */
	waitForInputRelease();
}

void View::renderMenu()
{
	theme.menuBackground.copy();
	lblCredits1.copy(mw->getWidth()-2,mw->getHeight(),
				ALIGN_X_RIGHT | ALIGN_Y_BOTTOM);
	lblCredits2.copy(mw->getWidth()-2,mw->getHeight() - theme.fSmall.getLineHeight(),
				ALIGN_X_RIGHT | ALIGN_Y_BOTTOM);
	curMenu->render();
}

void View::grabInput(int grab)
{
	if (grab) {
		SDL_ShowCursor(0);
		SDL_SetWindowGrab(mw->mw, SDL_TRUE );
		if (config.rel_motion) {
			SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1", SDL_HINT_OVERRIDE);
			SDL_SetRelativeMouseMode(SDL_TRUE);
			SDL_GetRelativeMouseState(0,0);
		}
	} else {
		SDL_ShowCursor(1);
		SDL_SetWindowGrab(mw->mw, SDL_FALSE );
		if (config.rel_motion) {
			SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "0", SDL_HINT_OVERRIDE);
			SDL_SetRelativeMouseMode(SDL_FALSE);
		}
	}
}

void View::saveGame()
{
	vector<unique_ptr<ClientPlayer>> &players = cgame.getPlayers();
	ofstream ofs(saveFileName);
	if (!ofs.is_open()) {
		_logerr("Could not open file %s\n",saveFileName.c_str());
		return;
	}

	ofs << "levelset=" << cgame.getLevelsetName() << "\n";
	ofs << "difficulty=" << config.diff << "\n";
	ofs << "curplayer=" << cgame.getCurrentPlayerId() << "\n";
	ofs << "players=" << players.size() << "\n";
	for (uint i = 0; i < players.size(); i++) {
		ofs << "player" << i << " {\n";
		ofs << "	name=" << players[i]->getName() << "\n";
		ofs << "	level=" << players[i]->getLevel() << "\n";
		ofs << "	score=" << players[i]->getScore() << "\n";
		ofs << "	lives=" << players[i]->getLives() << "\n";
		ofs << "}\n";
	}

	ofs.close();
	_loginfo("Game saved to %s\n",saveFileName.c_str());
}

/* Load game settings and init game, return 1 if successful, 0 otherwise */
int View::resumeGame()
{
	if (!fileExists(saveFileName)) {
		_logerr("No saved game found.\n");
		return 0;
	}

	FileParser fp(saveFileName);
	string setname;

	if (!fp.get("levelset",setname)) {
		_logerr("Save game corrupted, no levelset name.\n");
		return 0;
	}
	fp.get("difficulty",config.diff);
	if (!fp.get("players",config.player_count)) {
		_logerr("Save game corrupted, no player count\n");
		return 0;
	}
	for (int i = 0; i < config.player_count; i++)
		fp.get(string("player") + to_string(i) + ".name",config.player_names[i]);

	/* initialize game to level of current player */
	uint pid = 0;
	fp.get("curplayer",pid);
	if (pid >= MAX_PLAYERS)
		pid = 0;
	uint levid = 0;
	fp.get(string("player") + to_string(pid) + ".level",levid);
	cgame.init(setname,levid);

	/* adjust players */
	for (int i = 0; i < config.player_count; i++) {
		string prefix = string("player") + to_string(i) + ".";
		uint level = 0, lives = 3;
		int score = 0;
		fp.get(prefix + "score",score);
		fp.get(prefix + "level",level);
		fp.get(prefix + "lives",lives);
		cgame.resumePlayer(i,lives,score,level);
	}
	cgame.setCurrentPlayerId(pid);

	return 1;
}

void View::showFinalHiscores()
{
	int x, y, w, h;
	imgBackground.copy();
	darkenScreen();
	w = theme.fNormal.getCharWidth('M')*20;
	h = mw->getHeight()/2;
	x = (mw->getWidth() - w)/2;
	y = h/2;
	renderHiscore(theme.fNormal, theme.fNormal, x,y,w,h,true);
	SDL_RenderPresent(mrc);
	waitForKey(false);
}

int View::waitForKey(int type)
{
	SDL_Event ev;
	bool ret = true;
	bool leave = false;

	SDL_PumpEvents();
	SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
	while (!leave) {
		/* handle events */
		if (SDL_WaitEvent(&ev)) {
			if (ev.type == SDL_QUIT)
				quitReceived = leave = true;
			if (type == WT_ANYKEY && (ev.type == SDL_KEYUP ||
						ev.type == SDL_MOUSEBUTTONUP))
				leave = true;
			else if (ev.type == SDL_KEYUP) {
				int sc = ev.key.keysym.scancode;
				if (type == WT_YESNO) {
					if (sc == SDL_SCANCODE_Y || sc == SDL_SCANCODE_Z)
						ret = leave = true;
					if (sc == SDL_SCANCODE_N || sc == SDL_SCANCODE_ESCAPE) {
						ret = false;
						leave = true;
					}
				} else if (type == WT_PAUSE)
					if (sc == SDL_SCANCODE_P)
						ret = leave = true;
			}
		}
		SDL_FlushEvent(SDL_MOUSEMOTION);
	}
	SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
	return ret;
}

void View::darkenScreen(int alpha)
{
	Image img;
	img.createFromScreen();
	SDL_SetRenderDrawColor(mrc,0,0,0,255);
	SDL_RenderClear(mrc);
	img.setAlpha(alpha);
	img.copy();
}

void View::initTitleLabel()
{
	string n, a;
	cgame.getCurrentLevelNameAndAuthor(n, a);
	lblTitle.setText(theme.fNormal, n);
	lblTitleCounter.init(SCT_ONCE, 0, 5, 1000);
}

void View::renderBalls(bool shadow)
{
	Game *game = cgame.getGameContext();
	Ball *ball;
	list_reset(game->balls);
	while ( ( ball = (Ball*)list_next( game->balls ) ) != 0 ) {
		uint type;
		uint alpha = 255;
		int px, py;
		getBallViewInfo(ball, &px, &py, &type);
		if (type == 1) /* energy ball */
			alpha = energyBallAlphaCounter.get();
		if (cgame.darknessActive())
			alpha /= 2;
		if (shadow) {
			theme.ballsShadow.setAlpha(alpha);
			theme.ballsShadow.copy(type,0,
					px + theme.shadowOffset,
					py + theme.shadowOffset);
		} else {
			theme.balls.setAlpha(alpha);
			theme.balls.copy(type,0,px,py);
		}
	}
}

void View::updateResumeGameTooltip()
{
	if (!resumeMenuItem)
		return;
	if (!fileExists(saveFileName)) {
		resumeMenuItem->setTooltip(_("No saved game."));
		return;
	}

	/* XXX multiple locations... */
	const char *diffNames[] = {_("Kids"),_("Very Easy"),_("Easy"),_("Medium"),_("Hard") } ;
	string text, str;
	uint diff, pnum;
	FileParser fp(saveFileName);
	fp.get("levelset",text);
	fp.get("difficulty",diff);
	fp.get("players",pnum);
	text += " - ";
	if (diff < DIFF_COUNT)
		text += diffNames[diff];
	text += " - ";
	text += to_string(pnum);
	text += _(" player(s)\n");

	for (uint i = 0; i < pnum; i++) {
		string prefix = string("player") + to_string(i) + ".";
		string name;
		uint level = 0, lives = 3;
		int score = 0;
		fp.get(prefix + "name",name);
		fp.get(prefix + "score",score);
		fp.get(prefix + "level",level);
		fp.get(prefix + "lives",lives);
		text += name + ": " + to_string(score) + _("  (Lvl: ")
				+ to_string(level+1) + ")";
		if ( i < pnum-1)
			text += "\n";
	}

	resumeMenuItem->setTooltip(text);
}

void View::renderExtraHelp(GridImage &img, uint gx, uint gy, const string &str, int x, int y)
{
	img.copy(gx,gy,x,y);
	theme.fSmall.setAlign(ALIGN_X_LEFT | ALIGN_Y_CENTER);
	theme.fSmall.write(x + 1.1*img.getGridWidth(), y + img.getGridHeight()/2, str);
}

void View::showHelp()
{
	string helpText = _(
		"You can control your paddle either with the mouse or keyboard. "
		"Destroying bricks will release extras sometimes. Some are good, "
		"some are bad and some may be good or bad. "
		"Some of the bricks take more than one shot, regenerate over time, explode or grow new bricks "
		"(see README for more details). "
		"Balls will get faster (until life is lost) and the difficulty setting will influence "
		"number of lives, ball speed, paddle size and score.\n\n"
		"While playing you can press\n"
		"  - ESC to leave the game (you'll have to confirm)\n"
		"  - w to warp to next level (if enough bricks were destroyed)\n"
		"  - p to pause/unpause the game\n"
		"  - d to destroy a brick (workaround for bad level design)\n\n"
		"If you leave a game you can resume it later (no hiscores entry yet).\n\n"
		"If you loose all lives you can buy a continue (score set to 0, initial number of lives restored). "
		"If you don't buy a continue the game is over (hiscores entry is checked) and can no longer be resumed. "
		"\n\nEnjoy the game!\n"
		"          Michael Speck"
		);

	int x = brickScreenWidth, y = brickScreenHeight;
	uint maxw = brickScreenWidth * 10;

	darkenScreen();

	theme.fNormal.setAlign(ALIGN_X_LEFT | ALIGN_Y_TOP);
	theme.fNormal.write(x,y,_("Help"));
	y += 2*brickScreenHeight;

	theme.fSmall.writeText(x, y, helpText, maxw);

	y -= brickScreenHeight;
	x += maxw + brickScreenWidth;
	int x2 = x + 4*brickScreenWidth;
	renderExtraHelp(theme.extras, 2, 0, _("Extra Score"), x, y);
	renderExtraHelp(theme.extras, 6, 0, _("Gold Shower"), x2, y);
	y += 1.2*brickScreenHeight;
	renderExtraHelp(theme.extras, 9, 0, _("Extra Life"), x, y);
	renderExtraHelp(theme.extras, 12, 0, _("Extra Ball"), x2, y);
	y += 1.2*brickScreenHeight;
	renderExtraHelp(theme.extras, 8, 0, _("Expand Paddle"), x, y);
	renderExtraHelp(theme.extras, 7, 0, _("Shrink Paddle"), x2, y);
	y += 1.2*brickScreenHeight;
	renderExtraHelp(theme.extras, 10, 0, _("Sticky Paddle"), x, y);
	renderExtraHelp(theme.extras, 15, 0, _("Plasma Weapon"), x2, y);
	y += 1.2*brickScreenHeight;
	renderExtraHelp(theme.extras, 18, 0, _("Slow Balls"), x, y);
	renderExtraHelp(theme.extras, 17, 0, _("Fast Balls"), x2, y);
	y += 1.2*brickScreenHeight;
	renderExtraHelp(theme.extras, 11, 0, _("Energy Balls"), x, y);
	renderExtraHelp(theme.extras, 25, 0, _("Explosive Balls"), x2, y);
	y += 1.2*brickScreenHeight;
	renderExtraHelp(theme.extras, 13, 0, _("Extra Floor"), x, y);
	renderExtraHelp(theme.extras, 19, 0, _("Joker (collect goodies)"), x2, y);
	y += 1.2*brickScreenHeight;
	renderExtraHelp(theme.extras, 26, 0, _("Bonus Magnet"), x, y);
	renderExtraHelp(theme.extras, 27, 0, _("Malus Magnet"), x2, y);
	y += 1.2*brickScreenHeight;
	renderExtraHelp(theme.extras, 21, 0, _("Chaotic Balls"), x, y);
	renderExtraHelp(theme.extras, 28, 0, _("Weak Balls"), x2, y);
	y += 1.2*brickScreenHeight;
	renderExtraHelp(theme.extras, 14, 0, _("Frozen Paddle"), x, y);
	renderExtraHelp(theme.extras, 22, 0, _("Ghost Paddle"), x2, y);
	y += 1.2*brickScreenHeight;
	renderExtraHelp(theme.extras, 24, 0, _("Extra Time"), x, y);
	renderExtraHelp(theme.extras, 23, 0, _("Reset"), x2, y);
	y += 1.2*brickScreenHeight;
	renderExtraHelp(theme.extras, 16, 0, _("Random"), x, y);
	renderExtraHelp(theme.extras, 20, 0, _("Darkness"), x2, y);
	y += 2*brickScreenHeight;
	renderExtraHelp(theme.bricks, 0, 0, _("Indestructible"), x, y);
	renderExtraHelp(theme.bricks, 1, 0, _("Regular Wall"), x2, y);
	y += 1.2*brickScreenHeight;
	renderExtraHelp(theme.bricks, 2, 0, _("Chaotic Wall"), x, y);
	renderExtraHelp(theme.bricks, 14, 0, _("Regular Brick"), x2, y);
	y += 1.2*brickScreenHeight;
	renderExtraHelp(theme.bricks, 5, 0, _("Multi-hit Brick"), x, y);
	renderExtraHelp(theme.bricks, 9, 0, _("Regenerative Brick"), x2, y);
	y += 1.2*brickScreenHeight;
	renderExtraHelp(theme.bricks, 18, 0, _("Explosive Brick"), x, y);
	renderExtraHelp(theme.bricks, 19, 0, _("Growing Brick"), x2, y);

	SDL_RenderPresent(mrc);

	waitForInputRelease();
	waitForKey(false);
}

void View::runBrickDestroyDlg()
{
	SDL_Event ev;
	bool leave = false;
	int selx = -1, sely = -1;
	int texty = (EDITHEIGHT+2)*brickScreenHeight;

	imgBackground.copy();
	darkenScreen(64);
	imgBricks.setAlpha(128);
	imgBricks.copy(imgBricksX,imgBricksY);
	imgBricks.clearAlpha();
	theme.fNormal.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
	theme.fNormal.write(imgBricksX+imgBricks.getWidth()/2, texty,
				_("Plane of Inner Stability"));
	texty += theme.fNormal.getLineHeight();
	theme.fSmall.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
	theme.fSmall.write(imgBricksX+imgBricks.getWidth()/2, texty,
				_("Left-click on any brick to destroy it (costs 10% score)."));
	texty += theme.fSmall.getLineHeight();
	theme.fSmall.write(imgBricksX+imgBricks.getWidth()/2, texty,
				_("Press any key to cancel selection."));
	SDL_RenderPresent(mrc);

	grabInput(0);
	waitForInputRelease();
	while (!leave) {
		/* handle events */
		if (SDL_WaitEvent(&ev)) {
			if (ev.type == SDL_QUIT)
				quitReceived = leave = true;
			if (ev.type == SDL_KEYUP)
				leave = true;
			if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT) {
				selx = ev.button.x / brickScreenWidth;
				sely = ev.button.y / brickScreenHeight;
				if (!cgame.destroyBrick(selx,sely))
					selx = sely = -1; /* no brick found */
				else
					leave = 1;
			}
		}
		SDL_FlushEvent(SDL_MOUSEMOTION);
	}

	grabInput(1);
	waitForInputRelease();
}

/* wait up to 1s for release of all buttons and keys.
 * Clear SDL event loop afterwards.
 */
void View::waitForInputRelease()
{
	SDL_Event ev;
	bool leave = false;
	int timeout = 500, numkeys;
	Ticks ticks;

	SDL_PumpEvents();
	SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
	while (!leave) {
		if (SDL_PollEvent(&ev) && ev.type == SDL_QUIT)
			quitReceived = leave = true;
		leave = true;
		if (SDL_GetMouseState(NULL, NULL))
			leave = false;
		const Uint8 *keystate = SDL_GetKeyboardState(&numkeys);
		for (int i = 0; i < numkeys; i++)
			if (keystate[i]) {
				leave = false;
				break;
			}
		timeout -= ticks.get();
		if (timeout <= 0)
			leave = true;
		SDL_Delay(10);
	}
	SDL_PumpEvents();
	SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
}

void View::getNewShinePosition()
{
	/* multiple runs possible due to invis bricks */
	int pos = rand() % (cgame.getGameContext()->bricks_left+1);
	while (pos >= 0) {
		for (int i = 1; i <= EDITWIDTH; i++)
			for (int j = 1; j <= EDITHEIGHT; j++)
				if (pos >= 0 && cgame.isBrickAtPosition(i,j)) {
					shineX = i;
					shineY = j;
					pos--;
				}
		pos--; /* should always terminate but we make sure */
	}
}

/*
 * view.h
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

#ifndef VIEW_H_
#define VIEW_H_

enum {
	/* virtual geometry */
	VG_BRICKWIDTH = 40,
	VG_BRICKHEIGHT = 20,
	VG_BRICKAREAWIDTH = MAPWIDTH * VG_BRICKWIDTH,
	VG_BRICKAREAHEIGHT = MAPHEIGHT * VG_BRICKHEIGHT,
	VG_PADDLETILESIZE = 18,
	VG_PADDLEYPOS = VG_BRICKAREAHEIGHT - 2*VG_BRICKHEIGHT,
	VG_BALLRADIUS = 6,
	VG_BALLSIZE = 12,

	/* mixer */
	MIX_CHANNELNUM = 16,
	MIX_CUNKSIZE = 2048,

	/* waitForKey types */
	WT_ANYKEY = 0,
	WT_YESNO,
	WT_PAUSE
};

class View {
	/* general */
	Config &config;
	Theme theme;
	Mixer mixer;
	MainWindow *mw;
	SDL_Rect viewport; /* used if width not 0 */
	Gamepad gamepad;
	Editor editor;

	/* menu */
	unique_ptr<Menu> rootMenu;
	Menu *curMenu, *graphicsMenu;
	MenuItem *resumeMenuItem;
	vector<string> themeNames;
	vector<string> modeNames;
	string saveFileName;
	SelectDialog selectDlg;
	Label lblCredits1, lblCredits2;

	/* game */
	ClientGame &cgame;
	Uint32 brickAreaWidth, brickAreaHeight;
	Uint32 brickScreenWidth, brickScreenHeight;
	int scaleFactor; // *100, e.g., 140 means 1.4
	bool quitReceived;
	/* render parts */
	Label lblTitle;
	SmoothCounter lblTitleCounter;
	Label lblInfo; /* for mini games */
	int curWallpaperId;
	Image imgBackground;
	Image imgBricks;
	int imgBricksX, imgBricksY;
	Image imgScore;
	int imgScoreX, imgScoreY;
	Image imgExtras;
	int imgExtrasX, imgExtrasY;
	Image imgFloor; /* extra wall at bottom */
	int imgFloorX, imgFloorY;
	FrameCounter weaponFrameCounter;
	FrameCounter shotFrameCounter;
	FrameCounter shineFrameCounter;
	Timeout shineDelay;
	int shineX, shineY; /* map position to shine at, -1 = get new one */
	SmoothCounter warpIconAlphaCounter;
	SmoothCounter energyBallAlphaCounter;
	bool showWarpIcon;
	int warpIconX, warpIconY;
	list<unique_ptr<Sprite>> sprites;
	/* stats */
	Uint32 fpsCycles, fpsStart;
	double fps;

	int v2s(int i) { return i * scaleFactor / 100; }
	int s2v(int i) { return i * 100 / scaleFactor; }
	double v2s(double d) { return d * scaleFactor / 100; }
	double s2v(double d) { return d * 100 / scaleFactor; }
	void renderBackgroundImage();
	void renderHiscore(Font &fTitle, Font &fEntry, int x, int y, int w, int h, bool detailed);
	void renderBricksImage();
	void renderScoreImage();
	void renderExtrasImage();
	void renderActiveExtra(int id, int ms, int x, int y);
	void renderBalls(bool shadow = false);
	void dim();
	bool showInfo(const string &line, int type=WT_ANYKEY);
	bool showInfo(const vector<string> &text, int type=WT_ANYKEY);
	void showHelp();
	void createParticles(BrickHit *hit);
	void createSprites();
	void getBallViewInfo(Ball *ball, int *x, int *y, uint *type);
	void playSounds();
	void createMenus();
	void grabInput(int grab);
	void saveGame();
	int resumeGame();
	void showFinalHiscores();
	int waitForKey(int type);
	void darkenScreen(int alpha = 32);
	void initTitleLabel();
	void updateResumeGameTooltip();
	void renderExtraHelp(GridImage &img, uint gx, uint gy, const string &str, int x, int y);
	void runBrickDestroyDlg();
	void waitForInputRelease();
	void getNewShinePosition();
	void handleEditor(int type);
public:
	View(Config &cfg, ClientGame &_cg);
	~View();
	void init(string t, uint r);
	void run();
	void render();
	void runMenu();
	void renderMenu();
};

#endif /* VIEW_H_ */

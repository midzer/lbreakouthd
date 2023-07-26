/*
 * editor.cpp
 * (C) 2022 by Michael Speck
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "../libgame/gamedefs.h"
#include "../libgame/game.h"
#include "tools.h"
#include "sdl.h"
#include "clientgame.h"
#include "mixer.h"
#include "theme.h"
#include "editor.h"

extern SDL_Renderer *mrc;

/* extra conversion table may be found in ../libgame/bricks.c */
extern Extra_Conv extra_conv_table[EX_NUMBER];
extern Brick_Conv brick_conv_table[BRICK_COUNT];

/** Clear bricks/extras but keep title and author. */
void EditorLevel::clear() {
	for (uint j = 0; j < EDIT_HEIGHT; j++)
		for (uint i = 0; i < EDIT_WIDTH; i++) {
			bricks[i][j] = -1;
			extras[i][j] = -1;
		}

}

/** Reset level including title/author. */
void EditorLevel::reset() {
	clear();
	author = "unknown";
	title = "???";

}

/* Initialize editor for empty new set.
 * Resolution might have changed so recreate background as well. */
void Editor::init(const string &setname) {
	SDL_Texture *old;
	SDL_Color g1 = {20,20,20,0}, g2 = {30,30,30,0};
	int bw = theme.bricks.getGridWidth();
	int bh = theme.bricks.getGridHeight();

	/* set path and init one empty level */
	fpath = getCustomLevelsetDir() + "/" + setname;
	version = "1.00";
	numLevels = 0;
	addNewLevel(0);
	gotoLevel(0);
	selBrickId = 0;
	selExtraId = -1; /* bricks are on */
	editWidth = EDIT_WIDTH;
	editHeight = EDIT_HEIGHT;
	brickWidth = bw;
	brickHeight = bh;
	hasChanges = false;

	/* create selection frame image */
	selFrame.create(bw,bh);
	selFrame.fill(250,200,0,255);
	selFrame.fill(1,1,bw-2,bh-2,255,125,0,255);
	selFrame.fill(2,2,bw-4,bh-4,0,0,0,0);

	/* create simple buttons */
	buttons.create(EB_NUMBER*bw,bh);
	string captions[] = {"First", "Prev", "Next", "Last",
		"<Add", "Add>", "Clear", "Del", "MvUp", "MvDn",
		"Load", "Save", "Quit", "Test"
	};
	Font &f = theme.fSmall;
	f.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
	old = SDL_GetRenderTarget(mrc);
	SDL_SetRenderTarget(mrc,buttons.getTex());
	for (uint i = 0; i < EB_NUMBER; i++) {
		buttons.fill(i*bw+2,2,bw-4,bh-4,30,40,80);
		f.write(i*bw + bw/2,bh/2,captions[i]);
	}
	SDL_SetRenderTarget(mrc,old);

	/* create background */
	background.create();
	if (theme.numWallpapers > 1) {
		old = SDL_GetRenderTarget(mrc);
		SDL_SetRenderTarget(mrc,background.getTex());
		theme.wallpapers[0].setBlendMode(1);
		theme.wallpapers[0].setAlpha(48);
		theme.wallpapers[0].copy();
		theme.wallpapers[0].clearAlpha();
		theme.wallpapers[0].setBlendMode(0);
		SDL_SetRenderTarget(mrc,old);
	}
	background.setBlendMode(0);

	/* draw chess grid for editable part to background */
	rMap.x = bw;
	rMap.y = bh;
	rMap.w = editWidth*bw;
	rMap.h = editHeight*bh;
	for (uint j = 0; j < editHeight; j++)
		for (uint i = 0; i < editWidth; i++) {
			if (((i+j)%2)==0)
				background.fill((i+1)*bw,(j+1)*bh,bw,bh,g1);
			else
				background.fill((i+1)*bw,(j+1)*bh,bw,bh,g2);
		}

	/* draw bricks and extras on right hand side of background */
	uint dx = MAP_WIDTH*bw, dy = bh;
	old = SDL_GetRenderTarget(mrc);
	SDL_SetRenderTarget(mrc,background.getTex());

	numBrickCols = numExtraCols = 5;
	numBricks = theme.bricks.getGridSizeX();
	numExtras = EX_NUMBER;
	rBricks.x = dx;
	rBricks.y = dy;
	rBricks.w = numBrickCols * bw;
	rBricks.h = bh;
	for (uint i = 0; i < numBricks; i++) {
		if (i == INVIS_BRICK_ID) {
			theme.bricks.setAlpha(32);
			theme.bricks.copy(i-1,0,dx,dy); /* XXX use brick before */
			theme.bricks.clearAlpha();
		} else
			theme.bricks.copy(i,0,dx,dy);
		if (((i+1) % numBrickCols) == 0) {
			dy += bh;
			rBricks.h += bh;
			dx -= bw * numBrickCols;
		}
		dx += bw;
	}
	dx = MAP_WIDTH * bw;
	dy += bh;
	rExtras.x = dx;
	rExtras.y = dy;
	rExtras.w = numExtraCols * bw;
	rExtras.h = bh;
	for (uint i = 0; i < numExtras; i++) {
		theme.extras.copy(i,0,dx,dy);
		if (((i+1) % numExtraCols) == 0) {
			dy += bh;
			rExtras.h += bh;
			dx -= bw * numExtraCols;
		}
		dx += bw;
	}

	/* set rects for title, author and version */
	rTitle.x = rMap.x;
	rTitle.y = rMap.y + rMap.h;
	rTitle.w = rMap.w/2;
	rTitle.h = bh;
	rAuthor.x = rMap.x + rMap.w/2;
	rAuthor.y = rMap.y + rMap.h;
	rAuthor.w = rMap.w/2;
	rAuthor.h = bh;
	rVersion.x = rMap.x + rMap.w/2;
	rVersion.y = rMap.y - bh;
	rVersion.w = rMap.w/2;
	rVersion.h = bh;
	rButtons.x = rMap.x;
	rButtons.y = rMap.y+rMap.h+bh*2;
	rButtons.w = EB_NUMBER * bw;
	rButtons.h = bh;

	SDL_SetRenderTarget(mrc,old);
}

/** Run edit dialog and set quitRequested if quit was received. */
void Editor::runEditDlg(const string &c, string &v)
{
	string aux = v;
	int ret = runEditDialog(theme.fMenuNormal,c,aux);
	if (ret == -1)
		quitReceived = true;
	else if (ret == 1) {
		v = aux;
		hasChanges = true;
	}
}

/** Run confirm dialog and set quitRequested if quit was received.
 * Return 1 on confirm, 0 on not confirmed or quit requested. */
bool Editor::runConfirmDlg(const string &c)
{
	int ret = runConfirmDialog(theme.fNormal, c);
	return (ret == 1);
}

/* "jump" to level by updating cur level id and cur level pointer. */
void Editor::gotoLevel(uint pos) {
	if (pos >= numLevels)
		return;
	curLevelId = pos;
	curLevel = &levels[curLevelId];
}

/** Add a new level at position in vector levels moving all levels
 * including the one at the position one up. */
void Editor::addNewLevel(uint pos)
{
	EditorLevel newlev;
	/* set title to level title which screws up other standard level
	 * titles when not inserting at the end but that's how it is */
	newlev.title = _("Level ") + to_string(pos+1);

	if (pos == numLevels)
		levels.push_back(newlev);
	else
		levels.insert(levels.begin()+pos,newlev);
	numLevels++;
}

/** Convert brick id to character using conv table. */
char Editor::brickId2Char(int id) {
	for (uint k = 0; k < BRICK_COUNT; k++)
		if (brick_conv_table[k].id == id)
			return brick_conv_table[k].c;
	return '.'; /* empty tile for id -1 or invalid id */
}
char Editor::extraId2Char(int id) {
	if (id >= 0 && id < EX_NUMBER)
		return extra_conv_table[id].c;
	return '.'; /* empty tile for id -1 or invalid id */
}
int Editor::brickChar2Id(char c)
{
	for (uint k = 0; k < BRICK_COUNT; k++)
		if (brick_conv_table[k].c == c)
			return brick_conv_table[k].id;
	return -1; /* -1 for empty or invalid */
}
int Editor::extraChar2Id(char c)
{
	for (uint k = 0; k < EX_NUMBER; k++)
		if (extra_conv_table[k].c == c)
			return k;
	return -1; /* -1 for empty or invalid */
}

/** Load/save current set to file given by fpath. */
void Editor::load()
{
	string line;
	ifstream ifs(fpath);
	EditorLevel *l;

	if (!ifs.is_open()) {
		_logerr("Could not open file %s!\n",fpath.c_str());
		return;
	}
	_logdebug(1,"Loading levelset %s\n",fpath.c_str());

	levels.clear();
	numLevels = 0;
	hasChanges = false;

	getline(ifs,line);
	if (line.find("Version:") == string::npos)
		goto failure;
	version = line.substr(9);

	while (getline(ifs,line)) {
		/* new level */
		if (line != "Level:")
			goto failure;
		addNewLevel(numLevels);
		l = &levels[numLevels-1];
		getline(ifs,l->author);
		getline(ifs,l->title);
		getline(ifs,line);
		if (line != "Bricks:")
			goto failure;
		for (uint j = 0; j < EDIT_HEIGHT; j++) {
			if (!getline(ifs,line))
				goto failure;
			for (uint i = 0; i < EDIT_WIDTH; i++) {
				if (i >= line.length())
					goto failure;
				l->bricks[i][j] = brickChar2Id(line[i]);
			}
		}
		getline(ifs,line);
		if (line != "Bonus:")
			goto failure;
		for (uint j = 0; j < EDIT_HEIGHT; j++) {
			if (!getline(ifs,line))
				goto failure;
			for (uint i = 0; i < EDIT_WIDTH; i++) {
				if (i >= line.length())
					goto failure;
				l->extras[i][j] = extraChar2Id(line[i]);
			}
		}
	}
	ifs.close();

	gotoLevel(0); /* select first level */
	return;

failure:
	_logerr("  levelset corrupted at line '%s...'\n",line.substr(0,4).c_str());
	ifs.close();
	/* fallback to empty set */
	levels.clear();
	numLevels = 0;
	addNewLevel(0);
	gotoLevel(0);
}
void Editor::save()
{
	ofstream ofs(fpath);

	if (!ofs.is_open()) {
		_logerr("Could not open file %s!\n",fpath.c_str());
		return;
	}

	ofs << "Version: " << version << "\n";
	for (auto &l : levels) {
		ofs << "Level:\n";
		ofs << l.author << "\n" << l.title << "\n";
		ofs << "Bricks:\n";
		for (uint y = 0; y < EDIT_HEIGHT; y++) {
			for (uint x = 0; x < EDIT_WIDTH; x++)
				ofs << brickId2Char(l.bricks[x][y]);
			ofs << "\n";
		}
		ofs << "Bonus:\n";
		for (uint y = 0; y < EDIT_HEIGHT; y++) {
			for (uint x = 0; x < EDIT_WIDTH; x++)
				ofs << extraId2Char(l.extras[x][y]);
			ofs << "\n";
		}
	}
	ofs.close();
	_loginfo("Levelset saved to %s\n",fpath.c_str());
	hasChanges = false;
}

void Editor::swapLevels(uint pos1, uint pos2)
{
	if (pos1 >= numLevels || pos2 >= numLevels)
		return;

	EditorLevel swap;
	swap = levels[pos1];
	levels[pos1] = levels[pos2];
	levels[pos2] = swap;
}

void Editor::handleButton(uint id)
{
	EditorLevel newlev;
	string text = "";

	switch (id) {
	case EB_FIRST:
		gotoLevel(0);
		break;
	case EB_LAST:
		gotoLevel(numLevels-1);
		break;
	case EB_NEXT:
		gotoLevel(curLevelId+1);
		break;
	case EB_PREV:
		gotoLevel(curLevelId-1);
		break;
	case EB_ADDBEFORE:
	case EB_ADDAFTER:
		addNewLevel(curLevelId+((id==EB_ADDAFTER)?1:0));
		gotoLevel(curLevelId += ((id==EB_ADDAFTER)?1:0));
		break;
	case EB_CLEAR:
		if (runConfirmDlg(_("Clear level? y/n")))
			curLevel->clear();
		break;
	case EB_DELETE:
		if (numLevels > 1 && runConfirmDlg(_("Delete level? y/n"))) {
			levels.erase(levels.begin() + curLevelId);
			numLevels--;
			if (curLevelId == numLevels)
				gotoLevel(curLevelId-1);
		}
		break;
	case EB_MOVEUP:
		if (curLevelId < numLevels-1) {
			swapLevels(curLevelId, curLevelId+1);
			gotoLevel(curLevelId+1);
		}
		break;
	case EB_MOVEDOWN:
		if (curLevelId > 0) {
			swapLevels(curLevelId, curLevelId-1);
			gotoLevel(curLevelId-1);
		}
		break;
	case EB_QUIT:
		text = _("Quit editor? y/n");
		if (hasChanges)
			text += _(" (unsaved changes!)");
		if (runConfirmDlg(text))
			leaveRequested = true;
		break;
	case EB_SAVE:
		save(); /* don't confirm because for save this is annoying */
		break;
	case EB_LOAD:
		if (runConfirmDlg(_("Reload levelset from file? y/n")))
			load();
		break;
	case EB_TEST:
		testLevel = true;
		break;
	default:
		break;
	}
}

void Editor::handleClick(int mx, int my, int mb)
{
	uint rx, ry;
	if (inRect(mx, my, rBricks)) {
		rx = (mx - rBricks.x) / brickWidth;
		ry = (my - rBricks.y) / brickHeight;
		if (ry * numBrickCols + rx < numBricks) {
			selBrickId = ry * numBrickCols + rx;
			selExtraId = -1;
		}
	}
	if (inRect(mx, my, rExtras)) {
		rx = (mx - rExtras.x) / brickWidth;
		ry = (my - rExtras.y) / brickHeight;
		if (ry * numExtraCols + rx < numExtras) {
			selExtraId = ry * numExtraCols + rx;
			selBrickId = -1;
		}
	}
	if (inRect(mx, my, rMap)) {
		rx = (mx - rMap.x) / brickWidth;
		ry = (my - rMap.y) / brickHeight;
		if (mb == SDL_BUTTON_LEFT) {
			if (selBrickId != -1)
				curLevel->bricks[rx][ry] = selBrickId;
			else if (curLevel->bricks[rx][ry] != -1)
				curLevel->extras[rx][ry] = selExtraId;
		} else if (mb == SDL_BUTTON_RIGHT) {
			if (selBrickId != -1) {
				/* if brick gets deleted, delete extra as well */
				curLevel->bricks[rx][ry] = -1;
				curLevel->extras[rx][ry] = -1;
			} else
				curLevel->extras[rx][ry] = -1;
		}
		hasChanges = true;
	}
	if (inRect(mx,my,rTitle))
		runEditDlg(_("Title"),curLevel->title);
	if (inRect(mx,my,rAuthor))
		runEditDlg(_("Author"),curLevel->author);
	if (inRect(mx,my,rVersion))
		runEditDlg(_("Version"),version);
	if (inRect(mx,my,rButtons))
		handleButton((mx - rButtons.x)/brickWidth);

}

void Editor::render() {
	int alpha = 0;
	Font &font = theme.fSmall;

	background.copy();
	font.setAlign(ALIGN_X_LEFT | ALIGN_Y_BOTTOM);
	font.write(rMap.x, rMap.y, _("Editing: ") + fpath);
	font.setAlign(ALIGN_X_RIGHT | ALIGN_Y_BOTTOM);
	font.write(rVersion.x+rVersion.w, rVersion.y+rVersion.h,
				_("Version: ") + version);
	font.setAlign(ALIGN_X_LEFT | ALIGN_Y_TOP);
	font.write(rTitle.x, rTitle.y,
				_("Title: ") + levels[curLevelId].title);
	font.setAlign(ALIGN_X_RIGHT | ALIGN_Y_TOP);
	font.write(rAuthor.x+rAuthor.w, rAuthor.y,
				_("Author: ")+levels[curLevelId].author);

	if (selBrickId != -1)
		selFrame.copy(rBricks.x + (selBrickId%numBrickCols)*brickWidth,
				rBricks.y + (selBrickId/numBrickCols)*brickHeight);
	if (selExtraId != -1)
		selFrame.copy(rExtras.x + (selExtraId%numExtraCols)*brickWidth,
				rExtras.y + (selExtraId/numExtraCols)*brickHeight);

	for (uint j = 0; j < editHeight; j++)
		for (uint i = 0; i < editWidth; i++) {
			if (curLevel->bricks[i][j] != -1)
				theme.bricks.copy(curLevel->bricks[i][j],0,
						rMap.x + i*brickWidth,
						rMap.y + j*brickHeight);
			if (curLevel->bricks[i][j] == INVIS_BRICK_ID) {
				/* invisible brick is not seen so show prev brick
				 * with some transparency */
				theme.bricks.setAlpha(32);
				theme.bricks.copy(curLevel->bricks[i][j]-1,0,
						rMap.x + i*brickWidth,
						rMap.y + j*brickHeight);
				theme.bricks.clearAlpha();
			}
			if (curLevel->extras[i][j] != -1) {
				uint tick = SDL_GetTicks()%2000;
				if (tick < 1000)
					alpha = tick/4;
				else
					alpha = (2000-tick)/4;
				if (selExtraId != -1)
					alpha = 255;
				theme.extras.setAlpha(alpha);
				theme.extras.copy(curLevel->extras[i][j],0,
						rMap.x + i*brickWidth,
						rMap.y + j*brickHeight);
			}
		}

	int bx = rButtons.x;
	for (uint i = 0; i < EB_NUMBER; i++) {
		buttons.copy(i*brickWidth,0,brickWidth,brickHeight,bx,rButtons.y);
		bx += brickWidth;
	}

	font.setAlign(ALIGN_X_CENTER | ALIGN_Y_TOP);
	string levinfo = _("Current Level: ") + to_string((curLevelId+1))
					+ "/" + to_string(numLevels);
	font.write(rMap.x + rMap.w/2, rMap.y+rMap.h,levinfo);
	if (btnFocus != -1)
		font.write(rButtons.x + rButtons.w/2, rButtons.y + rButtons.h,
				btnTooltips[btnFocus]);

}

void Editor::run(const string &setname)
{
	SDL_Event ev;

	/* if testLevel is set we just tested a level so don't init/load
	 * anything */
	if (!testLevel) {
		init(setname);
		if (fileExists(fpath))
			load();
	}

	render();

	leaveRequested = false;
	testLevel = false;
	while (!quitReceived && !leaveRequested && !testLevel) {
		/* handle events */
		if (SDL_PollEvent(&ev)) {
			/* quit entirely? */
			if (ev.type == SDL_QUIT)
				quitReceived = true;
			/* check button focus for tooltip */
			if (ev.type == SDL_MOUSEMOTION &&
					inRect(ev.motion.x, ev.motion.y, rButtons))
				btnFocus = (ev.motion.x - rButtons.x)/brickWidth;
			else
				btnFocus = -1;
			/* handle click */
			if (ev.type == SDL_MOUSEBUTTONDOWN)
				handleClick(ev.button.x, ev.button.y,
							ev.button.button);
			/* handle drag */
			if (ev.type == SDL_MOUSEMOTION && ev.motion.state &&
					inRect(ev.motion.x, ev.motion.y, rMap))
				handleClick(ev.motion.x, ev.motion.y,
						(ev.motion.state&SDL_BUTTON_RMASK)?
						SDL_BUTTON_RIGHT:SDL_BUTTON_LEFT);
			/* check shortcuts */
			if (ev.type == SDL_KEYDOWN) {
				for (uint k = 0; k < EB_NUMBER; k++)
					if (btnShortcuts[k] ==
							ev.key.keysym.scancode)
						handleButton(k);			}
		}

		/* render */
		render();
		SDL_RenderPresent(mrc);
		SDL_Delay(10);
		SDL_FlushEvent(SDL_MOUSEMOTION); /* prevent event loop from dying */
	}

	/* clear events for menu loop */
	SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
}

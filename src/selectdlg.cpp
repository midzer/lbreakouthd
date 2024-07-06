/*
 * selectdlg.cpp
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

#include "sdl.h"
#include "tools.h"
#include "hiscores.h"
#include "clientgame.h"
#include "mixer.h"
#include "theme.h"
#include "selectdlg.h"

extern SDL_Renderer *mrc;
extern Brick_Conv brick_conv_table[BRICK_COUNT];

SetInfo::SetInfo(const string &n, Theme &theme)
{
	name = n;
	levels = 0;
	version = "1.00"; /* default if not found */
	author = "?";

	/* create empty preview */
	uint sw = theme.menuBackground.getWidth();
	uint sh = theme.menuBackground.getHeight();
	uint bw = theme.bricks.getGridWidth();
	uint bh = theme.bricks.getGridHeight();
	uint soff = bh/3;
	preview.create(MAPWIDTH*theme.bricks.getGridWidth(),
			MAPHEIGHT*theme.bricks.getGridHeight());
	SDL_SetRenderTarget(mrc, preview.getTex());
	Image& wallpaper = theme.wallpapers[rand()%theme.numWallpapers];
	for (uint wy = 0; wy < sh; wy += wallpaper.getHeight())
		for (uint wx = 0; wx < sw; wx += wallpaper.getWidth())
			wallpaper.copy(wx,wy);
	theme.frameShadow.copy(soff,soff);

	if (n[0] == '!') { /* special levels */
		version = "1.00";
		author = "LGames";
		levels = 1;
		/* finalize empty level */
		theme.fMenuNormal.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
		if (n == TOURNAMENT)
			theme.fMenuNormal.write(preview.getWidth()/2,preview.getHeight()/2,_("Superset with ALL levels"));
		else
			theme.fMenuNormal.write(preview.getWidth()/2,preview.getHeight()/2,_("Mini Game"));
		theme.frame.copy(0,0);
		SDL_SetRenderTarget(mrc, NULL);
		return;
	}

	/* get normal levelset info */
	string fpath = getFullLevelsetPath(n);
	string lines[5+EDIT_HEIGHT];
	ifstream ifs(fpath);
	uint offset = 0;

	if (!ifs.is_open()) {
		_logerr("Levelset %s not found, no preview created\n",n.c_str());
		theme.frame.copy(0,0);
		SDL_SetRenderTarget(mrc, NULL);
		return;
	}
	for (uint i = 0; i < 5+EDIT_HEIGHT; i++)
		readLine(ifs,lines[i]);
	if (lines[0].find("Version") != string::npos) {
		version = trimString(lines[0].substr(lines[0].find(':')+1));
		offset = 1;
	}
	author = lines[1 + offset];

	/* count levels */
	levels = 1;
	while (readLine(ifs, lines[0]))
		if (lines[0].find("Level:") != string::npos)
			levels++;
	ifs.close();

	/* add bricks of first level
	 * XXX direct access to brick conversion table from libgame */
	for (uint j = 0; j < EDITHEIGHT; j++)
		for (uint i = 0; i < EDITWIDTH; i++) {
			int k = -1;
			for ( k = 0; k < BRICK_COUNT; k++ )
				if (lines[4+offset+j][i] == brick_conv_table[k].c)
					break;
			if (k < BRICK_COUNT && k != INVIS_BRICK_ID)
				theme.bricksShadow.copy(brick_conv_table[k].id,0,
						(i+1)*bw+bh/3, (1+j)*bh+bh/3);
		}
	for (uint j = 0; j < EDITHEIGHT; j++)
		for (uint i = 0; i < EDITWIDTH; i++) {
			int k = -1;
			for ( k = 0; k < BRICK_COUNT; k++ )
				if (lines[4+offset+j][i] == brick_conv_table[k].c)
					break;
			if (k < BRICK_COUNT && k != INVIS_BRICK_ID)
				theme.bricks.copy(brick_conv_table[k].id,0,
							(i+1)*bw, (1+j)*bh);
		}
	theme.frame.copy(0,0);
	SDL_SetRenderTarget(mrc, NULL);
}


/** Create levelset list and previews + layout. */
void SelectDialog::init(int sd_type)
{
	uint sw = theme.menuBackground.getWidth();
	uint sh = theme.menuBackground.getHeight();
	vector<string> list, list2;
	string path;

	/* mini-games and installed sets */
	if (sd_type == SDT_ALL) {
		list.push_back(_(TOURNAMENT));
		list.push_back(_("!JUMPING_JACK!"));
		list.push_back(_("!OUTBREAK!"));
		list.push_back(_("!BARRIER!"));
		list.push_back(_("!SITTING_DUCKS!"));
		list.push_back(_("!HUNTER!"));
		list.push_back(_("!INVADERS!"));
		readDir(string(DATADIR)+"/levels", RD_FILES, list2);
		for (auto& s : list2)
			list.push_back(s);

		/* if game is installed, get customs from home as well */
		if (string(CONFIGDIR) != ".") {
			readDir(getCustomLevelsetDir(),RD_FILES, list2);
			for (auto& s : list2)
				list.push_back(string("~")+s);
		}
	} else { /* custom sets only */
		/* we need to add LBreakoutHD as it is ignored further down */
		list.push_back("LBreakoutHD");

		/* if not installed use regular levels */
		readDir(getCustomLevelsetDir(), RD_FILES, list2);
		for (auto& s : list2)
			list.push_back(s);
	}

	vlen = (0.7 * sh) / theme.fMenuNormal.getSize(); /* vlen = displayed entries */
	sel = SEL_NONE;
	pos = max = 0;
	if (list.size()-1 > vlen) /* we will skip LBreakoutHD so -1 */
		max = list.size()-1 - vlen;
	cw = 0.2*sw;
	ch = 1.1 * theme.fMenuNormal.getSize();
	lx = 0.1*sw;
	ly = (sh - vlen*ch)/2;
	tx = sw/2;
	ty = ly/2;
	px = 0.4*sw;
	pw = 0.5*sw;
	ph = MAPWIDTH * pw / MAPHEIGHT;
	py = (sh - ph - 3*theme.fMenuNormal.getSize())/2;

	background.createFromScreen();

	entries.clear();
	for (auto& e : list) {
		if (e == "LBreakoutHD")
			continue;
		SetInfo *si = new SetInfo(e, theme);
		entries.push_back(unique_ptr<SetInfo>(si));
	}
	/* select first entry if any */
	if (entries.size() > 0)
		sel = 0;
}

void SelectDialog::render()
{
	Font &font = theme.fMenuNormal;
	int y = ly;

	background.copy();

	theme.fMenuFocus.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
	theme.fMenuFocus.setColor(theme.menuFontColorNormal);
	theme.fMenuFocus.write(tx, ty, _("Select Levelset (press ESC to exit)"));
	theme.fMenuFocus.setColor(theme.menuFontColorFocus);

	font.setAlign(ALIGN_X_LEFT | ALIGN_Y_TOP);
	if (pos > 0) {
		if (sel == SEL_PREV)
			font.setColor(theme.menuFontColorFocus);
		else
			font.setColor(theme.menuFontColorNormal);
		font.write(lx, ly-ch, _("<Previous Page>"));
	}
	for (uint i = 0; i < vlen; i++, y += ch) {
		if (pos + i < entries.size() && sel == (int)(pos + i))
			font.setColor(theme.menuFontColorFocus);
		else
			font.setColor(theme.menuFontColorNormal);
		if (pos + i < entries.size())
			font.write(lx, y, entries[pos + i]->name);
	}
	if (pos < max) {
		if (sel == SEL_NEXT)
			font.setColor(theme.menuFontColorFocus);
		else
			font.setColor(theme.menuFontColorNormal);
		font.write(lx, y, _("<Next Page>"));
	}

	if (sel >= 0) {
		SetInfo *si = entries[sel].get();
		si->preview.copy(px,py,pw,ph);
		string str = si->name + " v" + si->version + _(" by ") + si->author;
		font.setAlign(ALIGN_X_CENTER | ALIGN_Y_TOP);
		font.setColor(theme.menuFontColorNormal);
		font.write(px + pw/2, py+ph+font.getSize(), str);
		str = "(" + to_string(si->levels) + _(" levels)");
		font.write(px + pw/2, py+ph+font.getSize()*2, str);
	}
}

/* Return 1 if selection made, 0 otherwise */
int SelectDialog::run()
{
	SDL_Event ev;
	bool leave = false;
	int ret = 0;

	render();
	while (!quitReceived && !leave) {
		/* handle events */
		if (SDL_WaitEvent(&ev)) {
			if (ev.type == SDL_QUIT)
				quitReceived = true;
			if (ev.type == SDL_KEYDOWN) {
				switch (ev.key.keysym.scancode) {
				case SDL_SCANCODE_ESCAPE:
					leave = true;
					break;
				case SDL_SCANCODE_PAGEUP:
					changeSelection(-vlen);
					break;
				case SDL_SCANCODE_PAGEDOWN:
					changeSelection(vlen);
					break;
				case SDL_SCANCODE_UP:
					changeSelection(-1);
					break;
				case SDL_SCANCODE_DOWN:
					changeSelection(1);
					break;
				default:
					break;
				}
			}
			if (ev.type == SDL_MOUSEMOTION) {
				int oldsel = sel;
				if (ev.motion.x >= lx && ev.motion.y >= ly &&
						ev.motion.x < (int)(lx + cw) &&
						ev.motion.y < int(ly + ch*vlen)) {
					sel = pos + (ev.motion.y - ly)/ch;
					if ((uint)sel >= entries.size())
						sel = entries.size()-1;
				} else if (ev.motion.y < ly)
					sel = SEL_PREV;
				else if (ev.motion.y > int(ly + ch*vlen))
					sel = SEL_NEXT;
				else
					sel = SEL_NONE;
				if (sel != oldsel)
					mixer.play(theme.sMenuMotion);
			}
			if (ev.type == SDL_MOUSEWHEEL) {
				if (ev.wheel.y < 0) {
					goNextPage();
					sel = SEL_NONE;
				} else if (ev.wheel.y > 0) {
					goPrevPage();
					sel = SEL_NONE;
				}
			}
			if (ev.type == SDL_MOUSEBUTTONDOWN) {
				if (sel == SEL_PREV)
					goPrevPage();
				else if (sel == SEL_NEXT)
					goNextPage();
				else if (sel != SEL_NONE) {
					ret = 1;
					leave = true;
				}
				if (sel != SEL_NONE)
					mixer.play(theme.sMenuClick);
			}
			if (ev.type == SDL_KEYDOWN && sel >= 0 &&
					ev.key.keysym.scancode == SDL_SCANCODE_RETURN) {
				ret = 1;
				leave = true;
				mixer.play(theme.sMenuClick);
			}
		}
		/* render */
		render();
		SDL_RenderPresent(mrc);
		SDL_Delay(10);
		SDL_FlushEvent(SDL_MOUSEMOTION); /* prevent event loop from dying */
	}

	/* clear events for menu loop */
	SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);

	return ret;
}

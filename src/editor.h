/*
 * editor.h
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

#ifndef SRC_EDITOR_H_
#define SRC_EDITOR_H_

class Editor;

class EditorLevel {
	friend Editor;
	friend View;

	string title, author;
	int bricks[EDIT_WIDTH][EDIT_HEIGHT]; /* -1 or brick id */
	int extras[EDIT_WIDTH][EDIT_HEIGHT]; /* -1 or extra id */
public:
	EditorLevel() { reset(); }
	void reset();
	void clear();
};

enum {
	EB_FIRST = 0,
	EB_PREV,
	EB_NEXT,
	EB_LAST,
	EB_ADDBEFORE,
	EB_ADDAFTER,
	EB_CLEAR,
	EB_DELETE,
	EB_MOVEUP,
	EB_MOVEDOWN,
	EB_LOAD,
	EB_SAVE,
	EB_QUIT,
	EB_TEST,
	EB_NUMBER
};

class Editor {
	Theme &theme;
	Mixer &mixer;
	bool quitReceived; /* close app entirely */
	bool leaveRequested; /* close editor and return to menu */
	bool testLevel; /* whether current level should be test played */
	bool hasChanges; /* any changes made that need to be saved? */

	Image background;
	Image selFrame;
	Image buttons;
	SDL_Rect rMap, rBricks, rExtras, rTitle, rAuthor, rVersion, rButtons;
	uint brickWidth, brickHeight;
	uint numBrickCols, numExtraCols;
	uint numBricks, numExtras;
	SDL_Scancode btnShortcuts[EB_NUMBER];
	string btnTooltips[EB_NUMBER];
	int btnFocus; /* -1 or id of button of mouse cursor on it */

	string fpath; /* edited file */
	vector<EditorLevel> levels; /* level data */
	string version; /* version of level set */
	uint numLevels; /* number of levels in set */
	uint curLevelId; /* current level id */
	EditorLevel *curLevel; /* pointer to current level in array */
	int selBrickId, selExtraId; /* selected brick/extra */
	uint editWidth, editHeight; /* size of editable part */

	void init(const string &setname);
	void render();
	bool inRect(int x, int y, const SDL_Rect &r) {
		if (x >= r.x && y >= r.y && x < r.x+r.w && y < r.y+r.h)
			return true;
		return false;
	};
	void addNewLevel(uint pos);
	void gotoLevel(uint pos);
	void handleButton(uint id);
	void handleClick(int x, int y, int b);
	void runEditDlg(const string &c, string &v);
	bool runConfirmDlg(const string &c);
	void load();
	void save();
	char brickId2Char(int id);
	char extraId2Char(int id);
	int brickChar2Id(char c);
	int extraChar2Id(char c);
	void swapLevels(uint pos1, uint pos2);
public:
	Editor(Theme &t, Mixer &m)
			: theme(t), mixer(m),
			  quitReceived(false), leaveRequested(false),
			  testLevel(false), hasChanges(false),
			  brickWidth(0), brickHeight(0),
			  numBrickCols(0), numExtraCols(0),
			  numBricks(0), numExtras(0), btnFocus(-1),
			  version("1.00"), numLevels(0), curLevelId(0), curLevel(NULL),
			  selBrickId(0), selExtraId(0),
			  editWidth(EDIT_WIDTH), editHeight(EDIT_HEIGHT) {
		/* set shortcuts and tooltips for buttons */
		btnShortcuts[EB_FIRST] = SDL_SCANCODE_UP;
		btnShortcuts[EB_PREV] = SDL_SCANCODE_LEFT;
		btnShortcuts[EB_NEXT] = SDL_SCANCODE_RIGHT;
		btnShortcuts[EB_LAST] = SDL_SCANCODE_DOWN;
		btnShortcuts[EB_ADDBEFORE] = SDL_SCANCODE_N;
		btnShortcuts[EB_ADDAFTER] = SDL_SCANCODE_M;
		btnShortcuts[EB_CLEAR] = SDL_SCANCODE_C;
		btnShortcuts[EB_DELETE] = SDL_SCANCODE_DELETE;
		btnShortcuts[EB_MOVEUP] = SDL_SCANCODE_U;
		btnShortcuts[EB_MOVEDOWN] = SDL_SCANCODE_D;
		btnShortcuts[EB_LOAD] = SDL_SCANCODE_L;
		btnShortcuts[EB_SAVE] = SDL_SCANCODE_S;
		btnShortcuts[EB_QUIT] = SDL_SCANCODE_ESCAPE;
		btnShortcuts[EB_TEST] = SDL_SCANCODE_T;
		btnTooltips[EB_FIRST] = _("Go to first level [Up Arrow]");
		btnTooltips[EB_PREV] = _("Go to previous level [Left Arrow]");
		btnTooltips[EB_NEXT] = _("Go to next level [Right Arrow]");
		btnTooltips[EB_LAST] = _("Go to last level [Down Arrow]");
		btnTooltips[EB_ADDBEFORE] = _("Add new level before this one [N]");
		btnTooltips[EB_ADDAFTER] = _("Add new level after this one [M]");
		btnTooltips[EB_CLEAR] = _("Clear bricks and extras [C]");
		btnTooltips[EB_DELETE] = _("Delete level completely [DEL]");
		btnTooltips[EB_MOVEUP] = _("Swap this level with next level [U]");
		btnTooltips[EB_MOVEDOWN] = _("Swap this level with previous level [D]");
		btnTooltips[EB_LOAD] = _("Reload levelset from file [L]");
		btnTooltips[EB_SAVE] = _("Save levelset to file [S]");
		btnTooltips[EB_QUIT] = _("Quit editor [ESC]");
		btnTooltips[EB_TEST] = _("Test current level [T] (will save all changes)");
	};
	bool quitRcvd() { return quitReceived; }
	bool testRequested() { return testLevel; }
	void run(const string &setname);
	EditorLevel *getCurrentLevel() { return curLevel; }
};

#endif /* SRC_EDITOR_H_ */

/*
 * selectdlg.h
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

#ifndef SRC_SELECTDLG_H_
#define SRC_SELECTDLG_H_

class SetInfo {
	friend SelectDialog;

	string name;
	string version;
	string author;
	uint levels;
	Image preview;
public:
	SetInfo(const string &name, Theme &theme);

};

enum {
	SDT_ALL = 0,
	SDT_CUSTOMONLY,

	SEL_NONE = -1,
	SEL_PREV = -2,
	SEL_NEXT = -3
};

class SelectDialog {
	Theme &theme;
	Mixer &mixer;
	bool quitReceived;
	vector<unique_ptr<SetInfo>> entries;
	int sel; /* actual selection */
	uint pos; /* offset for viewport */
	uint max; /* max value for pos */
	uint vlen; /* length of viewport */
	int tx, ty; /* centered title position */
	int lx, ly; /* list start */
	uint cw, ch; /* cell size */
	int px, py;
	uint pw, ph; /* preview geometry */

	Image background;

	void render();
	void renderPreview();
	void goNextPage() {
		if (pos == max)
			return;
		pos += vlen-2;
		if (pos > max)
			pos = max;
	}
	void goPrevPage() {
		if (pos == 0)
			return;
		if (pos < vlen-2)
			pos = 0;
		else
			pos -= vlen-2;
	}
	void changeSelection(int steps) {
		if (entries.size() == 0)
			return;
		if (sel < 0) {
			sel = pos;
			return;
		}
		sel += steps;
		if (steps < 0) {
			if (sel < 0)
				sel = 0;
			if (sel < (int)pos)
				pos = sel;
		}
		if (steps > 0) {
			if (sel > (int)(entries.size()-1))
				sel = entries.size()-1;
			if (sel >= (int)(pos+vlen))
				pos = sel-vlen+1;
		}
	}
public:
	SelectDialog(Theme &t, Mixer &m) : theme(t), mixer(m), quitReceived(false)
	{
		sel = SEL_NONE;
		pos = max = vlen = 0;
		tx = ty = 0;
		lx = ly = 0;
		cw = ch = 0;
		px = py = pw = ph = 0;
	}
	void init(int sd_type = SDT_ALL);
	int run();
	string get() {
		if (sel >= 0)
			return entries[sel]->name;
		return "none";
	}
	bool quitRcvd() { return quitReceived; }
};

#endif /* SRC_SELECTDLG_H_ */

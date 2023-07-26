/*
 * menu.cpp
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
#include "menu.h"

extern SDL_Renderer *mrc;

Font *MenuItem::fNormal = NULL;
Font *MenuItem::fFocus = NULL;
Font *MenuItem::fTooltip = NULL;
uint MenuItem::tooltipWidth = 300;

/** Helper to render a part of the menu item. Position is determined
 * by given alignment. */
void MenuItem::renderPart(Label &ln, Label &lf, int align)
{
	if (!MenuItem::fFocus || !MenuItem::fNormal)
		return;

	int tx = x, ty = y + h/2;
	int txoff = -MenuItem::fFocus->getSize()/5;

	if (align == ALIGN_X_RIGHT) {
		tx = x + w;
		txoff = MenuItem::fFocus->getSize()/5;
	}
	align = align | ALIGN_Y_CENTER;

	if (!focus) {
		ln.setAlpha(255 - fadingAlpha);
		ln.copy(tx, ty, align);
	}
	if (focus || fadingAlpha > 0) {
		lf.setAlpha(fadingAlpha);
		lf.copy(tx+txoff, ty, align);
	}
}

/** Run a dialog for editing a UTF8 string. ESC cancels editing (string
 * is not changed), Enter confirms changes. Return 1 if string was changed,
 * 0 if not changed. */
int MenuItemEdit::runEditDialog(const string &caption, string &str)
{
	int ret = 0;
	Font *f = MenuItem::fNormal;
	SDL_Event event;
	string backup = str;
	Image img;
	img.createFromScreen();
	img.setAlpha(64);
	bool done = false;

	/* should never fail but be safe */
	if (f == NULL) {
		_logerr("No font for edit dialog???\n");
		return ret;
	}

	f->setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);

	SDL_StartTextInput();
	while (!done) {
		if (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				done = true; // XXX should cancel everything...
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.scancode) {
				case SDL_SCANCODE_ESCAPE:
					str = backup;
					done = true;
					break;
				case SDL_SCANCODE_RETURN:
					ret = 1;
					done = true;
					break;
				case SDL_SCANCODE_BACKSPACE:
					if (str.length()>0)
						str = str.substr(0, str.length()-1);
					break;
				default: break;
				}
				break;
			case SDL_TEXTINPUT:
				str += event.text.text;
				break;
			}
		}

		/* redraw */
		SDL_SetRenderDrawColor(mrc,0,0,0,255);
		SDL_RenderClear(mrc);
		img.copy();
		string text(caption + ": " + str);
		f->write(img.getWidth()/2,img.getHeight()/2,text);
		SDL_RenderPresent(mrc);
		SDL_Delay(10);
		SDL_FlushEvent(SDL_MOUSEMOTION);
	}
	SDL_StopTextInput();

	return ret;
}

int Menu::handleEvent(const SDL_Event &ev)
{
	int ret = AID_NONE;

	if (ev.type == SDL_MOUSEMOTION) {
		bool onItem = false;
		uint id = 0;
		for (auto& i : items) {
			if (i->hasPointer(ev.motion.x,ev.motion.y)) {
				if (i.get() == curItem) {
					onItem = true;
					break;
				}
				if (curItem)
					curItem->setFocus(0);
				if (keySelectId != -1)
					items[keySelectId]->setFocus(0);
				curItem = i.get();
				keySelectId = id;
				curItem->setFocus(1);
				onItem = true;
				ret = AID_FOCUSCHANGED;
				break;
			}
			id++;
		}
		if (!onItem && curItem) {
			curItem->setFocus(0);
			curItem = NULL;
			keySelectId = -1;
			ret = AID_FOCUSCHANGED;
		}
		return ret;
	}

	if (ev.type == SDL_MOUSEBUTTONDOWN) {
		for (auto& i : items)
			if (i->hasPointer(ev.button.x,ev.button.y))
				return i->handleEvent(ev);
	}

	if (ev.type == SDL_KEYDOWN) {
		int oldselid = keySelectId;
		if (keySelectId == -1)
			keySelectId = 0;
		else if (ev.key.keysym.scancode == SDL_SCANCODE_UP ||
				ev.key.keysym.scancode == SDL_SCANCODE_DOWN) {
			if (ev.key.keysym.scancode == SDL_SCANCODE_UP) {
				keySelectId--;
				if (keySelectId < 0)
					keySelectId = items.size()-1;
				/* check for separators which only come single
				 * and are never at the beginning/end of the
				 * menu so this should work without further
				 * checks */
				else if (dynamic_cast<MenuItemSep*>(items[keySelectId].get()))
					keySelectId--;
			} else if (ev.key.keysym.scancode == SDL_SCANCODE_DOWN) {
				keySelectId++;
				if (keySelectId >= (int)items.size())
					keySelectId = 0;
				else if (dynamic_cast<MenuItemSep*>(items[keySelectId].get()))
					keySelectId++;
			}
		} else if (ev.key.keysym.scancode == SDL_SCANCODE_RETURN ||
				ev.key.keysym.scancode == SDL_SCANCODE_LEFT ||
				ev.key.keysym.scancode == SDL_SCANCODE_RIGHT)
			return items[keySelectId]->handleEvent(ev);
		if (oldselid != keySelectId) {
			if (oldselid != -1)
				items[oldselid]->setFocus(0);
			ret = AID_FOCUSCHANGED;
			items[keySelectId]->setFocus(1);
			if (curItem) {
				curItem->setFocus(0);
				curItem = NULL;
			}
		}
	}

	return ret;
}

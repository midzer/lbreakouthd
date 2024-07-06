/*
 * menu.h
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

#ifndef SRC_MENU_H_
#define SRC_MENU_H_

enum {
	/* menu action ids */
	AID_NONE = 0,
	AID_FOCUSCHANGED, /* XXX used to play sound */
	AID_CLICKED, /* dummy click to trigger sound */
	AID_ENTERMENU,
	AID_LEAVEMENU,
	AID_RESUME,
	AID_HELP,
	AID_QUIT,
	AID_STARTORIGINAL,
	AID_STARTCUSTOM,
	AID_CHANGEKEY,
	AID_ADJUSTKEYSPEED,
	AID_SOUND,
	AID_VOLUME,
	AID_APPLYAUDIO,
	AID_APPLYTHEMEMODE,
	AID_MAXBALLSPEEDCHANGED,
	AID_EDITNEWSET,
	AID_EDITCUSTOM
};

class Menu;

/** Basic menu item with just a caption, returns action id on click */
class MenuItem {
protected:
	string caption;
	Label tooltip;
	Label lblNormal;
	Label lblFocus;
	Timeout tooltipTimeout;
	int x, y, w, h;
	int focus;
	int actionId;
	double fadingAlpha;

	void renderPart(Label &ln, Label &lf, int align);
	void renderTooltip() {
		if (focus && !tooltipTimeout.running())
			tooltip.copy(x+1.1*w, y+h/2-tooltip.getHeight()/2,
						ALIGN_X_LEFT | ALIGN_Y_TOP);
	}
public:
	static Font *fNormal, *fFocus, *fTooltip;
	static uint tooltipWidth;

	MenuItem(const string &c, const string &tt, int aid = AID_NONE) :
			lblNormal(true), lblFocus(true),
			x(0), y(0), w(1), h(1),
			focus(0), actionId(aid), fadingAlpha(0) {
		setCaption(c);
		setTooltip(tt);
	}
	virtual ~MenuItem() {}
	void setGeometry(int _x, int _y, int _w, int _h) {
		x = _x;
		y = _y;
		w = _w;
		h = _h;
		_logdebug(2,"Geometry for %s: %d,%d,%d,%d\n",caption.c_str(),x,y,w,h);
	}
	void setCaption(const string &c) {
		caption = c;
		if (MenuItem::fNormal)
			lblNormal.setText(*(MenuItem::fNormal), c);
		if (MenuItem::fFocus)
			lblFocus.setText(*(MenuItem::fFocus), c);
	}
	void setTooltip(const string &tt) {
		if (MenuItem::fTooltip)
			tooltip.setText(*(MenuItem::fTooltip),tt,
					MenuItem::tooltipWidth);
	}
	bool hasPointer(int px, int py) {
		return (px >= x && px < x + w && py >= y && py < y + h);
	}
	void setFocus(int on) {
		focus = on;
		if (focus) {
			fadingAlpha = 255;
			tooltipTimeout.set(500);
		}

	}
	virtual void update(uint ms) {
		if (!focus && fadingAlpha > 0) {
			fadingAlpha -= 0.7*ms;
			if (fadingAlpha < 0)
				fadingAlpha = 0;
		}
		if (focus && tooltipTimeout.running())
			tooltipTimeout.update(ms);
	}
	virtual void render() {
		renderPart(lblNormal, lblFocus, ALIGN_X_LEFT);
		renderTooltip();
	}
	virtual int handleEvent(const SDL_Event &ev) {
		if (ev.type == SDL_MOUSEBUTTONDOWN || ev.type == SDL_KEYDOWN) {
			if (actionId == AID_NONE)
				return AID_CLICKED;
			return actionId;
		}
		return AID_NONE;
	}
};

/** Allow a string value in addition to caption. */
class MenuItemValue : public MenuItem {
protected:
	string value;
	Label lblValueNormal;
	Label lblValueFocus;
public:
	MenuItemValue(const string &c, const string &v,
				const string &tt, int aid = AID_NONE) :
				MenuItem(c + ":",tt,aid), value(""),
				lblValueNormal(true), lblValueFocus(true) {
		setValue(v);
	}
	void setValue(const string &v) {
		value = v;
		if (MenuItem::fNormal && MenuItem::fFocus) {
			lblValueNormal.setText(*(MenuItem::fNormal), value);
			lblValueFocus.setText(*(MenuItem::fFocus), value);
		}
	}
	virtual void render() {
		renderPart(lblValueNormal, lblValueFocus, ALIGN_X_RIGHT);
		MenuItem::render();
	}
};

class MenuItemSep : public MenuItem {
public:
	MenuItemSep() : MenuItem("","") {}
};

/* Sub items manage destruction of submenus ... */
class MenuItemSub : public MenuItem {
	unique_ptr<Menu> submenu;
public:
	MenuItemSub(const string &c, Menu *sub)
		: MenuItem(c,"",AID_ENTERMENU), submenu(sub) {}
	Menu *getSubMenu() { return submenu.get(); }
};

/* ... Back items only have pointers */
class MenuItemBack : public MenuItem {
	Menu *prevMenu;
public:
	MenuItemBack(Menu *last)
		: MenuItem(_("Back"),"",AID_LEAVEMENU), prevMenu(last) {}
	Menu *getLastMenu() { return prevMenu; }
};

/* Range items allow modifying a referenced value. */
class MenuItemRange : public MenuItemValue {
protected:
	int min, max, step;
	int &val;
public:
	MenuItemRange(const string &c, const string &tt, int aid, int &_val,
				int _min, int _max, int _step)
		: MenuItemValue(c,to_string(_val),tt,aid),
		  min(_min), max(_max), step(_step), val(_val) {}
	virtual int handleEvent(const SDL_Event &ev) {
		int oldval = val;
		int mod = 0;
		if (ev.type == SDL_MOUSEBUTTONDOWN) {
			if (ev.button.button == SDL_BUTTON_LEFT)
				mod = 1;
			else if (ev.button.button == SDL_BUTTON_RIGHT)
				mod = -1;
		}
		if (ev.type == SDL_KEYDOWN) {
			if (ev.key.keysym.scancode == SDL_SCANCODE_RIGHT)
				mod = 1;
			else if (ev.key.keysym.scancode == SDL_SCANCODE_LEFT)
				mod = -1;
		}
		val += step * mod;
		if (val > max)
			val = min;
		else if (val < min)
			val = max;
		if (val != oldval)
			setValue(to_string(val));
		if (mod) {
			if (actionId == AID_NONE)
				return AID_CLICKED;
			return actionId;
		}
		return AID_NONE;
	}
};

class MenuItemList : public MenuItemRange {
protected:
	vector<string> options;
public:
	MenuItemList(const string &c, const string &tt, int aid,
					int &v, vector<string> &opts)
			: MenuItemRange(c,tt,aid,v,0,opts.size()-1,1) {
		options = opts;
		setValue(options[v]);
	}
	MenuItemList(const string &c, const string &tt, int aid,
					int &v, const char **opts, uint optNum)
			: MenuItemRange(c,tt,aid,v,0,optNum-1,1) {
		for (uint i = 0; i < optNum; i++)
			options.push_back(opts[i]);
		setValue(options[v]);
	}
	MenuItemList(const string &c, const string &tt, int aid,
					int &v, const string &opt1, const string &opt2)
			: MenuItemRange(c,tt,aid,v,0,1,1) {
		options.push_back(opt1);
		options.push_back(opt2);
		setValue(options[v]);
	}
	void setOptions(vector<string> &opts, int _cur) {
		options = opts;
		min = 0;
		max = opts.size() - 1;
		val = _cur;
		setValue(options[val]);
	}
	virtual int handleEvent(const SDL_Event &ev) {
		int oldval = val;
		int ret = MenuItemRange::handleEvent(ev);
		if (val != oldval)
			setValue(options[val]);
		return ret;
	}
};

/* Value contains real integer value not the index. */
class MenuItemIntList : public MenuItemRange {
	int idx; /* value of menu item range */
	int &val;
	vector<int> options;
public:
	MenuItemIntList(const string &c, const string &tt,
					int &v, const int *opts, uint optNum)
				: MenuItemRange(c,tt,AID_NONE,idx,0,optNum-1,1), val(v) {
		idx = 0;
		for (uint i = 0; i < optNum; i++) {
			if (opts[i] == val)
				idx = i;
			options.push_back(opts[i]);
		}
		val = options[idx]; /* if not found, fallback to first value */
		setValue(to_string(options[idx]));
	}
	virtual int handleEvent(const SDL_Event &ev) {
		int oldval = val;
		int ret = MenuItemRange::handleEvent(ev);
		val = options[idx];
		if (val != oldval)
			setValue(to_string(options[idx]));
		return ret;
	}
};

class MenuItemSwitch : public MenuItemList {
public:
	MenuItemSwitch(const string &c, const string &tt, int aid, int &v)
			: MenuItemList(c,tt,aid,v,_("Off"),_("On")) {}
};

class MenuItemKey : public MenuItemValue {
	int &sc;
	bool waitForNewKey;
public:
	MenuItemKey(const string &c, const string &tt, int &_sc)
			: MenuItemValue(c,"",tt,AID_CHANGEKEY),
			  sc(_sc), waitForNewKey(false) {
		setValue(SDL_GetScancodeName((SDL_Scancode)sc));
	}
	virtual int handleEvent(const SDL_Event &ev) {
		if (ev.type == SDL_MOUSEBUTTONDOWN ||
				(ev.type == SDL_KEYDOWN &&
				ev.key.keysym.scancode == SDL_SCANCODE_RETURN)) {
			waitForNewKey = true;
			setValue("???");
		}
		return actionId;
	}
	void setKey(int nsc) {
		sc = nsc;
		waitForNewKey = false;
		setValue(SDL_GetScancodeName((SDL_Scancode)sc));
	}
	void cancelChange() {
		setValue(SDL_GetScancodeName((SDL_Scancode)sc));
		waitForNewKey = false;
	}
};

class MenuItemEdit : public MenuItemValue {
	string &str;
	bool hidval;

	int runEditDialog(const string &caption, string &str);
public:
	MenuItemEdit(const string &c, string &s, int aid = AID_NONE,
				bool hv = false)
				: MenuItemValue(c,s,"",aid), str(s), hidval(hv) {
		if (hidval) /* XXX value is hidden, remove : from caption */
			setCaption(c);
	}
	virtual int handleEvent(const SDL_Event &ev) {
		if (ev.type == SDL_MOUSEBUTTONDOWN ||
				(ev.type == SDL_KEYDOWN &&
				ev.key.keysym.scancode == SDL_SCANCODE_RETURN))
			if (runEditDialog(_("Enter Name"),str)) {
				setValue(str);
				return actionId;
			}
		return AID_NONE;
	}
	virtual void render() {
		if (!hidval)
			renderPart(lblValueNormal, lblValueFocus, ALIGN_X_RIGHT);
		MenuItem::render();
	}
};

class Menu {
	Theme &theme;
	vector<unique_ptr<MenuItem>> items;
	MenuItem *curItem;
	int keySelectId; /* if not -1, id of menu entry selected by keys
			   * equals selection by mouse if any made */
public:
	Menu(Theme &t) : theme(t), curItem(NULL), keySelectId(-1) {}
	~Menu() {}
	void add(MenuItem *item) {
		items.push_back(unique_ptr<MenuItem>(item));
	}
	MenuItem *getCurItem() {
		if (keySelectId != -1)
			return items[keySelectId].get();
		return curItem;
	}
	void adjust() {
		int h = items.size() * theme.menuItemHeight;
		int w = theme.menuItemWidth;
		int x = theme.menuX - w/2;
		int y = theme.menuY - h/2;
		for (uint i = 0; i < items.size(); i++) {
			MenuItemSub *sub = dynamic_cast<MenuItemSub*>(items[i].get());
			items[i]->setGeometry(x, y + i*theme.menuItemHeight,
						w, theme.menuItemHeight);
			if (sub)
				sub->getSubMenu()->adjust();
		}
	}
	void update(uint ms) {
		for (auto& i : items)
			i->update(ms);
	}
	void render() {
		for (auto& i : items)
			i->render();
	}
	int handleEvent(const SDL_Event &ev);
	Menu *getLastMenu() {
		MenuItemBack *bi = NULL;
		for (auto& item : items) {
			bi = dynamic_cast<MenuItemBack*>(item.get());
			if (bi)
				return bi->getLastMenu();
		}
		return NULL;
	}
	void resetSelection() {
		if (curItem) {
			curItem->setFocus(0);
			curItem = NULL;
		}
		if (keySelectId != -1)
			items[keySelectId]->setFocus(0);
		/* start with first entry key selected */
		keySelectId = 0;
		items[keySelectId]->setFocus(1);
	}
};

#endif /* SRC_MENU_H_ */

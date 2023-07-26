/*
 * hiscores.h
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

#ifndef SRC_HISCORES_H_
#define SRC_HISCORES_H_

typedef struct {
	string name;
	int level;
	int score;
	bool newEntry;
} ChartEntry;

enum {
	CHARTSIZE = 10,
	SORT_BY_SCORE = 0,
	SORT_BY_LEVEL
};

class HiscoreChart {
	string name; /* of levelset */
	ChartEntry entries[CHARTSIZE+1]; /* last entry for new item */

	void swapEntries(ChartEntry *entry1, ChartEntry *entry2);
	void sort(int type);
public:
	HiscoreChart(const string& name);

	void set(int id, const string& n, int l, int s);
	void add(const string& n, int l, int s);
	const ChartEntry *get(int id);
	const string& getName() { return name; }
};

class Hiscores {
	string fname;
	list<unique_ptr<HiscoreChart>> charts;

	HiscoreChart* addNewChart(const string& cname);
public:
	Hiscores();
	~Hiscores() { save(); }
	void save();
	HiscoreChart* get(const string& cname);
};

#endif /* SRC_HISCORES_H_ */

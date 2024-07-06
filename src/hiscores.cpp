/*
 * hiscores.cpp
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
#include "hiscores.h"

/** Create empty chart */
HiscoreChart::HiscoreChart(const string& _name)
{
	name = _name;
	for (int i = 0; i < CHARTSIZE+1; i++) {
		entries[i].name = "______";
		entries[i].level = 9 - i;
		entries[i].score = 100000 - i*10000;
		entries[i].newEntry = false;
	}
}

void HiscoreChart::set(int id, const string& n, int l, int s)
{
	if (id >= 0 && id < CHARTSIZE) {
		entries[id].name = n;
		entries[id].level = l;
		entries[id].score = s;
		entries[id].newEntry = false;
	}
}

/* add to 11th slot and stable sort by level then by name */
void HiscoreChart::add(const string& n, int l, int s)
{
	for (int i = 0; i < CHARTSIZE; i++)
		entries[i].newEntry = false;
	entries[CHARTSIZE].name = n;
	entries[CHARTSIZE].level = l;
	entries[CHARTSIZE].score = s;
	entries[CHARTSIZE].newEntry = true;

	/* sort */
	sort(SORT_BY_LEVEL);
	sort(SORT_BY_SCORE);
}

const ChartEntry* HiscoreChart::get(int id)
{
	if (id >= 0 && id < CHARTSIZE)
		return &entries[id];
	_logerr("Entry id %d outside limit\n",id);
	return 0;
}

void HiscoreChart::swapEntries( ChartEntry *entry1, ChartEntry *entry2 )
{
    ChartEntry dummy;
    dummy = *entry1;
    *entry1 = *entry2;
    *entry2 = dummy;
}
void HiscoreChart::sort( int type )
{
	int j;
	int changed = 0;
	/* use entry dummy as well so count is CHARTSIZE + 1 */
	do {
		changed = 0;
		for ( j = 0; j < CHARTSIZE; j++ )
			switch ( type ) {
			case SORT_BY_LEVEL:
				if ( entries[j].level < entries[j + 1].level ) {
					swapEntries( &entries[j], &entries[j + 1] );
					changed = 1;
				}
				break;
			case SORT_BY_SCORE:
				if ( entries[j].score < entries[j + 1].score ) {
					swapEntries( &entries[j], &entries[j + 1] );
					changed = 1;
				}
				break;
			}
	} while ( changed );
}


Hiscores::Hiscores()
{
	int chartnum = 0;
	string str;
	int l, s;
	string prefix;

	/* set path to highscores file */
	fname = string(HISCOREDIR) + "/lbreakouthd.hscr";
	if (string(HISCOREDIR) != "." && !fileIsWriteable(fname)) {
		/* if not found or no write access we can't use global
		 * highscores file so fallback to home directory */
		_loginfo("No permission to access global highscores\n");
		fname = getHomeDir() + "/" + CONFIGDIR + "/lbreakouthd.hscr";
		_loginfo("Falling back to %s\n",fname.c_str());
	}

	if (!fileExists(fname)) {
		_loginfo("No hiscores file %s yet\n",fname.c_str());
		return; /* no hiscores yet */
	}

	_loginfo("Loading hiscores %s\n",fname.c_str());

	FileParser fp(fname);
	fp.get("chartnum", chartnum);
	for (int i = 0; i < chartnum; i++) {
		prefix = string("chart") + to_string(i);
		fp.get(prefix + ".name",str);
		HiscoreChart *chart = new HiscoreChart(str);
		for (int j = 0; j < CHARTSIZE; j++) {
			str = "______";
			l = 0;
			s = 0;
			fp.get(prefix + ".entry" + to_string(j) + ".name",str);
			fp.get(prefix + ".entry" + to_string(j) + ".level",l);
			fp.get(prefix + ".entry" + to_string(j) + ".score",s);
			chart->set(j,str,l,s);
		}
		charts.push_back(unique_ptr<HiscoreChart>(chart));
	}
}

void Hiscores::save()
{
	ofstream ofs(fname);
	if (!ofs.is_open()) {
		_logerr("Could not open hiscores file %s\n",fname.c_str());
		return;
	}

	int cid = 0;
	for (auto& chart : charts) {
		ofs << "chartnum=" << charts.size() << "\n";
		ofs << "chart" << cid << "{\n";
		ofs << "	name=" << chart->getName() << "\n";
		for (int i = 0; i < CHARTSIZE; i++) {
			const ChartEntry *ce = chart->get(i);
			ofs << "	entry" << i << "{\n";
			ofs << "		name=" << ce->name << "\n";
			ofs << "		level=" << ce->level << "\n";
			ofs << "		score=" << ce->score << "\n";
			ofs << "	}\n";
		}
		ofs << "}\n";
		cid++;
	}

	_loginfo("Hiscores saved to %s\n",fname.c_str());
	ofs.close();
}

/** Try to get hiscore chart, if not found add new one. */
HiscoreChart* Hiscores::get(const string& cname)
{
	for (auto& chart : charts) {
		if (chart->getName() == cname)
			return chart.get();
	}
	_loginfo("Hiscore chart '%s' not found, creating empty chart\n",cname.c_str());
	return addNewChart(cname);
}

/** Add new empty chart to hiscores list and also return pointer to object */
HiscoreChart* Hiscores::addNewChart(const string& cname)
{
	HiscoreChart *c = new HiscoreChart(cname);
	charts.push_back(unique_ptr<HiscoreChart>(c));
	return c;
}

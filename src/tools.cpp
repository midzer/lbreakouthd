/*
 * tools.cpp
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

#ifdef WIN32
#define _GNU_SOURCE
#endif

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "tools.h"

string trimString(const string& str)
{
	size_t start = 0;
	while (str[start] <= 32 && start < str.length())
		start++;
	size_t len = 0;
	while (str[start + len] > 32 && start + len < str.length())
		len++;
	return str.substr(start,len);
}

/** Parse text file in format:
 * entry=...
 * set {
 * 	entry=...
 * 	...
 * }
 * ...
 */
FileParser::FileParser(const string&  fname)
{
	string line;
	ifstream ifs(fname);
	size_t pos;

	if (!ifs.is_open()) {
		_logerr("Could not open %s\n",fname.c_str());
		return;
	}
	_logdebug(1,"Parsing file %s\n",fname.c_str());

	prefix="";
	while (readLine(ifs,line)) {
		if ((pos = line.find('=')) != string::npos) {
			/* plain entry, add to list */
			string key = prefix + trimString(line.substr(0, pos));
			string val = trimString(line.substr(pos+1));
			ParserEntry pe = {key,val};
			entries.push_back(pe);
			_logdebug(2,"  %s=%s\n",key.c_str(),val.c_str());
		} else if ((pos = line.find('{')) != string::npos) {
			/* new subset, adjust prefix */
			prefix += trimString(line.substr(0,pos)) + ".";
		} else if ((pos = line.find('}')) != string::npos) {
			/* subset done, reduce prefix */
			prefix = prefix.substr(0,prefix.length()-1);
			if ((pos = prefix.rfind('.')) == string::npos)
				prefix = "";
			else
				prefix = prefix.substr(0,pos) + ".";
		}
	}
	ifs.close();
}

/** Get value as string, double or int. Return 1 on success, 0 otherwise.
 * Leave value unchanged if not found. */
int FileParser::get(const string& k, string &v)
{
	for (auto& e : entries)
		if (e.key == k) {
			v = e.value;
			return 1;
		}
	_logdebug(2,"Entry %s not found\n",k.c_str());
	return 0;
}
int FileParser::get(const string& k, int &v)
{
	string str;
	int ret = get(k,str);
	if (ret)
		v = stoi(str);
	return ret;
}
int FileParser::get(const string& k, uint &v)
{
	string str;
	int ret = get(k,str);
	if (ret) {
		int i = stoi(str);
		if (i < 0)
			_logerr("%s value %d is not uint\n",k.c_str(),i);
		v = i;
	}
	return ret;
}
int FileParser::get(const string& k, uint8_t &v)
{
	string str;
	int ret = get(k,str);
	if (ret) {
		int i = stoi(str);
		if (i < 0 || i > 255)
			_logerr("%s value %d is not uint8_t!\n",k.c_str(),i);
		v = i;
	}
	return ret;
}
int FileParser::get(const string& k, double &v)
{
	string str;
	int ret = get(k,str);
	if (ret)
		v = stod(str);
	return ret;
}

bool dirExists(const string& name) {
	struct stat info;
	return (stat(name.c_str(), &info) == 0);
}

bool makeDir(const string& name) {
#ifdef WIN32
	return mkdir(name.c_str()) == 0;
#else
	return mkdir(name.c_str(), S_IRWXU) == 0;
#endif
}

bool fileExists(const string& name) {
	if (FILE *file = fopen(name.c_str(), "r")) {
		fclose(file);
		return true;
	}
	return false;
}
bool fileIsWriteable(const string& name) {
	if (FILE *file = fopen(name.c_str(), "r+")) {
		fclose(file);
		return true;
	}
	return false;
}

/** Not the nicest but hands down most efficient way to do it. */
void strprintf(string& str, const char *fmt, ... )
{
	va_list args;
	char *buf = 0;

	va_start(args,fmt);
	if (vasprintf(&buf,fmt,args) >= 0)
		str = buf;
	else
		str = "";
	va_end(args);
	if (buf)
		free(buf);
}

/** Read all file names from directory exluding .* and Makefile.*
 * Return number of read items, -1 on error.
 */
int readDir(const string &dname, int type, vector<string> &fnames)
{
	DIR *dir;
	struct dirent *ent;
	struct stat sbuf;

	if ((dir = opendir (dname.c_str())) == NULL) {
		_logerr("Could not open %s\n",dname.c_str());
		return -1;
	}

	fnames.clear();
	while ((ent = readdir(dir)) != NULL) {
		string fname = dname + "/" + string(ent->d_name);
		stat(fname.c_str(),&sbuf);
		if (type == RD_FOLDERS) {
			if (!S_ISDIR(sbuf.st_mode))
				continue;
		} else if (S_ISDIR(sbuf.st_mode))
			continue;
		if (strncmp(ent->d_name,"Makefile",8) == 0)
			continue;
		if (ent->d_name[0] == '.')
			continue;
		fnames.push_back(ent->d_name);
	}
	closedir (dir);
	sort(fnames.begin(),fnames.end());
	return fnames.size();
}

string getHomeDir() {
	return string(getenv("HOME")?getenv("HOME"):".");
}

/* not thread safe */
const string &getCustomLevelsetDir() {
	static string path;
	if (string(CONFIGDIR) == ".")
		path = "./levels";
	else
		path = getHomeDir() + "/" + CONFIGDIR + "/levels/";
	return path;
}

/* not thread safe */
const string &getFullLevelsetPath(const string &n)
{
	static string path;
	if (n[0] != '~') {
		path = string(DATADIR) + "/levels/" + n;
	} else {
		if (string(CONFIGDIR) == ".")
			path = "./levels/";
		else
			path = getHomeDir() + "/" + CONFIGDIR + "/levels/";
		path += n.substr(1);
	}
	return path;
}

/** Read line from input stream and remove any return carriage char (13)
 * at the end (windows files). */
istream& readLine(ifstream &ifs, string &str)
{
	istream &ret = getline(ifs, str);
	if (str.length() > 0 && str[str.length()-1] == 13)
		str = str.substr(0,str.length()-1);
	return ret;
}

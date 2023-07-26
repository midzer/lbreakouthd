/*
 * clientgame.h
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

#ifndef SRC_CLIENTGAME_H_
#define SRC_CLIENTGAME_H_

#include "../libgame/gamedefs.h"
#include "../libgame/game.h"
#include "hiscores.h"

class ClientPlayer {
	string name;
	uint lives;
	uint maxLives;
	int score;
	uint level;
	Level snapshot;
public:
	ClientPlayer(const string &n, uint l, uint ml) :
			name(n), lives(l), maxLives(ml), score(0), level(0) {
		_loginfo("Added player %s\n",n.c_str());
	};
	const string& getName() { return name; }
	uint getLives() { return lives; }
	uint getMaxLives() { return maxLives; }
	void gainLife() {
		if (lives < maxLives)
			lives++;
	}
	int looseLife() {
		if (lives > 0)
			lives--;
		return lives;
	}
	int getScore() { return score; }
	int addScore(int s) {
		score += s;
		return score;
	}
	void setLives(uint l) { lives = l; }
	void setScore(int s) { score = s; }
	uint getLevel() { return level; }
	uint nextLevel() { return ++level; }
	void setLevel(uint l) { level = l; }
	void setLevelSnapshot(const Level *l) {
		if (l == NULL)
			game_get_level_snapshot(&snapshot);
		else
			snapshot = *l;
	}
	Level *getLevelSnapshot() { return &snapshot; }
};

class PaddleInputState {
public:
	int left = 0;
	int right = 0;
	int turbo = 0;
	int leftFire = 0;
	int rightFire = 0;
	int speedUp = 0;
	int recall = 0;
	int warp = 0;

	void reset() {
		left = right = turbo = leftFire = rightFire =
					speedUp = recall = warp = 0;
	}
};

enum {
	CGF_NONE = 0,
	CGF_UPDATEBACKGROUND = 1,
	CGF_UPDATEBRICKS = 2,
	CGF_UPDATESCORE = 4,
	CGF_UPDATEEXTRAS = 8,
	CGF_NEWLEVEL = 16,
	CGF_GAMEOVER = 32,
	CGF_PLAYERMESSAGE = 64,
	CGF_NEWANIMATIONS = 128,
	CGF_LIFELOST = 256,
	CGF_LASTLIFELOST = 512,
	CGF_WARPOK = 1024,
	CGF_UPDATEINFO = 2048,
	CGF_RESTARTLEVEL = 4096
};
class ClientGame {
	Config &config;
	LevelSet *levelset;
	Game *game; /* current game context */
	Hiscores hiscores;
	vector<unique_ptr<ClientPlayer>> players;
	uint curPlayer;
	ClientPlayer *lastDeadPlayer;
	string msg;
	Timeout frictionTimeout;
	bool extrasActive;
	Timeout extrasUpdateTimeout;
	double lastpx;
	/* paddle acceleration settings for key/gamepad input */
	double pvel, pacc, pvelmax, pvelmin;
	int pveldir; /* -1 left, 0 none, 1 right */

	ClientPlayer *getNextPlayer();
	void initLevel(Level *l);
	int loadAllLevels();
public:
	ClientGame(Config &cfg);
	~ClientGame();
	int init(const string& setname, int levelid = 0);
	int initTestlevel(const string &title, const string &author,
			int bricks[][EDIT_HEIGHT], int extras[][EDIT_HEIGHT]);
	int update(uint ms, double rx, PaddleInputState &pis);
	Game *getGameContext() { return game; }
	string getLevelsetName() { return levelset->name; }
	void getCurrentLevelNameAndAuthor(string &name, string &author) {
		ClientPlayer *p = players[curPlayer].get();
		uint lid = p->getLevel();
		if (lid >= (uint)levelset->count) {
			name = "none";
			author = "none"; /* is done, should not happen */
		} else {
			name = levelset->levels[lid]->name;
			author = levelset->levels[lid]->author;
		}
	}
	int getLevelCount() { return levelset->count; }
	HiscoreChart *getHiscoreChart() { return hiscores.get(levelset->name); }
	ClientPlayer* getCurrentPlayer() {return players[curPlayer].get(); }
	uint getCurrentPlayerId() {return curPlayer; }
	void setCurrentPlayerId(uint id) {
		curPlayer = id;
		/* adjust paddle score */
		game->paddles[0]->score = players[curPlayer]->getScore();
	}
	int darknessActive() { return game->extra_active[EX_DARKNESS]; }
	int floorActive() { return game->paddles[0]->extra_active[EX_WALL]; }
	int getFloorAlpha() { return game->paddles[0]->wall_alpha; }
	void updateHiscores();
	const string& getPlayerMessage() { return msg; }
	vector<unique_ptr<ClientPlayer>>& getPlayers() { return players; }
	void resumePlayer(uint pid, uint lives, int score, uint level) {
		players[pid]->setLives(lives);
		players[pid]->setScore(score);
		players[pid]->setLevel(level);
		players[pid]->setLevelSnapshot(levelset->levels[level]);
	}
	int getFloorTime() {
		if (!game || !game->paddles[0]->extra_active[EX_WALL])
			return -1;
		return game->paddles[0]->extra_time[EX_WALL];
	}
	void continueGame();
	int destroyBrick(int x, int y);
	int isBonusLevel() {
		return game->isBonusLevel;
	}
	int isBrickAtPosition(int x, int y) {
		if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) {
			_logerr("isBrickAtPosition: position %d,%d invalid\n",x,y);
			return 0;
		}
		if (game->bricks[x][y].type != MAP_EMPTY &&
					game->bricks[x][y].id != INVIS_BRICK_ID)
			return 1;
		return 0;
	}
	const string &getBonusLevelInfo();
	int restartLevel();
	double getPaddleVelocity() { return pvel; }
	void saveHiscores() { hiscores.save(); }
};

#endif /* SRC_CLIENTGAME_H_ */

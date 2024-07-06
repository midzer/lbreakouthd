/*
 * config.h
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

#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_

enum {
	BALL_BELOW_BONUS = 0,
	BALL_ABOVE_BONUS,
	DEBRIS_BELOW_BALL = 0,
	DEBRIS_ABOVE_BALL,
	MAXCONFIGPLAYERNAMES = 4
};

class View;
class ClientGame;

class Config {
public:
	string dname;
	string path;

	/* levels */
	int levelset_id_home;
	int levelset_count_home; /* save number of levelsets for safety
	                                (to reset id if count changed) */
	/* players */
	int player_count;
	string player_names[MAXCONFIGPLAYERNAMES];

	/* game */
	int diff; /* diffculty */
	int startlevel;
	int rel_warp_limit; /* percentage of bricks required to be destroyed
	                           before player may proceed to next level */
	int add_bonus_levels; /* add some bonus levels */
	int freakout_seed; /* seed for current freakout set */
	/* controls (SDL scancodes) */
	int k_left;
	int k_right;
	int k_lfire;
	int k_rfire;
	int k_return; /* return ball on click on this key */
	int k_turbo; /* double paddle speed while this key is pressed */
	int k_warp; /* warp to next level */
	int k_maxballspeed; /* go to maximum ball speed (while pressed) */
	int rel_motion; /* use relative mouse motion; motion_mod and invert need this enabled */
	int grab; /* keep mouse in window */
	int motion_mod; /* motion_mod in percent */
	int convex;
	int invert;
	double key_speed; /* move with key_speed pix per sec when keys are used */
	int random_angle;
	int maxballspeed_int1000; /* max ball speed in pixels/second */
	float maxballspeed_float; /* per millisecond */
	int ball_auto_turbo;

	/* gamepad */
	int gp_enabled;
	int gp_lfire;
	int gp_rfire;
	int gp_turbo;
	int gp_warp;
	int gp_maxballspeed;

	/* sound */
	int sound;
	int volume; /* 1 - 8 */
	int speech; /* enable speech? */
	int badspeech; /* if speech allowed, allow swearing? */
	int audio_buffer_size;
	int channels; /* number of mix channels */

	/* graphics */
	int anim;
	int mode; /* id of mode, see MainWindow for modes */
	int fade;
	int bonus_info;
	int fps; /* frames per second: 0 - no limit, 1 - 100, 2 - 200 */
	int show_fps;
	int ball_level;
	int i_key_speed; /* integer value that is divided by 1000 to get real key_speed */
	int antialiasing;
	/* various */
	int use_hints;
	int return_on_click; /* autoreturn on click if true else automatically */
	int theme_id; /* 0 == default theme */
	int theme_count; /* to check and properly reset id if number of themes changed */
	string edit_setname;
	int bcc_type; /* how to get targets: by clipping or trajectory */

	Config();
	~Config() { save(); }
	void save();
};

#endif /* SRC_CONFIG_H_ */

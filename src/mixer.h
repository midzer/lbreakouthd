/*
 * mixer.h
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

#ifndef MIXER_H_
#define MIXER_H_

class Mixer;

class Sound {
	friend Mixer;

	Mix_Chunk *chunk;
public:
	Sound() {
		chunk = NULL;
	}
	~Sound() {
		if (chunk)
			Mix_FreeChunk(chunk);
	}
	int load(const string &fname) {
		if (chunk) {
			Mix_FreeChunk(chunk);
			chunk = NULL;
		}
		if ((chunk = Mix_LoadWAV(fname.c_str())) == NULL) {
			_logsdlerr();
			return 0;
		}
		return 1;
	}
};

class Mixer {
	bool opened, mute;
public:
	Mixer() : opened(false), mute(false) {}
	int open(int numChans, int bufSize) {
		if (Mix_OpenAudio(22050,MIX_DEFAULT_FORMAT,2,bufSize) < 0) {
			_logsdlerr();
			return 0;
		}
		opened = true;
		Mix_AllocateChannels(numChans);
		_loginfo("Mixer opened (%d channels, %d buf size)\n",numChans,bufSize);
		return 1;
	}
	void close() {
		if (opened) {
			Mix_CloseAudio();
			opened = false;
			_loginfo("Mixer closed\n");
		}
	}
	void setVolume(uint v) { /* v is 0..100 */
		if (v > 100)
			v = 100;
		v = v * MIX_MAX_VOLUME / 100;
		Mix_Volume(-1, v);
	}
	void setMute(int m) { mute = (m?true:false); }
	void halt() { Mix_HaltChannel(-1); }
	void pause() { Mix_Pause(-1); }
	void resume() { Mix_Resume(-1); }
	void play(Sound &snd, int c = -1) {
		if (snd.chunk && opened && !mute)
			Mix_PlayChannel(c,snd.chunk,0);
	}
	void playx(Sound &snd, int x, int c = -1) {
		if(snd.chunk == NULL || !opened || mute)
			return;
		/* XXX must match VG_BRICKWIDTH=40/BRICKAREAWIDTH=640 */
		x = (x - 40) * 255 / (640 - 2*40);
		if (x < 0)
			x = 0;
		if (x > 255)
			x = 255;
		if (c == -1)
			c = Mix_GroupAvailable(-1);
		Mix_SetPanning(c, 255 - x, x);
		Mix_PlayChannel(c, snd.chunk, 0);
	}
};

#endif /* MIXER_H_ */

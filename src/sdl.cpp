/*
 * sdl.cpp
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

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "sdl.h"

int Geom::sw = 640; /* safe values before MainWindow is called */
int Geom::sh = 480;
bool Image::useColorKeyBlack = false; /* workaround for old color key in lbr2 themes */

SDL_Renderer *mrc = NULL; /* main window render context, got only one */

/** Main application window */

MainWindow::MainWindow(const char *title, int _w, int _h, int _full)
{
	int flags = 0;
	if (_w <= 0 || _h <= 0) { /* no width or height, use desktop setting */
		SDL_DisplayMode mode;
		SDL_GetCurrentDisplayMode(0,&mode);
		_w = mode.w;
		_h = mode.h;
		_full = 1;
	}
	if (_full)
		flags = SDL_WINDOW_FULLSCREEN;
	w = _w;
	h = _h;
	/* TEST  w = 1600; h = 1200; flags = 0; */
	Geom::sw = w;
	Geom::sh = h;
	_loginfo("Creating main window with %dx%d, fullscreen=%d\n",w,h,_full);
	if( (mw = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED, w, h, flags)) == NULL)
		_logsdlerr();
	if ((mr = SDL_CreateRenderer(mw, -1, SDL_RENDERER_ACCELERATED)) == NULL)
		_logsdlerr();
	mrc = mr;

	/* back to black */
	SDL_SetRenderDrawColor(mrc,0,0,0,0);
	SDL_RenderClear(mrc);
}
MainWindow::~MainWindow()
{
	if (mr)
		SDL_DestroyRenderer(mr);
	if (mw)
		SDL_DestroyWindow(mw);
}

void MainWindow::refresh()
{
	SDL_RenderPresent(mr);
}

/** Image */

/** Create new black image. If no size is given use screen size. */
int Image::create(int w, int h)
{
	if (w == 0 || h == 0)
		SDL_GetRendererOutputSize(mrc,&w,&h);

	_logdebug(1,"Creating new texture of size %dx%d\n",w,h);

	if (tex) {
		SDL_DestroyTexture(tex);
		tex = NULL;
	}

	this->w = w;
	this->h = h;
	if ((tex = SDL_CreateTexture(mrc,SDL_PIXELFORMAT_RGBA8888,
				SDL_TEXTUREACCESS_TARGET,w, h)) == NULL) {
		_logsdlerr();
		return 0;
	}
	fill(0,0,0,0);
	setBlendMode(1);
	return 1;
}

int Image::createFromScreen()
{
	int w, h;

	SDL_GetRendererOutputSize(mrc,&w,&h);
	SDL_Surface *sshot = SDL_CreateRGBSurface(0, w, h, 32,
			0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
	SDL_RenderReadPixels(mrc, NULL, SDL_PIXELFORMAT_ARGB8888,
					sshot->pixels, sshot->pitch);
	int ret = load(sshot);
	SDL_FreeSurface(sshot);
	return ret;
}

/** OLD STUFF FOR SOFTWARE SCALE AS FALLBACK */

/** Set pixel in surface. */
Uint32 set_pixel( SDL_Surface *surf, int x, int y, Uint32 pixel )
{
    int pos = 0;

    if (x < 0 || y < 0 || x >= surf->w || y >= surf->h)
	    return pixel;

    pos = y * surf->pitch + x * surf->format->BytesPerPixel;
    memcpy( (char*)(surf->pixels) + pos, &pixel, surf->format->BytesPerPixel );
    return pixel;
}

/** Get pixel from surface. */
Uint32 get_pixel( SDL_Surface *surf, int x, int y )
{
    int pos = 0;
    Uint32 pixel = 0;

    pos = y * surf->pitch + x * surf->format->BytesPerPixel;
    memcpy( &pixel, (char*)(surf->pixels) + pos, surf->format->BytesPerPixel );
    return pixel;
}

/** Scale surface to half the size */
SDL_Surface *create_small_surface(SDL_Surface *src) {
	SDL_Surface *dst = 0;
	SDL_PixelFormat *spf = src->format;

	if ((dst = SDL_CreateRGBSurface(SDL_SWSURFACE,
			src->w/2, src->h/2,
			spf->BitsPerPixel,
			spf->Rmask, spf->Gmask, spf->Bmask,
			spf->Amask)) == 0) {
		_logsdlerr();
		return 0;
	}

        for ( int j = 0; j < dst->h; j++ ) {
            for ( int i = 0; i < dst->w; i++ )
                set_pixel(dst, i, j,
                           get_pixel( src, i*2, j*2 ) );
        }
	return dst;
}

/** OLD STUFF END */

/** Load image from file. Return
 *   1 on success without any problems
 *   2 on successfully loading surface but failing to create hardware texture
 *     then surface is scaled to half size and smaller texture is created
 *   0 on complete failure
 */
int Image::load(const string& fname)
{
	_logdebug(1,"Loading texture %s\n",fname.c_str());

	/* delete old texture */
	if (tex) {
		SDL_DestroyTexture(tex);
		tex = NULL;
	}
	w = 0;
	h = 0;

	/* load image as software surface */
	SDL_Surface *surf = IMG_Load(fname.c_str());
	if (surf == NULL) {
		_logsdlerr();
		return 0;
	}

	/* set black as color key if requested for old images */
	if (Image::useColorKeyBlack)
		SDL_SetColorKey(surf, SDL_TRUE, 0x0);

	/* create hardware texture from software surface */
	if ((tex = SDL_CreateTextureFromSurface(mrc, surf))) {
		w = surf->w;
		h = surf->h;
		SDL_FreeSurface(surf);
		return 1;
	}
	_logsdlerr();

	/* if we get here, surface was loaded thus file found
	 * but we couldn't create the texture strongly suggesting
	 * it is too large for video memory. so we scale down and
	 * retry once before we give up. */

	/* get new surface half the size */
	SDL_Surface *newsurf = create_small_surface(surf);
	if (newsurf == 0) {
		_logsdlerr();
		SDL_FreeSurface(surf);
		return 0;
	}

	/* create texture with new surface */
	int ret = 0;
	if ((tex = SDL_CreateTextureFromSurface(mrc, newsurf))) {
		w = newsurf->w;
		h = newsurf->h;
		ret = 2;
	}
	SDL_FreeSurface(surf);
	SDL_FreeSurface(newsurf);
	return ret;
}

int Image::load(SDL_Surface *s)
{
	_logdebug(1,"Loading texture from surface %dx%d\n",s->w,s->h);

	if (tex) {
		SDL_DestroyTexture(tex);
		tex = NULL;
	}

	if ((tex = SDL_CreateTextureFromSurface(mrc,s)) == NULL) {
		_logsdlerr();
		return 0;
	}
	w = s->w;
	h = s->h;
	return 1;
}
int Image::load(Image *s, int x, int y, int w, int h)
{
	_logdebug(1,"Loading texture from surface %dx%d\n",s->w,s->h);

	if (tex) {
		SDL_DestroyTexture(tex);
		tex = NULL;
	}

	SDL_Rect srect = {x, y, w, h};
	SDL_Rect drect = {0, 0, w, h};

	this->w = w;
	this->h = h;
	if ((tex = SDL_CreateTexture(mrc,SDL_PIXELFORMAT_RGBA8888,
				SDL_TEXTUREACCESS_TARGET,w, h)) == NULL) {
		_logsdlerr();
		return 0;
	}
	SDL_SetRenderTarget(mrc, tex);
	SDL_RenderCopy(mrc, s->getTex(), &srect, &drect);
	SDL_SetRenderTarget(mrc, NULL);
	return 1;
}

SDL_Texture *Image::getTex()
{
	return tex;
}

void Image::copy() /* full scale */
{
	if (tex == NULL)
		return;

	SDL_RenderCopy(mrc, tex, NULL, NULL);
}
void Image::copy(int dx, int dy)
{
	if (tex == NULL)
		return;

	SDL_Rect drect = {dx , dy , w, h};
	SDL_RenderCopy(mrc, tex, NULL, &drect);
}
void Image::copy(int dx, int dy, int dw, int dh)
{
	if (tex == NULL)
		return;

	SDL_Rect drect = {dx , dy , dw, dh};
	SDL_RenderCopy(mrc, tex, NULL, &drect);
}
void Image::copy(int sx, int sy, int sw, int sh, int dx, int dy) {
	if (tex == NULL)
		return;

	SDL_Rect srect = {sx, sy, sw, sh};
	SDL_Rect drect = {dx , dy , sw, sh};
	SDL_RenderCopy(mrc, tex, &srect, &drect);
}

void Image::fill(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	SDL_Texture *old = SDL_GetRenderTarget(mrc);
	SDL_SetRenderTarget(mrc,tex);
	SDL_SetRenderDrawColor(mrc,r,g,b,a);
	SDL_RenderClear(mrc);
	SDL_SetRenderTarget(mrc,old);
}

void Image::fill(int x, int y, int w, int h, const SDL_Color &c) {
	SDL_Rect r = {x,y,w,h};
	SDL_Texture *old = SDL_GetRenderTarget(mrc);
	SDL_SetRenderTarget(mrc,tex);
	SDL_SetRenderDrawColor(mrc,c.r,c.g,c.b,c.a);
	SDL_RenderFillRect(mrc, &r);
	SDL_SetRenderTarget(mrc,old);
}

void Image::scale(int nw, int nh)
{
	_logdebug(1,"Scaling texture of size %dx%d to %dx%d\n",w,h,nw,nh);

	if (tex == NULL)
		return;
	if (nw == w && nh == h)
		return; /* already ok */

	SDL_Texture *newtex = 0;
	if ((newtex = SDL_CreateTexture(mrc,SDL_PIXELFORMAT_RGBA8888,
					SDL_TEXTUREACCESS_TARGET,nw,nh)) == NULL) {
		_logsdlerr();
		return;
	}
	SDL_Texture *oldTarget = SDL_GetRenderTarget(mrc);
	SDL_SetRenderTarget(mrc, newtex);
	SDL_SetTextureBlendMode(newtex, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(mrc,0,0,0,0);
	SDL_RenderClear(mrc);
	SDL_RenderCopy(mrc, tex, NULL, NULL);
	SDL_SetRenderTarget(mrc, oldTarget);

	SDL_DestroyTexture(tex);
	tex = newtex;
	w = nw;
	h = nh;
}

/** Create shadow image by clearing all r,g,b to 0 and cutting alpha in half. */
int Image::createShadow(Image &img)
{
	_logdebug(1,"Creating shadow of size %dx%d\n",
				img.getWidth(),img.getHeight());

	/* duplicate first */
	create(img.getWidth(),img.getHeight());
	SDL_Texture *oldTarget = SDL_GetRenderTarget(mrc);
	SDL_SetRenderTarget(mrc, tex);
	img.copy(0,0);

	/* change and apply */
	Uint32 *pixels = (Uint32*)calloc(w*h,sizeof(Uint32));
	if (!pixels) {
		_logerr("Cannot allocate pixel buffer of size %dx%d\n",w,h);
		SDL_SetRenderTarget(mrc, oldTarget);
		return 0;
	}
	int pitch = w*sizeof(Uint32);
	Uint8 r,g,b,a;
	Uint32 pft = 0;
	SDL_QueryTexture(tex, &pft, 0, 0, 0);
	SDL_PixelFormat* pf = SDL_AllocFormat(pft);
	if (SDL_RenderReadPixels(mrc, NULL, pft, pixels, pitch) < 0) {
		_logsdlerr();
		free(pixels);
		SDL_SetRenderTarget(mrc, oldTarget);
		return 0;
	}
	for (int i = 0; i < w * h; i++) {
		SDL_GetRGBA(pixels[i],pf,&r,&g,&b,&a);
		pixels[i] = SDL_MapRGBA(pf,0,0,0,a/2);
	}
	SDL_UpdateTexture(tex, NULL, pixels, pitch);
	SDL_FreeFormat(pf);
	SDL_SetRenderTarget(mrc, oldTarget);
	free(pixels);

	return 1;
}

/** Grid image: large bitmap with same sized icons */

int GridImage::load(const string& fname, int _gw, int _gh)
{
	/* set grid size and load basic image */
	gw = _gw;
	gh = _gh;
	int ret = Image::load(fname);
	/* if 2 is returned the image was scaled down to
	 * half the size so adjust grid size accordingly. */
	if (ret == 2) {
		gw /= 2;
		gh /= 2;
		_logerr("Grid image %s too large for hardware texture: scaled to half size %dx%d\n",
				fname.c_str(),gw,gh);
	}
	return ret;
}

int GridImage::load(SDL_Surface *s, int _gw, int _gh)
{
	gw = _gw;
	gh = _gh;
	return Image::load(s);
}

void GridImage::copy(int gx, int gy, int dx, int dy)
{
	if (tex == NULL )
		return;

	SDL_Rect srect = {gx * gw, gy * gh, gw, gh};
	SDL_Rect drect = {dx , dy , gw, gh};
	SDL_RenderCopy(mrc, tex, &srect, &drect);
}
void GridImage::copy(int gx, int gy, int dx, int dy, int dw, int dh)
{
	if (tex == NULL )
		return;

	SDL_Rect srect = {gx * gw, gy * gh, gw, gh};
	SDL_Rect drect = {dx , dy , dw, dh};
	SDL_RenderCopy(mrc, tex, &srect, &drect);
}
void GridImage::copy(int gx, int gy, int sx, int sy, int sw, int sh, int dx, int dy)
{
	if (tex == NULL )
		return;

	SDL_Rect srect = {gx * gw + sx, gy * gh + sy, sw, sh};
	SDL_Rect drect = {dx , dy , sw, sh};
	SDL_RenderCopy(mrc, tex, &srect, &drect);
}


/** Scale cell by cell to prevent artifacts. */
void GridImage::scale(int ncw, int nch)
{
	if (tex == NULL)
		return;

	int nw = ncw * getGridSizeX();
	int nh = nch * getGridSizeY();
	SDL_Texture *newtex = 0;
	SDL_Rect srect = {0, 0, gw, gh};
	SDL_Rect drect = {0, 0, ncw, nch};

	if (nw == w && nh == h)
		return; /* already ok */

	if ((newtex = SDL_CreateTexture(mrc,SDL_PIXELFORMAT_RGBA8888,
					SDL_TEXTUREACCESS_TARGET,nw,nh)) == NULL) {
		_logsdlerr();
		return;
	}
	SDL_SetRenderTarget(mrc, newtex);
	SDL_SetTextureBlendMode(newtex, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(mrc,0,0,0,0);
	SDL_RenderClear(mrc);
	for (uint j = 0; j < getGridSizeY(); j++)
		for (uint i = 0; i < getGridSizeX(); i++) {
			srect.x = i * gw;
			srect.y = j * gh;
			drect.x = i * ncw;
			drect.y = j * nch;
			SDL_RenderCopy(mrc, tex, &srect, &drect);
		}
	SDL_SetRenderTarget(mrc, NULL);

	SDL_DestroyTexture(tex);
	tex = newtex;
	w = nw;
	h = nh;
	gw = ncw;
	gh = nch;
}

int GridImage::createShadow(GridImage &img)
{
	gw = img.getGridWidth();
	gh = img.getGridHeight();
	return Image::createShadow(img);
}


/** Font */

Font::Font() : font(0), size(0)
{
}
Font::~Font() {
	if (font)
		TTF_CloseFont(font);
}

void Font::load(const string& fname, int sz) {
	if (font) {
		TTF_CloseFont(font);
		font = 0;
		size = 0;
	}

	if ((font = TTF_OpenFont(fname.c_str(), sz)) == NULL) {
		_logsdlerr();
		return;
	}
	size = sz;
	setColor(255,255,255,255);
	setAlign(ALIGN_X_LEFT | ALIGN_Y_TOP);
}

void Font::setColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	SDL_Color c = {r, g, b, a};
	clr = c;
}
void Font::setColor(SDL_Color c) {
	clr = c;
}
void Font::setAlign(int a) {
	align = a;
}
void Font::write(int x, int y, const string& _str, int alpha) {
	if (font == 0)
		return;

	/* XXX doesn't look good, why no rendering
	 * into texture directly available? */
	SDL_Surface *surf;
	SDL_Texture *tex;
	SDL_Rect drect;
	const char *str = _str.c_str();

	if (strlen(str) == 0)
		return;

	if ((surf = TTF_RenderUTF8_Blended(font, str, clr)) == NULL)
		_logsdlerr();
	if ((tex = SDL_CreateTextureFromSurface(mrc, surf)) == NULL)
		_logsdlerr();
	if (align & ALIGN_X_LEFT)
		drect.x = x;
	else if (align & ALIGN_X_RIGHT)
		drect.x = x - surf->w;
	else
		drect.x = x - surf->w/2; /* center */
	if (align & ALIGN_Y_TOP)
		drect.y = y;
	else if (align & ALIGN_Y_BOTTOM)
		drect.y = y - surf->h;
	else
		drect.y = y - surf->h/2;
	drect.w = surf->w;
	drect.h = surf->h;
	if (alpha < 255)
		SDL_SetTextureAlphaMod(tex, alpha);

	/* do a shadow first */
	if (1) {
		SDL_Rect drect2 = drect;
		SDL_Surface *surf2;
		SDL_Texture *tex2;
		SDL_Color clr2 = { 0,0,0,255 };
		if ((surf2 = TTF_RenderUTF8_Blended(font, str, clr2)) == NULL)
			_logsdlerr();
		if ((tex2 = SDL_CreateTextureFromSurface(mrc, surf2)) == NULL)
			_logsdlerr();
		drect2.x += size/10;
		drect2.y += size/10;
		SDL_SetTextureAlphaMod(tex2, alpha/2);
		SDL_RenderCopy(mrc, tex2, NULL, &drect2);
		SDL_FreeSurface(surf2);
		SDL_DestroyTexture(tex2);
	}

	SDL_RenderCopy(mrc, tex, NULL, &drect);
	SDL_FreeSurface(surf);
	SDL_DestroyTexture(tex);
}
void Font::writeText(int x, int y, const string& _text, int wrapwidth, int alpha)
{
	if (font == 0)
		return;

	/* XXX doesn't look good, why no rendering
	 * into texture directly available? */
	SDL_Surface *surf;
	SDL_Texture *tex;
	SDL_Rect drect;
	const char *text = _text.c_str();

	if (strlen(text) == 0)
		return;

	if ((surf = TTF_RenderUTF8_Blended_Wrapped(font, text, clr, wrapwidth)) == NULL)
		_logsdlerr();
	if ((tex = SDL_CreateTextureFromSurface(mrc, surf)) == NULL)
		_logsdlerr();
	drect.x = x;
	drect.y = y;
	drect.w = surf->w;
	drect.h = surf->h;
	if (alpha < 255)
		SDL_SetTextureAlphaMod(tex, alpha);
	SDL_RenderCopy(mrc, tex, NULL, &drect);
	SDL_FreeSurface(surf);
	SDL_DestroyTexture(tex);
}

void Label::setText(Font &font, const string &str, uint maxw)
{
	if (str == "") {
		empty = true;
		return;
	}

	int b = border;
	if (b == -1)
		b = 20 * font.getSize() / 100;
	int w = 0, h = 0;

	/* get size */
	if (maxw == 0) /* single centered line */
		font.getTextSize(str, &w, &h);
	else
		font.getWrappedTextSize(str, maxw, &w, &h);

	if (w == 0 || h == 0)
		return;

	img.create(w + 4*b, h + 2*b);
	img.fill(bgColor);
	SDL_Texture *old = SDL_GetRenderTarget(mrc);
	SDL_SetRenderTarget(mrc,img.getTex());
	if (maxw == 0) {
		font.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
		font.write(img.getWidth()/2,img.getHeight()/2,str);
	} else
		font.writeText(2*b, b, str, maxw);
	SDL_SetRenderTarget(mrc,old);
	empty = false;
}

/** Try to find and open a joystick (game controller) */
void Gamepad::open()
{
	if (js)
		close(); /* make sure none is opened yet */

	if (SDL_NumJoysticks() == 0) {
		_loginfo("No game controller found...\n");
		return;
	}

	if ((js = SDL_JoystickOpen(0)) == NULL) {
		_logerr("Couldn't open game controller: %s\n",SDL_GetError());
		return;
	}

	_loginfo("Opened game controller 0\n");
	_logdebug(1,"  num axes: %d, num buttons: %d, num balls: %d\n",
			SDL_JoystickNumAxes(js),SDL_JoystickNumButtons(js),
			SDL_JoystickNumBalls(js));

	numbuttons = SDL_JoystickNumButtons(js);
	if (numbuttons > 10)
		numbuttons = 10;
}

/** Update joystick state and return state array (0=off,1=on) */
const Uint8 *Gamepad::update() {
	SDL_JoystickUpdate();

	memset(state,0,sizeof(state));
	if (js == NULL)
		return state;

	if (SDL_JoystickGetAxis(js,0) < -3200)
		state[GPAD_LEFT] = 1;
	if (SDL_JoystickGetAxis(js,0) > 3200)
		state[GPAD_RIGHT] = 1;
	if (SDL_JoystickGetAxis(js,1) < -3200)
		state[GPAD_UP] = 1;
	if (SDL_JoystickGetAxis(js,1) > 3200)
		state[GPAD_DOWN] = 1;
	for (uint i = 0; i < numbuttons; i++)
		state[GPAD_BUTTON0 + i] = SDL_JoystickGetButton(js,i);

	return state;
}

/** Run a standalone dialog for editing a UTF8 string. ESC cancels editing
 * (string is not changed), Enter confirms changes. Return 1 if string was
 * changed, 0 if not changed, -1 if quit requested. */
int runEditDialog(Font &font, const string &caption, string &str)
{
	int ret = 0;
	SDL_Event event;
	string backup = str;
	Image img;
	bool done = false;

	img.createFromScreen();
	img.setAlpha(64);

	font.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);

	SDL_StartTextInput();
	while (!done) {
		if (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				done = true;
				ret = -1;
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
		font.write(img.getWidth()/2,img.getHeight()/2,text);
		SDL_RenderPresent(mrc);
		SDL_Delay(10);
		SDL_FlushEvent(SDL_MOUSEMOTION);
	}
	SDL_StopTextInput();

	return ret;
}

/** Run standalone confirm dialog. Return 1 if confirmed
 * changed, 0 if not, -1 if quit requested. */
int runConfirmDialog(Font &font, const string &caption)
{
	int ret = 0;
	bool done = false;
	SDL_Event event;
	Image img;

	img.createFromScreen();
	img.setAlpha(64);

	font.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);

	while (!done) {
		if (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				done = true;
				ret = -1;
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.scancode) {
				case SDL_SCANCODE_Y:
				case SDL_SCANCODE_Z:
					ret = 1;
					done = true;
					break;
				default:
					ret = 0;
					done = true;
					break;
				}
				break;
			}
		}

		/* redraw */
		SDL_SetRenderDrawColor(mrc,0,0,0,255);
		SDL_RenderClear(mrc);
		img.copy();
		font.write(img.getWidth()/2, img.getHeight()/2,caption);
		SDL_RenderPresent(mrc);
		SDL_Delay(10);
		SDL_FlushEvent(SDL_MOUSEMOTION);
	}

	return ret;

}

// Tyler Selwyn
// CPSC 440 - Game Programming
// Project 5 - SpriteSheet class for animated sprites

#pragma once
#include <allegro5/allegro.h>

class SpriteSheet {
private:
	ALLEGRO_BITMAP* sheet;
	int frameWidth;
	int frameHeight;
	int cols;
	int rows;

public:
	SpriteSheet() : sheet(nullptr), frameWidth(0), frameHeight(0), cols(0), rows(0) {}

	void init(ALLEGRO_BITMAP* bmp, int fw, int fh, int c, int r)
	{
		sheet = bmp;
		frameWidth = fw;
		frameHeight = fh;
		cols = c;
		rows = r;
	}

	void drawFrame(int col, int row, float x, float y, int flags = 0)
	{
		al_draw_bitmap_region(sheet,
			col * frameWidth, row * frameHeight,
			frameWidth, frameHeight,
			x, y, flags);
	}

	int getFrameWidth() { return frameWidth; }
	int getFrameHeight() { return frameHeight; }
	int getCols() { return cols; }
};

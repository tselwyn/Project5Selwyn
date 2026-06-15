// Tyler Selwyn
// CPSC 440 - Game Programming
// Project 5 - Space Shooter

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>

const int SCREEN_W = 800;
const int SCREEN_H = 600;
const int MAX_STARS = 100;
const float PLAYER_SPEED = 5.0;

enum GameState { INTRO, PLAYING, GAME_OVER, WIN };

// star struct for the scrolling background
struct Star {
	float x, y;
	float speed;
	int brightness;
};

int main()
{
	al_init();
	al_install_keyboard();
	al_init_primitives_addon();
	al_init_font_addon();
	al_init_ttf_addon();
	al_init_image_addon();

	ALLEGRO_DISPLAY* display = al_create_display(SCREEN_W, SCREEN_H);
	al_set_window_title(display, "Space Shooter - Tyler Selwyn");

	ALLEGRO_EVENT_QUEUE* event_queue = al_create_event_queue();
	ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);

	al_register_event_source(event_queue, al_get_keyboard_event_source());
	al_register_event_source(event_queue, al_get_display_event_source(display));
	al_register_event_source(event_queue, al_get_timer_event_source(timer));

	// fonts
	ALLEGRO_FONT* fontLarge = al_load_font("ARCADE_N.TTF", 48, 0);
	ALLEGRO_FONT* fontMed = al_load_font("ARCADE_N.TTF", 24, 0);
	ALLEGRO_FONT* fontSmall = al_load_font("ARCADE_N.TTF", 16, 0);

	// load player ship
	ALLEGRO_BITMAP* playerImg = al_load_bitmap("spaceShips_001.png");
	int playerW = al_get_bitmap_width(playerImg);
	int playerH = al_get_bitmap_height(playerImg);

	// player position - start at bottom center
	float playerX = SCREEN_W / 2 - playerW / 2;
	float playerY = SCREEN_H - playerH - 20;

	// key tracking for smooth movement
	bool keyUp = false, keyDown = false, keyLeft = false, keyRight = false;

	// init stars with random positions and speeds
	Star stars[MAX_STARS];
	for (int i = 0; i < MAX_STARS; i++)
	{
		stars[i].x = rand() % SCREEN_W;
		stars[i].y = rand() % SCREEN_H;
		stars[i].speed = (rand() % 3) + 1;
		stars[i].brightness = 100 + (rand() % 156);
	}

	GameState state = INTRO;
	bool done = false;
	bool redraw = true;

	al_start_timer(timer);

	while (!done)
	{
		ALLEGRO_EVENT ev;
		al_wait_for_event(event_queue, &ev);

		if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
			done = true;

		if (ev.type == ALLEGRO_EVENT_KEY_DOWN)
		{
			if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
				done = true;

			if (state == INTRO && ev.keyboard.keycode == ALLEGRO_KEY_ENTER)
				state = PLAYING;

			// track keys pressed
			if (ev.keyboard.keycode == ALLEGRO_KEY_UP) keyUp = true;
			if (ev.keyboard.keycode == ALLEGRO_KEY_DOWN) keyDown = true;
			if (ev.keyboard.keycode == ALLEGRO_KEY_LEFT) keyLeft = true;
			if (ev.keyboard.keycode == ALLEGRO_KEY_RIGHT) keyRight = true;
		}

		if (ev.type == ALLEGRO_EVENT_KEY_UP)
		{
			if (ev.keyboard.keycode == ALLEGRO_KEY_UP) keyUp = false;
			if (ev.keyboard.keycode == ALLEGRO_KEY_DOWN) keyDown = false;
			if (ev.keyboard.keycode == ALLEGRO_KEY_LEFT) keyLeft = false;
			if (ev.keyboard.keycode == ALLEGRO_KEY_RIGHT) keyRight = false;
		}

		if (ev.type == ALLEGRO_EVENT_TIMER)
		{
			// update stars every frame regardless of state
			for (int i = 0; i < MAX_STARS; i++)
			{
				stars[i].y += stars[i].speed;
				if (stars[i].y > SCREEN_H)
				{
					stars[i].y = 0;
					stars[i].x = rand() % SCREEN_W;
				}
			}

			// move player during gameplay
			if (state == PLAYING)
			{
				if (keyUp && playerY > 40)
					playerY -= PLAYER_SPEED;
				if (keyDown && playerY < SCREEN_H - playerH)
					playerY += PLAYER_SPEED;
				if (keyLeft && playerX > 0)
					playerX -= PLAYER_SPEED;
				if (keyRight && playerX < SCREEN_W - playerW)
					playerX += PLAYER_SPEED;
			}

			redraw = true;
		}

		if (redraw && al_is_event_queue_empty(event_queue))
		{
			redraw = false;
			al_clear_to_color(al_map_rgb(0, 0, 0));

			// draw stars on every screen
			for (int i = 0; i < MAX_STARS; i++)
			{
				al_draw_filled_circle(stars[i].x, stars[i].y, 1,
					al_map_rgb(stars[i].brightness, stars[i].brightness, stars[i].brightness));
			}

			if (state == INTRO)
			{
				al_draw_text(fontLarge, al_map_rgb(255, 255, 255),
					SCREEN_W / 2, 120, ALLEGRO_ALIGN_CENTER, "SPACE SHOOTER");

				al_draw_text(fontMed, al_map_rgb(200, 200, 200),
					SCREEN_W / 2, 250, ALLEGRO_ALIGN_CENTER, "Arrow Keys to Move");
				al_draw_text(fontMed, al_map_rgb(200, 200, 200),
					SCREEN_W / 2, 290, ALLEGRO_ALIGN_CENTER, "Space to Shoot");
				al_draw_text(fontMed, al_map_rgb(200, 200, 200),
					SCREEN_W / 2, 330, ALLEGRO_ALIGN_CENTER, "Avoid Enemies");
				al_draw_text(fontMed, al_map_rgb(200, 200, 200),
					SCREEN_W / 2, 370, ALLEGRO_ALIGN_CENTER, "Collect Power Ups");

				al_draw_text(fontSmall, al_map_rgb(255, 255, 0),
					SCREEN_W / 2, 480, ALLEGRO_ALIGN_CENTER, "Press ENTER to Start");

				al_draw_text(fontSmall, al_map_rgb(100, 100, 100),
					SCREEN_W / 2, 560, ALLEGRO_ALIGN_CENTER, "Tyler Selwyn - CPSC 440");
			}
			else if (state == PLAYING)
			{
				// draw player ship
				al_draw_bitmap(playerImg, playerX, playerY, 0);
			}

			al_flip_display();
		}
	}

	al_destroy_bitmap(playerImg);
	al_destroy_font(fontLarge);
	al_destroy_font(fontMed);
	al_destroy_font(fontSmall);
	al_destroy_timer(timer);
	al_destroy_event_queue(event_queue);
	al_destroy_display(display);

	return 0;
}
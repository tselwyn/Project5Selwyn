// Tyler Selwyn
// CPSC 440 - Game Programming
// Project 5 - Space Shooter

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>

// screen dimensions
const int SCREEN_W = 800;
const int SCREEN_H = 600;

// game states
enum GameState { INTRO, PLAYING, GAME_OVER, WIN };

int main()
{
	// init everything before display
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

	// register event sources - display source is required or keys break
	al_register_event_source(event_queue, al_get_keyboard_event_source());
	al_register_event_source(event_queue, al_get_display_event_source(display));
	al_register_event_source(event_queue, al_get_timer_event_source(timer));

	// load arcade font in different sizes
	ALLEGRO_FONT* fontLarge = al_load_font("ARCADE_N.TTF", 48, 0);
	ALLEGRO_FONT* fontMed = al_load_font("ARCADE_N.TTF", 24, 0);
	ALLEGRO_FONT* fontSmall = al_load_font("ARCADE_N.TTF", 16, 0);

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

			// press Enter to start the game from intro
			if (state == INTRO && ev.keyboard.keycode == ALLEGRO_KEY_ENTER)
				state = PLAYING;
		}

		if (ev.type == ALLEGRO_EVENT_TIMER)
			redraw = true;

		// only draw when needed
		if (redraw && al_is_event_queue_empty(event_queue))
		{
			redraw = false;
			al_clear_to_color(al_map_rgb(0, 0, 0));

			if (state == INTRO)
			{
				// title
				al_draw_text(fontLarge, al_map_rgb(255, 255, 255),
					SCREEN_W / 2, 120, ALLEGRO_ALIGN_CENTER, "SPACE SHOOTER");

				// instructions
				al_draw_text(fontMed, al_map_rgb(200, 200, 200),
					SCREEN_W / 2, 250, ALLEGRO_ALIGN_CENTER, "Arrow Keys to Move");
				al_draw_text(fontMed, al_map_rgb(200, 200, 200),
					SCREEN_W / 2, 290, ALLEGRO_ALIGN_CENTER, "Space to Shoot");
				al_draw_text(fontMed, al_map_rgb(200, 200, 200),
					SCREEN_W / 2, 330, ALLEGRO_ALIGN_CENTER, "Avoid Enemies");
				al_draw_text(fontMed, al_map_rgb(200, 200, 200),
					SCREEN_W / 2, 370, ALLEGRO_ALIGN_CENTER, "Collect Power Ups");

				// prompt to start
				al_draw_text(fontSmall, al_map_rgb(255, 255, 0),
					SCREEN_W / 2, 480, ALLEGRO_ALIGN_CENTER, "Press ENTER to Start");

				// credit
				al_draw_text(fontSmall, al_map_rgb(100, 100, 100),
					SCREEN_W / 2, 560, ALLEGRO_ALIGN_CENTER, "Tyler Selwyn - CPSC 440");
			}
			else if (state == PLAYING)
			{
				// placeholder until next commit
				al_draw_text(fontMed, al_map_rgb(255, 255, 255),
					SCREEN_W / 2, SCREEN_H / 2, ALLEGRO_ALIGN_CENTER, "Game starts here...");
			}

			al_flip_display();
		}
	}

	// cleanup
	al_destroy_font(fontLarge);
	al_destroy_font(fontMed);
	al_destroy_font(fontSmall);
	al_destroy_timer(timer);
	al_destroy_event_queue(event_queue);
	al_destroy_display(display);

	return 0;
}
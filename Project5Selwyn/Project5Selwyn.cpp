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
const int MAX_BULLETS = 10;
const int MAX_ENEMIES = 15;
const float PLAYER_SPEED = 5.0;
const float BULLET_SPEED = 7.0;
const int FIRE_COOLDOWN = 15; // frames between shots

enum GameState { INTRO, PLAYING, GAME_OVER, WIN };

struct Star {
	float x, y;
	float speed;
	int brightness;
};

struct Bullet {
	float x, y;
	bool active;
};

struct Enemy {
	float x, y;
	float speed;
	bool active;
	int type; // which ship image to use
};

// simple rectangle collision check
bool checkCollision(float x1, float y1, int w1, int h1,
	float x2, float y2, int w2, int h2)
{
	return (x1 < x2 + w2 && x1 + w1 > x2 &&
		y1 < y2 + h2 && y1 + h1 > y2);
}

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

	// load images
	ALLEGRO_BITMAP* playerImg = al_load_bitmap("spaceShips_001.png");
	ALLEGRO_BITMAP* enemyImg1 = al_load_bitmap("spaceShips_003.png");
	ALLEGRO_BITMAP* enemyImg2 = al_load_bitmap("spaceShips_005.png");
	ALLEGRO_BITMAP* enemyImg3 = al_load_bitmap("spaceShips_007.png");
	ALLEGRO_BITMAP* bulletImg = al_load_bitmap("spaceMissiles_004.png");

	int playerW = al_get_bitmap_width(playerImg);
	int playerH = al_get_bitmap_height(playerImg);
	int bulletW = al_get_bitmap_width(bulletImg);
	int bulletH = al_get_bitmap_height(bulletImg);

	// store enemy image info in arrays for easy lookup by type
	ALLEGRO_BITMAP* enemyImgs[3] = { enemyImg1, enemyImg2, enemyImg3 };
	int enemyW[3] = {
		al_get_bitmap_width(enemyImg1),
		al_get_bitmap_width(enemyImg2),
		al_get_bitmap_width(enemyImg3)
	};
	int enemyH[3] = {
		al_get_bitmap_height(enemyImg1),
		al_get_bitmap_height(enemyImg2),
		al_get_bitmap_height(enemyImg3)
	};

	// player state
	float playerX = SCREEN_W / 2 - playerW / 2;
	float playerY = SCREEN_H - playerH - 20;
	int lives = 3;
	int score = 0;
	int fireCooldown = 0;
	int shotsFired = 0;
	int shotsHit = 0;

	// key tracking
	bool keyUp = false, keyDown = false, keyLeft = false, keyRight = false;
	bool keySpace = false;

	// init stars
	Star stars[MAX_STARS];
	for (int i = 0; i < MAX_STARS; i++)
	{
		stars[i].x = rand() % SCREEN_W;
		stars[i].y = rand() % SCREEN_H;
		stars[i].speed = (rand() % 3) + 1;
		stars[i].brightness = 100 + (rand() % 156);
	}

	// init bullets
	Bullet bullets[MAX_BULLETS];
	for (int i = 0; i < MAX_BULLETS; i++)
		bullets[i].active = false;

	// init enemies
	Enemy enemies[MAX_ENEMIES];
	for (int i = 0; i < MAX_ENEMIES; i++)
		enemies[i].active = false;

	int spawnTimer = 0;
	int spawnRate = 60; // frames between enemy spawns

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

			if (ev.keyboard.keycode == ALLEGRO_KEY_UP) keyUp = true;
			if (ev.keyboard.keycode == ALLEGRO_KEY_DOWN) keyDown = true;
			if (ev.keyboard.keycode == ALLEGRO_KEY_LEFT) keyLeft = true;
			if (ev.keyboard.keycode == ALLEGRO_KEY_RIGHT) keyRight = true;
			if (ev.keyboard.keycode == ALLEGRO_KEY_SPACE) keySpace = true;
		}

		if (ev.type == ALLEGRO_EVENT_KEY_UP)
		{
			if (ev.keyboard.keycode == ALLEGRO_KEY_UP) keyUp = false;
			if (ev.keyboard.keycode == ALLEGRO_KEY_DOWN) keyDown = false;
			if (ev.keyboard.keycode == ALLEGRO_KEY_LEFT) keyLeft = false;
			if (ev.keyboard.keycode == ALLEGRO_KEY_RIGHT) keyRight = false;
			if (ev.keyboard.keycode == ALLEGRO_KEY_SPACE) keySpace = false;
		}

		if (ev.type == ALLEGRO_EVENT_TIMER)
		{
			// scroll stars always
			for (int i = 0; i < MAX_STARS; i++)
			{
				stars[i].y += stars[i].speed;
				if (stars[i].y > SCREEN_H)
				{
					stars[i].y = 0;
					stars[i].x = rand() % SCREEN_W;
				}
			}

			if (state == PLAYING)
			{
				// move player
				if (keyUp && playerY > 40)
					playerY -= PLAYER_SPEED;
				if (keyDown && playerY < SCREEN_H - playerH)
					playerY += PLAYER_SPEED;
				if (keyLeft && playerX > 0)
					playerX -= PLAYER_SPEED;
				if (keyRight && playerX < SCREEN_W - playerW)
					playerX += PLAYER_SPEED;

				// fire bullets
				if (fireCooldown > 0) fireCooldown--;

				if (keySpace && fireCooldown == 0)
				{
					for (int i = 0; i < MAX_BULLETS; i++)
					{
						if (!bullets[i].active)
						{
							bullets[i].x = playerX + playerW / 2 - bulletW / 2;
							bullets[i].y = playerY - bulletH;
							bullets[i].active = true;
							shotsFired++;
							fireCooldown = FIRE_COOLDOWN;
							break;
						}
					}
				}

				// move bullets upward
				for (int i = 0; i < MAX_BULLETS; i++)
				{
					if (bullets[i].active)
					{
						bullets[i].y -= BULLET_SPEED;
						if (bullets[i].y < -bulletH)
							bullets[i].active = false;
					}
				}

				// spawn enemies
				spawnTimer++;
				if (spawnTimer >= spawnRate)
				{
					spawnTimer = 0;
					for (int i = 0; i < MAX_ENEMIES; i++)
					{
						if (!enemies[i].active)
						{
							enemies[i].type = rand() % 3;
							enemies[i].x = rand() % (SCREEN_W - enemyW[enemies[i].type]);
							enemies[i].y = -enemyH[enemies[i].type];
							enemies[i].speed = 2.0 + (rand() % 3);
							enemies[i].active = true;
							break;
						}
					}
				}

				// move enemies downward
				for (int i = 0; i < MAX_ENEMIES; i++)
				{
					if (enemies[i].active)
					{
						enemies[i].y += enemies[i].speed;
						// deactivate if they go off screen
						if (enemies[i].y > SCREEN_H)
							enemies[i].active = false;
					}
				}

				// check bullet-enemy collisions
				for (int i = 0; i < MAX_BULLETS; i++)
				{
					if (!bullets[i].active) continue;
					for (int j = 0; j < MAX_ENEMIES; j++)
					{
						if (!enemies[j].active) continue;
						int et = enemies[j].type;
						if (checkCollision(bullets[i].x, bullets[i].y, bulletW, bulletH,
							enemies[j].x, enemies[j].y, enemyW[et], enemyH[et]))
						{
							bullets[i].active = false;
							enemies[j].active = false;
							score += 100;
							shotsHit++;
							break;
						}
					}
				}

				// check enemy-player collisions
				for (int i = 0; i < MAX_ENEMIES; i++)
				{
					if (!enemies[i].active) continue;
					int et = enemies[i].type;
					if (checkCollision(playerX, playerY, playerW, playerH,
						enemies[i].x, enemies[i].y, enemyW[et], enemyH[et]))
					{
						enemies[i].active = false;
						lives--;
						if (lives <= 0)
							state = GAME_OVER;
					}
				}
			}

			redraw = true;
		}

		if (redraw && al_is_event_queue_empty(event_queue))
		{
			redraw = false;
			al_clear_to_color(al_map_rgb(0, 0, 0));

			// stars on every screen
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
				// draw bullets
				for (int i = 0; i < MAX_BULLETS; i++)
				{
					if (bullets[i].active)
						al_draw_bitmap(bulletImg, bullets[i].x, bullets[i].y, 0);
				}

				// draw enemies
				for (int i = 0; i < MAX_ENEMIES; i++)
				{
					if (enemies[i].active)
						al_draw_bitmap(enemyImgs[enemies[i].type],
							enemies[i].x, enemies[i].y, 0);
				}

				// draw player
				al_draw_bitmap(playerImg, playerX, playerY, 0);
			}
			else if (state == GAME_OVER)
			{
				al_draw_text(fontLarge, al_map_rgb(255, 0, 0),
					SCREEN_W / 2, 200, ALLEGRO_ALIGN_CENTER, "GAME OVER");

				char buf[64];
				sprintf_s(buf, "Score: %d", score);
				al_draw_text(fontMed, al_map_rgb(255, 255, 255),
					SCREEN_W / 2, 320, ALLEGRO_ALIGN_CENTER, buf);
			}

			al_flip_display();
		}
	}

	al_destroy_bitmap(playerImg);
	al_destroy_bitmap(enemyImg1);
	al_destroy_bitmap(enemyImg2);
	al_destroy_bitmap(enemyImg3);
	al_destroy_bitmap(bulletImg);
	al_destroy_font(fontLarge);
	al_destroy_font(fontMed);
	al_destroy_font(fontSmall);
	al_destroy_timer(timer);
	al_destroy_event_queue(event_queue);
	al_destroy_display(display);

	return 0;
}
// Tyler Selwyn
// CPSC 440 - Game Programming
// Project 5 - Space Shooter

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <cmath>
#include <cstdio>
#include "SpriteSheet.h"

const int SCREEN_W = 800;
const int SCREEN_H = 600;
const int MAX_STARS = 100;
const int MAX_BULLETS = 10;
const int MAX_ENEMIES = 15;
const int MAX_EXPLOSIONS = 10;
const int MAX_POWERUPS = 5;
const float PLAYER_SPEED = 5.0;
const float BULLET_SPEED = 7.0;
const int FIRE_COOLDOWN = 15;
const int NUM_LEVELS = 3;

enum GameState { INTRO, PLAYING, LEVEL_TRANS, GAME_OVER, WIN };

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
	float angle;
	bool active;
	int type;
};

struct Explosion {
	float x, y;
	int frame;
	int frameTimer;
	bool active;
};

struct Powerup {
	float x, y;
	float scaleTimer;
	bool active;
};

// level config - spawn rate, base enemy speed, speed range, kills to advance
struct LevelConfig {
	int spawnRate;
	float baseSpeed;
	int speedRange;
	int killsNeeded;
};

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
	al_install_audio();
	al_init_acodec_addon();
	al_reserve_samples(10);

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
	ALLEGRO_BITMAP* powerupImg = al_load_bitmap("spaceParts_003.png");
	ALLEGRO_BITMAP* explosionSheet = al_load_bitmap("explosion_sheet.png");

	// load sounds
	ALLEGRO_SAMPLE* laserSound = al_load_sample("laser.wav");
	ALLEGRO_SAMPLE* explosionSound = al_load_sample("explosion.wav");
	ALLEGRO_SAMPLE* powerupSound = al_load_sample("powerup.wav");
	ALLEGRO_SAMPLE* musicSample = al_load_sample("music.wav");

	// explosion sprite sheet
	SpriteSheet explosionAnim;
	explosionAnim.init(explosionSheet, 128, 128, 9, 1);

	int playerW = al_get_bitmap_width(playerImg);
	int playerH = al_get_bitmap_height(playerImg);
	int bulletW = al_get_bitmap_width(bulletImg);
	int bulletH = al_get_bitmap_height(bulletImg);
	int powerupW = al_get_bitmap_width(powerupImg);
	int powerupH = al_get_bitmap_height(powerupImg);

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

	// level configs: each level gets harder
	LevelConfig levels[NUM_LEVELS] = {
		{ 60, 2.0, 3, 8 },   // level 1: slow spawns, slow enemies, 8 kills
		{ 40, 3.0, 3, 12 },  // level 2: faster spawns, faster enemies, 12 kills
		{ 25, 4.0, 4, 15 }   // level 3: fast spawns, fast enemies, 15 kills
	};

	// player state
	float playerX = SCREEN_W / 2 - playerW / 2;
	float playerY = SCREEN_H - playerH - 20;
	int lives = 3;
	int score = 0;
	int fireCooldown = 0;
	int shotsFired = 0;
	int shotsHit = 0;
	int hitFlash = 0;
	int level = 0; // index into levels array (0-2)
	int killsThisLevel = 0;
	int totalKills = 0;
	int gameFrames = 0;
	int transTimer = 0; // countdown for level transition screen

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

	Bullet bullets[MAX_BULLETS];
	for (int i = 0; i < MAX_BULLETS; i++)
		bullets[i].active = false;

	Enemy enemies[MAX_ENEMIES];
	for (int i = 0; i < MAX_ENEMIES; i++)
		enemies[i].active = false;

	Explosion explosions[MAX_EXPLOSIONS];
	for (int i = 0; i < MAX_EXPLOSIONS; i++)
		explosions[i].active = false;

	Powerup powerups[MAX_POWERUPS];
	for (int i = 0; i < MAX_POWERUPS; i++)
		powerups[i].active = false;

	int spawnTimer = 0;
	bool musicPlaying = false;
	ALLEGRO_SAMPLE_ID musicID;

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
			{
				// reset everything for a new game
				level = 0;
				killsThisLevel = 0;
				totalKills = 0;
				score = 0;
				lives = 3;
				shotsFired = 0;
				shotsHit = 0;
				gameFrames = 0;
				hitFlash = 0;
				fireCooldown = 0;
				spawnTimer = 0;
				playerX = SCREEN_W / 2 - playerW / 2;
				playerY = SCREEN_H - playerH - 20;

				for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
				for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
				for (int i = 0; i < MAX_EXPLOSIONS; i++) explosions[i].active = false;
				for (int i = 0; i < MAX_POWERUPS; i++) powerups[i].active = false;

				// show level 1 announcement first
				state = LEVEL_TRANS;
				transTimer = 120; // 2 seconds

				if (!musicPlaying)
				{
					al_play_sample(musicSample, 0.5, 0.0, 1.0,
						ALLEGRO_PLAYMODE_LOOP, &musicID);
					musicPlaying = true;
				}
			}

			// restart from end screens
			if ((state == GAME_OVER || state == WIN) && ev.keyboard.keycode == ALLEGRO_KEY_ENTER)
				state = INTRO;

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

			// level transition countdown
			if (state == LEVEL_TRANS)
			{
				transTimer--;
				if (transTimer <= 0)
					state = PLAYING;
			}

			if (state == PLAYING)
			{
				gameFrames++;

				// move player
				if (keyUp && playerY > 40)
					playerY -= PLAYER_SPEED;
				if (keyDown && playerY < SCREEN_H - playerH)
					playerY += PLAYER_SPEED;
				if (keyLeft && playerX > 0)
					playerX -= PLAYER_SPEED;
				if (keyRight && playerX < SCREEN_W - playerW)
					playerX += PLAYER_SPEED;

				if (hitFlash > 0) hitFlash--;

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
							al_play_sample(laserSound, 0.4, 0.0, 1.0,
								ALLEGRO_PLAYMODE_ONCE, NULL);
							break;
						}
					}
				}

				// move bullets
				for (int i = 0; i < MAX_BULLETS; i++)
				{
					if (bullets[i].active)
					{
						bullets[i].y -= BULLET_SPEED;
						if (bullets[i].y < -bulletH)
							bullets[i].active = false;
					}
				}

				// spawn enemies using current level config
				spawnTimer++;
				if (spawnTimer >= levels[level].spawnRate)
				{
					spawnTimer = 0;
					for (int i = 0; i < MAX_ENEMIES; i++)
					{
						if (!enemies[i].active)
						{
							enemies[i].type = rand() % 3;
							enemies[i].x = rand() % (SCREEN_W - enemyW[enemies[i].type]);
							enemies[i].y = -enemyH[enemies[i].type];
							enemies[i].speed = levels[level].baseSpeed + (rand() % levels[level].speedRange);
							enemies[i].angle = 0.0;
							enemies[i].active = true;
							break;
						}
					}
				}

				// move and rotate enemies
				for (int i = 0; i < MAX_ENEMIES; i++)
				{
					if (enemies[i].active)
					{
						enemies[i].y += enemies[i].speed;
						enemies[i].angle += 0.03;
						if (enemies[i].y > SCREEN_H)
							enemies[i].active = false;
					}
				}

				// update explosions
				for (int i = 0; i < MAX_EXPLOSIONS; i++)
				{
					if (explosions[i].active)
					{
						explosions[i].frameTimer++;
						if (explosions[i].frameTimer >= 4)
						{
							explosions[i].frameTimer = 0;
							explosions[i].frame++;
							if (explosions[i].frame >= 9)
								explosions[i].active = false;
						}
					}
				}

				// move powerups
				for (int i = 0; i < MAX_POWERUPS; i++)
				{
					if (powerups[i].active)
					{
						powerups[i].y += 2.0;
						powerups[i].scaleTimer += 0.08;
						if (powerups[i].y > SCREEN_H)
							powerups[i].active = false;
					}
				}

				// bullet-enemy collisions
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
							// explosion
							for (int k = 0; k < MAX_EXPLOSIONS; k++)
							{
								if (!explosions[k].active)
								{
									explosions[k].x = enemies[j].x + enemyW[et] / 2 - 64;
									explosions[k].y = enemies[j].y + enemyH[et] / 2 - 64;
									explosions[k].frame = 0;
									explosions[k].frameTimer = 0;
									explosions[k].active = true;
									break;
								}
							}

							al_play_sample(explosionSound, 0.5, 0.0, 1.0,
								ALLEGRO_PLAYMODE_ONCE, NULL);

							// chance to drop powerup
							if (rand() % 5 == 0)
							{
								for (int k = 0; k < MAX_POWERUPS; k++)
								{
									if (!powerups[k].active)
									{
										powerups[k].x = enemies[j].x + enemyW[et] / 2 - powerupW / 2;
										powerups[k].y = enemies[j].y;
										powerups[k].scaleTimer = 0.0;
										powerups[k].active = true;
										break;
									}
								}
							}

							bullets[i].active = false;
							enemies[j].active = false;
							score += 100;
							shotsHit++;
							totalKills++;
							killsThisLevel++;

							// check if level is complete
							if (killsThisLevel >= levels[level].killsNeeded)
							{
								killsThisLevel = 0;
								level++;

								// clear active enemies for clean transition
								for (int e = 0; e < MAX_ENEMIES; e++)
									enemies[e].active = false;

								if (level >= NUM_LEVELS)
								{
									// beat all 3 levels
									state = WIN;
									if (musicPlaying)
									{
										al_stop_sample(&musicID);
										musicPlaying = false;
									}
								}
								else
								{
									// show next level announcement
									state = LEVEL_TRANS;
									transTimer = 120;
									spawnTimer = 0;
								}
							}

							break;
						}
					}
				}

				// enemy-player collisions
				for (int i = 0; i < MAX_ENEMIES; i++)
				{
					if (!enemies[i].active) continue;
					int et = enemies[i].type;
					if (checkCollision(playerX, playerY, playerW, playerH,
						enemies[i].x, enemies[i].y, enemyW[et], enemyH[et]))
					{
						enemies[i].active = false;
						lives--;
						hitFlash = 30;
						al_play_sample(explosionSound, 0.6, 0.0, 1.0,
							ALLEGRO_PLAYMODE_ONCE, NULL);
						if (lives <= 0)
						{
							state = GAME_OVER;
							if (musicPlaying)
							{
								al_stop_sample(&musicID);
								musicPlaying = false;
							}
						}
					}
				}

				// powerup-player collisions
				for (int i = 0; i < MAX_POWERUPS; i++)
				{
					if (!powerups[i].active) continue;
					if (checkCollision(playerX, playerY, playerW, playerH,
						powerups[i].x, powerups[i].y, powerupW, powerupH))
					{
						powerups[i].active = false;
						lives++;
						score += 250;
						al_play_sample(powerupSound, 0.6, 0.0, 1.0,
							ALLEGRO_PLAYMODE_ONCE, NULL);
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
			else if (state == LEVEL_TRANS)
			{
				// level announcement screen
				char buf[32];
				sprintf_s(buf, "LEVEL %d", level + 1);
				al_draw_text(fontLarge, al_map_rgb(100, 255, 100),
					SCREEN_W / 2, 220, ALLEGRO_ALIGN_CENTER, buf);
				al_draw_text(fontSmall, al_map_rgb(200, 200, 200),
					SCREEN_W / 2, 310, ALLEGRO_ALIGN_CENTER, "Get Ready!");
			}
			else if (state == PLAYING)
			{
				// draw bullets
				for (int i = 0; i < MAX_BULLETS; i++)
				{
					if (bullets[i].active)
						al_draw_bitmap(bulletImg, bullets[i].x, bullets[i].y, 0);
				}

				// draw enemies with rotation
				for (int i = 0; i < MAX_ENEMIES; i++)
				{
					if (enemies[i].active)
					{
						int et = enemies[i].type;
						float cx = enemyW[et] / 2.0;
						float cy = enemyH[et] / 2.0;
						al_draw_rotated_bitmap(enemyImgs[et],
							cx, cy,
							enemies[i].x + cx, enemies[i].y + cy,
							enemies[i].angle, 0);
					}
				}

				// draw explosions
				for (int i = 0; i < MAX_EXPLOSIONS; i++)
				{
					if (explosions[i].active)
						explosionAnim.drawFrame(explosions[i].frame, 0,
							explosions[i].x, explosions[i].y);
				}

				// draw powerups with pulsing scale
				for (int i = 0; i < MAX_POWERUPS; i++)
				{
					if (powerups[i].active)
					{
						float scale = 1.0 + 0.3 * sin(powerups[i].scaleTimer);
						float scaledW = powerupW * scale;
						float scaledH = powerupH * scale;
						float drawX = powerups[i].x + powerupW / 2 - scaledW / 2;
						float drawY = powerups[i].y + powerupH / 2 - scaledH / 2;
						al_draw_scaled_bitmap(powerupImg,
							0, 0, powerupW, powerupH,
							drawX, drawY, scaledW, scaledH, 0);
					}
				}

				// draw player
				if (hitFlash > 0)
					al_draw_tinted_bitmap(playerImg,
						al_map_rgb(255, 100, 100),
						playerX, playerY, 0);
				else
					al_draw_bitmap(playerImg, playerX, playerY, 0);

				// status bar
				al_draw_filled_rectangle(0, 0, SCREEN_W, 35, al_map_rgba(0, 0, 0, 200));
				al_draw_line(0, 35, SCREEN_W, 35, al_map_rgb(100, 100, 255), 2);

				char buf[64];
				sprintf_s(buf, "Score: %d", score);
				al_draw_text(fontSmall, al_map_rgb(255, 255, 255),
					10, 8, 0, buf);

				sprintf_s(buf, "Lives: %d", lives);
				al_draw_text(fontSmall, al_map_rgb(255, 100, 100),
					250, 8, 0, buf);

				sprintf_s(buf, "Level: %d", level + 1);
				al_draw_text(fontSmall, al_map_rgb(100, 255, 100),
					450, 8, 0, buf);

				int seconds = gameFrames / 60;
				int mins = seconds / 60;
				int secs = seconds % 60;
				sprintf_s(buf, "Time: %d:%02d", mins, secs);
				al_draw_text(fontSmall, al_map_rgb(255, 255, 0),
					630, 8, 0, buf);
			}
			else if (state == GAME_OVER)
			{
				al_draw_text(fontLarge, al_map_rgb(255, 0, 0),
					SCREEN_W / 2, 100, ALLEGRO_ALIGN_CENTER, "GAME OVER");

				// game stats
				char buf[64];

				sprintf_s(buf, "Final Score: %d", score);
				al_draw_text(fontMed, al_map_rgb(255, 255, 255),
					SCREEN_W / 2, 200, ALLEGRO_ALIGN_CENTER, buf);

				sprintf_s(buf, "Enemies Destroyed: %d", totalKills);
				al_draw_text(fontSmall, al_map_rgb(200, 200, 200),
					SCREEN_W / 2, 260, ALLEGRO_ALIGN_CENTER, buf);

				int accuracy = (shotsFired > 0) ? (shotsHit * 100 / shotsFired) : 0;
				sprintf_s(buf, "Accuracy: %d%%", accuracy);
				al_draw_text(fontSmall, al_map_rgb(200, 200, 200),
					SCREEN_W / 2, 300, ALLEGRO_ALIGN_CENTER, buf);

				int seconds = gameFrames / 60;
				int mins = seconds / 60;
				int secs = seconds % 60;
				sprintf_s(buf, "Time Survived: %d:%02d", mins, secs);
				al_draw_text(fontSmall, al_map_rgb(200, 200, 200),
					SCREEN_W / 2, 340, ALLEGRO_ALIGN_CENTER, buf);

				sprintf_s(buf, "Reached Level: %d", level + 1);
				al_draw_text(fontSmall, al_map_rgb(200, 200, 200),
					SCREEN_W / 2, 380, ALLEGRO_ALIGN_CENTER, buf);

				al_draw_text(fontSmall, al_map_rgb(255, 255, 0),
					SCREEN_W / 2, 460, ALLEGRO_ALIGN_CENTER, "Press ENTER to Play Again");
			}
			else if (state == WIN)
			{
				al_draw_text(fontLarge, al_map_rgb(0, 255, 0),
					SCREEN_W / 2, 100, ALLEGRO_ALIGN_CENTER, "YOU WIN!");

				char buf[64];

				sprintf_s(buf, "Final Score: %d", score);
				al_draw_text(fontMed, al_map_rgb(255, 255, 255),
					SCREEN_W / 2, 200, ALLEGRO_ALIGN_CENTER, buf);

				sprintf_s(buf, "Enemies Destroyed: %d", totalKills);
				al_draw_text(fontSmall, al_map_rgb(200, 200, 200),
					SCREEN_W / 2, 260, ALLEGRO_ALIGN_CENTER, buf);

				int accuracy = (shotsFired > 0) ? (shotsHit * 100 / shotsFired) : 0;
				sprintf_s(buf, "Accuracy: %d%%", accuracy);
				al_draw_text(fontSmall, al_map_rgb(200, 200, 200),
					SCREEN_W / 2, 300, ALLEGRO_ALIGN_CENTER, buf);

				int seconds = gameFrames / 60;
				int mins = seconds / 60;
				int secs = seconds % 60;
				sprintf_s(buf, "Total Time: %d:%02d", mins, secs);
				al_draw_text(fontSmall, al_map_rgb(200, 200, 200),
					SCREEN_W / 2, 340, ALLEGRO_ALIGN_CENTER, buf);

				al_draw_text(fontSmall, al_map_rgb(100, 255, 100),
					SCREEN_W / 2, 380, ALLEGRO_ALIGN_CENTER, "All 3 Levels Cleared!");

				al_draw_text(fontSmall, al_map_rgb(255, 255, 0),
					SCREEN_W / 2, 460, ALLEGRO_ALIGN_CENTER, "Press ENTER to Play Again");
			}

			al_flip_display();
		}
	}

	if (musicPlaying) al_stop_sample(&musicID);
	al_destroy_sample(laserSound);
	al_destroy_sample(explosionSound);
	al_destroy_sample(powerupSound);
	al_destroy_sample(musicSample);
	al_destroy_bitmap(playerImg);
	al_destroy_bitmap(enemyImg1);
	al_destroy_bitmap(enemyImg2);
	al_destroy_bitmap(enemyImg3);
	al_destroy_bitmap(bulletImg);
	al_destroy_bitmap(powerupImg);
	al_destroy_bitmap(explosionSheet);
	al_destroy_font(fontLarge);
	al_destroy_font(fontMed);
	al_destroy_font(fontSmall);
	al_destroy_timer(timer);
	al_destroy_event_queue(event_queue);
	al_destroy_display(display);

	return 0;
}
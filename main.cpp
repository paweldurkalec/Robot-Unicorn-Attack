#include<math.h>
#include<stdio.h>
#include<string.h>

extern "C" {
#include"SDL2-2.0.10/include/SDL.h"
#include"SDL2-2.0.10/include/SDL_main.h"
}

//parametry opisujace wyswietlanie
#define SCREEN_WIDTH	1920
#define SCREEN_HEIGHT	1080
#define UNICORN_X 50 
#define UNICORN_Y SCREEN_HEIGHT/2
#define LEVEL_HEIGHT SCREEN_HEIGHT*2

//parametry opisujace rozgrywke
#define UNICORN_SPEED_Y 500
#define UNICORN_SPEED_X 400
#define WIDTH_OF_HOLE 700
#define X_ACCELERATION 20
#define GRAVITY_ACCELERATION 1000
#define JUMP_STRENGTH 800
#define JUMP_HOLD_LENGTH 2000 //(ms)
#define DASH_LENGTH 800	//(ms)
#define DASH_COOLDOWN_LENGTH 3000	//(ms)

//parametry techniczne opisujace ilosc poszczegolnych obiektow w grze
#define NUMBER_OF_OBJECTS 31
#define NUMBER_OF_OBSTACLES 16
#define NUMBER_OF_PLATFORMS 10

//obiekty w grze
//prawidlowa kolejnosc obiektow: tlo, jednorozec, podloga, platforma, przeszkoda
enum obj {
	BACKGROUND,
	BACKGROUND2,
	UNICORN,
	FLOOR,
	FLOOR2,
	PLATFORM,
	PLATFORM2,
	PLATFORM3,
	PLATFORM4,
	PLATFORM5,
	PLATFORM6,
	PLATFORM7,
	PLATFORM8,
	PLATFORM9,
	PLATFORM10,
	OBSTACLE,
	OBSTACLE2,
	OBSTACLE3,
	OBSTACLE4,
	OBSTACLE5,
	OBSTACLE6,
	OBSTACLE7,
	OBSTACLE8,
	OBSTACLE9,
	OBSTACLE10,
	OBSTACLE11,
	OBSTACLE12,
	OBSTACLE13
};

//typy obiektow
enum type {
	CHARACTER,
	GROUND,
	BACK,
	OBST
};

//podstawowe konstrukty SDL 
typedef struct baseTextures {
	SDL_Surface* screen = NULL, * charset = NULL;
	SDL_Texture* scrtex = NULL;
	SDL_Window* window = NULL;
	SDL_Renderer* renderer = NULL;
} baseTextures;

//obiekty wystepujace w grze
typedef struct objects {
	SDL_Surface* bmp;
	SDL_Rect box;
	double x = 0, y = 0, baseY;
	int type;
	int isStar = 0;
} objects;

typedef struct colors {
	int czarny;
	int zielony;
	int czerwony;
	int niebieski;
} colors;

//zmienne zawierajace dane o rozgrywce
typedef struct gamevars {
	int t1, t2, quit, frames, widthOfLevel, controlSwitch = 1, jumps = 0, dashUsed = 0, dashOnCooldown = 0, jumping = 0, hearths = 3, menu = 1, biggerQuit=0;
	double delta, worldTime, fpsTimer, fps, distanceX, distanceY, unicornSpeedX, unicornSpeedY, jumpAccelerationTimer = JUMP_HOLD_LENGTH;
	double  dashCooldownLength = DASH_COOLDOWN_LENGTH, dashLength = DASH_LENGTH, unicornRealY = LEVEL_HEIGHT - SCREEN_HEIGHT / 2;
	SDL_Surface* hearthBMP;
	SDL_Surface* menuBMP;
} gamevars;


// narysowanie napisu txt na powierzchni screen, zaczynajac od punktu (x, y)
// charset to bitmapa 128x128 zawierajaca znaki
void DrawString(SDL_Surface* screen, int x, int y, const char* text,
	SDL_Surface* charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};


// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt srodka obrazka sprite na ekranie
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};


// rysowanie pojedynczego pixela
// draw a single pixel
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32*)p = color;
};


// rysowanie linii o dlugosci l w pionie (gdy dx = 0, dy = 1) 
// badz poziomie (gdy dx = 1, dy = 0)
// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for (int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};


// rysowanie prostokata o dlugosci boków l i k
// draw a rectangle of size l by k
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k,
	Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

//inicjowanie rozgrywki poprzez odpowiednie funkcje SDL
int initGame(baseTextures& baseTextures) {
	int rc;
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}
	//rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
		//&baseTextures.window, &baseTextures.renderer);
	// tryb pelnoekranowy / fullscreen mode
	rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP,
		&baseTextures.window, &baseTextures.renderer);
	if (rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	};
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(baseTextures.renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(baseTextures.renderer, 0, 0, 0, 255);

	SDL_SetWindowTitle(baseTextures.window, "Robot unicorn attack");


	baseTextures.screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	baseTextures.scrtex = SDL_CreateTexture(baseTextures.renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);

	// wylaczenie widocznosci kursora myszy
	SDL_ShowCursor(SDL_DISABLE);
	return 0;
}

//wczytanie kolorow
void renderColors(colors* colors, baseTextures baseTextures) {
	(*colors).czarny = SDL_MapRGB(baseTextures.screen->format, 0x00, 0x00, 0x00);
	(*colors).zielony = SDL_MapRGB(baseTextures.screen->format, 0x00, 0xFF, 0x00);
	(*colors).czerwony = SDL_MapRGB(baseTextures.screen->format, 149, 59, 230);
	(*colors).niebieski = SDL_MapRGB(baseTextures.screen->format, 59, 230, 230);
}

//zainicjowanie zmiennych przechowujacych stan gry
void initGamevars(gamevars* gamevars) {
	gamevars->jumps = 0;
	gamevars->dashUsed = 0;
	gamevars->dashOnCooldown = 0;
	gamevars->jumping = 0;
	gamevars->menu = 1;
	gamevars->t1 = SDL_GetTicks();
	gamevars->frames = 0;
	gamevars->fpsTimer = 0;
	gamevars->fps = 0;
	gamevars->quit = 0;
	gamevars->worldTime = 0;
	gamevars->distanceX = 0;
	gamevars->distanceY = 0;
	gamevars->unicornSpeedX = 0.0;
	gamevars->unicornSpeedY = 0.0;
	gamevars->unicornRealY = LEVEL_HEIGHT - SCREEN_HEIGHT / 2;
}

//zwolnienie pamieci aby zakonczyc rozgrywke
void destroyEverything(baseTextures* baseTextures, objects objects[], gamevars* gamevars) {
	int count = NUMBER_OF_OBJECTS;
	SDL_FreeSurface(baseTextures->charset);
	SDL_FreeSurface(baseTextures->screen);
	SDL_FreeSurface(gamevars->hearthBMP);
	SDL_FreeSurface(gamevars->menuBMP);
	SDL_DestroyTexture(baseTextures->scrtex);
	SDL_DestroyWindow(baseTextures->window);
	SDL_DestroyRenderer(baseTextures->renderer);
	int i;
	for (i = 0; i < count; i++) {
		SDL_FreeSurface(objects[i].bmp);
	}
	SDL_Quit();
}

//aktualizacja pozycji prostokatow bedacych hitboxami
void updateRects(objects objects[]) {
	int count = NUMBER_OF_OBJECTS;
	int i;
	for (i = 0; i < count; i++) {
		objects[i].box.x = objects[i].x - (objects[i].bmp->w / 2);
		objects[i].box.y = objects[i].y - (objects[i].bmp->h / 2);
		if (objects[i].box.h != objects[i].bmp->h) {
			objects[i].box.h = objects[i].bmp->h;
			objects[i].box.w = objects[i].bmp->w;
		}
	}
}

//funkcje odpowiadajace za wczytanie odpowiednich textur, oraz wartosci opisujacych wyglad poziomu
int loadCs8x8(baseTextures* baseTextures, objects objects[]) {
	// wczytanie obrazka cs8x8.bmp
	baseTextures->charset = SDL_LoadBMP("cs8x8.bmp");
	if (baseTextures->charset == NULL) {
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		return 1;
	};
	SDL_SetColorKey(baseTextures->charset, true, 0x000000);
	return 0;
}

int loadUnicorn(baseTextures* baseTextures, objects objects[]) {
	objects[obj{ UNICORN }].bmp = SDL_LoadBMP("unicorn.bmp");
	objects[UNICORN].y = UNICORN_Y;
	objects[UNICORN].baseY= UNICORN_Y;
	objects[UNICORN].x = UNICORN_X;
	objects[UNICORN].type = CHARACTER;
	if (objects[obj{ UNICORN }].bmp == NULL) {
		printf("SDL_LoadBMP(unicorn.bmp) error: %s\n", SDL_GetError());
		return 1;
	}
	return 0;
}

int loadBackground(baseTextures* baseTextures, objects objects[]) {
	objects[BACKGROUND].bmp = SDL_LoadBMP("background.bmp");
	objects[BACKGROUND].y = SCREEN_HEIGHT - 600;
	objects[BACKGROUND].x = 640;
	objects[BACKGROUND].type = BACK;
	if (objects[BACKGROUND].bmp == NULL) {
		printf("SDL_LoadBMP(background.bmp) error: %s\n", SDL_GetError());
		return 1;
	}
	objects[BACKGROUND2].bmp = SDL_LoadBMP("background.bmp");
	objects[BACKGROUND2].y = objects[BACKGROUND].y;
	objects[BACKGROUND2].x = objects[BACKGROUND].x + 1915;
	objects[BACKGROUND].type = BACK;
	return 0;
}

int loadFloor(baseTextures* baseTextures, objects objects[]) {
	objects[FLOOR].bmp = SDL_LoadBMP("floor.bmp");
	objects[FLOOR].y = SCREEN_HEIGHT - 70;
	objects[FLOOR].baseY = SCREEN_HEIGHT - 70;
	objects[FLOOR].x = objects[BACKGROUND].x;
	objects[FLOOR].type = GROUND;
	if (objects[FLOOR].bmp == NULL) {
		printf("SDL_LoadBMP(floor.bmp) error: %s\n", SDL_GetError());
		return 1;
	}
	objects[FLOOR2].bmp = SDL_LoadBMP("floor.bmp");
	objects[FLOOR2].y = objects[FLOOR].y;
	objects[FLOOR2].baseY = objects[FLOOR].y;
	objects[FLOOR2].x = objects[FLOOR].x + 1920 + WIDTH_OF_HOLE;
	objects[FLOOR2].type = GROUND;
	return 0;
}

int loadObstacles(objects objects[]) {
	int obst1y = SCREEN_HEIGHT - 220;
	int obst2y = SCREEN_HEIGHT - 157;
	int obst3y = SCREEN_HEIGHT - 232;
	int i;
	int temp = 0;
	int xes[NUMBER_OF_OBSTACLES] = { 1000, 2500, 3000, 4300, 7000, 9800, 10500, 12000, 13000, 2500, 7000, 11600, 1000, 5500, 9000, 15000};
	int ys[NUMBER_OF_OBSTACLES] = { obst2y, obst1y, obst2y, obst3y, obst2y, obst3y, obst1y, obst3y, obst1y, 245, -155, -5, -145, 5, 255, obst2y};
	char paths[NUMBER_OF_OBSTACLES][100] = { "obstacle2.bmp", "obstacle1.bmp", "obstacle2.bmp", "obstacle3.bmp",
		"obstacle2.bmp", "obstacle3.bmp", "obstacle1.bmp", "obstacle3.bmp", "obstacle1.bmp", "stalactite.bmp", 
		"stalactite.bmp", "stalactite.bmp", "star.bmp", "star.bmp", "star.bmp", "obstacle2.bmp" };
	for (i = (NUMBER_OF_OBJECTS - NUMBER_OF_OBSTACLES); i < NUMBER_OF_OBJECTS; i++) {
		objects[i].x = xes[temp];
		objects[i].y = ys[temp];
		objects[i].baseY = objects[i].y;
		objects[i].type = OBST;
		if (strcmp(paths[temp], "star.bmp")==0) {
			objects[i].isStar = 1;
		}
		objects[i].bmp = SDL_LoadBMP(paths[temp]);
		if (objects[i].bmp == NULL) {
			printf("SDL_LoadBMP(obstacleX.bmp) error: %s\n", SDL_GetError());
			return 1;
		}
		temp++;
	}
	return 0;
}

int loadPlatforms(baseTextures* baseTextures, objects objects[]) {
	int i;
	int temp = 0;
	int xes[NUMBER_OF_PLATFORMS] = { 1000, 2500, 3000, 5000, 5500, 7000, 9000, 11000, 11600, 13000};
	int ys[NUMBER_OF_PLATFORMS] = { 0, 100, -300, 200, 150, -300, 400, -100, -150, -300};
	for (i = (NUMBER_OF_OBJECTS - NUMBER_OF_OBSTACLES - NUMBER_OF_PLATFORMS); i < NUMBER_OF_OBJECTS; i++) {
		objects[i].type = GROUND;
		objects[i].x = xes[temp];
		objects[i].y = ys[temp];
		objects[i].baseY = ys[temp];
		objects[i].bmp = SDL_LoadBMP("platform.bmp");
		if (objects[i].bmp == NULL) {
			printf("SDL_LoadBMP(platform.bmp) error: %s\n", SDL_GetError());
			return 1;
		}
		temp++;
	}
	return 0;
}

int loadHearths(baseTextures* baseTextures, gamevars* gamevars) {

	if (gamevars->hearths == 3) {
		gamevars->hearthBMP = SDL_LoadBMP("hearth3.bmp");
	}
	else if (gamevars->hearths == 2) {
		gamevars->hearthBMP = SDL_LoadBMP("hearth2.bmp");
	}
	else if (gamevars->hearths == 1) {
		gamevars->hearthBMP = SDL_LoadBMP("hearth1.bmp");
	}

	if (gamevars->hearthBMP == NULL) {
		printf("SDL_LoadBMP(hearthx.bmp) error: %s\n", SDL_GetError());
		return 1;
	}
	return 0;
}

int loadMenu(baseTextures* baseTextures, gamevars* gamevars) {
	gamevars->menuBMP = SDL_LoadBMP("menu.bmp");
	if (gamevars->menuBMP == NULL) {
		printf("SDL_LoadBMP(menu.bmp) error: %s\n", SDL_GetError());
		return 1;
	}
	return 0;
}

//funkcja uruchamiajace powyzsze funkcje wczytujace
int loadObjects(baseTextures* baseTextures, objects objects[], gamevars* gamevars) {

	//wczytanie czcionki
	if (loadCs8x8(baseTextures, objects)) {
		destroyEverything(baseTextures, objects, gamevars);
		return 1;
	}

	// wczytanie jednorozca
	if (loadUnicorn(baseTextures, objects)) {
		destroyEverything(baseTextures, objects, gamevars);
		return 1;
	}

	// wczytanie tla
	if (loadBackground(baseTextures, objects)) {
		destroyEverything(baseTextures, objects, gamevars);
		return 1;
	}

	// wczytanie podloza
	if (loadFloor(baseTextures, objects)) {
		destroyEverything(baseTextures, objects, gamevars);
		return 1;
	}

	// wczytanie platform
	if (loadPlatforms(baseTextures, objects)) {
		destroyEverything(baseTextures, objects, gamevars);
		return 1;
	}

	// wczytanie przeszkod
	if (loadObstacles(objects)) {
		destroyEverything(baseTextures, objects, gamevars);
		return 1;
	}

	//wczytanie serc
	if (loadHearths(baseTextures, gamevars)) {
		destroyEverything(baseTextures, objects, gamevars);
		return 1;
	}

	//wczytanie menu
	if (loadMenu(baseTextures, gamevars)) {
		destroyEverything(baseTextures, objects, gamevars);
		return 1;
	}
	//szerokosc poziomu to pozioma wspolrzedna ostatniej przeszkody
	gamevars->widthOfLevel = objects[NUMBER_OF_OBJECTS - 1].x;

	updateRects(objects);

	return 0;
}

void restartGame(gamevars* gamevars, objects objects[], baseTextures baseTextures, int speedX) {
	int count = NUMBER_OF_OBJECTS, i;
	for (i = 0; i < count; i++) {
		SDL_FreeSurface(objects[i].bmp);
	}
	initGamevars(gamevars);
	gamevars->unicornSpeedX = speedX;
	loadObjects(&baseTextures, objects, gamevars);
}

//narysowanie obiektow objects na ekran
void drawObjects(baseTextures baseTextures, gamevars gamevars, objects objects[]) {
	int counter = NUMBER_OF_OBJECTS, i;
	for (i = 0; i < counter; i++) {
		DrawSurface(baseTextures.screen, objects[i].bmp, objects[i].x, objects[i].y);
	}
}

//narysowanie wszystkiego 
void draw(baseTextures baseTextures, colors colors, gamevars gamevars, objects objects[]) {

	SDL_FillRect(baseTextures.screen, NULL, colors.czarny);

	//obiekty
	drawObjects(baseTextures, gamevars, objects);

	//serca
	DrawSurface(baseTextures.screen, gamevars.hearthBMP, 80, 30);

	// tekst informacyjny
	char text[128];
	DrawRectangle(baseTextures.screen, SCREEN_WIDTH / 4, 4, SCREEN_WIDTH/2, SCREEN_WIDTH/75, colors.czerwony, colors.niebieski);
	sprintf(text, "Robot unicorn attack, czas trwania = %.1lf s  %.0lf klatek / s", gamevars.worldTime, gamevars.fps);
	DrawString(baseTextures.screen, baseTextures.screen->w / 2 - strlen(text) * 8 / 2, 10, text, baseTextures.charset);

	SDL_UpdateTexture(baseTextures.scrtex, NULL, baseTextures.screen->pixels, baseTextures.screen->pitch);
	SDL_RenderCopy(baseTextures.renderer, baseTextures.scrtex, NULL, NULL);
	SDL_RenderPresent(baseTextures.renderer);
}

//aktualizuje liczbe klatek na sekunde oraz oblicza czas od poprzedniej klatki
void handleTimeAndFPS(gamevars* gamevars) {
	gamevars->t2 = SDL_GetTicks();

	// w tym momencie t2-t1 to czas w milisekundach,
	// jaki uplynal od ostatniego narysowania ekranu
	// delta to ten sam czas w sekundach
	gamevars->delta = (gamevars->t2 - gamevars->t1) * 0.001;
	gamevars->t1 = gamevars->t2;
	gamevars->worldTime += gamevars->delta;

	gamevars->fpsTimer += gamevars->delta;
	if (gamevars->fpsTimer > 0.5) {
		gamevars->fps = gamevars->frames * 2;
		gamevars->frames = 0;
		gamevars->fpsTimer -= 0.5;
	};
}

//obsluguje klawiature podczas gry
void handleEvents(SDL_Event* event, gamevars* gamevars, objects objects[], baseTextures baseTextures) {
	while (SDL_PollEvent(event)) {
		switch (event->type) {
		case SDL_KEYDOWN:
			if (gamevars->controlSwitch == 1) {
				//sterowanie uproszczone
				if (event->key.keysym.sym == SDLK_DOWN) gamevars->unicornSpeedY = UNICORN_SPEED_Y;
				else if (event->key.keysym.sym == SDLK_UP) gamevars->unicornSpeedY = -UNICORN_SPEED_Y;
				else if (event->key.keysym.sym == SDLK_RIGHT) gamevars->unicornSpeedX = UNICORN_SPEED_X;
			}
			else {
				//sterowanie docelowe
				if (event->key.keysym.sym == SDLK_z) {
					if ((gamevars->jumps < 2) && (gamevars->jumping == 0)) {
						gamevars->unicornSpeedY = -JUMP_STRENGTH;
						gamevars->jumps += 1;
						gamevars->jumping = 1;
					}
				}
				if (event->key.keysym.sym == SDLK_x) {
					if ((gamevars->dashUsed == 0) && (gamevars->dashOnCooldown == 0)) {
						gamevars->dashUsed = 1;
					}
				}
			}
			break;
		case SDL_KEYUP:
			//sterowanie uproszczone
			if (gamevars->controlSwitch == 1) {
				if (event->key.keysym.sym == SDLK_n) {
					gamevars->hearths = 3;
					restartGame(gamevars, objects, baseTextures, 0);
				}
				else if (event->key.keysym.sym == SDLK_ESCAPE) {
					gamevars->quit = 1;
					gamevars->hearths = 0;
				}
				else if (event->key.keysym.sym == SDLK_DOWN) gamevars->unicornSpeedY = 0.0;
				else if (event->key.keysym.sym == SDLK_UP) gamevars->unicornSpeedY = 0.0;
				else if (event->key.keysym.sym == SDLK_RIGHT) gamevars->unicornSpeedX = 0.0;
				else if (event->key.keysym.sym == SDLK_d) {
					gamevars->controlSwitch = 0;
					gamevars->unicornSpeedX = UNICORN_SPEED_X;
				}
			}
			else {
				//sterowanie docelowe
				if (event->key.keysym.sym == SDLK_n) {
					gamevars->hearths = 3;
					restartGame(gamevars, objects, baseTextures, UNICORN_SPEED_X);
				}
				else if (event->key.keysym.sym == SDLK_ESCAPE) {
					gamevars->quit = 1;
					gamevars->hearths = 0;
				}
				else if (event->key.keysym.sym == SDLK_d) {
					gamevars->controlSwitch = 1;
					gamevars->unicornSpeedX = 0;
					gamevars->unicornSpeedY = 0;
				}
				else if (event->key.keysym.sym == SDLK_z) {
					gamevars->jumping = 0;
					gamevars->jumpAccelerationTimer = JUMP_HOLD_LENGTH;
				}
			}
			break;
		case SDL_QUIT:
			gamevars->hearths = 0;
			gamevars->quit = 1;
			break;
		}
	}
	gamevars->frames++;
}

//zwraca 1 jezeli jednorozec dotyka podloza
int checkCollisionWithGround(gamevars* gamevars, objects objects[]) {
	updateRects(objects);
	int counter = NUMBER_OF_OBJECTS, i;
	for (i = 0; i < counter; i++) {
		if (SDL_HasIntersection(&objects[UNICORN].box, &objects[i].box) && (objects[i].type == GROUND)) {
			return 1;
		}
	}
	return 0;
}

//zwraca 1 jezeli trzymanie przycisku z dalej powinno powodowac wzlot jednorozca
int isInJumpAcceleration(gamevars* gamevars) {
	if ((gamevars->jumping) && (gamevars->jumpAccelerationTimer > 0)) {
		return 1;
	}
	else return 0;
}

//obsluguje kolizje jednorozca z gruntem
void handleGroundCollision(gamevars* gamevars, objects objects[]) {
	int i;
	//jezeli dotyka podloza
	if (checkCollisionWithGround(gamevars, objects)) {

		gamevars->unicornSpeedY = 0;

		//w takim wypadku neguje wszelkie przesuniecia jednorozca lub obiektow wzdluz osi Y

		//jezeli jednorozec znajduje sie gdzies na srodku poziomu
		if (((gamevars->unicornRealY + gamevars->distanceY) <= LEVEL_HEIGHT - SCREEN_HEIGHT / 2) && ((gamevars->unicornRealY + gamevars->distanceY) >= SCREEN_HEIGHT / 2)) {
			for (i = 3; i < NUMBER_OF_OBJECTS; i++) {
				objects[i].y += gamevars->distanceY;
			}
			gamevars->unicornRealY += -gamevars->distanceY;
		}
		//jezeli jednorozec znajduje sie na gorze lub dole poziomu
		else {
			gamevars->unicornRealY += -gamevars->distanceY;
			objects[UNICORN].y += -gamevars->distanceY;
		}
		updateRects(objects);
		gamevars->jumps = 0;
	}
}

//odpowiada za przemieszczanie jednorozca (ew. obiektow - zaleznie od kamery) wzdluz osi Y
void moveUnicornY(gamevars* gamevars, objects objects[]) {
	int i;
	//jezeli jednorozec znajduje sie gdzies na srodku poziomu
	if ((gamevars->unicornRealY <= LEVEL_HEIGHT - SCREEN_HEIGHT / 2) && (gamevars->unicornRealY >= SCREEN_HEIGHT / 2)) {
		for (i = 3; i < NUMBER_OF_OBJECTS; i++) {
			objects[i].y += -gamevars->distanceY;
		}
		gamevars->unicornRealY += gamevars->distanceY;
		objects[UNICORN].y = UNICORN_Y;
	}
	//jezeli jednorozec znajduje sie na gorze lub dole poziomu
	else {
		if (gamevars->unicornRealY > LEVEL_HEIGHT / 2) {
			for (i = 3; i < NUMBER_OF_OBJECTS; i++) {
				objects[i].y = objects[i].baseY;
			}
		}
		if (gamevars->unicornRealY + gamevars->distanceY > 100) {
			gamevars->unicornRealY += gamevars->distanceY;
			objects[UNICORN].y += gamevars->distanceY;
		}
	}
}

void basePhysicsMovement(gamevars* gamevars, objects objects[]) {

	//zmienia przyciaganie grawitacyjne zaleznie czy trzymany jest z oraz dodaje iloczyn przyspieszenia i czasu do predkosci jednorozca wzdluz osi Y
	if (isInJumpAcceleration(gamevars)) {
		gamevars->unicornSpeedY += (((double)(GRAVITY_ACCELERATION)) * gamevars->delta) / 2;
	}
	else {
		gamevars->unicornSpeedY += ((double)(GRAVITY_ACCELERATION)) * gamevars->delta;
	}

	moveUnicornY(gamevars, objects);

	handleGroundCollision(gamevars, objects);
}

//obsluga przemieszczenia jednorozca w trakcie dashowania
void dashMovement(gamevars* gamevars, objects objects[]) {
	static double savedSpeed;

	static int isFirstTimeHere = 1;
	gamevars->unicornSpeedY = 0;

	//zapisuje predkosc jednorozca na poczatku dasha aby mu ja potem przywrocic
	if (isFirstTimeHere) {
		isFirstTimeHere = 0;
		savedSpeed = gamevars->unicornSpeedX;
	}
	//podwaja predkosc jednorozca wzdluz Y oraz oblicza czas jaki juz dashuje
	gamevars->unicornSpeedX = 2 * savedSpeed;
	gamevars->dashLength += -(gamevars->delta * 1000);

	//obsluga konca dasha (zleci timer)
	if (gamevars->dashLength <= 0) {
		gamevars->dashLength = DASH_LENGTH;
		gamevars->dashOnCooldown = 1;
		gamevars->dashUsed = 0;
		gamevars->unicornSpeedX = gamevars->unicornSpeedX;
		isFirstTimeHere = 1;
		gamevars->unicornSpeedX = savedSpeed;
		if (gamevars->jumps > 0) {
			gamevars->jumps -= 1;
		}
	}
}

//obsluga przemieszczenia jednorozca jezeli zostalo wybrane "fizyczne" sterowanie
void moveUnicornWithPhisics(gamevars* gamevars, objects objects[]) {

	if ((gamevars->dashUsed == 1)) {
		dashMovement(gamevars, objects);
	}
	else {
		basePhysicsMovement(gamevars, objects);
	}

	//obsluga czasu odnowienia dasha
	if (gamevars->dashOnCooldown == 1) {
		gamevars->dashCooldownLength += -(gamevars->delta * 1000);
		if (gamevars->dashCooldownLength <= 0) {
			gamevars->dashCooldownLength = DASH_COOLDOWN_LENGTH;
			gamevars->dashOnCooldown = 0;
		}
	}

	//obsluga czasu trzymania "z" podczas skoku
	if (gamevars->jumping == 1) {
		if (gamevars->jumpAccelerationTimer >= 0) {
			gamevars->jumpAccelerationTimer += -(gamevars->delta * 1000);
		}
	}
}

//porusza obiekty wzdlug osi X w lewo zgodnie z predkoscia jednorozca 
void moveMap(gamevars* gamevars, objects objects[]) {
	int counter = NUMBER_OF_OBJECTS, i;
	for (i = 0; i < counter; i++) {
		if (i != UNICORN) {
			objects[i].x += -gamevars->distanceX;
		}
	}
	
	//jezeli jednorozec uderzy pionowo w podloze (platforme) traci zycie
	if (checkCollisionWithGround(gamevars, objects)) {
		gamevars->quit = 1;
	}

}
//obsluga nieskonczonej petli poziomu
void generateLoop(gamevars* gamevars, objects objects[]) {

	//przeniesienie tla
	if (objects[BACKGROUND].x <= -960) {
		objects[BACKGROUND].x = objects[BACKGROUND2].x + 1915;
	}
	else if (objects[BACKGROUND2].x <= -960) {
		objects[BACKGROUND2].x = objects[BACKGROUND].x + 1915;
	}

	//przeniesienie podlogi
	if (objects[FLOOR].x <= -960) {
		objects[FLOOR].x = objects[FLOOR2].x + objects[FLOOR2].bmp->w;
	}
	else if (objects[FLOOR2].x <= -960) {
		objects[FLOOR2].x = objects[FLOOR].x + objects[FLOOR].bmp->w + WIDTH_OF_HOLE;
	}

	//przeniesienie przeszkod
	int counter = NUMBER_OF_OBJECTS, i;
	for (i = 0; i < counter; i++) {
		if (((objects[i].type == OBST) || (objects[i].type == GROUND)) && (objects[i].x <= -960)) {
			if (i >= NUMBER_OF_OBJECTS - NUMBER_OF_OBSTACLES - NUMBER_OF_PLATFORMS) {
				objects[i].x = gamevars->widthOfLevel - 700;
			}
		}
	}
}

//obsluga uproszczonego poruszania jednorozcem
void moveUnicornBase(gamevars* gamevars, objects objects[]) {

	int i = 0;

	moveUnicornY(gamevars, objects);

	handleGroundCollision(gamevars, objects);
}

//obsluga wszelkiego ruchu obiektow
void move(gamevars* gamevars, objects objects[]) {

	//aktualizacja dystansu pokonanego od ostatniej klatki
	gamevars->distanceY = gamevars->unicornSpeedY * gamevars->delta;
	gamevars->distanceX = gamevars->unicornSpeedX * gamevars->delta;

	//przemieszczenie obiektow zaleznie od wybranego sterowania 
	if (gamevars->controlSwitch == 1) {
		moveUnicornBase(gamevars, objects);
	}
	else {
		//doliczenie przyspieszenia wzdluz osi X przed wejsciem do funkcji odpowiadajacej za ruch z "fizyka"
		gamevars->unicornSpeedX += X_ACCELERATION * gamevars->delta;
		moveUnicornWithPhisics(gamevars, objects);
	}
	moveMap(gamevars, objects);
	generateLoop(gamevars, objects);
	updateRects(objects);
}

//sprawdza potencjalne zderzenie jednorozca z przeszkoda
void checkHitboxes(gamevars* gamevars, objects objects[]) {

	int counter = NUMBER_OF_OBJECTS, i;

	for (i = 0; i < counter; i++) {
		if (SDL_HasIntersection(&objects[UNICORN].box, &objects[i].box) && (objects[i].type == OBST)) {
			//jezeli jednorozec jest w trakcie dasha to nie traci zycia od uderzenia w gwiazde
			if (!(gamevars->dashUsed == 1 && objects[i].isStar == 1)) {
				gamevars->quit = 1;
			}
		}
	}
	
	//jezeli wpadnie w dziure w ziemi traci zycie
	if (objects[UNICORN].y > SCREEN_HEIGHT) {
		gamevars->quit = 1;
	}
}

//obsluga zyc jednorozca
void handleHearths(gamevars* gamevars, baseTextures baseTextures, objects objects[]) {

	if (gamevars->quit == 1) {

		gamevars->hearths--;

		//jezeli ma jeszcze zycia to restartuje rozgrywke
		if ((gamevars->quit == 1) && (gamevars->hearths > 0)) {
			gamevars->quit = 0;
		}


		if (gamevars->quit == 0) {
			//nadaje mu predkosc wzdluz osi X po restarcie lub nie, zaleznie od wybranego sterowania 
			if (gamevars->controlSwitch == 1) {
				restartGame(gamevars, objects, baseTextures, 0);
			}
			else {
				restartGame(gamevars, objects, baseTextures, UNICORN_SPEED_X);
			}
		}

	}
}

//rysowanie menu
void drawMenu(gamevars gamevars, baseTextures baseTextures, colors colors) {

	SDL_FillRect(baseTextures.screen, NULL, colors.czarny);

	DrawSurface(baseTextures.screen, gamevars.menuBMP, SCREEN_WIDTH/2, SCREEN_HEIGHT/2);

	SDL_UpdateTexture(baseTextures.scrtex, NULL, baseTextures.screen->pixels, baseTextures.screen->pitch);
	SDL_RenderCopy(baseTextures.renderer, baseTextures.scrtex, NULL, NULL);
	SDL_RenderPresent(baseTextures.renderer);

}

//obsluga przyciskow esc i enter podczas "przebywania" w menu
void handleMenuEvents(SDL_Event* event, gamevars* gamevars) {
	while (SDL_PollEvent(event)) {
		switch (event->type) {
		case SDL_KEYUP:
			if (event->key.keysym.sym == SDLK_ESCAPE) {
				gamevars->biggerQuit = 1;
			}
			else if (event->key.keysym.sym == 13) {
				gamevars->menu = 0;
			}
			break;
		case SDL_QUIT:
			break;
		}
	}
}

//obsluga menu
void menu(gamevars* gamevars, baseTextures baseTextures, colors colors, SDL_Event* event) {

	drawMenu(*gamevars, baseTextures, colors);

	handleMenuEvents(event, gamevars);

}

//obsluga rozgrywki
void game(gamevars* gamevars, baseTextures baseTextures, colors colors, SDL_Event* event, objects objects[]) {

	handleTimeAndFPS(gamevars);

	draw(baseTextures, colors, *gamevars, objects);

	handleEvents(event, gamevars, objects, baseTextures);

	move(gamevars, objects);

	checkHitboxes(gamevars, objects);


	handleHearths(gamevars, baseTextures, objects);

}

// main
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char** argv) {
	colors colors;
	gamevars gamevars;
	SDL_Event event;
	baseTextures baseTextures;
	objects objects[NUMBER_OF_OBJECTS];

	//przygotowanie rozgrywki
	if (initGame(baseTextures) == 1) {
		return 1;
	}

	if (loadObjects(&baseTextures, objects, &gamevars) == 1) {
		return 1;
	}

	renderColors(&colors, baseTextures);

	initGamevars(&gamevars);

	//petla z rozgrywka 
	while (!gamevars.biggerQuit) {
		if (gamevars.menu == 1) {
			menu(&gamevars, baseTextures, colors, &event);
		}
		else {
			while (!gamevars.quit) {
				game(&gamevars, baseTextures, colors, &event, objects);
			}
			gamevars.hearths = 3;
			gamevars.controlSwitch = 1;
			restartGame(&gamevars, objects, baseTextures, 0);
		}
	}

	//zwolnienie pamieci
	destroyEverything(&baseTextures, objects, &gamevars);

	return 0;
};

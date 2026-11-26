#include "Game.h"
#include "Draw.h"


Game::Game(SDL_Renderer* renderer) : renderer(renderer)
{
	canvas = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, GAME_W, GAME_H);
	if (!canvas) {
		SDL_Log("No se puedo creasr el lienzo: %s", SDL_GetError());
		running = false;
		return;
	}

	SDL_SetTextureScaleMode(canvas, SDL_SCALEMODE_NEAREST); // pixeles nitidos al ampliar

	for (int i = 0; i < NUM_LAUNCHERS; i++) launchers[i].x = LAUNCHER_X[i];
	for (int i = 0; i < NUM_DOMES; i++) domes[i].x = DOME_X[i];
}

Game::~Game() {
	if (canvas) SDL_DestroyTexture(canvas);
}

void Game::handleEvent(const SDL_Event& event) {
	if (event.type == SDL_EVENT_QUIT) running = false;
	if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) running = false;
}

void Game::update(float dt) {

}

void Game::render() {

	SDL_SetRenderTarget(renderer, canvas);
	setColor(renderer, Pal::SPACE);
	SDL_RenderClear(renderer);

	drawGround();
	drawLaunchers();
	drawDomes();


	// 2. Volver a la ventana y estampar el lienzo ampliado
	SDL_SetRenderTarget(renderer, nullptr);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_RenderTexture(renderer, canvas, nullptr, nullptr);
	SDL_RenderPresent(renderer);
}


void Game::drawGround() {
	setColor(renderer, Pal::MOON_GRAY);
	fillRect(renderer, 0.0f, GROUND_Y, (float)GAME_W, GAME_H - GROUND_Y);
	setColor(renderer, Pal::LIGHT_GRAY);
	fillRect(renderer, 0.0f, GROUND_Y, (float)GAME_W, 1.0f); // borde iluminado
}


void Game::drawLaunchers() {
	for (const Launcher& l : launchers) {
		// la colina
		setColor(renderer, Pal::MOON_GRAY);
		fillTrapezoid(renderer, l.x, HILL_TOP_Y, GROUND_Y, HILL_TOP_HALF_W, HILL_BOTTOM_HALF_W);
		setColor(renderer, Pal::LIGHT_GRAY);
		fillRect(renderer, l.x - HILL_TOP_HALF_W, HILL_TOP_Y, HILL_TOP_HALF_W * 2.0f + 1.0f, 1.0f);

		// La torreta: una cúpula pequeña y un cañón apuntando arriba
		setColor(renderer, Pal::WHITE);
		SDL_RenderLine(renderer, l.x, HILL_TOP_Y - 3.0f, l.x, HILL_TOP_Y - 10.0f);
		setColor(renderer, Pal::LIGHT_GRAY);
		fillDome(renderer, l.x, HILL_TOP_Y, 4.0f);
	}
}

void Game::drawDomes()
{
	for (const Dome& d : domes) {
		if (!d.alive) continue;
		setColor(renderer, Pal::CYAN);
		fillDome(renderer, d.x, GROUND_Y, DOME_RADIUS);
		setColor(renderer, Pal::DEEP_BLUE);
		fillDome(renderer, d.x, GROUND_Y, DOME_RADIUS - 2.0f);
		// Tres ventanas encendidas de la colonia
		setColor(renderer, Pal::YELLOW);
		fillRect(renderer, d.x - 4.0f, GROUND_Y - 3.0f, 1.0f, 1.0f);
		fillRect(renderer, d.x, GROUND_Y - 5.0f, 1.0f, 1.0f);
		fillRect(renderer, d.x + 4.0f, GROUND_Y - 3.0f, 1.0f, 1.0f);
	}
}

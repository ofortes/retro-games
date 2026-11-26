#pragma once
#include <SDL3/SDL.h>
#include <array>
#include "Config.h"
#include "Entities.h"


class Game {
public:
	explicit Game(SDL_Renderer* renderer);
	~Game();


	// Game es dueño de una textura: prohibimos copiarlo para no liberarla dos veces
	Game(const Game&) = delete;
	Game& operator=(const Game&) = delete;

	bool isRunning() const { return running; }

	void handleEvent(const SDL_Event& event);
	void update(float dt);
	void render();

private:
	void drawGround();
	void drawLaunchers();
	void drawDomes();

	SDL_Renderer* renderer = nullptr;
	SDL_Texture* canvas = nullptr;
	bool running = true;

	std::array<Launcher, NUM_LAUNCHERS> launchers;
	std::array<Dome, NUM_DOMES> domes;
};


#pragma once

// Los actores del juego. Son structs sencillos: solo datos
// La logica (moverlos, dibujarlos) vive en Game

struct Launcher
{
	float x = 0.0f;
	bool alive = true;
};

struct Dome
{
	float x = 0.0f;
	bool alive = true;
};

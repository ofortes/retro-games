#pragma once
#include <SDL3/SDL.h>
#include "Config.h"

// Pequeñas funciones de dibujo que SDL no tare cde serie

void setColor(SDL_Renderer* renderer, Color color);
void fillRect(SDL_Renderer* renderer, float x, float y, float w, float h);
void fillCircle(SDL_Renderer* renderer, float cx, float cy, float radius);
void fillDome(SDL_Renderer* renderer, float cx, float baseY, float radius); // medio circulo
void fillTrapezoid(SDL_Renderer* renderer, float cx, float topY, float bottomY, float topHalfW, float bottomHalfW);


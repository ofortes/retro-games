#include "Draw.h"
#include <cmath>


void setColor(SDL_Renderer* renderer, Color color) {
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
}

void fillRect(SDL_Renderer* renderer, float x, float y, float w, float h) {
	SDL_FRect rect{ x,y,w,h };
	SDL_RenderFillRect(renderer, &rect);
}

// Un círculo relleno dibujado como una pila de líneas horizontales (filas de píxeles).
// Para cada fila dy, el ancho sale de Pitágoras: halfW = sqrt(r² - dy²)
void fillCircle(SDL_Renderer* renderer, float cx, float cy, float radius) {
	cx = std::round(cx);
	cy = std::round(cy);
	int r = (int)radius;
	float rr = radius * (radius + 1.0f); // r² + r: truco clásico para bordes más redondos
	for (int dy = -r; dy <= r; ++dy) {
		float halfW = std::floor(std::sqrt(rr - float(dy * dy)));
		fillRect(renderer, cx - halfW, cy + dy, halfW * 2.0f + 1.0f, 1.0f);
	}
}

void fillDome(SDL_Renderer* renderer, float cx, float baseY, float radius) {
	cx = std::round(cx);
	int r = (int)radius;
	float rr = radius * (radius + 1.0f);
	for (int dy = -r; dy < 0; ++dy) {
		float halfW = std::floor(std::sqrt(rr - float(dy * dy)));
		fillRect(renderer, cx - halfW, baseY + dy, halfW * 2.0f + 1.0f, 1.0f);
	}
}

// Trapecio: una fila por cada y, con el ancho interpolado entre arriba y abajo
void fillTrapezoid(SDL_Renderer* renderer, float cx, float topY, float bottomY,
	float topHalfW, float bottomHalfW)
{
	for (float y = topY; y < bottomY; y += 1.0f) {
		float t = (y - topY) / (bottomY - topY);               // 0 arriba, 1 abajo
		float halfW = topHalfW + (bottomHalfW - topHalfW) * t; // interpolación lineal
		fillRect(renderer, cx - halfW, y, halfW * 2.0f + 1.0f, 1.0f);
	}
}


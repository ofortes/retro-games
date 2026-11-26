#pragma once
#include <cstdint>

// =====================================================================================
// Config.h: todas las "perillas" del juego en un solo sitio
// =====================================================================================

constexpr const char* WINDOW_TITLE	= "Cupulas de Selene";
constexpr int GAME_W = 320;
constexpr int GAME_H = 180;
constexpr int WINDOW_SCALE = 4;  // La ventana empieza en 1280x720
constexpr float MAX_DT = 0.05f;

// ----- Paleta "Noche lunar" : 16 colores, ni uno mas...
struct Color { uint8_t r, g, b; };

constexpr Color hexColor(uint32_t hex) {
	return { uint8_t(hex >> 16), uint8_t(hex >> 8), uint8_t(hex) };
}

namespace Pal {
    constexpr Color SPACE = hexColor(0x0B0B1A); // fondo del cielo
    constexpr Color NIGHT = hexColor(0x1A1C3D);
    constexpr Color DEEP_BLUE = hexColor(0x2E3A73);
    constexpr Color MOON_GRAY = hexColor(0x6B6F8C); // suelo lunar
    constexpr Color LIGHT_GRAY = hexColor(0xA9AEC7);
    constexpr Color WHITE = hexColor(0xF2F2E9);
    constexpr Color CYAN = hexColor(0x45D6E0); // cúpulas
    constexpr Color GREEN = hexColor(0x5BD15B);
    constexpr Color DARK_GREEN = hexColor(0x2B7A4B);
    constexpr Color YELLOW = hexColor(0xF5D547);
    constexpr Color ORANGE = hexColor(0xF08C2E);
    constexpr Color RED = hexColor(0xD93A3A);
    constexpr Color CRIMSON = hexColor(0x7A1E3C);
    constexpr Color MAGENTA = hexColor(0xC94FC9);
    constexpr Color VIOLET = hexColor(0x6A3FA0);
    constexpr Color PINK = hexColor(0xF29BB0);
}

// ---- Escenario -----

constexpr float GROUND_Y = 166.0f;  // superficie lunar
constexpr float HILL_TOP_Y = 156.0f;  // cima de las colinas de las torretas
constexpr float HILL_TOP_HALF_W = 10.0f;
constexpr float HILL_BOTTOM_HALF_W = 18.0f;
constexpr int   NUM_LAUNCHERS = 3;
constexpr float LAUNCHER_X[NUM_LAUNCHERS] = { 20.0f, 160.0f, 300.0f };
constexpr int   NUM_DOMES = 4;
constexpr float DOME_X[NUM_DOMES] = { 64.0f, 108.0f, 212.0f, 256.0f };
constexpr float DOME_RADIUS = 10.0f;
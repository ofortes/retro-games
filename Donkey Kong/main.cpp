#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>   // necesario en Windows para que SDL gestione main()
#include <vector>
#include <cstring>   // std::strlen
#include <cmath>     // std::fabs
#include <iterator>  // std::size


// ------------------------ Constantes -------------------------
constexpr int ANCHO = 256;          // resolución lógica retro
constexpr int ALTO = 224;
constexpr int ESCALA = 3;           // tamaño de la ventana = resolución x escala
constexpr float GROSOR_PLATAFORMA = 4.0f;
constexpr float ANCHO_ESCALERA = 8.0F;

constexpr float VEL_JUGADOR = 60.0f;				// pixeles por segundo
constexpr float ANCHO_JUGADOR = 8.0f;
constexpr float ALTO_JUGADOR = 12.0f;
constexpr float GRAVEDAD = 500.0f;						// pixeles por segundo al cuadrado
constexpr float VEL_SALTO = -150.0f;					// negativa = hacia arriba
constexpr int VIDAS_INICIALES = 3;


// ---------- Escenario ----------
struct Plataforma { float x1, x2, y; };                 // y = superficie, donde se apoyan los pies
struct Escalera { float x, yArriba, yAbajo; };          // x = borde izquierdo

const Plataforma PLATAFORMAS[] = {
	{0, 256, 208 },						// 0: suelo
	{0, 232, 168},						// 1: hueco a la derecha
	{24, 256, 128},						// 2: hueco a la izquierda
	{0, 232, 88},						// 3: hueco a la derecha
	{24, 256, 48},						// 4: aqui vive el robot
	{100, 156, 20},						// 5: la meta
};

constexpr int NUM_PLATAFORMAS = (int)std::size(PLATAFORMAS);
constexpr int PLATAFORMA_META = 5;

const Escalera ESCALERAS[] = {
	{ 60, 168, 208 },					// SUELO -> 1
	{ 200, 168, 208 },					// suelo > 1
	{ 100, 128, 168 },					// 1 -> 2
	{ 180, 88, 128 },					// 2 -> 3
	{ 70, 48, 88 },						// 3 -> 4
	{ 124, 20, 48 },					// 4 -> meta
};

constexpr int NUM_ESCALERAS = (int)std::size(ESCALERAS);

constexpr float			VEL_ESCALAR = 40.0f;

constexpr float TAM_BARRIL = 8.0f;
constexpr float VEL_BARRIL_BASE = 60.0f;
constexpr float VEL_BARRIL_ESCALERA = 40.0f;
constexpr float INTERVALO_BARRIL_BASE = 2.5f;  // segundos entre barriles en el nivel 1
constexpr int PROB_BAJAR_ESCALERA = 30;	// % de veces que un barril baja por una escalera

constexpr float ROBOT_X = 224.0f;  // el robot que lanza barriles
constexpr float ROBOT_Y = 32.0f;




// ---------- Sprites: cada letra es un color, '.' es transparente ----------
const char* SPRITE_JUGADOR[] = {
	 "..VVVV..",
	".VVVVVVV",
	"..PPPKP.",
	"..PPPPP.",
	"...PPP..",
	"..YYYY..",
	".PYYYYP.",
	".PYYYYP.",
	"..AAAA..",
	"..A..A..",
	"..A..A..",
	".MM..MM.",
};

const char* SPRITE_BARRIL[] = {
	"..MMMM..",
	".MNNNNM.",
	"MNNNNNNM",
	"MMMMMMMM",
	"MNNNNNNM",
	"MNNNNNNM",
	".MNNNNM.",
	"..MMMM..",
};
const char* SPRITE_ROBOT[] = {
	"...GGGGGGGGGG...",
	"..GGGGGGGGGGGG..",
	"..GGRRGGGGRRGG..",
	"..GGRRGGGGRRGG..",
	"..GGGGGGGGGGGG..",
	"..GGKKKKKKKKGG..",
	"...GGGGGGGGGG...",
	"......GGGG......",
	".GGGGGGGGGGGGGG.",
	"GG.GGGGGGGGGG.GG",
	"GG.GGAAAAAAGG.GG",
	"GG.GGAAAAAAGG.GG",
	"...GGGGGGGGGG...",
	"...GGG....GGG...",
	"...GGG....GGG...",
	"..GGGG....GGGG..",
};

const char* SPRITE_AMIGA[] = {
	"..YYYY..",
	".YYYYYY.",
	".YKPPKY.",
	"..PPPP..",
	"...PP...",
	"..SSSS..",
	".SSSSSS.",
	".SSSSSS.",
	"SSSSSSSS",
	"SSSSSSSS",
	"..P..P..",
	"..P..P..",
};

SDL_Color colorDe(char c) {
	switch (c) {
	case 'V': return { 40, 180, 60, 255 };    // verde
	case 'Y': return { 240, 200, 40, 255 };   // amarillo
	case 'P': return { 250, 190, 140, 255 };  // piel
	case 'K': return { 0, 0, 0, 255 };        // negro
	case 'M': return { 120, 60, 20, 255 };    // marrón
	case 'N': return { 220, 120, 40, 255 };   // naranja
	case 'A': return { 60, 90, 220, 255 };    // azul
	case 'G': return { 150, 150, 170, 255 };  // gris
	case 'R': return { 230, 30, 30, 255 };    // rojo
	case 'S': return { 250, 110, 180, 255 };  // rosa
	default:  return { 255, 255, 255, 255 };  // blanco
	}
}

// Dibuja un sprite pixel a pixel. Con espejo = true se dibuja mirando al otro lado
void dibujaSprite(SDL_Renderer* render, const char* const* filas, int numFilas, float x, float y, bool espejo = false) {

	for (int f = 0; f < numFilas; ++f) {
		int ancho = (int)std::strlen(filas[f]);
		for (int c = 0; c < ancho; ++c) {
			char letra = filas[f][espejo ? ancho - 1 - c : c];
			if (letra == '.') continue;
			SDL_Color col = colorDe(letra);
			SDL_SetRenderDrawColor(render, col.r, col.g, col.b, 255);
			SDL_FRect pixel{ x + c, y + f,1,1 };
			SDL_RenderFillRect(render, &pixel);
		}
	}
}

// ---------- Estado del juego ----------
struct Jugador {
	float x = 0, y = 0;					//esquina superior izquierda
	float vy = 0;						// velocidad vertical
	bool enSuelo = true;
	bool escalando = false;
	int escalera = -1;					// escalera en la que esta subido
	bool mirandoDerecha = true;
};

struct Barril {
	float x = 0, y = 0;
	float vx = 0;		// velocidad al rodar
	float vy = 0;		// velocidad al caer
	bool cayendo = false;
	int escalera = -1;		// >= 0 si esta bajando por esa escalera
	bool puntuando = false;	// ya nos ha dado puntos por saltarlo
};

enum class Estado { Jugando, GameOver };

Jugador jugador;
std::vector<Barril> barriles;
Estado estado = Estado::Jugando;
int puntos = 0;
int vidas = VIDAS_INICIALES;
int nivel = 1;
float tiempoBarril = 0;				// cuenta atras hasta el siguiente barril
float tiempoMensaje = 0;			// segundo que sigue visible el cartel "NIVEL x"


// ---------- Dibujo ----------
void dibujarEscenario(SDL_Renderer* render) {

	// Escaleras primero, para que las plataformas queden por encima
	SDL_SetRenderDrawColor(render, 60, 200, 220, 255);
	for (const Escalera& e : ESCALERAS) {
		float der = e.x + ANCHO_ESCALERA - 1;
		SDL_RenderLine(render, e.x, e.yArriba, e.x, e.yAbajo);
		SDL_RenderLine(render, der, e.yArriba, der, e.yAbajo);
		for (float y = e.yArriba + 4; y < e.yAbajo; y += 4)
			SDL_RenderLine(render, e.x, y, der, y);			// peldaños
	}

	// Plataformas: viga roja con remaches oscuros
	for (const Plataforma& p : PLATAFORMAS) {
		SDL_FRect viga{ p.x1, p.y, p.x2 - p.x1, GROSOR_PLATAFORMA };
		SDL_SetRenderDrawColor(render, 200, 40, 70, 255);
		SDL_RenderFillRect(render, &viga);
		SDL_SetRenderDrawColor(render, 90, 10, 30, 255);
		for (float x = p.x1 + 2; x < p.x2 - 1; x += 8) {
			SDL_FRect remache{ x, p.y + 1,2,2 };
			SDL_RenderFillRect(render, &remache);
		}
	}
}

void dibujatHUD(SDL_Renderer* render) {
	SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
	SDL_RenderDebugTextFormat(render, 2, 2, "PUNTOS %06d", puntos);
	SDL_RenderDebugTextFormat(render, 184, 2, "VIDAS %d", vidas);
	SDL_RenderDebugTextFormat(render, 184, 12, "NIVEL %d", nivel);

	if (estado == Estado::Jugando && tiempoMensaje > 0) {
		SDL_SetRenderDrawColor(render, 255, 255, 0, 255);
		SDL_RenderDebugTextFormat(render, 100, 100, "NIVEL %d", nivel);
	}

	if (estado == Estado::GameOver) {
		SDL_FRect caja{ 48, 90, 160, 32 };
		SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
		SDL_RenderFillRect(render, &caja);
		SDL_SetRenderDrawColor(render, 255, 60, 60, 255);
		SDL_RenderDebugTextFormat(render, 92, 96, "GAME OVER");
		SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
		SDL_RenderDebugTextFormat(render, 56, 110, "PULSA R PARA JUGAR");

	}
}





// ---------- Ayudas de colisión con el escenario ----------

// ¿Hay una plataforma justo bajo los pies? Devuelve su índice o -1
int plataformaBajo(float centroX, float pies) {
	for (int i = 0; i < NUM_PLATAFORMAS; i++) {
		const Plataforma& p = PLATAFORMAS[i];
		if (centroX >= p.x1 && centroX <= p.x2 && std::fabs(pies - p.y) < 0.5f) 
			return i;
	}
	return -1;
}

// ¿Hay una escalera alineada con nosotros y los pies están dentro de su recorrido?
int escaleraEn(float centroX, float pies) {

	for (int i = 0; i < NUM_ESCALERAS; ++i) {
		const Escalera& e = ESCALERAS[i];
		float centroEscalera = e.x + ANCHO_ESCALERA / 2;
		if (std::fabs(centroX - centroEscalera) <= 4.0f &&
			pies >= e.yArriba - 0.5f && pies <= e.yAbajo + 0.5f) return i;
	}

	return -1;
}

// Al caer, ¿ los pies han atrevasado la supereficie de una plataforma? Devuelve esa y, o -1
float superficieAlCaer(float centroX, float piesAntes, float piesAhora) {
	for (const Plataforma& p : PLATAFORMAS) {
		if (centroX >= p.x1 && centroX <= p.x2 && piesAntes <= p.y && piesAhora >= p.y)
			return p.y;
	}
	return -1.0f;
}




// ----------------------- Jugador ---------------------------
void moverHorizontal(Jugador& j, const bool* teclas, float dt) {
	float dx = 0;
	if (teclas[SDL_SCANCODE_LEFT]) dx -= VEL_JUGADOR * dt;
	if (teclas[SDL_SCANCODE_RIGHT]) dx += VEL_JUGADOR * dt;
	if (dx < 0) j.mirandoDerecha = false;
	if (dx > 0) j.mirandoDerecha = true;
	j.x += dx;
	// No salir de la pantalla
	if (j.x < 0) j.x = 0;
	if (j.x > ANCHO - ANCHO_JUGADOR) j.x = ANCHO - ANCHO_JUGADOR;

}


void aplicarGravedad(Jugador& j, float dt) {
	float centroX = j.x + ANCHO_JUGADOR / 2;
	if (j.enSuelo) {
		// ¿ Seguimos pisando algo ? Si no, empezamos a caer
		if (plataformaBajo(centroX, j.y + ALTO_JUGADOR) >= 0) return;
		j.enSuelo = false;
		j.vy = 0;
	}

	float piesAntes = j.y + ALTO_JUGADOR;
	j.vy += GRAVEDAD * dt;			// la gravedad cambia la velocidad
	j.y += j.vy * dt;				// ... y la velocidad cambia la posicion
	if (j.vy >= 0) {				// solo aterrizamos al bajar
		float suelo = superficieAlCaer(centroX, piesAntes, j.y + ALTO_JUGADOR);
		if (suelo >= 0) {
			j.y = suelo - ALTO_JUGADOR;
			j.vy = 0;
			j.enSuelo = true;
		}
	}
}
// Si estamos junto a una escalera y pulsamos arriba/abajo, nos agarramos a ella
bool intentarEscalar(Jugador& j, const bool* teclas) {

	if (!j.enSuelo) return false;
	bool arriba = teclas[SDL_SCANCODE_UP];
	bool abajo = teclas[SDL_SCANCODE_DOWN];
	if (!arriba && !abajo) return false;

	float pies = j.y + ALTO_JUGADOR;
	int i = escaleraEn(j.x + ANCHO_JUGADOR / 2, pies);
	if (i < 0) return false;
	const Escalera& e = ESCALERAS[i];

	//Desde arriba del todo solo se puede bajar, y desde abajo del todo solo se puede subir
	if (arriba && pies <= e.yArriba + 0.5f) return false;
	if (abajo && pies >= e.yAbajo - 0.5f) return false;

	j.escalando = true;
	j.escalera = i;
	j.enSuelo = false;
	j.x = e.x + (ANCHO_ESCALERA - ANCHO_JUGADOR) / 2;  // centramos en la escalera
	return true;

}

void actualizarEscalada(Jugador& j, const bool* teclas, float dt) {
	const Escalera& e = ESCALERAS[j.escalera];
	float pies = j.y + ALTO_JUGADOR;
	if (teclas[SDL_SCANCODE_UP]) pies -= VEL_ESCALAR * dt;
	if (teclas[SDL_SCANCODE_DOWN]) pies += VEL_ESCALAR * dt;

	// Al llegar a un extremo nos bajamos de la escalera

	if (pies <= e.yArriba) {
		pies = e.yArriba;
		j.escalando = false;
		j.enSuelo = true;
	}
	else if (pies >= e.yAbajo) {
		pies = e.yAbajo;
		j.escalando = false;
		j.enSuelo = true;
	}

	j.y = pies - ALTO_JUGADOR;
}


void actualizarJugador(Jugador& j, const bool* teclas, bool pedirSalto, float dt) {
	if (j.escalando) { actualizarEscalada(j, teclas, dt); return; }
	
	if (intentarEscalar(j, teclas)) return;
	
	moverHorizontal(j, teclas, dt);
	if (pedirSalto && j.enSuelo) {
		j.vy = VEL_SALTO;
		j.enSuelo = false;
	}
	aplicarGravedad(j, dt);
		
}

// ---------- Barriles ----------
float velocidadBarril() {
	return VEL_BARRIL_BASE + 10.0f * (nivel - 1);
}

float intervaloBarril() {
	float t = INTERVALO_BARRIL_BASE - 0.3f * (nivel - 1);
	return t < 1.0f ? 1.0f : t;
}

void lanzarBarril() {
	Barril b;
	b.x = ROBOT_X - TAM_BARRIL - 8;
	b.y = PLATAFORMAS[4].y - TAM_BARRIL;
	b.vx = -velocidadBarril();	// sale rodando hacia la izquierda
	barriles.push_back(b);
}

void actualizarBarril(Barril& b, float dt) {
	float centroX = b.x + TAM_BARRIL / 2;
	float pies = b.y + TAM_BARRIL;

	// 1. Bajando por una escalera
	if (b.escalera >= 0) {
		const Escalera& e = ESCALERAS[b.escalera];
		pies += VEL_BARRIL_ESCALERA * dt;
		if (pies >= e.yAbajo) {
			pies = e.yAbajo;
			b.escalera = -1;
			b.vx = -b.vx;				// abajo sigue rodando en sentido contrario
		}
		b.y = pies - TAM_BARRIL;
		return;
	}

	// 2 Cayendo por el borde de una plataforma
	if (b.cayendo) {
		b.vy += GRAVEDAD * dt;
		b.y += b.vy * dt;
		float suelo = superficieAlCaer(centroX, pies, b.y + TAM_BARRIL);
		if (suelo >= 0) {
			b.y = suelo - TAM_BARRIL;
			b.vy = 0;
			b.cayendo = false;
			b.vx = -b.vx;				// al aterrizar cambia de sentido
		}
		return;
	}

	// 3. Rodando sobre una plataforma
	float centroAntes = centroX;
	b.x += b.vx * dt;
	centroX = b.x + TAM_BARRIL / 2;
	if (plataformaBajo(centroX, pies) < 0) {				// se acabo la plataforma
		b.cayendo = true;
		return;
	}		

	// ¿ Acabamos de pasar por encima de una escalera que baja desde aqui?
	for (int i = 0; i < NUM_ESCALERAS; ++i) {
		const Escalera& e = ESCALERAS[i];
		float centroEscalera = e.x + ANCHO_ESCALERA / 2;
		bool cruzada = (centroAntes - centroEscalera) * (centroX - centroEscalera) <= 0;
		if (cruzada && std::fabs(pies - e.yArriba) < 0.5f && SDL_rand(100) < PROB_BAJAR_ESCALERA) {
			b.escalera = i;
			b.x = e.x + (ANCHO_ESCALERA - TAM_BARRIL) / 2;
			return;
		}
	}
}

void borrarBarriles() {
	for (size_t i = 0; i < barriles.size();)
	{
		if (barriles[i].y > ALTO) barriles.erase(barriles.begin() + i);
		else ++i;
	}
}



//------------- Partida ------------------------
void empezarNivel() {
	jugador = Jugador{};
	jugador.x = 16;
	jugador.y = PLATAFORMAS[0].y - ALTO_JUGADOR;
	barriles.clear();
	tiempoBarril = 1.0f;
	tiempoMensaje = 2.0f;

}

void nuevaPartida() {
	puntos = 0;
	vidas = VIDAS_INICIALES;
	nivel = 1;
	estado = Estado::Jugando;
	empezarNivel();
}

void perderVida() {
	vidas--;
	if (vidas <= 0) estado = Estado::GameOver;
	else empezarNivel();
}

void comprobarColisiones() {
	// Caja del jugador un poco mas pequeña que el sprite: asi el juego es mas justo
	SDL_FRect cj{ jugador.x + 1, jugador.y + 2, ANCHO_JUGADOR - 2 , ALTO_JUGADOR - 2 };

	SDL_FRect robot{ ROBOT_X, ROBOT_Y, 16, 16 };
	if (SDL_HasRectIntersectionFloat(&cj, &robot)) {
		perderVida(); return;
	}

	bool enElAire = !jugador.enSuelo && !jugador.escalando;
	float pies = jugador.y + ALTO_JUGADOR;
	for (Barril& b : barriles) {
		SDL_FRect cb{ b.x + 1,b.y + 1, TAM_BARRIL - 2 , TAM_BARRIL - 2 };
		if (SDL_HasRectIntersectionFloat(&cj, &cb)) { perderVida(); return; }


		// puntos por saltar por encima de un barril
		float distX = std::fabs((jugador.x + ANCHO_JUGADOR / 2) - (b.x + TAM_BARRIL / 2));
		if (enElAire && !b.puntuando && distX < 4 && pies <= b.y && b.y - pies < 24) {
			b.puntuando = true;
			puntos += 100;
		}
	}
}

void comprobarMeta() {
	if (estado != Estado::Jugando || !jugador.enSuelo) return;
	if (plataformaBajo(jugador.x + ANCHO_JUGADOR / 2, jugador.y + ALTO_JUGADOR) == PLATAFORMA_META) {
		puntos += 1000;
		nivel++;
		empezarNivel();
	}
}



int main(int argc, char* argv[]) {

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("Error al iniciar SDL: %s", SDL_GetError());
		return 1;
	}

	SDL_Window* ventana = nullptr;
	SDL_Renderer* render = nullptr;
	if (!SDL_CreateWindowAndRenderer("Escala de barriles", ANCHO * ESCALA, ALTO * ESCALA, 0, &ventana, &render)) {
		SDL_Log("Error al crear la ventana: %s", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	SDL_SetRenderLogicalPresentation(render, ANCHO, ALTO, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
	SDL_SetRenderVSync(render, 1);
	SDL_srand(0);       // 0 ? semilla distinta en cada partida

	Uint64 ultimo = SDL_GetTicksNS();
	bool ejecutando = true;
	
	nuevaPartida();
	
	while (ejecutando) {
		// 1. Tiempo transcurrido desde el fotograma anterior, en segundos
		Uint64 ahora = SDL_GetTicksNS();
		float dt = (ahora - ultimo) / 1'000'000'000.0f;
		ultimo = ahora;

		if (dt > 0.05f) dt = 0.05f;         // evita saltos enormes si la ventana se congela

		// 2 Eventos
		bool pedirSalto = false;
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_EVENT_QUIT) ejecutando = false;
			if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
				if (e.key.key == SDLK_ESCAPE) ejecutando = false;
				if (e.key.key == SDLK_SPACE) pedirSalto = true;
				if (e.key.key == SDLK_R && estado == Estado::GameOver) nuevaPartida();
			}
		}

		// 3. Actualizar

		const bool* teclas = SDL_GetKeyboardState(nullptr);
		if (estado == Estado::Jugando) {
			actualizarJugador(jugador, teclas, pedirSalto, dt);

			tiempoBarril -= dt;
			if (tiempoBarril <= 0) {
				lanzarBarril();
				tiempoBarril = intervaloBarril();
			}

			for (Barril& b : barriles) actualizarBarril(b, dt);
			borrarBarriles();


			comprobarColisiones();
			comprobarMeta();
			if (tiempoMensaje > 0) tiempoMensaje -= dt;
		}
		

		// 4 Dibujar
		SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
		SDL_RenderClear(render);
		dibujarEscenario(render);

		dibujaSprite(render, SPRITE_ROBOT, (int)std::size(SPRITE_ROBOT), ROBOT_X, ROBOT_Y);
		dibujaSprite(render, SPRITE_AMIGA, (int)std::size(SPRITE_AMIGA), 140, PLATAFORMAS[PLATAFORMA_META].y -12);

		for(const Barril& b: barriles) 
			dibujaSprite(render, SPRITE_BARRIL, (int)std::size(SPRITE_BARRIL), b.x, b.y);
		
		dibujaSprite(render, SPRITE_JUGADOR, (int)std::size(SPRITE_JUGADOR), jugador.x, jugador.y, !jugador.mirandoDerecha);
		dibujatHUD(render);
		SDL_RenderPresent(render);

	}

	SDL_DestroyRenderer(render);
	SDL_DestroyWindow(ventana);
	SDL_Quit();
	return 0;

}
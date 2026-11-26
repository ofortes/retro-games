Juego del día: Donkey Kong en SDL3 + C++
17 nov 2026 · @oscar
Qué vamos a construir
Hoy toca el juego nº 20 del catálogo: un Donkey Kong de una sola pantalla con personajes propios. Un robot lanza barriles desde arriba; los barriles ruedan en zigzag por las vigas y a veces bajan por las escaleras. Tú manejas a Pixel, que tiene que subir hasta la plataforma de arriba para rescatar a su amiga.
• Controles: flechas izquierda/derecha para andar, arriba/abajo para usar escaleras, espacio para saltar, R para reiniciar tras el game over, Esc para salir.
• Reglas: si te toca un barril o el robot, pierdes una vida (empiezas con 3). Saltar un barril da 100 puntos y llegar arriba da 1000. Cada nivel los barriles van más rápido y salen más a menudo.
PUNTOS 000300            VIDAS 3
              [A]        NIVEL 1
            ==H==
   ===========H=========== [R]
      H              <o
=============================
                  H
   o>             H
=============================
         H
=========H=================
    H              H   <o
=============================
  P
P = Pixel, o = barril, [R] = robot, [A] = amiga, H = escalera.
Qué vas a aprender hoy
• Plataformas de un solo sentido: atraviesas la viga desde abajo, pero aterrizas en ella al caer desde arriba.
• Salto con gravedad: la gravedad cambia la velocidad y la velocidad cambia la posición.
• Escaleras: un modo de movimiento distinto que apaga la gravedad mientras subes.
• Enemigos con estados: cada barril está rodando, cayendo o bajando una escalera, y se comporta distinto en cada caso.
Preparar el proyecto
Crea una carpeta (por ejemplo EscaladaBarriles) con dos archivos: CMakeLists.txt y main.cpp. CMake descarga y compila SDL3 por ti; la versión fijada es la 3.4.18, la última estable que he encontrado hoy (anuncio de SDL 3.4.18).
cmake_minimum_required(VERSION 3.24)
project(JuegoRetro LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
# SDL3 enlazado de forma estática: así no hace falta copiar SDL3.dll junto al .exe
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.4.18
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SDL3)

add_executable(juego main.cpp)
target_link_libraries(juego PRIVATE SDL3::SDL3)
1.  En el instalador de Visual Studio 2026, activa la carga de trabajo Desarrollo para el escritorio con C++ (trae CMake y Git).
2.  Archivo → Abrir → Carpeta y elige tu carpeta. Visual Studio detecta CMake; la primera vez descarga y compila SDL3, así que tarda unos minutos.
3.  En el desplegable de elemento de inicio elige juego.exe y pulsa F5.
Paso a paso
Cada paso compila y muestra algo nuevo. El orden del archivo importa: una función tiene que estar escrita antes de usarla. Si dudas de dónde va algo, mira el código completo, que está ordenado por secciones (Constantes, Escenario, Sprites, Estado del juego…).
Paso 1: La ventana y el bucle de juego
Todo juego es un bucle que se repite unas 60 veces por segundo: medir el tiempo, leer eventos, actualizar y dibujar. Fijamos una resolución pequeña (256×224, como una consola de 16 bits) y SDL la escala para que se vea pixelada. Limitamos dt a 0,05 s: si arrastras la ventana el juego se congela, y sin ese límite el siguiente fotograma daría un salto enorme y los personajes atravesarían las vigas.
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>   // necesario en Windows para que SDL gestione main()
#include <vector>
#include <cstring>   // std::strlen
#include <cmath>     // std::fabs
#include <iterator>  // std::size

// ---------- Constantes ----------
constexpr int ANCHO = 256;     // resolución lógica retro
constexpr int ALTO  = 224;
constexpr int ESCALA = 3;      // tamaño de la ventana = resolución x escala

int main(int argc, char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Error al iniciar SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* ventana = nullptr;
    SDL_Renderer* render = nullptr;
    if (!SDL_CreateWindowAndRenderer("Escalada de barriles", ANCHO * ESCALA, ALTO * ESCALA, 0, &ventana, &render)) {
        SDL_Log("Error al crear la ventana: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderLogicalPresentation(render, ANCHO, ALTO, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    SDL_SetRenderVSync(render, 1);
    SDL_srand(0);   // 0 = semilla distinta en cada partida

    Uint64 ultimo = SDL_GetTicksNS();
    bool ejecutando = true;
    while (ejecutando) {
        // 1. Tiempo transcurrido desde el fotograma anterior, en segundos
        Uint64 ahora = SDL_GetTicksNS();
        float dt = (ahora - ultimo) / 1'000'000'000.0f;
        ultimo = ahora;
        if (dt > 0.05f) dt = 0.05f;   // evita saltos enormes si la ventana se congela

        // 2. Eventos
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) ejecutando = false;
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) ejecutando = false;
        }

        // 3. Actualizar

        // 4. Dibujar
        SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
        SDL_RenderClear(render);
        SDL_RenderPresent(render);
    }

    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(ventana);
    SDL_Quit();
    return 0;
}
Qué deberías ver: una ventana negra de 768×672 que se cierra con Esc o con la X.
Paso 2: Vigas y escaleras
El escenario son datos: cada viga es un tramo horizontal (x1, x2) a una altura y, y cada escalera une la y de dos vigas. Guardamos la superficie de la viga, que es donde se apoyan los pies; así las comprobaciones posteriores son más sencillas. Fíjate en que las vigas dejan huecos alternos a izquierda y derecha: por ahí caerán los barriles en zigzag.
Añade estas constantes al final de la sección Constantes:
constexpr float GROSOR_PLATAFORMA = 4.0f;
constexpr float ANCHO_ESCALERA    = 8.0f;
Debajo, la sección del escenario:
// ---------- Escenario ----------
struct Plataforma { float x1, x2, y; };         // y = superficie, donde se apoyan los pies
struct Escalera   { float x, yArriba, yAbajo; }; // x = borde izquierdo

const Plataforma PLATAFORMAS[] = {
    {   0, 256, 208 },   // 0: suelo
    {   0, 232, 168 },   // 1: hueco a la derecha
    {  24, 256, 128 },   // 2: hueco a la izquierda
    {   0, 232,  88 },   // 3: hueco a la derecha
    {  24, 256,  48 },   // 4: aquí vive el robot
    { 100, 156,  20 },   // 5: la meta
};
constexpr int NUM_PLATAFORMAS = (int)std::size(PLATAFORMAS);
constexpr int PLATAFORMA_META = 5;

const Escalera ESCALERAS[] = {
    {  60, 168, 208 },   // suelo -> 1
    { 200, 168, 208 },   // suelo -> 1
    { 100, 128, 168 },   // 1 -> 2
    { 180,  88, 128 },   // 2 -> 3
    {  70,  48,  88 },   // 3 -> 4
    { 124,  20,  48 },   // 4 -> meta
};
constexpr int NUM_ESCALERAS = (int)std::size(ESCALERAS);
Y la función que lo dibuja (ponla justo antes de main, en una sección Dibujo):
// ---------- Dibujo ----------
void dibujarEscenario(SDL_Renderer* render)
{
    // Escaleras primero, para que las plataformas queden por encima
    SDL_SetRenderDrawColor(render, 60, 200, 220, 255);
    for (const Escalera& e : ESCALERAS) {
        float der = e.x + ANCHO_ESCALERA - 1;
        SDL_RenderLine(render, e.x, e.yArriba, e.x, e.yAbajo);
        SDL_RenderLine(render, der, e.yArriba, der, e.yAbajo);
        for (float y = e.yArriba + 4; y < e.yAbajo; y += 4)
            SDL_RenderLine(render, e.x, y, der, y);   // peldaños
    }
    // Plataformas: viga roja con remaches oscuros
    for (const Plataforma& p : PLATAFORMAS) {
        SDL_FRect viga{ p.x1, p.y, p.x2 - p.x1, GROSOR_PLATAFORMA };
        SDL_SetRenderDrawColor(render, 200, 40, 70, 255);
        SDL_RenderFillRect(render, &viga);
        SDL_SetRenderDrawColor(render, 90, 10, 30, 255);
        for (float x = p.x1 + 2; x < p.x2 - 1; x += 8) {
            SDL_FRect remache{ x, p.y + 1, 2, 2 };
            SDL_RenderFillRect(render, &remache);
        }
    }
}
En main, dentro de // 4. Dibujar, justo después de SDL_RenderClear(render);:
        dibujarEscenario(render);
Qué deberías ver: seis vigas rojas escalonadas y seis escaleras azul claro que las unen.
Paso 3: Sprites de píxeles y un jugador que anda
Un sprite de 8 bits no necesita archivos: es un array de textos donde cada letra es un color y el punto es transparente. Lo dibujamos píxel a píxel con rectángulos de 1×1. Para que Pixel mire a la izquierda no hace falta otro sprite: leemos cada fila al revés (espejo). Para moverlo usamos SDL_GetKeyboardState, que dice qué teclas están pulsadas ahora mismo; es lo adecuado para movimientos continuos.
Añade a Constantes:
constexpr float VEL_JUGADOR   = 60.0f;    // píxeles por segundo
constexpr float ANCHO_JUGADOR = 8.0f;
constexpr float ALTO_JUGADOR  = 12.0f;
Después de Escenario, una sección de sprites:
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

SDL_Color colorDe(char c)
{
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

// Dibuja un sprite píxel a píxel. Con espejo = true se dibuja mirando al otro lado
void dibujarSprite(SDL_Renderer* render, const char* const* filas, int numFilas, float x, float y, bool espejo = false)
{
    for (int f = 0; f < numFilas; ++f) {
        int ancho = (int)std::strlen(filas[f]);
        for (int c = 0; c < ancho; ++c) {
            char letra = filas[f][espejo ? ancho - 1 - c : c];
            if (letra == '.') continue;
            SDL_Color col = colorDe(letra);
            SDL_SetRenderDrawColor(render, col.r, col.g, col.b, 255);
            SDL_FRect pixel{ x + c, y + f, 1, 1 };
            SDL_RenderFillRect(render, &pixel);
        }
    }
}
Después, el estado del juego y el movimiento horizontal:
// ---------- Estado del juego ----------
struct Jugador {
    float x = 0, y = 0;        // esquina superior izquierda
    float vy = 0;              // velocidad vertical
    bool enSuelo = true;
    bool escalando = false;
    int escalera = -1;         // escalera en la que está subido
    bool mirandoDerecha = true;
};

Jugador jugador;

// ---------- Jugador ----------
void moverHorizontal(Jugador& j, const bool* teclas, float dt)
{
    float dx = 0;
    if (teclas[SDL_SCANCODE_LEFT])  dx -= VEL_JUGADOR * dt;
    if (teclas[SDL_SCANCODE_RIGHT]) dx += VEL_JUGADOR * dt;
    if (dx < 0) j.mirandoDerecha = false;
    if (dx > 0) j.mirandoDerecha = true;
    j.x += dx;
    // No salir de la pantalla
    if (j.x < 0) j.x = 0;
    if (j.x > ANCHO - ANCHO_JUGADOR) j.x = ANCHO - ANCHO_JUGADOR;
}

// Versión provisional: en los pasos 4 y 5 la ampliamos
void actualizarJugador(Jugador& j, const bool* teclas, float dt)
{
    moverHorizontal(j, teclas, dt);
}
En main, justo antes del while, coloca a Pixel sobre el suelo (en el paso 7 cambiaremos esta línea):
    jugador.x = 16;
    jugador.y = PLATAFORMAS[0].y - ALTO_JUGADOR;
En // 3. Actualizar:
        const bool* teclas = SDL_GetKeyboardState(nullptr);
        actualizarJugador(jugador, teclas, dt);
Y en // 4. Dibujar, después de dibujarEscenario:
        dibujarSprite(render, SPRITE_JUGADOR, (int)std::size(SPRITE_JUGADOR), jugador.x, jugador.y, !jugador.mirandoDerecha);
Qué deberías ver: Pixel (casco verde, chaqueta amarilla) en el suelo, abajo a la izquierda. Con las flechas anda y se gira. Todavía no cae ni salta.
Paso 4: Gravedad, salto y vigas de un solo sentido
La gravedad no mueve al jugador directamente: cambia su velocidad, y la velocidad cambia la posición. Por eso el salto sube rápido, se frena y vuelve a caer, como una parábola. Para aterrizar comprobamos si, en este fotograma, los pies han cruzado la superficie de una viga mientras bajaban (piesAntes <= y y piesAhora >= y). Solo miramos al bajar, así que desde abajo las vigas no estorban: son plataformas de un solo sentido. Si andas y la viga se acaba bajo tus pies, empiezas a caer.
Añade a Constantes:
constexpr float GRAVEDAD      = 500.0f;   // píxeles por segundo al cuadrado
constexpr float VEL_SALTO     = -150.0f;  // negativa = hacia arriba
Después de Estado del juego, una sección con ayudas de colisión:
// ---------- Ayudas de colisión con el escenario ----------

// ¿Hay una plataforma justo bajo los pies? Devuelve su índice o -1
int plataformaBajo(float centroX, float pies)
{
    for (int i = 0; i < NUM_PLATAFORMAS; ++i) {
        const Plataforma& p = PLATAFORMAS[i];
        if (centroX >= p.x1 && centroX <= p.x2 && std::fabs(pies - p.y) < 0.5f) return i;
    }
    return -1;
}

// Al caer, ¿los pies han atravesado la superficie de una plataforma? Devuelve esa y, o -1
float superficieAlCaer(float centroX, float piesAntes, float piesAhora)
{
    for (const Plataforma& p : PLATAFORMAS) {
        if (centroX >= p.x1 && centroX <= p.x2 && piesAntes <= p.y && piesAhora >= p.y) return p.y;
    }
    return -1.0f;
}
En la sección Jugador, después de moverHorizontal, añade aplicarGravedad y sustituye actualizarJugador por esta versión:
void aplicarGravedad(Jugador& j, float dt)
{
    float centroX = j.x + ANCHO_JUGADOR / 2;
    if (j.enSuelo) {
        // ¿Seguimos pisando algo? Si no, empezamos a caer
        if (plataformaBajo(centroX, j.y + ALTO_JUGADOR) >= 0) return;
        j.enSuelo = false;
        j.vy = 0;
    }
    float piesAntes = j.y + ALTO_JUGADOR;
    j.vy += GRAVEDAD * dt;    // la gravedad cambia la velocidad...
    j.y  += j.vy * dt;        // ...y la velocidad cambia la posición
    if (j.vy >= 0) {          // solo aterrizamos al bajar
        float suelo = superficieAlCaer(centroX, piesAntes, j.y + ALTO_JUGADOR);
        if (suelo >= 0) {
            j.y = suelo - ALTO_JUGADOR;
            j.vy = 0;
            j.enSuelo = true;
        }
    }
}

// Versión provisional: en el paso 5 añadimos las escaleras
void actualizarJugador(Jugador& j, const bool* teclas, bool pedirSalto, float dt)
{
    moverHorizontal(j, teclas, dt);
    if (pedirSalto && j.enSuelo) {
        j.vy = VEL_SALTO;
        j.enSuelo = false;
    }
    aplicarGravedad(j, dt);
}
El salto es una pulsación puntual, así que lo leemos como evento (y con !e.key.repeat, para que mantener el espacio no repita saltos). En main, sustituye el bloque // 2. Eventos por:
        // 2. Eventos
        bool pedirSalto = false;
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) ejecutando = false;
            if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
                if (e.key.key == SDLK_ESCAPE) ejecutando = false;
                if (e.key.key == SDLK_SPACE) pedirSalto = true;
            }
        }
Y cambia la llamada en // 3. Actualizar:
        actualizarJugador(jugador, teclas, pedirSalto, dt);
Qué deberías ver: con espacio, Pixel salta unos 22 píxeles y vuelve a aterrizar en el suelo. Todavía no puede subir a las vigas: para eso están las escaleras.
Paso 5: Subir y bajar escaleras
En una escalera el jugador cambia de modo: no hay gravedad, no se mueve en horizontal y las flechas arriba/abajo cambian su altura. Para agarrarse tiene que estar en el suelo, alineado con la escalera (a 4 píxeles o menos de su centro) y con los pies dentro de su recorrido. Al llegar a un extremo, vuelve a estar "en el suelo" sobre la viga. Desde lo alto de una escalera solo se puede bajar, y desde abajo solo subir.
Añade a Constantes:
constexpr float VEL_ESCALAR   = 40.0f;
Al final de Ayudas de colisión con el escenario:
// ¿Hay una escalera alineada con nosotros y los pies están dentro de su recorrido?
int escaleraEn(float centroX, float pies)
{
    for (int i = 0; i < NUM_ESCALERAS; ++i) {
        const Escalera& e = ESCALERAS[i];
        float centroEscalera = e.x + ANCHO_ESCALERA / 2;
        if (std::fabs(centroX - centroEscalera) <= 4.0f &&
            pies >= e.yArriba - 0.5f && pies <= e.yAbajo + 0.5f) return i;
    }
    return -1;
}
En la sección Jugador, después de aplicarGravedad, añade estas dos funciones y sustituye actualizarJugador por su versión definitiva:
// Si estamos junto a una escalera y pulsamos arriba/abajo, nos agarramos a ella
bool intentarEscalar(Jugador& j, const bool* teclas)
{
    if (!j.enSuelo) return false;              // no nos agarramos en pleno salto
    bool arriba = teclas[SDL_SCANCODE_UP];
    bool abajo  = teclas[SDL_SCANCODE_DOWN];
    if (!arriba && !abajo) return false;

    float pies = j.y + ALTO_JUGADOR;
    int i = escaleraEn(j.x + ANCHO_JUGADOR / 2, pies);
    if (i < 0) return false;
    const Escalera& e = ESCALERAS[i];
    // Desde arriba del todo solo se puede bajar, y desde abajo del todo solo subir
    if (arriba && pies <= e.yArriba + 0.5f) return false;
    if (abajo  && pies >= e.yAbajo - 0.5f) return false;

    j.escalando = true;
    j.escalera = i;
    j.enSuelo = false;
    j.x = e.x + (ANCHO_ESCALERA - ANCHO_JUGADOR) / 2;   // centrarnos en la escalera
    return true;
}

void actualizarEscalada(Jugador& j, const bool* teclas, float dt)
{
    const Escalera& e = ESCALERAS[j.escalera];
    float pies = j.y + ALTO_JUGADOR;
    if (teclas[SDL_SCANCODE_UP])   pies -= VEL_ESCALAR * dt;
    if (teclas[SDL_SCANCODE_DOWN]) pies += VEL_ESCALAR * dt;
    // Al llegar a un extremo nos bajamos de la escalera
    if (pies <= e.yArriba) {
        pies = e.yArriba;
        j.escalando = false;
        j.enSuelo = true;
    } else if (pies >= e.yAbajo) {
        pies = e.yAbajo;
        j.escalando = false;
        j.enSuelo = true;
    }
    j.y = pies - ALTO_JUGADOR;
}

void actualizarJugador(Jugador& j, const bool* teclas, bool pedirSalto, float dt)
{
    if (j.escalando) { actualizarEscalada(j, teclas, dt); return; }
    if (intentarEscalar(j, teclas)) return;
    moverHorizontal(j, teclas, dt);
    if (pedirSalto && j.enSuelo) {
        j.vy = VEL_SALTO;
        j.enSuelo = false;
    }
    aplicarGravedad(j, dt);
}
Qué deberías ver: pon a Pixel delante de una escalera y pulsa arriba: se centra y sube. Ya puedes recorrer todas las vigas. Si andas hasta un hueco, caes a la viga de abajo.
Paso 6: El robot y los barriles
Cada barril es una pequeña máquina de estados con tres modos: rodando, cayendo o bajando una escalera. Al rodar, si la viga se acaba, pasa a cayendo; al aterrizar, cambia de sentido. Así sale el zigzag sin programarlo explícitamente. Para bajar escaleras comprobamos si el centro del barril acaba de cruzar el centro de una escalera que baja desde su viga: si el signo de la resta cambia entre el fotograma anterior y este, la ha cruzado. Solo entonces tiramos el dado (SDL_rand(100)), una vez por escalera.
Añade a Constantes:
constexpr float TAM_BARRIL            = 8.0f;
constexpr float VEL_BARRIL_BASE       = 60.0f;
constexpr float VEL_BARRIL_ESCALERA   = 40.0f;
constexpr float INTERVALO_BARRIL_BASE = 2.5f;  // segundos entre barriles en el nivel 1
constexpr int   PROB_BAJAR_ESCALERA   = 30;    // % de veces que un barril baja por una escalera

constexpr float ROBOT_X = 224.0f;   // el robot que lanza barriles
constexpr float ROBOT_Y = 32.0f;
En Sprites, debajo de SPRITE_JUGADOR:
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
En Estado del juego, debajo de struct Jugador:
struct Barril {
    float x = 0, y = 0;
    float vx = 0;              // velocidad al rodar
    float vy = 0;              // velocidad al caer
    bool cayendo = false;
    int escalera = -1;         // >= 0 si está bajando por esa escalera
    bool puntuado = false;     // ya nos ha dado puntos por saltarlo
};
Y debajo de Jugador jugador;:
std::vector<Barril> barriles;
int nivel = 1;
float tiempoBarril = 0;     // cuenta atrás hasta el siguiente barril
Una sección nueva después de Jugador:
// ---------- Barriles ----------
float velocidadBarril() { return VEL_BARRIL_BASE + 10.0f * (nivel - 1); }

float intervaloBarril()
{
    float t = INTERVALO_BARRIL_BASE - 0.3f * (nivel - 1);
    return t < 1.0f ? 1.0f : t;
}

void lanzarBarril()
{
    Barril b;
    b.x = ROBOT_X - TAM_BARRIL - 8;
    b.y = PLATAFORMAS[4].y - TAM_BARRIL;
    b.vx = -velocidadBarril();      // sale rodando hacia la izquierda
    barriles.push_back(b);
}

void actualizarBarril(Barril& b, float dt)
{
    float centroX = b.x + TAM_BARRIL / 2;
    float pies = b.y + TAM_BARRIL;

    // 1. Bajando por una escalera
    if (b.escalera >= 0) {
        const Escalera& e = ESCALERAS[b.escalera];
        pies += VEL_BARRIL_ESCALERA * dt;
        if (pies >= e.yAbajo) {
            pies = e.yAbajo;
            b.escalera = -1;
            b.vx = -b.vx;           // abajo sigue rodando en sentido contrario
        }
        b.y = pies - TAM_BARRIL;
        return;
    }

    // 2. Cayendo por el borde de una plataforma
    if (b.cayendo) {
        b.vy += GRAVEDAD * dt;
        b.y  += b.vy * dt;
        float suelo = superficieAlCaer(centroX, pies, b.y + TAM_BARRIL);
        if (suelo >= 0) {
            b.y = suelo - TAM_BARRIL;
            b.vy = 0;
            b.cayendo = false;
            b.vx = -b.vx;           // al aterrizar cambia de sentido
        }
        return;
    }

    // 3. Rodando sobre una plataforma
    float centroAntes = centroX;
    b.x += b.vx * dt;
    centroX = b.x + TAM_BARRIL / 2;
    if (plataformaBajo(centroX, pies) < 0) {   // se acabó la plataforma
        b.cayendo = true;
        return;
    }
    // ¿Acabamos de pasar por encima de una escalera que baja desde aquí?
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

void borrarBarrilesFuera()
{
    for (size_t i = 0; i < barriles.size(); ) {
        if (barriles[i].y > ALTO) barriles.erase(barriles.begin() + i);
        else ++i;
    }
}
En main, en // 3. Actualizar, después de actualizarJugador(...):
            tiempoBarril -= dt;
            if (tiempoBarril <= 0) {
                lanzarBarril();
                tiempoBarril = intervaloBarril();
            }
            for (Barril& b : barriles) actualizarBarril(b, dt);
            borrarBarrilesFuera();
En // 4. Dibujar, después de dibujarEscenario y antes del jugador (lo que se dibuja después queda encima):
        dibujarSprite(render, SPRITE_ROBOT, (int)std::size(SPRITE_ROBOT), ROBOT_X, ROBOT_Y);
        for (const Barril& b : barriles)
            dibujarSprite(render, SPRITE_BARRIL, (int)std::size(SPRITE_BARRIL), b.x, b.y);


Qué deberías ver: el robot gris arriba a la derecha. Cada 2,5 s sale un barril que baja en zigzag, a veces por las escaleras, y desaparece por abajo a la izquierda. Todavía te atraviesan sin hacerte nada.

Paso 7: Colisiones, vidas y puntos
Ahora los barriles hacen daño. Usamos SDL_HasRectIntersectionFloat con cajas un poco más pequeñas que los sprites: así un roce de un píxel no te mata y el juego parece más justo. Para dar puntos por saltar un barril, miramos si Pixel está en el aire, justo encima de él y a menos de 24 píxeles. Marcamos el barril como puntuado para no sumar 100 en cada fotograma. También añadimos un estado de juego (Jugando o GameOver).
Añade a Constantes:
constexpr int   VIDAS_INICIALES = 3;
En Estado del juego, debajo de struct Barril:
enum class Estado { Jugando, GameOver };
Y junto a las otras variables globales:
Estado estado = Estado::Jugando;
int puntos = 0;
int vidas = VIDAS_INICIALES;
float tiempoMensaje = 0;    // segundos que sigue visible el cartel "NIVEL X"
Una sección nueva después de Barriles:
// ---------- Partida ----------
void empezarNivel()
{
    jugador = Jugador{};
    jugador.x = 16;
    jugador.y = PLATAFORMAS[0].y - ALTO_JUGADOR;
    barriles.clear();
    tiempoBarril = 1.0f;
    tiempoMensaje = 2.0f;
}

void nuevaPartida()
{
    puntos = 0;
    vidas = VIDAS_INICIALES;
    nivel = 1;
    estado = Estado::Jugando;
    empezarNivel();
}

void perderVida()
{
    vidas--;
    if (vidas <= 0) estado = Estado::GameOver;
    else empezarNivel();
}

void comprobarColisiones()
{
    // Caja del jugador un poco más pequeña que el sprite: así el juego es más justo
    SDL_FRect cj{ jugador.x + 1, jugador.y + 2, ANCHO_JUGADOR - 2, ALTO_JUGADOR - 2 };

    SDL_FRect robot{ ROBOT_X, ROBOT_Y, 16, 16 };
    if (SDL_HasRectIntersectionFloat(&cj, &robot)) { perderVida(); return; }

    bool enElAire = !jugador.enSuelo && !jugador.escalando;
    float pies = jugador.y + ALTO_JUGADOR;
    for (Barril& b : barriles) {
        SDL_FRect cb{ b.x + 1, b.y + 1, TAM_BARRIL - 2, TAM_BARRIL - 2 };
        if (SDL_HasRectIntersectionFloat(&cj, &cb)) { perderVida(); return; }

        // Puntos por saltar por encima de un barril
        float distX = std::fabs((jugador.x + ANCHO_JUGADOR / 2) - (b.x + TAM_BARRIL / 2));
        if (enElAire && !b.puntuado && distX < 4 && pies <= b.y && b.y - pies < 24) {
            b.puntuado = true;
            puntos += 100;
        }
    }
}
Fíjate en el return justo después de perderVida(): esa función vacía la lista de barriles, y seguir recorriéndola después sería un error grave.
En main, sustituye las dos líneas que colocaban al jugador antes del while por:
    nuevaPartida();
Y sustituye todo el bloque // 3. Actualizar por:
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
            borrarBarrilesFuera();

            comprobarColisiones();
        }
Qué deberías ver: si un barril te toca, vuelves al inicio y la pantalla se limpia. Al perder la tercera vida el juego se queda quieto: en el siguiente paso le ponemos el cartel de game over y la forma de reiniciar.
Paso 8: Meta, niveles, marcador y game over
El último paso cierra el juego: llegar a la viga de arriba suma 1000 puntos y sube de nivel (barriles más rápidos y frecuentes, gracias a velocidadBarril() e intervaloBarril()). El texto se dibuja con SDL_RenderDebugText, que usa una fuente de 8×8 incluida en SDL y el color de dibujo actual.
En Sprites, debajo de SPRITE_ROBOT:
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
Al final de Partida:
void comprobarMeta()
{
    if (estado != Estado::Jugando || !jugador.enSuelo) return;
    if (plataformaBajo(jugador.x + ANCHO_JUGADOR / 2, jugador.y + ALTO_JUGADOR) == PLATAFORMA_META) {
        puntos += 1000;
        nivel++;
        empezarNivel();
    }
}
Al final de Dibujo, después de dibujarEscenario:
void dibujarHUD(SDL_Renderer* render)
{
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
        SDL_RenderDebugText(render, 92, 96, "GAME OVER");
        SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
        SDL_RenderDebugText(render, 56, 110, "PULSA R PARA JUGAR");
    }
}
En main, dentro del bloque de eventos, debajo de la línea del espacio:
                if (e.key.key == SDLK_R && estado == Estado::GameOver) nuevaPartida();
En // 3. Actualizar, debajo de comprobarColisiones();:
            comprobarMeta();
            if (tiempoMensaje > 0) tiempoMensaje -= dt;
Y en // 4. Dibujar, la amiga justo después del robot y el marcador justo antes de SDL_RenderPresent:
        dibujarSprite(render, SPRITE_AMIGA, (int)std::size(SPRITE_AMIGA), 140, PLATAFORMAS[PLATAFORMA_META].y - 12);
        // ...barriles y jugador como antes...
        dibujarHUD(render);
Qué deberías ver: el juego completo. Marcador arriba, cartel "NIVEL 1" al empezar, 100 puntos por cada barril saltado y 1000 al llegar arriba. Al perder las tres vidas aparece GAME OVER, y R empieza otra partida.
Código completo
Este es el main.cpp final, igual a la suma de los ocho pasos. Si algo no compila, compáralo con el tuyo o pega este directamente.
// Escalada de barriles: un Donkey Kong de una pantalla con SDL3 + C++17
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>   // necesario en Windows para que SDL gestione main()
#include <vector>
#include <cstring>   // std::strlen
#include <cmath>     // std::fabs
#include <iterator>  // std::size

// ---------- Constantes ----------
constexpr int ANCHO = 256;     // resolución lógica retro
constexpr int ALTO  = 224;
constexpr int ESCALA = 3;      // tamaño de la ventana = resolución x escala

constexpr float GRAVEDAD      = 500.0f;   // píxeles por segundo al cuadrado
constexpr float VEL_JUGADOR   = 60.0f;    // píxeles por segundo
constexpr float VEL_SALTO     = -150.0f;  // negativa = hacia arriba
constexpr float VEL_ESCALAR   = 40.0f;
constexpr float ANCHO_JUGADOR = 8.0f;
constexpr float ALTO_JUGADOR  = 12.0f;

constexpr float TAM_BARRIL            = 8.0f;
constexpr float VEL_BARRIL_BASE       = 60.0f;
constexpr float VEL_BARRIL_ESCALERA   = 40.0f;
constexpr float INTERVALO_BARRIL_BASE = 2.5f;  // segundos entre barriles en el nivel 1
constexpr int   PROB_BAJAR_ESCALERA   = 30;    // % de veces que un barril baja por una escalera

constexpr float GROSOR_PLATAFORMA = 4.0f;
constexpr float ANCHO_ESCALERA    = 8.0f;
constexpr float ROBOT_X = 224.0f;   // el robot que lanza barriles
constexpr float ROBOT_Y = 32.0f;
constexpr int   VIDAS_INICIALES = 3;

// ---------- Escenario ----------
struct Plataforma { float x1, x2, y; };         // y = superficie, donde se apoyan los pies
struct Escalera   { float x, yArriba, yAbajo; }; // x = borde izquierdo

const Plataforma PLATAFORMAS[] = {
    {   0, 256, 208 },   // 0: suelo
    {   0, 232, 168 },   // 1: hueco a la derecha
    {  24, 256, 128 },   // 2: hueco a la izquierda
    {   0, 232,  88 },   // 3: hueco a la derecha
    {  24, 256,  48 },   // 4: aquí vive el robot
    { 100, 156,  20 },   // 5: la meta
};
constexpr int NUM_PLATAFORMAS = (int)std::size(PLATAFORMAS);
constexpr int PLATAFORMA_META = 5;

const Escalera ESCALERAS[] = {
    {  60, 168, 208 },   // suelo -> 1
    { 200, 168, 208 },   // suelo -> 1
    { 100, 128, 168 },   // 1 -> 2
    { 180,  88, 128 },   // 2 -> 3
    {  70,  48,  88 },   // 3 -> 4
    { 124,  20,  48 },   // 4 -> meta
};
constexpr int NUM_ESCALERAS = (int)std::size(ESCALERAS);

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

SDL_Color colorDe(char c)
{
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

// Dibuja un sprite píxel a píxel. Con espejo = true se dibuja mirando al otro lado
void dibujarSprite(SDL_Renderer* render, const char* const* filas, int numFilas, float x, float y, bool espejo = false)
{
    for (int f = 0; f < numFilas; ++f) {
        int ancho = (int)std::strlen(filas[f]);
        for (int c = 0; c < ancho; ++c) {
            char letra = filas[f][espejo ? ancho - 1 - c : c];
            if (letra == '.') continue;
            SDL_Color col = colorDe(letra);
            SDL_SetRenderDrawColor(render, col.r, col.g, col.b, 255);
            SDL_FRect pixel{ x + c, y + f, 1, 1 };
            SDL_RenderFillRect(render, &pixel);
        }
    }
}

// ---------- Estado del juego ----------
struct Jugador {
    float x = 0, y = 0;        // esquina superior izquierda
    float vy = 0;              // velocidad vertical
    bool enSuelo = true;
    bool escalando = false;
    int escalera = -1;         // escalera en la que está subido
    bool mirandoDerecha = true;
};

struct Barril {
    float x = 0, y = 0;
    float vx = 0;              // velocidad al rodar
    float vy = 0;              // velocidad al caer
    bool cayendo = false;
    int escalera = -1;         // >= 0 si está bajando por esa escalera
    bool puntuado = false;     // ya nos ha dado puntos por saltarlo
};

enum class Estado { Jugando, GameOver };

Jugador jugador;
std::vector<Barril> barriles;
Estado estado = Estado::Jugando;
int puntos = 0;
int vidas = VIDAS_INICIALES;
int nivel = 1;
float tiempoBarril = 0;     // cuenta atrás hasta el siguiente barril
float tiempoMensaje = 0;    // segundos que sigue visible el cartel "NIVEL X"

// ---------- Ayudas de colisión con el escenario ----------

// ¿Hay una plataforma justo bajo los pies? Devuelve su índice o -1
int plataformaBajo(float centroX, float pies)
{
    for (int i = 0; i < NUM_PLATAFORMAS; ++i) {
        const Plataforma& p = PLATAFORMAS[i];
        if (centroX >= p.x1 && centroX <= p.x2 && std::fabs(pies - p.y) < 0.5f) return i;
    }
    return -1;
}

// Al caer, ¿los pies han atravesado la superficie de una plataforma? Devuelve esa y, o -1
float superficieAlCaer(float centroX, float piesAntes, float piesAhora)
{
    for (const Plataforma& p : PLATAFORMAS) {
        if (centroX >= p.x1 && centroX <= p.x2 && piesAntes <= p.y && piesAhora >= p.y) return p.y;
    }
    return -1.0f;
}

// ¿Hay una escalera alineada con nosotros y los pies están dentro de su recorrido?
int escaleraEn(float centroX, float pies)
{
    for (int i = 0; i < NUM_ESCALERAS; ++i) {
        const Escalera& e = ESCALERAS[i];
        float centroEscalera = e.x + ANCHO_ESCALERA / 2;
        if (std::fabs(centroX - centroEscalera) <= 4.0f &&
            pies >= e.yArriba - 0.5f && pies <= e.yAbajo + 0.5f) return i;
    }
    return -1;
}

// ---------- Jugador ----------
void moverHorizontal(Jugador& j, const bool* teclas, float dt)
{
    float dx = 0;
    if (teclas[SDL_SCANCODE_LEFT])  dx -= VEL_JUGADOR * dt;
    if (teclas[SDL_SCANCODE_RIGHT]) dx += VEL_JUGADOR * dt;
    if (dx < 0) j.mirandoDerecha = false;
    if (dx > 0) j.mirandoDerecha = true;
    j.x += dx;
    // No salir de la pantalla
    if (j.x < 0) j.x = 0;
    if (j.x > ANCHO - ANCHO_JUGADOR) j.x = ANCHO - ANCHO_JUGADOR;
}

void aplicarGravedad(Jugador& j, float dt)
{
    float centroX = j.x + ANCHO_JUGADOR / 2;
    if (j.enSuelo) {
        // ¿Seguimos pisando algo? Si no, empezamos a caer
        if (plataformaBajo(centroX, j.y + ALTO_JUGADOR) >= 0) return;
        j.enSuelo = false;
        j.vy = 0;
    }
    float piesAntes = j.y + ALTO_JUGADOR;
    j.vy += GRAVEDAD * dt;    // la gravedad cambia la velocidad...
    j.y  += j.vy * dt;        // ...y la velocidad cambia la posición
    if (j.vy >= 0) {          // solo aterrizamos al bajar
        float suelo = superficieAlCaer(centroX, piesAntes, j.y + ALTO_JUGADOR);
        if (suelo >= 0) {
            j.y = suelo - ALTO_JUGADOR;
            j.vy = 0;
            j.enSuelo = true;
        }
    }
}

// Si estamos junto a una escalera y pulsamos arriba/abajo, nos agarramos a ella
bool intentarEscalar(Jugador& j, const bool* teclas)
{
    if (!j.enSuelo) return false;              // no nos agarramos en pleno salto
    bool arriba = teclas[SDL_SCANCODE_UP];
    bool abajo  = teclas[SDL_SCANCODE_DOWN];
    if (!arriba && !abajo) return false;

    float pies = j.y + ALTO_JUGADOR;
    int i = escaleraEn(j.x + ANCHO_JUGADOR / 2, pies);
    if (i < 0) return false;
    const Escalera& e = ESCALERAS[i];
    // Desde arriba del todo solo se puede bajar, y desde abajo del todo solo subir
    if (arriba && pies <= e.yArriba + 0.5f) return false;
    if (abajo  && pies >= e.yAbajo - 0.5f) return false;

    j.escalando = true;
    j.escalera = i;
    j.enSuelo = false;
    j.x = e.x + (ANCHO_ESCALERA - ANCHO_JUGADOR) / 2;   // centrarnos en la escalera
    return true;
}

void actualizarEscalada(Jugador& j, const bool* teclas, float dt)
{
    const Escalera& e = ESCALERAS[j.escalera];
    float pies = j.y + ALTO_JUGADOR;
    if (teclas[SDL_SCANCODE_UP])   pies -= VEL_ESCALAR * dt;
    if (teclas[SDL_SCANCODE_DOWN]) pies += VEL_ESCALAR * dt;
    // Al llegar a un extremo nos bajamos de la escalera
    if (pies <= e.yArriba) {
        pies = e.yArriba;
        j.escalando = false;
        j.enSuelo = true;
    } else if (pies >= e.yAbajo) {
        pies = e.yAbajo;
        j.escalando = false;
        j.enSuelo = true;
    }
    j.y = pies - ALTO_JUGADOR;
}

void actualizarJugador(Jugador& j, const bool* teclas, bool pedirSalto, float dt)
{
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
float velocidadBarril() { return VEL_BARRIL_BASE + 10.0f * (nivel - 1); }

float intervaloBarril()
{
    float t = INTERVALO_BARRIL_BASE - 0.3f * (nivel - 1);
    return t < 1.0f ? 1.0f : t;
}

void lanzarBarril()
{
    Barril b;
    b.x = ROBOT_X - TAM_BARRIL - 8;
    b.y = PLATAFORMAS[4].y - TAM_BARRIL;
    b.vx = -velocidadBarril();      // sale rodando hacia la izquierda
    barriles.push_back(b);
}

void actualizarBarril(Barril& b, float dt)
{
    float centroX = b.x + TAM_BARRIL / 2;
    float pies = b.y + TAM_BARRIL;

    // 1. Bajando por una escalera
    if (b.escalera >= 0) {
        const Escalera& e = ESCALERAS[b.escalera];
        pies += VEL_BARRIL_ESCALERA * dt;
        if (pies >= e.yAbajo) {
            pies = e.yAbajo;
            b.escalera = -1;
            b.vx = -b.vx;           // abajo sigue rodando en sentido contrario
        }
        b.y = pies - TAM_BARRIL;
        return;
    }

    // 2. Cayendo por el borde de una plataforma
    if (b.cayendo) {
        b.vy += GRAVEDAD * dt;
        b.y  += b.vy * dt;
        float suelo = superficieAlCaer(centroX, pies, b.y + TAM_BARRIL);
        if (suelo >= 0) {
            b.y = suelo - TAM_BARRIL;
            b.vy = 0;
            b.cayendo = false;
            b.vx = -b.vx;           // al aterrizar cambia de sentido
        }
        return;
    }

    // 3. Rodando sobre una plataforma
    float centroAntes = centroX;
    b.x += b.vx * dt;
    centroX = b.x + TAM_BARRIL / 2;
    if (plataformaBajo(centroX, pies) < 0) {   // se acabó la plataforma
        b.cayendo = true;
        return;
    }
    // ¿Acabamos de pasar por encima de una escalera que baja desde aquí?
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

void borrarBarrilesFuera()
{
    for (size_t i = 0; i < barriles.size(); ) {
        if (barriles[i].y > ALTO) barriles.erase(barriles.begin() + i);
        else ++i;
    }
}

// ---------- Partida ----------
void empezarNivel()
{
    jugador = Jugador{};
    jugador.x = 16;
    jugador.y = PLATAFORMAS[0].y - ALTO_JUGADOR;
    barriles.clear();
    tiempoBarril = 1.0f;
    tiempoMensaje = 2.0f;
}

void nuevaPartida()
{
    puntos = 0;
    vidas = VIDAS_INICIALES;
    nivel = 1;
    estado = Estado::Jugando;
    empezarNivel();
}

void perderVida()
{
    vidas--;
    if (vidas <= 0) estado = Estado::GameOver;
    else empezarNivel();
}

void comprobarColisiones()
{
    // Caja del jugador un poco más pequeña que el sprite: así el juego es más justo
    SDL_FRect cj{ jugador.x + 1, jugador.y + 2, ANCHO_JUGADOR - 2, ALTO_JUGADOR - 2 };

    SDL_FRect robot{ ROBOT_X, ROBOT_Y, 16, 16 };
    if (SDL_HasRectIntersectionFloat(&cj, &robot)) { perderVida(); return; }

    bool enElAire = !jugador.enSuelo && !jugador.escalando;
    float pies = jugador.y + ALTO_JUGADOR;
    for (Barril& b : barriles) {
        SDL_FRect cb{ b.x + 1, b.y + 1, TAM_BARRIL - 2, TAM_BARRIL - 2 };
        if (SDL_HasRectIntersectionFloat(&cj, &cb)) { perderVida(); return; }

        // Puntos por saltar por encima de un barril
        float distX = std::fabs((jugador.x + ANCHO_JUGADOR / 2) - (b.x + TAM_BARRIL / 2));
        if (enElAire && !b.puntuado && distX < 4 && pies <= b.y && b.y - pies < 24) {
            b.puntuado = true;
            puntos += 100;
        }
    }
}

void comprobarMeta()
{
    if (estado != Estado::Jugando || !jugador.enSuelo) return;
    if (plataformaBajo(jugador.x + ANCHO_JUGADOR / 2, jugador.y + ALTO_JUGADOR) == PLATAFORMA_META) {
        puntos += 1000;
        nivel++;
        empezarNivel();
    }
}

// ---------- Dibujo ----------
void dibujarEscenario(SDL_Renderer* render)
{
    // Escaleras primero, para que las plataformas queden por encima
    SDL_SetRenderDrawColor(render, 60, 200, 220, 255);
    for (const Escalera& e : ESCALERAS) {
        float der = e.x + ANCHO_ESCALERA - 1;
        SDL_RenderLine(render, e.x, e.yArriba, e.x, e.yAbajo);
        SDL_RenderLine(render, der, e.yArriba, der, e.yAbajo);
        for (float y = e.yArriba + 4; y < e.yAbajo; y += 4)
            SDL_RenderLine(render, e.x, y, der, y);   // peldaños
    }
    // Plataformas: viga roja con remaches oscuros
    for (const Plataforma& p : PLATAFORMAS) {
        SDL_FRect viga{ p.x1, p.y, p.x2 - p.x1, GROSOR_PLATAFORMA };
        SDL_SetRenderDrawColor(render, 200, 40, 70, 255);
        SDL_RenderFillRect(render, &viga);
        SDL_SetRenderDrawColor(render, 90, 10, 30, 255);
        for (float x = p.x1 + 2; x < p.x2 - 1; x += 8) {
            SDL_FRect remache{ x, p.y + 1, 2, 2 };
            SDL_RenderFillRect(render, &remache);
        }
    }
}

void dibujarHUD(SDL_Renderer* render)
{
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
        SDL_RenderDebugText(render, 92, 96, "GAME OVER");
        SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
        SDL_RenderDebugText(render, 56, 110, "PULSA R PARA JUGAR");
    }
}

int main(int argc, char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Error al iniciar SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* ventana = nullptr;
    SDL_Renderer* render = nullptr;
    if (!SDL_CreateWindowAndRenderer("Escalada de barriles", ANCHO * ESCALA, ALTO * ESCALA, 0, &ventana, &render)) {
        SDL_Log("Error al crear la ventana: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderLogicalPresentation(render, ANCHO, ALTO, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    SDL_SetRenderVSync(render, 1);
    SDL_srand(0);   // 0 = semilla distinta en cada partida

    nuevaPartida();

    Uint64 ultimo = SDL_GetTicksNS();
    bool ejecutando = true;
    while (ejecutando) {
        // 1. Tiempo transcurrido desde el fotograma anterior, en segundos
        Uint64 ahora = SDL_GetTicksNS();
        float dt = (ahora - ultimo) / 1'000'000'000.0f;
        ultimo = ahora;
        if (dt > 0.05f) dt = 0.05f;   // evita saltos enormes si la ventana se congela

        // 2. Eventos
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
            borrarBarrilesFuera();

            comprobarColisiones();
            comprobarMeta();
            if (tiempoMensaje > 0) tiempoMensaje -= dt;
        }

        // 4. Dibujar
        SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
        SDL_RenderClear(render);
        dibujarEscenario(render);
        dibujarSprite(render, SPRITE_ROBOT, (int)std::size(SPRITE_ROBOT), ROBOT_X, ROBOT_Y);
        dibujarSprite(render, SPRITE_AMIGA, (int)std::size(SPRITE_AMIGA), 140, PLATAFORMAS[PLATAFORMA_META].y - 12);
        for (const Barril& b : barriles)
            dibujarSprite(render, SPRITE_BARRIL, (int)std::size(SPRITE_BARRIL), b.x, b.y);
        dibujarSprite(render, SPRITE_JUGADOR, (int)std::size(SPRITE_JUGADOR), jugador.x, jugador.y, !jugador.mirandoDerecha);
        dibujarHUD(render);
        SDL_RenderPresent(render);
    }

    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(ventana);
    SDL_Quit();
    return 0;
}
Retos para ampliar
1.  Fácil: anima a Pixel al andar alternando dos sprites cada 0,15 s (por ejemplo, con las piernas abiertas y cerradas).
2.  Fácil: haz que el robot levante los brazos durante medio segundo justo antes de lanzar cada barril, para avisar al jugador.
3.  Medio: como en el original, pierde una vida si caes desde más de 30 píxeles. Guarda la y de los pies al empezar a caer y compárala al aterrizar.
4.  Medio: añade un martillo que aparezca en una viga: al cogerlo, durante 8 s los barriles que toques se destruyen y dan 300 puntos.
5.  Difícil: vigas inclinadas como en la recreativa. Cambia y por yIzquierda y yDerecha, e interpola la altura de la superficie según la x del personaje.
Errores frecuentes
• Pixel atraviesa las vigas al caer. Suele pasar si comparas pies == p.y: con decimales casi nunca coinciden exactamente. Usa la comprobación de cruce (piesAntes <= y y piesAhora >= y) y no quites el límite if (dt > 0.05f).
• No consigo subir la escalera. Tienes que estar casi centrado en ella (a 4 píxeles o menos) y en el suelo, no saltando. Si nunca funciona, revisa que en ESCALERAS cada yArriba/yAbajo coincida exactamente con la y de una viga.
• El programa se cierra o hace cosas raras al chocar con un barril. Falta el return después de perderVida(): esa función vacía barriles mientras el bucle aún la recorre.
• Errores como SDL_RenderDrawLine no declarado o e.key.keysym. Ese código es de SDL2. En SDL3 se usa SDL_RenderLine y e.key.key. Y si CMake falla al descargar SDL3, comprueba que Git está instalado y que tienes conexión; después usa Proyecto → Eliminar caché y volver a configurar.
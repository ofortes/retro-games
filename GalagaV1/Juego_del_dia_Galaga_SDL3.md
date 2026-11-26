# 🚀 Juego del día: Galaga en SDL3 + C++

> 🎨 **Cómo ver los colores:** abre este archivo en **Visual Studio 2026** (clic derecho → *Abrir con… → Editor de Markdown*, vista previa) o en **VS Code** (`Ctrl+Shift+V`). Los bloques de código salen resaltados: el **código** en colores (palabras clave, números, textos) y los **comentarios `// ...` en verde/gris**, así distingues de un vistazo qué es explicación y qué es código.

---

## 1. Qué vamos a construir

Un **Galaga simplificado**: tu nave está abajo y se mueve a izquierda y derecha. Arriba hay una formación de 24 naves enemigas que se balancea. De vez en cuando una se lanza **en picado** haciendo eses y disparando. Si sale por abajo, vuelve a su sitio. Tienes 3 vidas; si limpias la pantalla, empieza otra ola más rápida.

Y hoy, por primera vez, con **gráficos de verdad** en pixel art cargados desde archivos PNG.

```text
 PUNTOS 1250             VIDAS 3
      ▼ ▼ ▼ ▼ ▼ ▼ ▼ ▼        <- fila 0 (150 puntos)
      ▼ ▼ ▼ ▼ ▼   ▼ ▼        <- filas 1 y 2 (50 puntos)
      ▼ ▼ ▼   ▼ ▼ ▼ ▼
                 ▼  \        <- enemigo en picado (vale el doble)
            |      :         <- tu bala / bala enemiga
   ·      ·      ·     ·     <- estrellas que bajan
               ▲             <- tu nave
```

**Controles:** `←` `→` mover · `Espacio` disparar · `R` reiniciar tras Game Over · `Esc` salir.

## 2. Qué vas a aprender hoy

- **Cargar imágenes PNG** con `SDL_LoadPNG` (nuevo en SDL 3.4, sin librerías extra) y convertirlas en **texturas**.
- **Dibujar sprites** con `SDL_RenderTexture` y animar una explosión recortando una **tira de fotogramas**.
- **Estados de enemigo** con `enum class` + `switch`: en formación → en picado → volviendo.
- Movimiento hacia un punto (vector normalizado) y movimiento ondulado con `sin`/`cos`.

---

## 3. Preparar el proyecto

### 3.1 Descargar los gráficos

Los dos packs son gratuitos en [free-game-assets.itch.io](https://free-game-assets.itch.io/):

| Pack | Enlace | Qué sacar del zip |
|---|---|---|
| Free Enemy Spaceship 2D Sprites Pixel Art | https://free-game-assets.itch.io/free-enemy-spaceship-2d-sprites-pixel-art | **3 PNG** de naves *enteras* (no las carpetas de "body parts") |
| Free Pixel Art Explosions | https://free-game-assets.itch.io/11-free-pixel-art-explosion-sprites | **1 PNG** de explosión en *tira horizontal* (sprite sheet) |

En cada página: **Download Now** → *"No thanks, just take me to the downloads"* → descarga el `.zip`. Descomprime y **renombra** los archivos así:

| Nombre que usa el juego | Qué es |
|---|---|
| `nave.png` | Tu nave (elige la que más te guste del pack de naves) |
| `enemigo1.png` | Enemigo de la fila de arriba |
| `enemigo2.png` | Enemigo de las otras dos filas |
| `explosion.png` | Tira de animación de la explosión |

> ℹ️ El pack no trae "nave del jugador", así que usamos una de las enemigas. Si sale mirando al revés, cambia `VOLTEAR_NAVE` o `VOLTEAR_ENEMIGOS` a `true` en el código.
>
> 🛟 **Red de seguridad:** si falta algún PNG, el juego **no se rompe**: dibuja un cuadrado de color en su lugar y escribe un aviso en la ventana *Salida* de Visual Studio. Así puedes empezar a programar aunque aún no tengas los gráficos.
>
> 📜 Licencia: puedes usarlos en tus juegos (también comerciales), pero no redistribuir el pack tal cual. Detalles en https://craftpix.net/file-licenses/

### 3.2 Carpeta del proyecto

```text
Galaga/
├── CMakeLists.txt
├── main.cpp
└── assets/
    ├── nave.png
    ├── enemigo1.png
    ├── enemigo2.png
    └── explosion.png
```

### 3.3 `CMakeLists.txt`

Dos novedades respecto a juegos anteriores: subimos a **C++20** (para usar `std::erase_if`, que borra elementos de un vector en una línea) y **copiamos la carpeta `assets`** junto al `.exe` después de compilar.

```cmake
cmake_minimum_required(VERSION 3.24)
project(Galaga LANGUAGES C CXX)

# C++20: nos da std::erase_if para limpiar vectores fácilmente
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
# SDL3 enlazado de forma estática: así no hace falta copiar SDL3.dll junto al .exe
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.4.18   # 3.4 o superior: trae SDL_LoadPNG
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SDL3)

add_executable(juego main.cpp)
target_link_libraries(juego PRIVATE SDL3::SDL3)

# Copiar assets/ junto al .exe cada vez que se compila
add_custom_command(TARGET juego POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:juego>/assets
)
```

### 3.4 Abrir en Visual Studio 2026

1. Ten instalada la carga de trabajo **"Desarrollo para el escritorio con C++"** (incluye CMake y Git).
2. **Archivo → Abrir → Carpeta…** y elige `Galaga/`. La primera vez descarga y compila SDL3 (unos minutos).
3. En el desplegable de inicio elige **`juego.exe`** y pulsa **F5**.

---

## 4. Paso a paso

### Paso 1: Ventana vertical y bucle de juego

Galaga es una recreativa **vertical**, así que usamos 224×256 píxeles lógicos (escalados ×3). Empezamos con la plantilla de siempre más todas las constantes del juego, para tenerlas ya a mano.

```cpp
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>   // necesario en Windows para que SDL gestione main()
#include <cmath>
#include <string>
#include <vector>

// ---------- Pantalla (vertical, como la recreativa) ----------
constexpr int ANCHO  = 224;
constexpr int ALTO   = 256;
constexpr int ESCALA = 3;

// ---------- Jugador ----------
constexpr float TAM_NAVE          = 16.0f;
constexpr float VELOCIDAD_JUGADOR = 110.0f;
constexpr float Y_JUGADOR         = ALTO - 20.0f;
constexpr int   MAX_BALAS_JUGADOR = 2;      // en Galaga solo hay 2 disparos a la vez
constexpr float VELOCIDAD_BALA    = 240.0f;
constexpr float TIEMPO_INVENCIBLE = 2.0f;
constexpr int   VIDAS_INICIALES   = 3;

// ---------- Enemigos ----------
constexpr int   COLUMNAS          = 8;
constexpr int   FILAS             = 3;
constexpr float TAM_ENEMIGO       = 16.0f;
constexpr float SEPARACION_X      = 20.0f;
constexpr float SEPARACION_Y      = 18.0f;
constexpr float Y_FORMACION       = 40.0f;
constexpr float AMPLITUD_VAIVEN   = 16.0f;  // cuánto se balancea la formación
constexpr float VELOCIDAD_PICADO  = 70.0f;
constexpr float VELOCIDAD_REGRESO = 90.0f;
constexpr float INTERVALO_PICADO  = 2.0f;   // segundos entre picados (ola 1)
constexpr float VELOCIDAD_BALA_ENEMIGA = 110.0f;

// ---------- Efectos ----------
constexpr float DURACION_EXPLOSION = 0.5f;
constexpr int   NUM_ESTRELLAS      = 60;

// Si tus PNG miran hacia el lado contrario, pon esto a true
constexpr bool VOLTEAR_NAVE     = false;
constexpr bool VOLTEAR_ENEMIGOS = false;
```

Debajo va `main()` con la plantilla estándar (ventana, `SDL_SetRenderLogicalPresentation`, VSync, bucle con `dt`, eventos y `SDL_RenderClear`). Cámbiale el título a `"Galaga SDL3"` y añade esta línea tras calcular `dt`:

```cpp
if (dt > 0.05f) dt = 0.05f;   // evita saltos enormes si la ventana se congela
```

✅ **Qué deberías ver:** una ventana negra más alta que ancha. `Esc` la cierra.

---

### Paso 2: Cargar y dibujar tu primer sprite

**El porqué:** una imagen pasa por dos fases. `SDL_LoadPNG` la lee del disco a la **RAM** (`SDL_Surface`). Después `SDL_CreateTextureFromSurface` la sube a la **tarjeta gráfica** (`SDL_Texture`), que es lo que el renderer sabe dibujar rápido. La surface ya no hace falta y se libera.

Usamos `SDL_GetBasePath()` (la carpeta del `.exe`) para que encuentre `assets/` aunque Visual Studio ejecute el juego desde otra carpeta. Y `SDL_SCALEMODE_NEAREST` para que al escalar los píxeles se vean **nítidos**, no borrosos.

Añade esto **después de las constantes y antes de `main()`**:

```cpp
struct Sprite {
    SDL_Texture* tex = nullptr;   // nullptr = no se encontró el PNG
    float w = 0, h = 0;           // tamaño original de la imagen
};

// Carga un PNG de la carpeta assets/ que está junto al .exe
Sprite cargarSprite(SDL_Renderer* render, const char* nombre)
{
    Sprite s;
    std::string ruta = std::string(SDL_GetBasePath()) + "assets/" + nombre;

    SDL_Surface* sup = SDL_LoadPNG(ruta.c_str());       // PNG -> memoria RAM
    if (!sup) {
        SDL_Log("No se pudo cargar %s: %s", ruta.c_str(), SDL_GetError());
        return s;                                       // se dibujará un rectángulo
    }
    s.tex = SDL_CreateTextureFromSurface(render, sup);  // RAM -> tarjeta gráfica
    SDL_DestroySurface(sup);                            // la copia en RAM ya no hace falta
    if (s.tex) {
        SDL_SetTextureScaleMode(s.tex, SDL_SCALEMODE_NEAREST);  // píxeles nítidos
        SDL_GetTextureSize(s.tex, &s.w, &s.h);
    }
    return s;
}

// Dibuja un sprite centrado en (cx, cy) que cabe en un cuadrado de lado 'tam'.
// Si no hay imagen, dibuja un rectángulo del color indicado.
void dibujarSprite(SDL_Renderer* render, const Sprite& s, float cx, float cy, float tam,
                   bool voltear, Uint8 r, Uint8 g, Uint8 b)
{
    if (!s.tex) {
        SDL_FRect rect{ cx - tam / 2, cy - tam / 2, tam, tam };
        SDL_SetRenderDrawColor(render, r, g, b, 255);
        SDL_RenderFillRect(render, &rect);
        return;
    }
    // Mantener la proporción de la imagen
    float escala = tam / SDL_max(s.w, s.h);
    float w = s.w * escala, h = s.h * escala;
    SDL_FRect dst{ cx - w / 2, cy - h / 2, w, h };
    SDL_RenderTextureRotated(render, s.tex, nullptr, &dst, 0.0, nullptr,
                             voltear ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE);
}
```

> 💡 Los PNG del pack son grandes (más de 16 píxeles). `dibujarSprite` los **reduce** para que quepan en 16×16 sin deformarlos. Por eso trabajamos con el **centro** del objeto (`cx, cy`) en vez de la esquina: es más cómodo con sprites.

En `main()`, **justo después de `SDL_SetRenderVSync`**:

```cpp
SDL_srand(0);   // semilla aleatoria basada en la hora

// --- Cargar los gráficos ---
Sprite sprNave      = cargarSprite(render, "nave.png");
Sprite sprEnemigo1  = cargarSprite(render, "enemigo1.png");
Sprite sprEnemigo2  = cargarSprite(render, "enemigo2.png");
Sprite sprExplosion = cargarSprite(render, "explosion.png");

// --- Estado de la partida ---
float jugadorX = ANCHO / 2.0f;
```

En la parte de **dibujar**, entre `SDL_RenderClear` y `SDL_RenderPresent`:

```cpp
dibujarSprite(render, sprNave, jugadorX, Y_JUGADOR, TAM_NAVE, VOLTEAR_NAVE, 60, 200, 255);
```

Y al final, **antes de `SDL_DestroyRenderer`**, libera las texturas:

```cpp
// Liberar texturas (SDL_DestroyTexture acepta nullptr sin problema)
SDL_DestroyTexture(sprNave.tex);
SDL_DestroyTexture(sprEnemigo1.tex);
SDL_DestroyTexture(sprEnemigo2.tex);
SDL_DestroyTexture(sprExplosion.tex);
```

✅ **Qué deberías ver:** tu nave pixelada abajo en el centro. Si ves un **cuadrado azul**, el PNG no se ha encontrado: mira la ventana *Salida*.

---

### Paso 3: Mover la nave, disparar y cielo de estrellas

**El porqué:** el movimiento continuo se lee con `SDL_GetKeyboardState` (mientras mantienes la tecla). El disparo, en cambio, va en el evento `KEY_DOWN` con `!e.key.repeat`: un disparo por pulsación. Como en Galaga, **máximo 2 balas** en pantalla: si ya hay 2, no se dispara.

Añade estas estructuras **junto a `struct Sprite`**:

```cpp
struct Bala      { float x, y, vy; };
struct Explosion { float x, y, t; };
struct Estrella  { float x, y, vel; Uint8 brillo; };
```

Tras cargar los gráficos, crea las estrellas y los vectores (amplía el bloque de estado):

```cpp
// --- Estrellas del fondo ---
std::vector<Estrella> estrellas;
for (int i = 0; i < NUM_ESTRELLAS; i++) {
    estrellas.push_back({ SDL_randf() * ANCHO, SDL_randf() * ALTO,
                          20.0f + SDL_randf() * 60.0f, (Uint8)(80 + SDL_rand(176)) });
}

// --- Estado de la partida ---
float jugadorX = ANCHO / 2.0f;
int   vidas = VIDAS_INICIALES;
int   puntos = 0;
int   ola = 1;
float invencible = 0;          // >0 = parpadea y no le dan
bool  gameOver = false;
float tiempo = 0;              // reloj general (para el vaivén)
float temporizadorPicado = INTERVALO_PICADO;
float mensajeOla = 2.0f;       // tiempo que se muestra "OLA n"

std::vector<Bala>      balasJugador;
std::vector<Bala>      balasEnemigas;
std::vector<Explosion> explosiones;
```

En los **eventos**, cambia el `if` de la tecla por este (más adelante le añadiremos la R):

```cpp
if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
    if (e.key.key == SDLK_ESCAPE) jugando = false;

    // Disparar (máximo 2 balas en pantalla)
    if (e.key.key == SDLK_SPACE && !gameOver &&
        (int)balasJugador.size() < MAX_BALAS_JUGADOR) {
        balasJugador.push_back({ jugadorX, Y_JUGADOR - 8, -VELOCIDAD_BALA });
    }
}
```

En **actualizar**:

```cpp
tiempo += dt;

// Estrellas: bajan siempre, aunque sea game over
for (Estrella& s : estrellas) {
    s.y += s.vel * dt;
    if (s.y > ALTO) { s.y = 0; s.x = SDL_randf() * ANCHO; }
}

if (!gameOver) {
    // --- Mover al jugador ---
    const bool* teclas = SDL_GetKeyboardState(nullptr);
    if (teclas[SDL_SCANCODE_LEFT])  jugadorX -= VELOCIDAD_JUGADOR * dt;
    if (teclas[SDL_SCANCODE_RIGHT]) jugadorX += VELOCIDAD_JUGADOR * dt;
    jugadorX = SDL_clamp(jugadorX, TAM_NAVE / 2, ANCHO - TAM_NAVE / 2);

    // --- Mover balas ---
    for (Bala& b : balasJugador)  b.y += b.vy * dt;
    for (Bala& b : balasEnemigas) b.y += b.vy * dt;
    // std::erase_if (C++20) borra del vector las balas que han salido de la pantalla
    std::erase_if(balasJugador,  [](const Bala& b) { return b.y < -8; });
    std::erase_if(balasEnemigas, [](const Bala& b) { return b.y > ALTO + 8; });

    // (aquí irán los enemigos en los próximos pasos)
}
```

En **dibujar**, las estrellas van **primero** (detrás de todo) y las balas después de la nave:

```cpp
for (const Estrella& s : estrellas) {
    SDL_SetRenderDrawColor(render, s.brillo, s.brillo, s.brillo, 255);
    SDL_RenderPoint(render, s.x, s.y);
}

// ... aquí la nave ...

SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
for (const Bala& b : balasJugador) {
    SDL_FRect r{ b.x - 1, b.y - 3, 2, 6 };
    SDL_RenderFillRect(render, &r);
}
SDL_SetRenderDrawColor(render, 255, 80, 80, 255);
for (const Bala& b : balasEnemigas) {
    SDL_FRect r{ b.x - 1, b.y - 3, 2, 6 };
    SDL_RenderFillRect(render, &r);
}
```

> 💡 `[](const Bala& b) { return b.y < -8; }` es una **lambda**: una función pequeñita escrita en el sitio. `erase_if` la llama para cada bala y borra las que devuelven `true`.

✅ **Qué deberías ver:** estrellas cayendo (sensación de avanzar por el espacio), la nave moviéndose y disparando de 2 en 2.

---

### Paso 4: La formación enemiga con vaivén

**El porqué:** cada enemigo tiene un **hueco fijo** en la formación (fila, columna). La formación entera se balancea sumando `sin(tiempo)` a la X: el seno va de -1 a 1 suavemente, así que multiplicado por 16 da un ir y venir de ±16 píxeles.

También preparamos ya los **estados** del enemigo. Un `enum class` es una lista cerrada de valores con nombre; mucho más claro que usar `0, 1, 2, 3`.

Junto a las demás estructuras:

```cpp
enum class Estado { Formacion, Picado, Volviendo, Muerto };

struct Enemigo {
    int fila = 0, col = 0;
    float x = 0, y = 0;           // centro del enemigo
    Estado estado = Estado::Formacion;
    float t = 0;                  // tiempo que lleva en picado
    float cadencia = 0;           // tiempo hasta su próximo disparo
};
```

Y estas funciones **antes de `main()`** (después de las de sprites):

```cpp
// Rectángulo de colisión a partir del centro y el tamaño
SDL_FRect rectDe(float cx, float cy, float tam)
{
    return SDL_FRect{ cx - tam / 2, cy - tam / 2, tam, tam };
}

// Posición del hueco de un enemigo dentro de la formación
void posicionHueco(int fila, int col, float vaiven, float& x, float& y)
{
    float anchoFormacion = (COLUMNAS - 1) * SEPARACION_X;
    x = (ANCHO - anchoFormacion) / 2 + col * SEPARACION_X + vaiven;
    y = Y_FORMACION + fila * SEPARACION_Y;
}

void crearFormacion(std::vector<Enemigo>& enemigos)
{
    enemigos.clear();
    for (int f = 0; f < FILAS; f++) {
        for (int c = 0; c < COLUMNAS; c++) {
            Enemigo e;
            e.fila = f;
            e.col = c;
            posicionHueco(f, c, 0, e.x, e.y);
            enemigos.push_back(e);
        }
    }
}
```

En el estado de la partida, junto a los otros vectores:

```cpp
std::vector<Enemigo> enemigos;
crearFormacion(enemigos);
```

En **actualizar**, dentro de `if (!gameOver)`, donde dejamos el hueco:

```cpp
// --- Mover enemigos según su estado ---
float vaiven = std::sin(tiempo * 1.2f) * AMPLITUD_VAIVEN;
for (Enemigo& en : enemigos) {
    float huecoX, huecoY;
    posicionHueco(en.fila, en.col, vaiven, huecoX, huecoY);

    switch (en.estado) {
    case Estado::Formacion:
        en.x = huecoX;
        en.y = huecoY;
        break;
    // (los demás estados en el paso 6)
    default:
        break;
    }
}
```

En **dibujar**, antes de la nave:

```cpp
for (const Enemigo& en : enemigos) {
    if (en.estado == Estado::Muerto) continue;
    if (en.fila == 0) dibujarSprite(render, sprEnemigo1, en.x, en.y, TAM_ENEMIGO, VOLTEAR_ENEMIGOS, 220, 40, 40);
    else              dibujarSprite(render, sprEnemigo2, en.x, en.y, TAM_ENEMIGO, VOLTEAR_ENEMIGOS, 240, 220, 40);
}
```

✅ **Qué deberías ver:** 3 filas de 8 naves balanceándose suavemente. Las balas aún las atraviesan.

---

### Paso 5: Derribar enemigos con explosiones animadas

**El porqué:** la explosión es una **tira horizontal** de fotogramas cuadrados. Si la imagen mide 384×64, hay 384/64 = **6 fotogramas**. Según el tiempo que lleva la explosión (de 0 a 1), elegimos qué trozo recortar con el rectángulo `src`, y `SDL_RenderTexture` dibuja solo ese trozo.

Función nueva, junto a `dibujarSprite`:

```cpp
// Dibuja un fotograma de una tira horizontal de animación (fotogramas cuadrados)
void dibujarExplosion(SDL_Renderer* render, const Sprite& s, const Explosion& ex)
{
    float progreso = ex.t / DURACION_EXPLOSION;          // de 0 a 1
    if (!s.tex) {
        float tam = 4 + progreso * 16;                   // cuadrado que crece
        SDL_FRect rect{ ex.x - tam / 2, ex.y - tam / 2, tam, tam };
        SDL_SetRenderDrawColor(render, 255, 140, 0, 255);
        SDL_RenderFillRect(render, &rect);
        return;
    }
    int fotogramas = SDL_max(1, (int)(s.w / s.h));       // p. ej. 384x64 -> 6
    int actual = SDL_min((int)(progreso * fotogramas), fotogramas - 1);
    SDL_FRect src{ actual * s.h, 0, s.h, s.h };          // trozo de la tira
    SDL_FRect dst{ ex.x - 12, ex.y - 12, 24, 24 };
    SDL_RenderTexture(render, s.tex, &src, &dst);
}
```

En **actualizar**, justo después de mover las estrellas (fuera del `if (!gameOver)`, para que las explosiones terminen aunque pierdas):

```cpp
// Explosiones: avanzan y se borran al terminar
for (Explosion& ex : explosiones) ex.t += dt;
std::erase_if(explosiones, [](const Explosion& ex) { return ex.t >= DURACION_EXPLOSION; });
```

Dentro de `if (!gameOver)`, **después** de mover los enemigos:

```cpp
// --- Colisión: balas del jugador contra enemigos ---
for (Bala& b : balasJugador) {
    SDL_FRect rBala{ b.x - 1, b.y - 3, 2, 6 };
    for (Enemigo& en : enemigos) {
        if (en.estado == Estado::Muerto) continue;
        SDL_FRect rEnemigo = rectDe(en.x, en.y, TAM_ENEMIGO - 2);
        if (SDL_HasRectIntersectionFloat(&rBala, &rEnemigo)) {
            // En picado vale el doble, como en la recreativa
            int valor = (en.fila == 0) ? 150 : 50;
            if (en.estado != Estado::Formacion) valor *= 2;
            puntos += valor;
            en.estado = Estado::Muerto;
            explosiones.push_back({ en.x, en.y, 0 });
            b.y = -100;              // la bala se borrará en el próximo fotograma
            break;
        }
    }
}
```

En **dibujar**, después de las balas, y el marcador encima de todo:

```cpp
for (const Explosion& ex : explosiones) dibujarExplosion(render, sprExplosion, ex);

// Marcador (fuente de 8x8 píxeles)
SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
SDL_RenderDebugTextFormat(render, 4, 4, "PUNTOS %d", puntos);
SDL_RenderDebugTextFormat(render, ANCHO - 60, 4, "VIDAS %d", vidas);
```

> 💡 ¿Por qué `b.y = -100` en vez de borrar la bala? Porque **no se debe borrar elementos de un vector mientras lo recorres** con `for (Bala& b : ...)`: el bucle se rompería. La mandamos fuera de la pantalla y el `erase_if` del siguiente fotograma la limpia.

✅ **Qué deberías ver:** al acertar, la nave enemiga desaparece con una explosión animada y suben los puntos.

---

### Paso 6: Los picados (máquina de estados)

**El porqué:** cada enemigo está siempre en **un** estado, y el `switch` decide qué hace según cuál:

```text
 Formacion ──(lo elige el temporizador)──> Picado
     ▲                                       │
     └──(llega a su hueco)── Volviendo <──(sale por abajo)
```

- **Picado:** baja a velocidad constante. En X se acerca un poco al jugador y además hace **eses** con `cos(t)`.
- **Volviendo:** reaparece arriba y va **en línea recta** a su hueco. Para ir de A a B: calculamos el vector `(dx, dy)`, lo dividimos por su longitud (así mide 1, se dice *normalizar*) y lo multiplicamos por la velocidad.

En **actualizar**, dentro de `if (!gameOver)`, **antes** del bucle de enemigos:

```cpp
// --- Elegir un enemigo para que baje en picado ---
temporizadorPicado -= dt;
if (temporizadorPicado <= 0) {
    std::vector<int> candidatos;
    for (int i = 0; i < (int)enemigos.size(); i++)
        if (enemigos[i].estado == Estado::Formacion) candidatos.push_back(i);
    if (!candidatos.empty()) {
        Enemigo& elegido = enemigos[candidatos[SDL_rand((int)candidatos.size())]];
        elegido.estado = Estado::Picado;
        elegido.t = 0;
        elegido.cadencia = 0.5f + SDL_randf();
    }
    temporizadorPicado = INTERVALO_PICADO / (1.0f + (ola - 1) * 0.3f);
}
```

Y **sustituye** el `default:` del `switch` por los tres estados que faltan:

```cpp
case Estado::Picado: {
    en.t += dt;
    en.y += (VELOCIDAD_PICADO + ola * 5) * dt;
    // Se acerca al jugador y además hace eses
    float direccion = (jugadorX > en.x) ? 1.0f : -1.0f;
    en.x += direccion * 35.0f * dt + std::cos(en.t * 4.0f) * 70.0f * dt;

    // Dispara de vez en cuando, solo si aún está por encima del jugador
    en.cadencia -= dt;
    if (en.cadencia <= 0 && en.y < Y_JUGADOR - 50) {
        balasEnemigas.push_back({ en.x, en.y + 8, VELOCIDAD_BALA_ENEMIGA });
        en.cadencia = 0.8f + SDL_randf();
    }

    // Si sale por abajo, reaparece arriba y vuelve a su hueco
    if (en.y > ALTO + TAM_ENEMIGO) {
        en.y = -TAM_ENEMIGO;
        en.estado = Estado::Volviendo;
    }
    break;
}

case Estado::Volviendo: {
    float dx = huecoX - en.x, dy = huecoY - en.y;
    float distancia = std::sqrt(dx * dx + dy * dy);
    float paso = VELOCIDAD_REGRESO * dt;
    if (distancia <= paso) {
        en.estado = Estado::Formacion;   // ya ha llegado
    } else {
        en.x += dx / distancia * paso;   // avanza en línea recta
        en.y += dy / distancia * paso;
    }
    break;
}

case Estado::Muerto:
    break;
```

> 💡 Las llaves `{ }` en `case Estado::Picado: { ... }` son necesarias porque declaramos variables (`direccion`) dentro del `case`. Sin ellas, el compilador protesta.

✅ **Qué deberías ver:** cada 2 segundos una nave sale de la formación, baja haciendo eses y disparando balas rojas, desaparece por abajo y vuelve a su sitio desde arriba. Derribarla en picado da el doble de puntos. Las balas rojas aún no te hacen nada.

---

### Paso 7: Vidas, daño e invencibilidad

**El porqué:** el jugador muere si le toca una **bala enemiga** o un **enemigo en picado** (choque). Tras recibir un golpe pasa 2 segundos **invencible** y parpadeando, para que no pierda todas las vidas de golpe.

En **actualizar**, al principio de `if (!gameOver)`, tras limitar `jugadorX`:

```cpp
if (invencible > 0) invencible -= dt;
if (mensajeOla > 0) mensajeOla -= dt;
```

Después de la colisión de las balas del jugador:

```cpp
// --- Colisión: balas enemigas y enemigos en picado contra el jugador ---
if (invencible <= 0) {
    SDL_FRect rJugador = rectDe(jugadorX, Y_JUGADOR, TAM_NAVE - 4);
    bool tocado = false;
    for (Bala& b : balasEnemigas) {
        SDL_FRect rBala{ b.x - 1, b.y - 3, 2, 6 };
        if (SDL_HasRectIntersectionFloat(&rBala, &rJugador)) tocado = true;
    }
    for (Enemigo& en : enemigos) {
        if (en.estado != Estado::Picado) continue;
        SDL_FRect rEnemigo = rectDe(en.x, en.y, TAM_ENEMIGO - 2);
        if (SDL_HasRectIntersectionFloat(&rEnemigo, &rJugador)) {
            tocado = true;
            en.estado = Estado::Muerto;          // el enemigo también explota
            explosiones.push_back({ en.x, en.y, 0 });
        }
    }
    if (tocado) {
        vidas--;
        explosiones.push_back({ jugadorX, Y_JUGADOR, 0 });
        balasEnemigas.clear();
        invencible = TIEMPO_INVENCIBLE;
        if (vidas <= 0) gameOver = true;
    }
}
```

En **dibujar**, **sustituye** la línea de la nave por:

```cpp
// La nave parpadea mientras es invencible (se dibuja 1 de cada 2 décimas)
bool visible = invencible <= 0 || ((int)(invencible * 10) % 2 == 0);
if (!gameOver && visible)
    dibujarSprite(render, sprNave, jugadorX, Y_JUGADOR, TAM_NAVE, VOLTEAR_NAVE, 60, 200, 255);
```

> 💡 Las colisiones usan rectángulos **un poco más pequeños** que el sprite (`TAM_NAVE - 4`). Es un truco clásico: el jugador siente que el juego es justo cuando una bala que "casi" le roza no le mata.

✅ **Qué deberías ver:** si te dan, tu nave explota, pierdes una vida y parpadea 2 segundos.

---

### Paso 8: Olas, Game Over y reiniciar

**El porqué:** cerramos el bucle del juego: si no queda ningún enemigo vivo, nueva ola (los picados son más frecuentes y rápidos porque dependen de `ola`). Si las vidas llegan a 0, Game Over, y con **R** se reinicia todo.

En **actualizar**, al final de `if (!gameOver)`:

```cpp
// --- ¿Ola superada? ---
bool quedanEnemigos = false;
for (const Enemigo& en : enemigos)
    if (en.estado != Estado::Muerto) quedanEnemigos = true;
if (!quedanEnemigos) {
    ola++;
    mensajeOla = 2.0f;
    balasEnemigas.clear();
    crearFormacion(enemigos);
}
```

En los **eventos**, dentro del `if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat)`, tras el disparo:

```cpp
// Reiniciar la partida
if (e.key.key == SDLK_R && gameOver) {
    vidas = VIDAS_INICIALES;
    puntos = 0;
    ola = 1;
    jugadorX = ANCHO / 2.0f;
    invencible = 0;
    gameOver = false;
    mensajeOla = 2.0f;
    temporizadorPicado = INTERVALO_PICADO;
    balasJugador.clear();
    balasEnemigas.clear();
    explosiones.clear();
    crearFormacion(enemigos);
}
```

En **dibujar**, después del marcador:

```cpp
if (mensajeOla > 0 && !gameOver)
    SDL_RenderDebugTextFormat(render, ANCHO / 2 - 24, ALTO / 2, "OLA %d", ola);

if (gameOver) {
    SDL_RenderDebugText(render, ANCHO / 2 - 36, ALTO / 2 - 8, "GAME OVER");
    SDL_RenderDebugText(render, ANCHO / 2 - 84, ALTO / 2 + 8, "PULSA R PARA REINICIAR");
}
```

✅ **Qué deberías ver:** "OLA 1" al empezar; al limpiar la pantalla, "OLA 2" con enemigos más agresivos. Al perder las 3 vidas, "GAME OVER" y la R te devuelve al inicio. ¡Juego completo! 🎉

---

## 5. Código completo (`main.cpp`)

```cpp
// ============================================================
//  GALAGA SIMPLIFICADO - SDL3 + C++20
//  Gráficos: packs gratuitos de free-game-assets.itch.io
// ============================================================
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>   // necesario en Windows para que SDL gestione main()
#include <cmath>
#include <string>
#include <vector>

// ---------- Pantalla (vertical, como la recreativa) ----------
constexpr int ANCHO  = 224;
constexpr int ALTO   = 256;
constexpr int ESCALA = 3;

// ---------- Jugador ----------
constexpr float TAM_NAVE          = 16.0f;
constexpr float VELOCIDAD_JUGADOR = 110.0f;
constexpr float Y_JUGADOR         = ALTO - 20.0f;
constexpr int   MAX_BALAS_JUGADOR = 2;      // en Galaga solo hay 2 disparos a la vez
constexpr float VELOCIDAD_BALA    = 240.0f;
constexpr float TIEMPO_INVENCIBLE = 2.0f;
constexpr int   VIDAS_INICIALES   = 3;

// ---------- Enemigos ----------
constexpr int   COLUMNAS          = 8;
constexpr int   FILAS             = 3;
constexpr float TAM_ENEMIGO       = 16.0f;
constexpr float SEPARACION_X      = 20.0f;
constexpr float SEPARACION_Y      = 18.0f;
constexpr float Y_FORMACION       = 40.0f;
constexpr float AMPLITUD_VAIVEN   = 16.0f;  // cuánto se balancea la formación
constexpr float VELOCIDAD_PICADO  = 70.0f;
constexpr float VELOCIDAD_REGRESO = 90.0f;
constexpr float INTERVALO_PICADO  = 2.0f;   // segundos entre picados (ola 1)
constexpr float VELOCIDAD_BALA_ENEMIGA = 110.0f;

// ---------- Efectos ----------
constexpr float DURACION_EXPLOSION = 0.5f;
constexpr int   NUM_ESTRELLAS      = 60;

// Si tus PNG miran hacia el lado contrario, pon esto a true
constexpr bool VOLTEAR_NAVE     = false;
constexpr bool VOLTEAR_ENEMIGOS = false;

// ============================================================
//  Estructuras
// ============================================================
struct Sprite {
    SDL_Texture* tex = nullptr;   // nullptr = no se encontró el PNG
    float w = 0, h = 0;           // tamaño original de la imagen
};

enum class Estado { Formacion, Picado, Volviendo, Muerto };

struct Enemigo {
    int fila = 0, col = 0;
    float x = 0, y = 0;           // centro del enemigo
    Estado estado = Estado::Formacion;
    float t = 0;                  // tiempo que lleva en picado
    float cadencia = 0;           // tiempo hasta su próximo disparo
};

struct Bala      { float x, y, vy; };
struct Explosion { float x, y, t; };
struct Estrella  { float x, y, vel; Uint8 brillo; };

// ============================================================
//  Funciones de imágenes
// ============================================================

// Carga un PNG de la carpeta assets/ que está junto al .exe
Sprite cargarSprite(SDL_Renderer* render, const char* nombre)
{
    Sprite s;
    std::string ruta = std::string(SDL_GetBasePath()) + "assets/" + nombre;

    SDL_Surface* sup = SDL_LoadPNG(ruta.c_str());       // PNG -> memoria RAM
    if (!sup) {
        SDL_Log("No se pudo cargar %s: %s", ruta.c_str(), SDL_GetError());
        return s;                                       // se dibujará un rectángulo
    }
    s.tex = SDL_CreateTextureFromSurface(render, sup);  // RAM -> tarjeta gráfica
    SDL_DestroySurface(sup);                            // la copia en RAM ya no hace falta
    if (s.tex) {
        SDL_SetTextureScaleMode(s.tex, SDL_SCALEMODE_NEAREST);  // píxeles nítidos
        SDL_GetTextureSize(s.tex, &s.w, &s.h);
    }
    return s;
}

// Dibuja un sprite centrado en (cx, cy) que cabe en un cuadrado de lado 'tam'.
// Si no hay imagen, dibuja un rectángulo del color indicado.
void dibujarSprite(SDL_Renderer* render, const Sprite& s, float cx, float cy, float tam,
                   bool voltear, Uint8 r, Uint8 g, Uint8 b)
{
    if (!s.tex) {
        SDL_FRect rect{ cx - tam / 2, cy - tam / 2, tam, tam };
        SDL_SetRenderDrawColor(render, r, g, b, 255);
        SDL_RenderFillRect(render, &rect);
        return;
    }
    // Mantener la proporción de la imagen
    float escala = tam / SDL_max(s.w, s.h);
    float w = s.w * escala, h = s.h * escala;
    SDL_FRect dst{ cx - w / 2, cy - h / 2, w, h };
    SDL_RenderTextureRotated(render, s.tex, nullptr, &dst, 0.0, nullptr,
                             voltear ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE);
}

// Dibuja un fotograma de una tira horizontal de animación (fotogramas cuadrados)
void dibujarExplosion(SDL_Renderer* render, const Sprite& s, const Explosion& ex)
{
    float progreso = ex.t / DURACION_EXPLOSION;          // de 0 a 1
    if (!s.tex) {
        float tam = 4 + progreso * 16;                   // cuadrado que crece
        SDL_FRect rect{ ex.x - tam / 2, ex.y - tam / 2, tam, tam };
        SDL_SetRenderDrawColor(render, 255, 140, 0, 255);
        SDL_RenderFillRect(render, &rect);
        return;
    }
    int fotogramas = SDL_max(1, (int)(s.w / s.h));       // p. ej. 384x64 -> 6
    int actual = SDL_min((int)(progreso * fotogramas), fotogramas - 1);
    SDL_FRect src{ actual * s.h, 0, s.h, s.h };          // trozo de la tira
    SDL_FRect dst{ ex.x - 12, ex.y - 12, 24, 24 };
    SDL_RenderTexture(render, s.tex, &src, &dst);
}

// ============================================================
//  Funciones del juego
// ============================================================

// Rectángulo de colisión a partir del centro y el tamaño
SDL_FRect rectDe(float cx, float cy, float tam)
{
    return SDL_FRect{ cx - tam / 2, cy - tam / 2, tam, tam };
}

// Posición del hueco de un enemigo dentro de la formación
void posicionHueco(int fila, int col, float vaiven, float& x, float& y)
{
    float anchoFormacion = (COLUMNAS - 1) * SEPARACION_X;
    x = (ANCHO - anchoFormacion) / 2 + col * SEPARACION_X + vaiven;
    y = Y_FORMACION + fila * SEPARACION_Y;
}

void crearFormacion(std::vector<Enemigo>& enemigos)
{
    enemigos.clear();
    for (int f = 0; f < FILAS; f++) {
        for (int c = 0; c < COLUMNAS; c++) {
            Enemigo e;
            e.fila = f;
            e.col = c;
            posicionHueco(f, c, 0, e.x, e.y);
            enemigos.push_back(e);
        }
    }
}

// ============================================================
//  Programa principal
// ============================================================
int main(int argc, char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Error al iniciar SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* ventana = nullptr;
    SDL_Renderer* render = nullptr;
    if (!SDL_CreateWindowAndRenderer("Galaga SDL3", ANCHO * ESCALA, ALTO * ESCALA, 0, &ventana, &render)) {
        SDL_Log("Error al crear la ventana: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderLogicalPresentation(render, ANCHO, ALTO, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    SDL_SetRenderVSync(render, 1);
    SDL_srand(0);   // semilla aleatoria basada en la hora

    // --- Cargar los gráficos ---
    Sprite sprNave      = cargarSprite(render, "nave.png");
    Sprite sprEnemigo1  = cargarSprite(render, "enemigo1.png");
    Sprite sprEnemigo2  = cargarSprite(render, "enemigo2.png");
    Sprite sprExplosion = cargarSprite(render, "explosion.png");

    // --- Estrellas del fondo ---
    std::vector<Estrella> estrellas;
    for (int i = 0; i < NUM_ESTRELLAS; i++) {
        estrellas.push_back({ SDL_randf() * ANCHO, SDL_randf() * ALTO,
                              20.0f + SDL_randf() * 60.0f, (Uint8)(80 + SDL_rand(176)) });
    }

    // --- Estado de la partida ---
    float jugadorX = ANCHO / 2.0f;
    int   vidas = VIDAS_INICIALES;
    int   puntos = 0;
    int   ola = 1;
    float invencible = 0;          // >0 = parpadea y no le dan
    bool  gameOver = false;
    float tiempo = 0;              // reloj general (para el vaivén)
    float temporizadorPicado = INTERVALO_PICADO;
    float mensajeOla = 2.0f;       // tiempo que se muestra "OLA n"

    std::vector<Enemigo>   enemigos;
    std::vector<Bala>      balasJugador;
    std::vector<Bala>      balasEnemigas;
    std::vector<Explosion> explosiones;
    crearFormacion(enemigos);

    Uint64 ultimo = SDL_GetTicksNS();
    bool jugando = true;
    while (jugando) {
        // 1. Tiempo transcurrido desde el fotograma anterior, en segundos
        Uint64 ahora = SDL_GetTicksNS();
        float dt = (ahora - ultimo) / 1'000'000'000.0f;
        ultimo = ahora;
        if (dt > 0.05f) dt = 0.05f;   // evita saltos enormes si la ventana se congela

        // 2. Eventos
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) jugando = false;
            if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
                if (e.key.key == SDLK_ESCAPE) jugando = false;

                // Disparar (máximo 2 balas en pantalla)
                if (e.key.key == SDLK_SPACE && !gameOver &&
                    (int)balasJugador.size() < MAX_BALAS_JUGADOR) {
                    balasJugador.push_back({ jugadorX, Y_JUGADOR - 8, -VELOCIDAD_BALA });
                }

                // Reiniciar la partida
                if (e.key.key == SDLK_R && gameOver) {
                    vidas = VIDAS_INICIALES;
                    puntos = 0;
                    ola = 1;
                    jugadorX = ANCHO / 2.0f;
                    invencible = 0;
                    gameOver = false;
                    mensajeOla = 2.0f;
                    temporizadorPicado = INTERVALO_PICADO;
                    balasJugador.clear();
                    balasEnemigas.clear();
                    explosiones.clear();
                    crearFormacion(enemigos);
                }
            }
        }

        // 3. Actualizar
        tiempo += dt;

        // Estrellas: bajan siempre, aunque sea game over
        for (Estrella& s : estrellas) {
            s.y += s.vel * dt;
            if (s.y > ALTO) { s.y = 0; s.x = SDL_randf() * ANCHO; }
        }

        // Explosiones: avanzan y se borran al terminar
        for (Explosion& ex : explosiones) ex.t += dt;
        std::erase_if(explosiones, [](const Explosion& ex) { return ex.t >= DURACION_EXPLOSION; });

        if (!gameOver) {
            // --- Mover al jugador ---
            const bool* teclas = SDL_GetKeyboardState(nullptr);
            if (teclas[SDL_SCANCODE_LEFT])  jugadorX -= VELOCIDAD_JUGADOR * dt;
            if (teclas[SDL_SCANCODE_RIGHT]) jugadorX += VELOCIDAD_JUGADOR * dt;
            jugadorX = SDL_clamp(jugadorX, TAM_NAVE / 2, ANCHO - TAM_NAVE / 2);
            if (invencible > 0) invencible -= dt;
            if (mensajeOla > 0) mensajeOla -= dt;

            // --- Mover balas ---
            for (Bala& b : balasJugador)  b.y += b.vy * dt;
            for (Bala& b : balasEnemigas) b.y += b.vy * dt;
            // std::erase_if (C++20) borra del vector las balas que han salido de la pantalla
            std::erase_if(balasJugador,  [](const Bala& b) { return b.y < -8; });
            std::erase_if(balasEnemigas, [](const Bala& b) { return b.y > ALTO + 8; });

            // --- Elegir un enemigo para que baje en picado ---
            temporizadorPicado -= dt;
            if (temporizadorPicado <= 0) {
                std::vector<int> candidatos;
                for (int i = 0; i < (int)enemigos.size(); i++)
                    if (enemigos[i].estado == Estado::Formacion) candidatos.push_back(i);
                if (!candidatos.empty()) {
                    Enemigo& elegido = enemigos[candidatos[SDL_rand((int)candidatos.size())]];
                    elegido.estado = Estado::Picado;
                    elegido.t = 0;
                    elegido.cadencia = 0.5f + SDL_randf();
                }
                temporizadorPicado = INTERVALO_PICADO / (1.0f + (ola - 1) * 0.3f);
            }

            // --- Mover enemigos según su estado ---
            float vaiven = std::sin(tiempo * 1.2f) * AMPLITUD_VAIVEN;
            for (Enemigo& en : enemigos) {
                float huecoX, huecoY;
                posicionHueco(en.fila, en.col, vaiven, huecoX, huecoY);

                switch (en.estado) {
                case Estado::Formacion:
                    en.x = huecoX;
                    en.y = huecoY;
                    break;

                case Estado::Picado: {
                    en.t += dt;
                    en.y += (VELOCIDAD_PICADO + ola * 5) * dt;
                    // Se acerca al jugador y además hace eses
                    float direccion = (jugadorX > en.x) ? 1.0f : -1.0f;
                    en.x += direccion * 35.0f * dt + std::cos(en.t * 4.0f) * 70.0f * dt;

                    // Dispara de vez en cuando, solo si aún está por encima del jugador
                    en.cadencia -= dt;
                    if (en.cadencia <= 0 && en.y < Y_JUGADOR - 50) {
                        balasEnemigas.push_back({ en.x, en.y + 8, VELOCIDAD_BALA_ENEMIGA });
                        en.cadencia = 0.8f + SDL_randf();
                    }

                    // Si sale por abajo, reaparece arriba y vuelve a su hueco
                    if (en.y > ALTO + TAM_ENEMIGO) {
                        en.y = -TAM_ENEMIGO;
                        en.estado = Estado::Volviendo;
                    }
                    break;
                }

                case Estado::Volviendo: {
                    float dx = huecoX - en.x, dy = huecoY - en.y;
                    float distancia = std::sqrt(dx * dx + dy * dy);
                    float paso = VELOCIDAD_REGRESO * dt;
                    if (distancia <= paso) {
                        en.estado = Estado::Formacion;   // ya ha llegado
                    } else {
                        en.x += dx / distancia * paso;   // avanza en línea recta
                        en.y += dy / distancia * paso;
                    }
                    break;
                }

                case Estado::Muerto:
                    break;
                }
            }

            // --- Colisión: balas del jugador contra enemigos ---
            for (Bala& b : balasJugador) {
                SDL_FRect rBala{ b.x - 1, b.y - 3, 2, 6 };
                for (Enemigo& en : enemigos) {
                    if (en.estado == Estado::Muerto) continue;
                    SDL_FRect rEnemigo = rectDe(en.x, en.y, TAM_ENEMIGO - 2);
                    if (SDL_HasRectIntersectionFloat(&rBala, &rEnemigo)) {
                        // En picado vale el doble, como en la recreativa
                        int valor = (en.fila == 0) ? 150 : 50;
                        if (en.estado != Estado::Formacion) valor *= 2;
                        puntos += valor;
                        en.estado = Estado::Muerto;
                        explosiones.push_back({ en.x, en.y, 0 });
                        b.y = -100;              // la bala se borrará en el próximo fotograma
                        break;
                    }
                }
            }

            // --- Colisión: balas enemigas y enemigos en picado contra el jugador ---
            if (invencible <= 0) {
                SDL_FRect rJugador = rectDe(jugadorX, Y_JUGADOR, TAM_NAVE - 4);
                bool tocado = false;
                for (Bala& b : balasEnemigas) {
                    SDL_FRect rBala{ b.x - 1, b.y - 3, 2, 6 };
                    if (SDL_HasRectIntersectionFloat(&rBala, &rJugador)) tocado = true;
                }
                for (Enemigo& en : enemigos) {
                    if (en.estado != Estado::Picado) continue;
                    SDL_FRect rEnemigo = rectDe(en.x, en.y, TAM_ENEMIGO - 2);
                    if (SDL_HasRectIntersectionFloat(&rEnemigo, &rJugador)) {
                        tocado = true;
                        en.estado = Estado::Muerto;          // el enemigo también explota
                        explosiones.push_back({ en.x, en.y, 0 });
                    }
                }
                if (tocado) {
                    vidas--;
                    explosiones.push_back({ jugadorX, Y_JUGADOR, 0 });
                    balasEnemigas.clear();
                    invencible = TIEMPO_INVENCIBLE;
                    if (vidas <= 0) gameOver = true;
                }
            }

            // --- ¿Ola superada? ---
            bool quedanEnemigos = false;
            for (const Enemigo& en : enemigos)
                if (en.estado != Estado::Muerto) quedanEnemigos = true;
            if (!quedanEnemigos) {
                ola++;
                mensajeOla = 2.0f;
                balasEnemigas.clear();
                crearFormacion(enemigos);
            }
        }

        // 4. Dibujar
        SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
        SDL_RenderClear(render);

        for (const Estrella& s : estrellas) {
            SDL_SetRenderDrawColor(render, s.brillo, s.brillo, s.brillo, 255);
            SDL_RenderPoint(render, s.x, s.y);
        }

        for (const Enemigo& en : enemigos) {
            if (en.estado == Estado::Muerto) continue;
            if (en.fila == 0) dibujarSprite(render, sprEnemigo1, en.x, en.y, TAM_ENEMIGO, VOLTEAR_ENEMIGOS, 220, 40, 40);
            else              dibujarSprite(render, sprEnemigo2, en.x, en.y, TAM_ENEMIGO, VOLTEAR_ENEMIGOS, 240, 220, 40);
        }

        // La nave parpadea mientras es invencible (se dibuja 1 de cada 2 décimas)
        bool visible = invencible <= 0 || ((int)(invencible * 10) % 2 == 0);
        if (!gameOver && visible)
            dibujarSprite(render, sprNave, jugadorX, Y_JUGADOR, TAM_NAVE, VOLTEAR_NAVE, 60, 200, 255);

        SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
        for (const Bala& b : balasJugador) {
            SDL_FRect r{ b.x - 1, b.y - 3, 2, 6 };
            SDL_RenderFillRect(render, &r);
        }
        SDL_SetRenderDrawColor(render, 255, 80, 80, 255);
        for (const Bala& b : balasEnemigas) {
            SDL_FRect r{ b.x - 1, b.y - 3, 2, 6 };
            SDL_RenderFillRect(render, &r);
        }

        for (const Explosion& ex : explosiones) dibujarExplosion(render, sprExplosion, ex);

        // Marcador (fuente de 8x8 píxeles)
        SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
        SDL_RenderDebugTextFormat(render, 4, 4, "PUNTOS %d", puntos);
        SDL_RenderDebugTextFormat(render, ANCHO - 60, 4, "VIDAS %d", vidas);

        if (mensajeOla > 0 && !gameOver)
            SDL_RenderDebugTextFormat(render, ANCHO / 2 - 24, ALTO / 2, "OLA %d", ola);

        if (gameOver) {
            SDL_RenderDebugText(render, ANCHO / 2 - 36, ALTO / 2 - 8, "GAME OVER");
            SDL_RenderDebugText(render, ANCHO / 2 - 84, ALTO / 2 + 8, "PULSA R PARA REINICIAR");
        }

        SDL_RenderPresent(render);
    }

    // Liberar texturas (SDL_DestroyTexture acepta nullptr sin problema)
    SDL_DestroyTexture(sprNave.tex);
    SDL_DestroyTexture(sprEnemigo1.tex);
    SDL_DestroyTexture(sprEnemigo2.tex);
    SDL_DestroyTexture(sprExplosion.tex);
    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(ventana);
    SDL_Quit();
    return 0;
}
```

---

## 6. Retos para ampliar

1. ⭐ **Fácil — récord de la sesión:** guarda `int record` y actualízalo en el Game Over. Muéstralo arriba en el centro.
2. ⭐ **Fácil — más variedad:** añade `enemigo3.png` para la fila de abajo (otra nave del pack) y otra puntuación.
3. ⭐⭐ **Medio — enemigos que giran:** en picado, calcula el ángulo de movimiento con `std::atan2(vy, vx)` (pásalo a grados) y úsalo en el 5.º parámetro de `SDL_RenderTextureRotated` para que la nave "mire" hacia donde va.
4. ⭐⭐ **Medio — animación de la nave:** si el pack trae varios fotogramas del motor, ponlos en tira y reutiliza la idea de `dibujarExplosion` para animarla en bucle.
5. ⭐⭐⭐ **Difícil — sonido:** carga un `.wav` con `SDL_LoadWAV` y reprodúcelo con `SDL_OpenAudioDeviceStream` + `SDL_PutAudioStreamData` al disparar y al explotar.

---

## 7. Errores frecuentes

| Problema | Causa y solución |
|---|---|
| Veo **cuadrados de colores** en vez de naves | El PNG no está en `assets/` junto al `.exe`. Mira la ventana **Salida**: el mensaje `No se pudo cargar ...` te dice la ruta exacta que buscó. Revisa los nombres (minúsculas, extensión `.png` real y no `.png.png`, que Windows oculta). Tras añadir o cambiar PNG, usa **Compilar → Recompilar todo** para que CMake los vuelva a copiar. |
| `SDL_LoadPNG` **no está declarado** | Tu SDL3 es anterior a 3.4. Comprueba `GIT_TAG release-3.4.18` en el `CMakeLists.txt` y borra la caché: **Proyecto → Eliminar caché y volver a configurar**. |
| `'erase_if' no es miembro de 'std'` | Falta C++20. Revisa `set(CMAKE_CXX_STANDARD 20)` y vuelve a configurar CMake. |
| Las naves salen **borrosas** | Falta `SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST)` en `cargarSprite`. |
| Las naves salen **boca abajo** o la explosión se ve como una tira estirada | Cambia `VOLTEAR_NAVE` / `VOLTEAR_ENEMIGOS`. Para la explosión, elige un PNG en **tira horizontal** (más ancho que alto); si solo tienes fotogramas sueltos, sirve uno solo (se verá estático). |

# Retro Games

Colección de juegos retro 2D de estilo 8 bits escritos en **C++** con **SDL3**, desarrollados con Visual Studio. Cada juego es una versión propia, con personajes y arte originales, inspirada en un clásico arcade.

## Juegos

| Juego | Carpeta | Inspirado en | Estado |
|-------|---------|--------------|--------|
| Escalada de barriles | [`Donkey Kong/`](Donkey%20Kong/) | Donkey Kong | Jugable |
| Cúpulas de Selene | [`missile-command/`](missile-command/) | Missile Command | En desarrollo |

### Escalada de barriles

Un robot lanza barriles desde lo alto. Manejas a **Pixel**, que tiene que subir por vigas y escaleras hasta la plataforma superior para rescatar a su amiga.

- **Controles:** ← → para andar, ↑ ↓ para subir y bajar escaleras, `Espacio` para saltar, `R` para reiniciar tras el game over y `Esc` para salir.
- **Reglas:** empiezas con 3 vidas. Saltar un barril da 100 puntos y llegar arriba da 1000. En cada nivel los barriles van más rápido y salen más a menudo.
- El tutorial paso a paso está en [`Donkey Kong/tutorial.md`](Donkey%20Kong/tutorial.md).

### Cúpulas de Selene

Hay que defender las cúpulas de una base lunar de los misiles que caen del cielo. El juego se dibuja a 320×180 con una paleta de 16 colores ("Noche lunar") y se escala ×4 a una ventana de 1280×720.

- El código está organizado en módulos: `Config.h`, `Entities.h`, `Game.*` y `Draw.*`.

## Requisitos

- Windows x64
- Visual Studio 2022 o posterior con la carga de trabajo **Desarrollo para el escritorio con C++** (toolset v143)
- Bibliotecas de desarrollo de SDL (paquetes `-VC`):
  - SDL3 3.3.4
  - SDL3_image 3.2.4
  - SDL3_ttf 3.1.0
  - SDL2_mixer 2.8.1

## Compilar y ejecutar

1. Clona el repositorio:
   ```bash
   git clone https://github.com/ofortes/retro-games.git
   ```
2. Abre `RetroGames.slnx` en Visual Studio.
3. Comprueba las rutas de SDL. Los proyectos buscan las bibliotecas en `D:\DEV JUEGOS\C++\SDL3\...`. Si las tienes en otro sitio, cambia en las propiedades de cada proyecto:
   - **C/C++ → General → Directorios de inclusión adicionales**
   - **Vinculador → General → Directorios de bibliotecas adicionales**
4. Copia las DLL (`SDL3.dll`, `SDL3_image.dll`, `SDL3_ttf.dll`, `SDL2_mixer.dll`) a la carpeta del ejecutable (`x64/Debug` o `x64/Release`).
5. Elige el juego como proyecto de inicio y pulsa `F5`.

## Flujo de trabajo

- `develop` es la rama de trabajo y la rama por defecto.
- `main` solo se actualiza con merges desde `develop`.

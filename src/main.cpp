#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "../include/pokeapp/App.h"

int main() {
	App app;
	if (!app.init()) {
		return 1;
	}
	while (app.running()) {
		app.tick();
		SDL_Delay(16); // Roughly 60 FPS
	}
	return 0;
}
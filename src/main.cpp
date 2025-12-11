#include <SDL2/SDL.h>
#include <iostream>

#include "Game.hpp"

int main() {
	Game game;
	if (!game.init("SDL2 Test Window", 800, 600)) {
		return 1;
	}
	game.run();
	game.clean();
	return 0;
}

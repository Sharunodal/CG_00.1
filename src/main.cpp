#include <SDL3/SDL.h>
#include <iostream>

#include "Game.hpp"

int main() {
	Game game;
	if (!game.init("SDL3 Test Window", 800, 600)) {
		return 1;
	}
	game.run();
	game.clean();
	return 0;
}

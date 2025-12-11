#ifndef GAME_HPP
#define GAME_HPP

# include <SDL2/SDL.h>
# include <string>
# include "Player.hpp"

class Game {
	public:
		bool init(const std::string& title, int width, int height);
		void run();
		void clean();

	private:
		void initGL();
		void loadShaders();
		void createFloorMesh();
		void loadFloorTexture();
		void processEvents();
		void update(float dt);
		void render();

		Player player;
		SDL_Window* window {nullptr};
		SDL_GLContext glContext {nullptr};
		bool running {false};

		unsigned int shaderProgram = 0;
		unsigned int vao = 0;
		unsigned int textureID = 0;

		// shadow texture (generated at runtime)
		unsigned int shadowTexture = 0;

		int winWidth = 1200;
		int winHeight = 1000;
};

#endif

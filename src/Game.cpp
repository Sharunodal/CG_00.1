#include "Game.hpp"
#include <iostream>
#include "thirdparty/glad/include/glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "thirdparty/stb_image.h"
#include <fstream>
#include <sstream>

static std::string loadFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

void Game::loadShaders() {
    std::string vertSrc = loadFile("src/shaders/floor.vert");
    std::string fragSrc = loadFile("src/shaders/floor.frag");

    GLuint v = glCreateShader(GL_VERTEX_SHADER);
    const char* vsrc = vertSrc.c_str();
    glShaderSource(v, 1, &vsrc, nullptr);
    glCompileShader(v);

    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fsrc = fragSrc.c_str();
    glShaderSource(f, 1, &fsrc, nullptr);
    glCompileShader(f);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, v);
    glAttachShader(shaderProgram, f);
    glLinkProgram(shaderProgram);

    glDeleteShader(v);
    glDeleteShader(f);
}

bool Game::init(const std::string& title, int width, int height) {
    winWidth = width;
    winHeight = height;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "Window error: " << SDL_GetError() << std::endl;
        return false;
    }

    glContext = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, glContext);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to init GLAD" << std::endl;
        return false;
    }

    glEnable(GL_DEPTH_TEST);

    loadShaders();
    createFloorMesh();
    loadFloorTexture();

    player.loadTexture("assets/Characters/Chicken.png");
    player.initMesh();

    running = true;
    return true;
}

void Game::createFloorMesh() {
    float verts[] = {
        // x,y,z    u,v
        -5,0,-5,   0,0,
         5,0,-5,   1,0,
         5,0, 5,   1,1,
        -5,0, 5,   0,1
    };

    unsigned int idx[] = { 0,1,2,  2,3,0 };

    GLuint vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Game::loadFloorTexture() {
    int w, h, n;
    unsigned char* data = stbi_load("assets/textures/AoE/g_gr6_00_color.png", &w, &h, &n, 4);
    if (!data) {
        std::cerr << "Failed to load texture\n";
        return;
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
}

void Game::run() {
    Uint64 last = SDL_GetPerformanceCounter();
    const double freq = static_cast<double>(SDL_GetPerformanceFrequency());

    while (running) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>((now - last) / freq);
        last = now;

        processEvents();
        update(dt);
        render();
    }
}

void Game::processEvents() {
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) running = false;
	}
}

void Game::update(float dt) {
    const Uint8* keys = SDL_GetKeyboardState(NULL);

    const float speed = 3.0f;

    if (keys[SDL_SCANCODE_W]) player.position.z -= speed * dt;
    if (keys[SDL_SCANCODE_S]) player.position.z += speed * dt;
    if (keys[SDL_SCANCODE_A]) player.position.x -= speed * dt;
    if (keys[SDL_SCANCODE_D]) player.position.x += speed * dt;
}

void Game::render() {
    glViewport(0, 0, winWidth, winHeight);
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);

    // Camera
    glm::vec3 cameraPos    = glm::vec3(5.0f, 5.0f, 5.0f);
    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 cameraUp     = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
    glm::mat4 proj = glm::perspective(glm::radians(60.0f),
                                      float(winWidth) / float(winHeight),
                                      0.1f,
                                      100.0f);

    GLint loc = glGetUniformLocation(shaderProgram, "uMVP");

    // Render floor
    {
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 mvp   = proj * view * model;

        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mvp));

        glBindTexture(GL_TEXTURE_2D, textureID);    // floor texture
        glBindVertexArray(vao);                     // floor mesh
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    // Render player sprite
    {
        glm::mat4 model = glm::mat4(1.0f);
        
        // place player into world
        model = glm::translate(model, player.position);

        // texture faces camera
        glm::mat4 billboard = glm::mat4(glm::mat3(view));
        model = billboard * model;

        glm::mat4 mvp = proj * view * model;
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mvp));

        glBindTexture(GL_TEXTURE_2D, player.textureID);
        glBindVertexArray(player.vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    SDL_GL_SwapWindow(window);
}

void Game::clean() {
    glDeleteTextures(1, &textureID);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgram);

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

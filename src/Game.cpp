#include "Game.hpp"
#include <iostream>
#include "thirdparty/glad/include/glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib>
#include <cstring>
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

    // Try initializing events subsystem
    bool evInit = SDL_Init(SDL_INIT_EVENTS);
    if (!evInit) {
        std::cerr << "SDL_Init(SDL_INIT_EVENTS) failed, SDL_GetError(): '" << SDL_GetError() << "'\n";
        return false;
    }

    bool initResult = SDL_InitSubSystem(SDL_INIT_VIDEO);
    if (!initResult) {
        std::cerr << "SDL_InitSubSystem(SDL_INIT_VIDEO) failed, SDL_GetError(): '" << SDL_GetError() << "'\n";

        // Try forcing a dummy video driver
        const char* cur = getenv("SDL_VIDEODRIVER");
        if (!cur || strlen(cur) == 0) {
#if defined(_WIN32) || defined(__MINGW32__)
            _putenv_s("SDL_VIDEODRIVER", "dummy");
#else
            setenv("SDL_VIDEODRIVER", "dummy", 1);
#endif
            bool retry = SDL_InitSubSystem(SDL_INIT_VIDEO);
            if (!retry) {
                std::cerr << "Retry SDL_InitSubSystem(SDL_INIT_VIDEO) failed, SDL_GetError(): '" << SDL_GetError() << "'\n";
                return false;
            }
        } else {
            return false;
        }
    }

    // Probe audio subsystem
    bool audioInit = SDL_InitSubSystem(SDL_INIT_AUDIO);
    if (!audioInit) {
        std::cerr << "Warning: SDL_InitSubSystem(SDL_INIT_AUDIO) failed, SDL_GetError(): '" << SDL_GetError() << "'\n";

        // Try a dummy audio backend if available
        const char* curAud = getenv("SDL_AUDIODRIVER");
        if (!curAud || strlen(curAud) == 0) {
#if defined(_WIN32) || defined(__MINGW32__)
            _putenv_s("SDL_AUDIODRIVER", "dummy");
#else
            setenv("SDL_AUDIODRIVER", "dummy", 1);
#endif
            bool retryAud = SDL_InitSubSystem(SDL_INIT_AUDIO);
            if (!retryAud) {
                std::cerr << "Retry SDL_InitSubSystem(SDL_INIT_AUDIO) failed, SDL_GetError(): '" << SDL_GetError() << "'\n";
            }
        }
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // SDL3 CreateWindow
    window = SDL_CreateWindow(
        title.c_str(),
        width, height,
        SDL_WINDOW_OPENGL
    );

    if (!window) {
        std::cerr << "Window error: " << SDL_GetError() << std::endl;
        return false;
    }

    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "Failed to create GL context: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_MakeCurrent(window, glContext);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to init GLAD" << std::endl;
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    loadShaders();
    createFloorMesh();
    loadFloorTexture();

    player.loadTexture("assets/Characters/Sheet2.png");
    player.initMesh();
    player.setFloorHeight(0.0f);  // Floor is at Y = 0
    player.setAnimation(4, 7, 4, 0.1f); // 4 columns x 7 rows, 4 frames per row

    // Load shadow PNG
    {
        int w, h, n;
        unsigned char* data = stbi_load("assets/Characters/shadow.png", &w, &h, &n, 4);
        if (!data) {
            std::cerr << "Failed to load shadow texture: assets/Characters/shadow.png\n";
            return false;
        }

        glGenTextures(1, &shadowTexture);
        glBindTexture(GL_TEXTURE_2D, shadowTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        stbi_image_free(data);
    }

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
        if (e.type == SDL_EVENT_QUIT) {
            running = false;
        }
    }
}

void Game::update(float dt) {
    const bool* keys = SDL_GetKeyboardState(NULL);
    const float speed = 3.0f;

    // Forward and right projected onto ground plane
    glm::vec3 cameraPos = glm::vec3(5,5,5);
    glm::vec3 cameraTarget = glm::vec3(0,0,0);

    glm::vec3 forward = glm::normalize(glm::vec3(cameraTarget - cameraPos));
    forward.y = 0.0f;
    forward = glm::normalize(forward);

    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0,1,0)));

    int movementDirection = 0;  // 1 for right, -1 for left, 0 for no movement
    bool moving = false;

    glm::vec2 dir2(0.0f, 0.0f);
    if (keys[SDL_SCANCODE_W]) {
        player.position += forward * speed * dt;
        moving = true;
        movementDirection = 1;
        dir2.y += 1.0f;
    }
    if (keys[SDL_SCANCODE_S]) {
        player.position -= forward * speed * dt;
        moving = true;
        movementDirection = 1;
        dir2.y -= 1.0f;
    }
    if (keys[SDL_SCANCODE_A]) {
        player.position -= right * speed * dt;
        moving = true;
        movementDirection = -1;
        dir2.x -= 1.0f;
    }
    if (keys[SDL_SCANCODE_D]) {
        player.position += right * speed * dt;
        moving = true;
        movementDirection = 1;
        dir2.x += 1.0f;
    }
    
    // Handle jump
    if (keys[SDL_SCANCODE_SPACE]) {
        player.jump();
    }
    
    // track last non-zero move direction for idle facing selection
    if (glm::length(dir2) > 0.001f) {
        lastMoveDir = glm::normalize(dir2);
    }

    // compute facing index from lastMoveDir
    glm::vec2 lv = glm::normalize(lastMoveDir);
    float angleDeg = glm::degrees(atan2(lv.x, lv.y));
    float aa = fabs(angleDeg);
    int facingIdx = 0;
    if (aa < 22.5f) facingIdx = 0;
    else if (aa < 67.5f) facingIdx = 1;
    else if (aa < 112.5f) facingIdx = 2;
    else if (aa < 157.5f) facingIdx = 3;
    else facingIdx = 4;
    player.facingIndex = facingIdx;

    // set activeRow depending on movement vs idle
    if (moving) {
        int walkingRow = 6;
        if (facingIdx == 3 || facingIdx == 4) walkingRow = 5;
        else if (facingIdx == 2) walkingRow = (lv.y >= 0.0f) ? 6 : 5;
        if (player.activeRow != walkingRow) {
            player.activeRow = walkingRow;
            player.frameIndex = 0;
        }
    } else {
        int idleRow = glm::clamp(player.facingIndex, 0, 4);
        if (player.activeRow != idleRow) {
            player.activeRow = idleRow;
            player.frameIndex = 0;
        }
    }

    player.updateAnimation(dt, moving, movementDirection);
    // player handles gravity
    player.update(dt);
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
    GLint locCols = glGetUniformLocation(shaderProgram, "uCols");
    GLint locRows = glGetUniformLocation(shaderProgram, "uRows");
    GLint locFrame = glGetUniformLocation(shaderProgram, "uFrame");
    GLint locMirror = glGetUniformLocation(shaderProgram, "uMirror");
    GLint locUseColor = glGetUniformLocation(shaderProgram, "uUseColor");
    GLint locColor = glGetUniformLocation(shaderProgram, "uColor");
    GLint locShadowInner = glGetUniformLocation(shaderProgram, "uShadowInner");
    GLint locShadowOuter = glGetUniformLocation(shaderProgram, "uShadowOuter");

    // Render floor
    {
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 mvp   = proj * view * model;

        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniform1i(locCols, 1);
        glUniform1i(locRows, 1);
        glUniform1i(locFrame, 0);
        glUniform1i(locMirror, 1);
        // ensure no tinting from previous draws
        glUniform4f(locColor, 1.0f, 1.0f, 1.0f, 1.0f);

        glBindTexture(GL_TEXTURE_2D, textureID);    // floor texture
        glBindVertexArray(vao);                     // floor mesh
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    // Render player sprite
    {
        // Render soft shadow under player
        {
            glm::mat4 shadowModel = glm::mat4(1.0f);
            // position at player's x/z, just above the floor to avoid z-fighting
            glm::vec3 shadowPos = player.position;
            shadowPos.y = player.floorY + 0.01f;
            shadowModel = glm::translate(shadowModel, shadowPos);
            // rotate quad to lie flat on XZ plane
            shadowModel = glm::rotate(shadowModel, glm::radians(-90.0f), glm::vec3(1,0,0));

            // shrink and soften shadow while airborne
            float sx = player.isGrounded ? 0.8f : 0.5f;
            float sz = player.isGrounded ? 0.5f : 0.3f;
            float shadowAlpha = player.isGrounded ? 1.0f : 0.55f;
            shadowModel = glm::scale(shadowModel, glm::vec3(sx, 1.0f, sz));

            glm::mat4 shadowMVP = proj * view * shadowModel;
            glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(shadowMVP));

            if (shadowTexture != 0) {
                // textured shadow: bind generated soft texture and modulate alpha via uColor
                glUniform1i(locUseColor, 0);
                glBindTexture(GL_TEXTURE_2D, shadowTexture);
                glUniform4f(locColor, 1.0f, 1.0f, 1.0f, shadowAlpha);
                glUniform1i(locCols, 1);
                glUniform1i(locRows, 1);
                glUniform1i(locFrame, 0);
                glUniform1i(locMirror, 1);
            } else {
                // fallback: procedural radial shadow
                glUniform1i(locUseColor, 2);
                glUniform4f(locColor, 0.0f, 0.0f, 0.0f, shadowAlpha);
                glUniform1f(locShadowInner, 0.12f);
                glUniform1f(locShadowOuter, 0.42f);
                glUniform1i(locCols, 1);
                glUniform1i(locRows, 1);
                glUniform1i(locFrame, 0);
                glUniform1i(locMirror, 1);
            }

            // draw using the player quad VAO (rotated to lie flat)
            glBindVertexArray(player.vao);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // restore to texture mode and reset tint to white so subsequent draws are unaffected
            glUniform1i(locUseColor, 0);
            glUniform4f(locColor, 1.0f, 1.0f, 1.0f, 1.0f);
        }

        // compute view/proj
        glm::mat4 model = glm::mat4(1.0f);

        // place player into world
        model = glm::translate(model, player.position);

        // create billboard rotation so quad faces camera
        glm::mat4 billboard = glm::mat4(glm::transpose(glm::mat3(view)));
        // rotate upside down (appeared wrong before)
        billboard = glm::rotate(billboard, glm::radians(180.0f), glm::vec3(1,0,0));
        model = model * billboard;

        glm::mat4 mvp = proj * view * model;
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mvp));

        // Set animation uniforms
        glUniform1i(locCols, player.animCols);
        glUniform1i(locRows, player.animRows);
        // compose global frame number = row*cols + frameIndex
        int frameNumber = player.activeRow * player.animCols + player.frameIndex;
        glUniform1i(locFrame, frameNumber);
        glUniform1i(locMirror, player.facingDirection);

        glDepthMask(GL_FALSE);  // Disable depth writing for transparent sprite
        glBindTexture(GL_TEXTURE_2D, player.textureID);
        glBindVertexArray(player.vao);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glDepthMask(GL_TRUE);   // Re-enable depth writing
    }

    SDL_GL_SwapWindow(window);
}

void Game::clean() {
    glDeleteTextures(1, &textureID);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgram);

    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

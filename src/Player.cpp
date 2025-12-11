#include "Player.hpp"
#include "thirdparty/stb_image.h"
#include "thirdparty/glad/include/glad/glad.h"
#include <iostream>

Player::Player() : position(0.0f, 0.5f, 0.0f) {}

void Player::setFloorHeight(float floorHeight) {
    floorY = floorHeight;
    // Place player on top of the floor (mesh is now centered, so bottom is at -height/2)
    position.y = floorY + height * 0.5f;
    velocity.y = 0.0f;
}

void Player::setAnimation(int cols, int rows, int count, float duration) {
    animCols = cols;
    animRows = rows;
    frameCount = count;
    frameDuration = duration;
    frameIndex = 0;
    frameTimer = 0.0f;
}

void Player::updateAnimation(float dt, bool moving, int direction) {
    // Update facing direction
    if (direction != 0) {
        facingDirection = direction;
    }

    if (moving) {
        animPlaying = true;
        frameTimer += dt;
        
        if (frameTimer >= frameDuration) {
            frameIndex = (frameIndex + 1) % frameCount;
            frameTimer -= frameDuration;
        }
    } else {
        animPlaying = false;
        frameIndex = 0;
        frameTimer = 0.0f;
    }
}

void Player::loadTexture(const char* path) {
    int w, h, n;
    unsigned char* data = stbi_load(path, &w, &h, &n, 4);
    if (!data) {
        std::cerr << "Failed to load player texture\n";
        return;
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stbi_image_free(data);
}

void Player::initMesh() {
    float vertices[] = {
        // pos         // uv
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,   1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,   1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,   0.0f, 1.0f
    };

    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
}

void Player::update(float dt) {
    // Apply gravity
    velocity.y += gravity * dt;

    // Apply velocity
    position.y += velocity.y * dt;

    // Collision with ground - player's bottom is at position.y - height * 0.5f
    float bottomY = position.y - height * 0.5f;
    if (bottomY < floorY) {
        position.y = floorY + height * 0.5f;
        velocity.y = 0.0f;
    }
}

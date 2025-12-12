#ifndef PLAYER_HPP
#define PLAYER_HPP

# include <glm/glm.hpp>

class Player {
public:
    glm::vec3 position {0.0f, 0.0f, 0.0f};
    glm::vec3 velocity {0.0f, 0.0f, 0.0f};

    float height = 1.0f;
    float gravity = -9.8f;
    float jumpForce = 5.0f;
    float floorY = 0.0f;
    bool isGrounded = true;

    // Animation
    int animCols = 4;
    int animRows = 1;
    int frameCount = 4;
    int frameIndex = 0;
    float frameDuration = 0.1f;
    float frameTimer = 0.0f;
    bool animPlaying = false;
    int facingDirection = 1;  // 1 = right, -1 = left
    int facingIndex = 0; // discrete facing (0..4)
    int activeRow = 0; // current animation row (0-based)

    unsigned int textureID = 0;
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;

    Player();

    void loadTexture(const char* path);
    void initMesh();
    void setFloorHeight(float floorHeight);
    void setAnimation(int cols, int rows, int count, float duration);
    void updateAnimation(float dt, bool moving, int direction);
    void jump();
    void update(float dt);
};

#endif

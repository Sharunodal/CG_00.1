#ifndef PLAYER_HPP
#define PLAYER_HPP

# include <glm/glm.hpp>

class Player {
public:
    glm::vec3 position {0.0f, 0.0f, 0.0f};
    glm::vec3 velocity {0.0f, 0.0f, 0.0f};

    float height = 1.0f;
    float gravity = -9.8f;
    float floorY = 0.0f;

    unsigned int textureID = 0;
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;

    Player();

    void loadTexture(const char* path);
    void initMesh();
    void setFloorHeight(float floorHeight);
    void update(float dt);
};

#endif

#ifndef PLAYER_HPP
#define PLAYER_HPP

# include <glm/glm.hpp>

class Player {
public:
    glm::vec3 position;
    unsigned int textureID;
    unsigned int vao;
    unsigned int vbo;
    unsigned int ebo;

    Player();
    void loadTexture(const char* path);
    void initMesh();
};

#endif

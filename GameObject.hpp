#pragma once

#include <glm/glm.hpp>

struct TextureInfo {
    unsigned int id;
    float aspectRatio;
};

struct GameObject {
    glm::vec3 position;
    float size;
    unsigned int textureID;
    bool isVisible;
    float aspectRatio;

    glm::vec2 getMin() const {
        return glm::vec2(position.x - size * aspectRatio * 0.5f, position.y - size * 0.5f);
    }
    glm::vec2 getMax() const {
        return glm::vec2(position.x + size * aspectRatio * 0.5f, position.y + size * 0.5f);
    }
};
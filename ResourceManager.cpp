#include "ResourceManager.hpp"

#include <iostream>
#include <GL/glew.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


std::map<std::string, TextureInfo> ResourceManager::map_Textures;


TextureInfo ResourceManager::loadTexture(const std::string& filePath, const std::string& name)
{
    if (map_Textures.find(name) == map_Textures.end())
    {
        TextureInfo texInfo = { 0, 1.0f };

        int width, height, nrComponents;
        
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            if (height > 0) {
                texInfo.aspectRatio = (float)width / (float)height;
            }

            GLenum format = GL_RGBA;
            if (nrComponents == 1) format = GL_RED;
            else if (nrComponents == 3) format = GL_RGB;
            else if (nrComponents == 4) format = GL_RGBA;

            glGenTextures(1, &texInfo.id);
            glBindTexture(GL_TEXTURE_2D, texInfo.id);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);

            map_Textures[name] = texInfo;
        }
        else
        {
            std::cout << "Texture failed to load at path: " << filePath << std::endl;
        }
    }

    return map_Textures[name];
}

TextureInfo ResourceManager::getTexture(const std::string& name)
{
    return map_Textures[name];
}

void ResourceManager::clear()
{
    for (auto iter : map_Textures)
        glDeleteTextures(1, &iter.second.id);
    map_Textures.clear();
}
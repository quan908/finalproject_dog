#pragma once

#include <map>
#include <string>
#include "GameObject.hpp" 

class ResourceManager
{
public:
    static TextureInfo loadTexture(const std::string& filePath, const std::string& name);
    static TextureInfo getTexture(const std::string& name);
    static void clear();

private:
    ResourceManager() {}
    static std::map<std::string, TextureInfo> map_Textures;
};
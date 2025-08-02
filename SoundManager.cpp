#include "SoundManager.hpp"
#include"miniaudio.h"
namespace XQ
{
    SoundManager::SoundManager()
    {

    }
    SoundManager::~SoundManager()
    {
        ma_engine_uninit(&engine);
    }
    int SoundManager::init()
    {
        ma_result result;
        result = ma_engine_init(NULL, &engine);
        if (result != MA_SUCCESS) {
            return -1;
        }
        return 1;
    }
    void SoundManager::playMusic(const std::string& filePath)
    {
        ma_engine_play_sound(&engine, filePath.c_str(), NULL);
    }
 
    void SoundManager::playSoundEffect(const std::string& filePath)
    {
        ma_engine_play_sound(&engine, filePath.c_str(), NULL);
    }
}
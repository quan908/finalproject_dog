#pragma once
#include<string>

#include"miniaudio.h"
namespace XQ
{
	class SoundManager
	{
	private:
		ma_engine engine;
	public:
		SoundManager();
		~SoundManager();
		int init();
		void playMusic(const std::string& filePath);
		void playSoundEffect(const std::string& filePath);
	};
}
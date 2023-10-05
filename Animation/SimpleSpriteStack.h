#pragma once

#include <string>
#include <vector>

#include "Defs.h"
#include <graphics/sprite.h>
#include <system/platform.h>
#include "load_texture.h"

namespace Animation
{
	class SimpleSpriteStack
	{
		public:
			SimpleSpriteStack(Label name);
			~SimpleSpriteStack();

			void load(Label path, gef::Platform& platform, Label ext = ".png");
			
			gef::Texture* getDisplay(Float elapsed);

			inline void setRate(Float fps) { frametime = 1 / fps; }
			inline Float getFrameTime() const { return frametime; }
			inline Float getDuration() const { return frametime * Float(getCount()); }
			inline size_t getCount() const { return frames.size(); }
		protected:
			inline std::string getSuffix(UInt num) {
				return " (" + std::to_string(num + 1) + ")";
			}
			void wipe();

		private:
		std::string stackName;
		std::vector<gef::Texture*> frames;
		Float frametime;
	};

	class SimpleSpriteAnimator
	{
		public:
		SimpleSpriteAnimator();

		UInt registerAnimation(Label name, Float fps);
		void loadAll(Label path, gef::Platform& platform, Label ext = ".png");
		void update(Float dt);
		
		void play(UInt animID);
		void setTexture(gef::Sprite& sprite);

		protected:

		private:
		std::vector<SimpleSpriteStack> animations;
		bool playing;
		SimpleSpriteStack* currentAnim;
		Float epoch; // The constant denoting current timeline (for precision)
		Float elapsedTime;
	};
}
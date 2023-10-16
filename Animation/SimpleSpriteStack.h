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
			
			gef::Texture* getDisplay(float elapsed);

			inline void setRate(float fps) { frametime = 1 / fps; }
			inline float getFrameTime() const { return frametime; }
			inline float getDuration() const { return frametime * float(getCount()); }
			inline size_t getCount() const { return frames.size(); }
		protected:
			inline std::string getSuffix(UInt num) {
				return " (" + std::to_string(num + 1) + ")";
			}
			void wipe();

		private:
		std::string stackName;
		std::vector<gef::Texture*> frames;
		float frametime;
	};

	class SimpleSpriteAnimator
	{
		public:
		SimpleSpriteAnimator();

		UInt registerAnimation(Label name, float fps);
		void loadAll(Label path, gef::Platform& platform, Label ext = ".png");
		void update(float dt);
		
		void play(UInt animID);
		void setTexture(gef::Sprite& sprite);

		protected:

		private:
		std::vector<SimpleSpriteStack> animations;
		bool playing;
		SimpleSpriteStack* currentAnim;
		float epoch; // The constant denoting current timeline (for precision)
		float elapsedTime;
	};
}
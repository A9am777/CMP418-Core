#include "Animation/SimpleSpriteStack.h"
#include "Defs.h"
#include "Data/Structs.h"

namespace Animation
{
	SimpleSpriteStack::SimpleSpriteStack(Label name) : frametime(0.1f)
	{
		stackName = name;
	}
	SimpleSpriteStack::~SimpleSpriteStack()
	{
		wipe();
	}
	void SimpleSpriteStack::load(Label path, gef::Platform& platform, Label ext)
	{
		// Pull all file textures whilst good
		do
		{
			std::string frameName = path + stackName + getSuffix(UInt(frames.size())) + ext;
			frames.push_back(CreateTextureFromPNG(frameName.c_str(), platform));
		} while (frames.back());
		frames.pop_back(); // Remove the invalid 'flag' texture
	}
	gef::Texture* SimpleSpriteStack::getDisplay(float elapsed)
	{
		FrameSignature sig(elapsed, frametime);
		return getCount() ? frames[sig.major % getCount()] : nullptr; // Return frame if it exists
	}
	void SimpleSpriteStack::wipe()
	{
		for (auto frame : frames)
		{
			delete frame;
		}
		frames.clear();

		stackName = "invalid";
	}

	SimpleSpriteAnimator::SimpleSpriteAnimator() : elapsedTime(0), currentAnim(nullptr), playing(false), epoch(1)
	{

	}
	UInt SimpleSpriteAnimator::registerAnimation(Label name, float fps)
	{
		animations.push_back(SimpleSpriteStack(name));

		{
			SimpleSpriteStack& stack = animations.back();
			stack.setRate(fps);
		}

		return UInt(animations.size() - 1);
	}
	void SimpleSpriteAnimator::loadAll(Label path, gef::Platform& platform, Label ext)
	{
		for (auto& stack : animations)
		{
			if (!stack.getCount())
			{
				stack.load(path, platform, ext);
			}
		}
	}
	void SimpleSpriteAnimator::update(float dt)
	{
		if (playing)
		{
			if (elapsedTime > epoch) { elapsedTime -= epoch; } // Keep within epoch timeframe
			elapsedTime += dt;
		}
	}
	void SimpleSpriteAnimator::play(UInt animID)
	{
		// Set the animation context
		currentAnim = size_t(animID) < animations.size() ? animations.data() + animID : nullptr;

		if (playing = currentAnim)
		{
			// Retrieve metadata
			epoch = currentAnim->getDuration();
		}
	}

	void SimpleSpriteAnimator::setTexture(gef::Sprite& sprite)
	{
		if (currentAnim)
		{
			if (auto texture = currentAnim->getDisplay(elapsedTime))
			{
				sprite.set_texture(texture);
			}
		}
	}
}

#include "Animation/Data/Structs.h"

namespace Animation
{
	FrameSignature::FrameSignature(float time, float frametime)
	{
		minor = fmodf(time, frametime);
		major = UInt(time / frametime);
	}
}
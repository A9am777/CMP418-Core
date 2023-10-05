#include "Animation/Data/Structs.h"

namespace Animation
{
	FrameSignature::FrameSignature(Float time, Float frametime)
	{
		#ifdef Animation_DoublePrecision
		minor = fmod(time, frametime);
		#else
		minor = fmodf(time, frametime);
		#endif
		major = UInt(time / frametime);
	}
}
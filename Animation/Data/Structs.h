#pragma once

#include "Animation/Defs.h"

namespace Animation
{
	struct FrameSignature
	{
		UInt major; // The frame ID itself
		Float minor; // The exact moment within the frame
		FrameSignature(Float time, Float frametime); // Breakdown
	};
}
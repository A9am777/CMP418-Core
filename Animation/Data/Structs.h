#pragma once

#include "Defs.h"

namespace Animation
{
	// To decompose a time to frame + subframe
	struct FrameSignature
	{
		UInt major; // The frame ID itself
		float minor; // The exact moment within the frame
		FrameSignature(float time, float frametime); // Breakdown
	};
}
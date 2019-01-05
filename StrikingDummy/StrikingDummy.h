#pragma once

#include "Job.h"
#include "Rotation.h"

namespace StrikingDummy
{
	struct StrikingDummy
	{
		Job& job;
		MyRotation rotation;

		StrikingDummy(Job& job);

		void start();
	};
}
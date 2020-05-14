#include "StrikingDummy.h"
#include "Logger.h"
#include <chrono>
#include <iostream>
#include <random>

namespace StrikingDummy
{
	StrikingDummy::StrikingDummy(Job& job) : job(job), rotation(job)
	{

	}

	void StrikingDummy::start()
	{
		job.reset();

		while(true)
		{
			// State
			std::cout << "================================\n";
			std::cout << job.get_info() << std::flush;

			rotation.step();

			// DPS
			std::cout << "DPS: " << 1000.0f / job.timeline.time * job.total_damage << std::endl;
			std::cout << "Time (seconds): " << job.timeline.time / 1000.0f << std::endl;
		}
	}
}
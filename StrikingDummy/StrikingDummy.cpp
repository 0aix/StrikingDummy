#include "StrikingDummy.h"
#include "BlackMage.h"
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
		//BlackMage& blm = (BlackMage&)job;
		//blm.reset();
		job.reset();

		while(true)
		{
			// State
			std::cout << "================================\n";
			//std::cout << "MP: " << blm.mp << "\n";
			//std::cout << "Procs: ";
			//if (blm.fs_proc.count)
			//	std::cout << "F3p ";
			//if (blm.tc_proc.count)
			//	std::cout << "T3p ";
			//if (blm.foul_timer.ready)
			//	std::cout << "Foul";
			//std::cout << "\n";
			//std::cout << "Gauge: " << blm.gauge.time / 100.0f << std::endl;

			rotation.step();

			// DPS
			std::cout << "DPS: " << 100.0f / job.timeline.time * job.total_damage << std::endl;
			std::cout << "Time (seconds): " << job.timeline.time / 100.0f << std::endl;
		}
	}
}
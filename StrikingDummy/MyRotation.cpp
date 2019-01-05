#include "Rotation.h"
#include "Job.h"
#include <sstream>
#include <iostream>
#include <assert.h>

namespace StrikingDummy
{
	MyRotation::MyRotation(Job& job) : Rotation(job)
	{

	}

	void MyRotation::step()
	{
		// recode job
		// guarantee always at least 1 action and that action is not NONE
		//assert(job.actions.size() > 0);

		if (job.actions.size() > 0)
		{
			std::stringstream ss;
			ss << "Actions: " << job.actions.front();
			for (auto iter = job.actions.cbegin() + 1; iter != job.actions.cend(); iter++)
				ss << ", " << *iter;
			std::cout << ss.str() << std::endl << "Use: " << std::flush;

			int action;
			std::cin >> action;
			job.use_action(action);
		}
		job.step();
	}
}
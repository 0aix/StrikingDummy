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
		std::stringstream ss;
		ss << "Actions: " << job.get_action_name(job.actions.front()) << " (" << job.actions.front() << ")";
		for (auto iter = job.actions.cbegin() + 1; iter != job.actions.cend(); iter++)
			ss << ", " << job.get_action_name(*iter) << " (" << *iter << ")";
		std::cout << ss.str() << std::endl << "Use: " << std::flush;
		int action;
		std::cin >> action;
		job.use_action(action);
		job.step();
	}
}
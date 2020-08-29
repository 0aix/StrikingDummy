#include "Job.h"
#include "Rotation.h"
#include <assert.h>
#include <chrono>

#ifdef _DEBUG 
#define DBG(x) x
#else 
#define DBG(x)
#endif

namespace StrikingDummy
{
	// ============================================ Job ============================================

	Job::Job()
	{
		rng = std::mt19937(std::chrono::high_resolution_clock::now().time_since_epoch().count());
		prob = std::uniform_real_distribution<float>(0.0f, 1.0f);
	}

	void Job::step()
	{
		update();
	}
}
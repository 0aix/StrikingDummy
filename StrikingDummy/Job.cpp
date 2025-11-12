#include "Job.h"
#include "Rotation.h"
#include <assert.h>
#include <chrono>

#ifdef _DEBUG 
#define DBG(x) x
#else 
#define DBG(x)
#endif

#define LV_SUB 420.0f
#define LV_DIV 2780.0f
#define LV_MAIN 440.0f

namespace StrikingDummy
{
	// ============================================ Job ============================================

	Job::Job(Stats& job_stats)
	{
		stats = job_stats;

		rng = std::mt19937(std::chrono::high_resolution_clock::now().time_since_epoch().count());
		prob = std::uniform_real_distribution<float>(0.0f, 1.0f);
		damage_range = std::uniform_real_distribution<float>(0.95f, 1.05f);
		tick = std::uniform_int_distribution<int>(1, 1000);
	}

	void Job::step()
	{
		int elapsed;
		while (!timeline.events.empty())
		{
			if ((elapsed = timeline.next_event()) > 0)
			{
				update(elapsed);
				// need at least 1 useable action that is not NONE (0)
				if (actions.empty())
					continue;
				break;
			}
		}
	}

	void Job::push_event(int offset)
	{
		if (offset > 0)
			timeline.push_event(offset);
	}

	// ============================================ Timeline ============================================

	int Timeline::next_event()
	{
		if (events.empty())
			return 0;
		int temp = events.top();
		int elapsed = temp - time;
		time = temp;
		events.pop();
		return elapsed;
	}

	void Timeline::push_event(int offset)
	{
		events.push(time + offset);
	}

	// ============================================ Timer ============================================

	void Timer::update(int elapsed)
	{
		if (time > 0 && (time -= elapsed) == 0)
			ready = true;
		assert(time >= 0);
	}

	void Timer::reset(int duration, bool ready)
	{
		this->time = duration;
		this->ready = ready;
	}

	// ============================================ Buff ============================================

	void Buff::update(int elapsed)
	{
		if (time > 0 && (time -= elapsed) == 0)
			count = 0;
		assert(time >= 0);
	}

	void Buff::reset(int duration, int count)
	{
		this->time = duration;
		this->count = count;
	}
}
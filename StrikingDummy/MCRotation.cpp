#include "Rotation.h"
#include "Job.h"
#include "Model.h"
#include "BlackMage.h"
#include <chrono>
#include <random>
#include <iostream>

namespace StrikingDummy
{
	MCRotation::MCRotation(Job& job, ModelRotation& rotation) : Rotation(job), rotation(rotation), blm((BlackMage&)job), temp((BlackMage&)rotation.job)
	{
		input = rotation.model.getModelComputeInput();
	}

	void MCRotation::reset(float eps, float exp)
	{

	}

	void MCRotation::step()
	{
		const float OUTPUT_LOWER = 9.090f;
		const float OUTPUT_UPPER = 9.340f;
		const int N = 2;
		const int K = 600;
		const int L = 30000;
		const float WINDOW = 600000.0f;

		if (job.actions.size() == 1)
		{
			history.push_back(job.actions[0]);
			job.use_action(job.actions[0]);
		}
		else
		{
			memcpy(input.m_x0.data(), job.get_state(), sizeof(float) * job.get_state_size());
			float* output = rotation.model.compute(input);
			std::sort(job.actions.begin(), job.actions.end(), [output](int a, int b) -> bool { return output[a] > output[b]; });
			history.push_back(job.actions[0]);
			int length = std::min((int)job.actions.size(), N);
			int time = job.timeline.time;
			int action = -1;
			float best_est_dps = 0.0f;
			for (int i = 0; i < length; i++)
			{
				float est_dps = 0.0f;
				for (int k = 0; k < K; k++)
				{
					temp.reset(blm);
					temp.use_action(job.actions[i]);
					temp.step();
					while (temp.timeline.time < job.timeline.time + L)
						rotation.step();
					float time_passed = temp.timeline.time - job.timeline.time;
					float total_damage = temp.total_damage;
					rotation.step();
					float future_est_dps = (OUTPUT_LOWER + (OUTPUT_UPPER - OUTPUT_LOWER) / (1.0f + expf(-rotation.stored_max_weight)));
					est_dps += (total_damage + future_est_dps * (WINDOW - time_passed)) / WINDOW;
				}
				if (est_dps > best_est_dps)
				{
					best_est_dps = est_dps;
					action = job.actions[i];
				}
			}
			job.use_action(action);
		}
		job.step();
	}
}
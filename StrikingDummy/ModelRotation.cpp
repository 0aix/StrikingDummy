#include "Rotation.h"
#include "Job.h"
#include "Model.h"
#include "BlackMage.h"
#include <chrono>
#include <random>
#include <iostream>

namespace StrikingDummy
{
	std::mt19937 model_rotation_rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
	std::uniform_real_distribution<double> unif(0.0, 1.0);
	
	ModelRotation::ModelRotation(Job& job, Model& model) : Rotation(job), model(model)
	{
		random_action.push_back(-1);
		eps = 0.0;
	}

	void ModelRotation::step()
	{
		if (job.actions.size() == 1)
			job.use_action(job.actions[0]);
		else
		{
			int action;
			if (unif(model_rotation_rng) < eps)
			{
				std::sample(job.actions.begin(), job.actions.end(), random_action.begin(), 1, model_rotation_rng);
				action = random_action.front();
			}
			else
			{
				//memcpy(model.x0.data(), job.get_state(), sizeof(State));
				//memcpy(model.x0, job.get_state(), sizeof(State));
				memcpy(model.m_x0.data(), job.get_state(), sizeof(State));
				model.compute();
				//float* output = model.xk.data();
				//float* output = model.x3;
				float* output = model.m_x3.data();
				int max_action = job.actions[0];
				float max_weight = output[max_action];
				auto cend = job.actions.cend();
				for (auto iter = job.actions.cbegin() + 1; iter != cend; iter++)
				{
					int index = *iter;
					if (output[index] > max_weight)
					{
						max_weight = output[index];
						max_action = index;
					}
				}
				action = max_action;
			}
			job.use_action(action);
		}
		job.step();
	}
}
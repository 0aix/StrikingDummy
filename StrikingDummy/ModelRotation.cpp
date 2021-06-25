#include "Rotation.h"
#include "Job.h"
#include "Model.h"
#include "BlackMage.h"
#include <chrono>
#include <random>
#include <iostream>

namespace StrikingDummy
{
	ModelRotation::ModelRotation(Job& job, Model& model, long long offset) : Rotation(job), model(model)
	{
		rng = std::mt19937(std::chrono::high_resolution_clock::now().time_since_epoch().count() + offset);
		input = model.getModelComputeInput();
		random_action.push_back(-1);
		eps = 0.0f;
		exp = 0.0f;
		exploring = false;
	}

	void ModelRotation::reset(float eps, float exp)
	{
		this->eps = eps;
		this->exp = exp;
		//this->exploring = false;
		//this->input = model.getModelComputeInput();
	}

	void ModelRotation::step()
	{
		if (job.actions.size() == 1)
			job.use_action(job.actions[0]);
		else
		{
			int action;
			//if (unif(rng) < (exploring ? std::max(eps, exp) : eps))
			if (unif(rng) < eps)
			{
				std::sample(job.actions.begin(), job.actions.end(), random_action.begin(), 1, rng);
				action = random_action.front();
				//exploring = true;
			}
			else
			{
				memcpy(input.m_x0.data(), job.get_state(), sizeof(float) * job.get_state_size());
				float* output = model.compute(input);
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
				stored_max_weight = max_weight;
				action = max_action;
				//exploring = false;
			}
			job.use_action(action);
		}
		job.step();
	}
}
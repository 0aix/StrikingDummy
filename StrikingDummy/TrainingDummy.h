#pragma once

#include "Job.h"
#include "Model.h"
#include "Rotation.h"

namespace StrikingDummy
{
	struct TrainingDummy
	{
		Job& job;
		Model model;
		ModelRotation rotation;

		double best_mean = 0.0;
		Model best_model;

		long long best_time;
		int best_epoch;

		TrainingDummy(Job& job) : job(job), model(job.get_state_size(), job.get_num_actions()), rotation(job, model), best_model(job.get_state_size(), job.get_num_actions()) {}

		void train();
		void test(float& start, float& end, int& num_steps);
		bool best();
		void trace();
	};
}
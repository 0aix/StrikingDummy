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
		float test();
		bool best();
		void trace();
		void metrics();
		void dist(int seconds, int times);
		void study();
		void mp_offset();
	};
}
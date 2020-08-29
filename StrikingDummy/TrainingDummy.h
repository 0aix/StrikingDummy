#pragma once

#include "Job.h"
#include "Model.h"
#include "Rotation.h"
#include "Solver.h"

namespace StrikingDummy
{
	struct TrainingDummy
	{
		Job& job;
		Model model;
		ModelRotation rotation;
		Solver solver;

		TrainingDummy(Job& job);
		~TrainingDummy();

		void train();
		int test();
		void trace();
	};
}
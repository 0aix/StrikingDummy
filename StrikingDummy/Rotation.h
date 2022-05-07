#pragma once

#include <vector>
#include <random>
#include "Model.h"

namespace StrikingDummy
{
	struct Job;
	struct BlackMage;

	struct Rotation
	{
		Job& job;

		Rotation(Job& job) : job(job) {}
		
		virtual void step() = 0;
	};

	struct ModelRotation : Rotation
	{
		Model& model;
		ModelComputeInput input;

		std::vector<int> random_action;
		float eps;
		float stored_max_weight;

		std::mt19937 rng;
		std::uniform_real_distribution<float> unif = std::uniform_real_distribution<float>(0.0f, 1.0f);

		ModelRotation(Job& job, Model& model, long long offset = 0);

		void reset(float eps, float exp);
		void step();
	};

	struct MCRotation : Rotation
	{
		ModelRotation& rotation;
		ModelComputeInput input;
		BlackMage& blm;
		BlackMage& temp;
		std::vector<int> history;

		MCRotation(Job& job, ModelRotation& rotation);

		void reset(float eps, float exp);
		void step();
	};

	struct MyRotation : Rotation
	{
		MyRotation(Job& job);

		void step();
	};
}
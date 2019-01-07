#pragma once

#include <vector>

namespace StrikingDummy
{
	struct Job;
	struct Model;

	struct Rotation
	{
		Job& job;

		Rotation(Job& job) : job(job) {}
		
		virtual void step() = 0;
	};

	struct ModelRotation : Rotation
	{
		Model& model;
		std::vector<int> random_action;
		double eps;

		ModelRotation(Job& job, Model& model);

		void step();
	};



	struct MyRotation : Rotation
	{
		MyRotation(Job& job);

		void step();
	};
}
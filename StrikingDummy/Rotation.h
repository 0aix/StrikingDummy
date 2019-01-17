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
		float eps;
		float exp;
		bool exploring;

		ModelRotation(Job& job, Model& model);

		void reset(float eps, float exp);
		void step();
	};



	struct MyRotation : Rotation
	{
		MyRotation(Job& job);

		void step();
	};
}
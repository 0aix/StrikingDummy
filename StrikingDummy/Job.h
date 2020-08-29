#pragma once

#include <queue>
#include <random>

namespace StrikingDummy
{
	struct Transition
	{
		float t0[288];
		float t1[288];
		int action = 0;
		bool c0 = false;
		bool c1 = false;
	};

	struct Job
	{
		std::vector<int> actions;
		std::vector<Transition> history;

		std::mt19937 rng;
		std::uniform_real_distribution<float> prob;

		float total_damage = 0.0f;

		Job();
		void step();
		float* get_state() { return history.back().t0; }

		virtual void reset() = 0;
		virtual void use_action(int action) = 0;
		virtual void get_state(float* state) = 0;
		virtual int get_state_size() = 0;
		virtual int get_num_actions() = 0;
		virtual std::string get_action_name(int action) = 0;

	protected:
		virtual void update() = 0;
	};
}
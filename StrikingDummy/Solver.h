#pragma once

namespace StrikingDummy
{
	struct Model;

	struct State
	{
		int data[6];
		float encoded[288];

		State() {}
		State(int* data, float* encoded);
		State(State& state, int rotate);

		inline bool is_solved()
		{
			return data[0] == 0 && data[1] == 0x11111111 && data[2] == 0x22222222 && data[3] == 0x33333333 && data[4] == 0x44444444 && data[5] == 0x55555555;
		}
	};

	struct Tuple
	{
		int index;
		int action;
		float score;

		Tuple(int index, int action, float score) : index(index), action(action), score(score) {}

		bool operator <(const Tuple& other)
		{
			return score > other.score;
		}
	};

	struct Solver
	{
		static constexpr int MAX_STEPS = 30;
		static constexpr int MAX_BREADTH = 10000;
		static constexpr int STATE_SIZE = 288;

		Model& model;

		Solver(Model& model);

		int solve(int* data, float* encoded);
	};
}
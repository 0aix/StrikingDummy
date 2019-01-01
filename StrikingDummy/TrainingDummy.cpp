#include "TrainingDummy.h"
#include "BlackMage.h"
#include "Logger.h"
#include <chrono>
#include <iostream>
#include <random>

namespace StrikingDummy
{
	TrainingDummy::TrainingDummy(Job& job) : job(job), rotation(job, model)
	{

	}

	TrainingDummy::~TrainingDummy()
	{

	}
	
	void TrainingDummy::train()
	{
		Logger::open();

		std::cout.precision(2);

		long long start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();

		const int NUM_EPOCHS = 1000000;
		const int NUM_STEPS_PER_EPISODE = 1000;
		const int NUM_EPISODES_PER_EPOCH = 10;
		const int NUM_BATCHES_PER_EPOCH = 1;
		const int CAPACITY = 1000000;
		const int BATCH_SIZE = 50000;
		const double EPS_DECAY = 0.99;
		const double EPS_MIN = 0.01;
		const float WINDOW = 4000.0f;

		std::mt19937 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
		std::uniform_int_distribution<int> range(0, CAPACITY - 1);
		std::vector<int> indices(BATCH_SIZE);
		std::vector<int> actions(BATCH_SIZE);
		std::vector<float> rewards(BATCH_SIZE);

		Transition* memory = new Transition[CAPACITY];
		int m_index = 0;
		int m_size = 0;

		auto indices_gen = [&]() 
		{
			if (m_size < CAPACITY)
				return range(rng) % m_size;
			return range(rng);
		};

		BlackMage& blm = (BlackMage&)job;

		// Initialize model
		model.init(BATCH_SIZE);

		rotation.eps = 1.0;

		float nu = 0.0001f;

		for (int epoch = 0; epoch < NUM_EPOCHS; epoch++)
		{
			float dps = 0.0f;
			int fouls = 0;
			int f4s = 0;
			// generate a bunch of sequences
			for (int ep = 0; ep < NUM_EPISODES_PER_EPOCH; ep++)
			{
				job.reset();
				for (int step = 0; step < NUM_STEPS_PER_EPISODE; step++)
					rotation.step();
				std::vector<Transition>* history = (std::vector<Transition>*)job.get_history();
				auto last = history->end() - 1;
				for (auto iter = history->begin(); iter != last; iter++)
				{
					memory[m_index] = std::move(*iter);
					if (m_index == CAPACITY - 1)
						m_index = 0;
					else
						m_index++;
					if (m_size < CAPACITY)
						m_size++;
				}
				dps += 0.1f * job.total_damage / job.timeline.time;
				fouls += blm.foul_count;
				f4s += blm.f4_count;
			}
			std::cout << "eps: " << rotation.eps << ", avg dps: " << (1.0f / NUM_EPISODES_PER_EPOCH) * dps << ", avg fouls: " << (1.0f / NUM_EPISODES_PER_EPOCH) * fouls << ", avg F4s: " << (1.0f / NUM_EPISODES_PER_EPOCH) * f4s << std::endl;
			if (m_size == CAPACITY)
			{
				// batch train a bunch
				for (int batch = 0; batch < NUM_BATCHES_PER_EPOCH; batch++)
				{
					std::generate(indices.begin(), indices.end(), indices_gen);

					// compute Q1
					for (int i = 0; i < BATCH_SIZE; i++)
						memcpy(model.X0.col(i).data(), &memory[indices[i]].t1, sizeof(State));

					MatrixXf& Q1 = model.batch_compute();

					// calculate rewards
					for (int i = 0; i < BATCH_SIZE; i++)
					{
						Transition& t = memory[indices[i]];
						float* q = Q1.col(i).data();
						float max_q = q[t.actions[0]];
						auto cend = t.actions.cend();
						for (auto iter = t.actions.cbegin() + 1; iter != cend; iter++)
						{
							int i = *iter;
							if (q[i] > max_q)
								max_q = q[i];
						}
						//rewards[i] = t.reward * NORMALIZE + (1.0f - t.dt / WINDOW) * max_q;
						//rewards[i] = 0.0001f * (t.dt / WINDOW) * t.reward + (1.0f - t.dt / WINDOW) * max_q;
						rewards[i] = (1.0f / WINDOW) * (0.01f * t.reward + (WINDOW - t.dt) * max_q);
						actions[i] = t.action;
					}

					// compute Q0
					for (int i = 0; i < BATCH_SIZE; i++)
						memcpy(model.X0.col(i).data(), &memory[indices[i]].t0, sizeof(State));

					model.batch_compute();

					// calculate target
					model.target = model.Xk;
					for (int i = 0; i < BATCH_SIZE; i++)
						model.target.col(i)(actions[i]) = rewards[i];

					// train
					model.train(nu);
				}
				//if (m_size == CAPACITY)
					//rotation.eps = std::max(rotation.eps * EPS_DECAY, EPS_MIN);
				rotation.eps -= 0.01f;
				if (rotation.eps < 0.0f)
					rotation.eps = 1.0f;
			}
		}

		delete[] memory;

		long long end_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();

		std::cout << "running time: " << (end_time - start_time) / 1000000000.0 << " seconds" << std::endl;

		Logger::close();
	}
}
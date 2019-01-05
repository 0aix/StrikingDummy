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

		std::cout.precision(4);

		long long start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();

		const int NUM_EPOCHS = 1000000;
		const int NUM_STEPS_PER_EPISODE = 2500;
		const int NUM_EPISODES_PER_EPOCH = 12;
		const int NUM_BATCHES_PER_EPOCH = 20;
		const int CAPACITY = 20000;
		const int BATCH_SIZE = 1000;
		const double EPS_DECAY = 0.9998;
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
			//if (m_size < CAPACITY)
			//	return range(rng) % m_size;
			return range(rng);
		};

		BlackMage& blm = (BlackMage&)job;

		// Initialize model
		model.init(BATCH_SIZE);

		double _eps = 0.25;
		rotation.eps = _eps;

		float nu = 0.01f; // sigmoid can use a larger learning rate
		//float nu = 0.00001f;
		float max_dps = 0.0f;

		for (int epoch = 0; epoch < NUM_EPOCHS; epoch++)
		{
			float dps = 0.0f;
			int fouls = 0;
			int f4s = 0;
			int b4s = 0;

			m_size = 0;
			m_index = 0;

			// generate a bunch of sequences
			for (int ep = 0; ep < NUM_EPISODES_PER_EPOCH; ep++)
			{
				job.reset();
				int precast = blm.get_cast_time(2);
				if (ep % 3 == 0)
				{
					blm.timeline.time = -precast;
					blm.mp_timer.time += precast;
					blm.dot_timer.time += precast;
					blm.use_action(2);
					blm.step();
				}
				else if (ep % 3 == 1)
				{
					blm.timeline.time = -precast;
					blm.mp_timer.time += precast;
					blm.dot_timer.time += precast;
					blm.use_action(5);
					blm.step();
				}
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
				//dps += 0.1f * job.total_damage / job.timeline.time;
				//fouls += blm.foul_count;
				//f4s += blm.f4_count;
				//b4s += blm.b4_count;
			}
			//std::stringstream ss;
			//float avg_dps = (1.0f / NUM_EPISODES_PER_EPOCH) * dps;
			//ss << "eps: " << rotation.eps << ", avg dps: " << avg_dps << ", avg fouls: " << (1.0f / NUM_EPISODES_PER_EPOCH) * fouls << ", avg F4s: " << (1.0f / NUM_EPISODES_PER_EPOCH) * f4s << ", avg B4s: " << (1.0f / NUM_EPISODES_PER_EPOCH) * b4s << std::endl;
			//Logger::log(ss.str().c_str());
			//std::cout << ss.str();

			range = std::uniform_int_distribution<int>(0, m_size - 1);

			//if (m_size == CAPACITY)
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
				//rotation.eps -= 0.01f;
				//if (rotation.eps < 0.0f)
				//	rotation.eps = 1.0f;
				//rotation.eps *= EPS_DECAY;
				//if (rotation.eps > 0.0)
				//	rotation.eps = 0.0;
				//else
				//	rotation.eps = _eps;
				//if (_eps > 0.0)
				//	_eps -= 0.0001;
			}

			rotation.eps = 0.0;
			test();
			rotation.eps = _eps;

			dps = 0.1f * blm.total_damage / blm.timeline.time;
			if (dps > max_dps)
				max_dps = dps;

			std::stringstream ss;
			ss << "epoch: " << epoch << ", max dps: " << max_dps << ", " << "dps: " << dps << ", fouls: " << blm.foul_count << ", f4s: " << blm.f4_count << ", b4s: " << blm.b4_count << std::endl;

			Logger::log(ss.str().c_str());
			std::cout << ss.str();
		}

		delete[] memory;

		long long end_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();

		std::cout << "running time: " << (end_time - start_time) / 1000000000.0 << " seconds" << std::endl;

		Logger::close();
	}

	void TrainingDummy::test()
	{
		BlackMage& blm = (BlackMage&)job;
		blm.reset();

		// B3 precast
		int precast = blm.get_cast_time(2);
		blm.timeline.time = -precast;
		blm.mp_timer.time += precast;
		blm.dot_timer.time += precast;
		blm.use_action(2);
		blm.step();

		while (blm.timeline.time < 60000)
			rotation.step();
	}
}
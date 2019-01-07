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

		const int NUM_ENVIRONMENTS = 1000;
		const int NUM_EPOCHS = 1000000;
		const int NUM_STEPS_PER_EPISODE = 100;
		const int NUM_EPISODES_PER_EPOCH = 3;
		const int NUM_BATCHES_PER_EPOCH = 100;
		const int CAPACITY = 100000;
		const int BATCH_SIZE = 1000;
		const double EPS_DECAY = 0.9998;
		const double EPS_MIN = 0.01;
		const float WINDOW = 18000.0f;

		float* window = new float;
		*window = 4000.0f;

		std::mt19937 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
		std::uniform_int_distribution<int> range(0, CAPACITY - 1);
		std::vector<int> indices(BATCH_SIZE);
		std::vector<int> actions(BATCH_SIZE);
		std::vector<float> rewards(BATCH_SIZE);

		std::vector<BlackMage> blms;
		for (int i = 0; i < NUM_ENVIRONMENTS; i++)
		{
			blms.push_back((BlackMage&)job);
			blms.back().seed(start_time + i);
		}

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

		double _eps = 0.25;
		rotation.eps = _eps;

		float nu = 0.01f; // sigmoid can use a larger learning rate
		//float nu = 0.00001f;
		float max_dps = 0.0f;

		std::uniform_real_distribution<double> unif(0.0, 1.0);

		std::vector<int> random_action;
		random_action.push_back(0);

		for (int epoch = 0; epoch < NUM_EPOCHS; epoch++)
		{
			float dps = 0.0f;
			int fouls = 0;
			int f4s = 0;
			int b4s = 0;

			m_index = 0;
			m_size = 0;
			
			for (int env = 0; env < NUM_ENVIRONMENTS; env++)
				blms[env].reset();
			for (int step = 0; step < NUM_STEPS_PER_EPISODE; step++)
			{
				for (int env = 0; env < NUM_ENVIRONMENTS; env++)
					memcpy(&model.X0[env * 47], blms[env].get_state(), sizeof(State));

				float* Q = model.batch_compute();

				for (int env = 0; env < NUM_ENVIRONMENTS; env++)
				{
					int action;
					if (unif(rng) < _eps)
					{
						std::sample(blms[env].actions.begin(), blms[env].actions.end(), random_action.begin(), 1, rng);
						action = random_action.front();
					}
					else
					{
						float* output = &model.X3[env * 15];
						int max_action = blms[env].actions[0];
						float max_weight = output[max_action];
						auto cend = blms[env].actions.cend();
						for (auto iter = blms[env].actions.cbegin() + 1; iter != cend; iter++)
						{
							int index = *iter;
							if (output[index] > max_weight)
							{
								max_weight = output[index];
								max_action = index;
							}
						}
						action = max_action;
					}
					blms[env].use_action(action);
					blms[env].step();
				}
			}
			for (int env = 0; env < NUM_ENVIRONMENTS; env++)
			{
				std::vector<Transition>* history = (std::vector<Transition>*)blms[env].get_history();
				auto last = history->end() - 1;
				for (auto iter = history->begin(); iter != last; iter++)
				{
					memory[m_index] = std::move(*iter);
					m_index++;
					m_size++;
				}
			}

			//m_size = 0;
			//m_index = 0;

			// generate a bunch of sequences
			/*
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
			}
			*/
			//std::stringstream ss;
			//float avg_dps = (1.0f / NUM_EPISODES_PER_EPOCH) * dps;
			//ss << "eps: " << rotation.eps << ", avg dps: " << avg_dps << ", avg fouls: " << (1.0f / NUM_EPISODES_PER_EPOCH) * fouls << ", avg F4s: " << (1.0f / NUM_EPISODES_PER_EPOCH) * f4s << ", avg B4s: " << (1.0f / NUM_EPISODES_PER_EPOCH) * b4s << std::endl;
			//Logger::log(ss.str().c_str());
			//std::cout << ss.str();

			//range = std::uniform_int_distribution<int>(0, m_size - 1);

			//if (m_size == CAPACITY)
			{
				// batch train a bunch
				for (int batch = 0; batch < NUM_BATCHES_PER_EPOCH; batch++)
				{
					std::generate(indices.begin(), indices.end(), indices_gen);

					// compute Q1
					for (int i = 0; i < BATCH_SIZE; i++)
						//memcpy(model.X0.col(i).data(), &memory[indices[i]].t1, sizeof(State));
						memcpy(&model.X0[i * 47], &memory[indices[i]].t1, sizeof(State));

					//MatrixXf& Q1 = model.batch_compute();
					float* Q1 = model.batch_compute();

					// calculate rewards
					for (int i = 0; i < BATCH_SIZE; i++)
					{
						Transition& t = memory[indices[i]];
						//float* q = Q1.col(i).data();
						float* q = &Q1[i * 15];
						float max_q = q[t.actions[0]];
						auto cend = t.actions.cend();
						for (auto iter = t.actions.cbegin() + 1; iter != cend; iter++)
						{
							int i = *iter;
							if (q[i] > max_q)
								max_q = q[i];
						}
						rewards[i] = (1.0f / *window) * (0.01f * t.reward + (*window - t.dt) * max_q);
						actions[i] = t.action;
					}

					// compute Q0
					for (int i = 0; i < BATCH_SIZE; i++)
						//memcpy(model.X0.col(i).data(), &memory[indices[i]].t0, sizeof(State));
						memcpy(&model.X0[i * 47], &memory[indices[i]].t0, sizeof(State));

					model.batch_compute();

					// calculate target
					//model.target = model.Xk;
					memcpy(model.target, model.X3, sizeof(float) * 15 * BATCH_SIZE);
					for (int i = 0; i < BATCH_SIZE; i++)
						//model.target.col(i)(actions[i]) = rewards[i];
						model.target[i * 15 + actions[i]] = rewards[i];

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

			if (epoch % 10 == 0)
			{
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
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

		const int NUM_ENVIRONMENTS = 100;
		const int NUM_EPOCHS = 1000000;
		const int NUM_STEPS_PER_EPISODE = 1000;
		const int NUM_STEPS_PER_EPISODE_START = 20;
		const int NUM_EPISODES_PER_EPOCH = 10;
		const int NUM_BATCHES_PER_EPOCH = 100;
		const int CAPACITY = 1000000;
		const int BATCH_SIZE = 1000;
		const float WINDOW = 3000.0f;
		const float WINDOW_MAX = 18000.0f;
		const float WINDOW_GROWTH = 1.0002f;
		const float EPS_DECAY = 0.9995f;
		const float EPS_MIN = 0.25f;
		const float STEPS_GROWTH = 1.0004f;
		const float STEPS_MAX = NUM_STEPS_PER_EPISODE;

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
			//if (m_size < CAPACITY)
			//	return range(rng) % m_size;
			return range(rng);
		};

		BlackMage& blm = (BlackMage&)job;

		// Initialize model
		model.init(BATCH_SIZE);

		float nu = 0.01f; // sigmoid can use a larger learning rate
		//float nu = 0.001f;
		float max_dps = 0.0f;

		//rotation.eps = 1.0f;
		float eps = 1.0f;
		float window = WINDOW;
		float steps_per_episode = NUM_STEPS_PER_EPISODE_START;

		std::uniform_real_distribution<float> unif(0.0f, 1.0f);

		std::vector<int> random_action;
		random_action.push_back(0);

		float avg_dps = 0.0f;
		float beta = 0.9f;

		for (int epoch = 0; epoch < NUM_EPOCHS; epoch++)
		{
			float dps = 0.0f;
			int fouls = 0;
			int f4s = 0;
			int b4s = 0;

			//m_index = 0;
			//m_size = 0;
			
			for (int env = 0; env < NUM_ENVIRONMENTS; env++)
				blms[env].reset();
			for (int step = 0; step < (int)steps_per_episode + 10; step++)
			{
				for (int env = 0; env < NUM_ENVIRONMENTS; env++)
					memcpy(&model.X0[env * 47], blms[env].get_state(), sizeof(State));
			
				float* Q = model.batch_compute(NUM_ENVIRONMENTS);
			
				for (int env = 0; env < NUM_ENVIRONMENTS; env++)
				{
					int action;
					if (unif(rng) < eps)
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
				for (int i = 0; i < (int)steps_per_episode; i++)
				{
					memory[m_index] = std::move((*history)[i]);
					m_index++;
					if (m_index == CAPACITY)
						m_index = 0;
					if (m_size < CAPACITY)
						m_size++;
				}
			}
			/*
			for (int episode = 0; episode < NUM_EPISODES_PER_EPOCH; episode++)
			{
				blm.reset();
				for (int step = 0; step < NUM_STEPS_PER_EPISODE + 15; step++)
				{
					int action;
					if (blm.actions.size() == 1)
						action = blm.actions[0];
					if (unif(rng) < EPS)
					{
						std::sample(blm.actions.begin(), blm.actions.end(), random_action.begin(), 1, rng);
						action = random_action.front();
					}
					else
					{
						memcpy(model.m_x0.data(), blm.get_state(), sizeof(State));
						//memcpy(model.X0, blm.get_state(), sizeof(State));
						model.compute();
						//model.batch_compute(1);
						float* output = model.m_x3.data();
						//float* output = model.X3;
						int max_action = blm.actions[0];
						float max_weight = output[max_action];
						auto cend = blm.actions.cend();
						for (auto iter = blm.actions.cbegin() + 1; iter != cend; iter++)
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
					blm.use_action(action);
					blm.step();
				}
				std::vector<Transition>* history = (std::vector<Transition>*)blm.get_history();
				auto last = history->end() - 1;
				for (int i = 0; i < NUM_STEPS_PER_EPISODE; i++)
				{
					memory[m_index] = std::move((*history)[i]);
					m_index++;
					if (m_index == CAPACITY)
						m_index = 0;
					if (m_size < CAPACITY)
						m_size++;
				}
			}
			*/
			//range = std::uniform_int_distribution<int>(0, m_size - 1);

			if (m_size == CAPACITY)
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
						rewards[i] = (1.0f / WINDOW) * (0.01f * t.reward + (WINDOW - t.dt) * max_q);
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
				eps *= EPS_DECAY;
				if (eps < EPS_MIN)
					eps = EPS_MIN;
				window *= WINDOW_GROWTH;
				if (window > WINDOW_MAX)
					window = WINDOW_MAX;
				steps_per_episode *= STEPS_GROWTH;
				if (steps_per_episode > STEPS_MAX)
					steps_per_episode = STEPS_MAX;
				//if (epoch % 1 == 0)
				{
					test();

					dps = 0.1f * blm.total_damage / blm.timeline.time;

					avg_dps = 0.9f * avg_dps + 0.1f * dps;
					float est_dps = avg_dps / (1.0f - beta);
					beta *= 0.9f;

					//if (dps > max_dps)
					//	max_dps = dps;

					std::stringstream ss;
					ss << "epoch: " << epoch << ", eps: " << eps << ", window: " << window << ", steps: " << steps_per_episode << ", avg dps: " << est_dps << ", " << "dps: " << dps << ", fouls: " << blm.foul_count << ", f4s: " << blm.f4_count << ", b4s: " << blm.b4_count << std::endl;

					Logger::log(ss.str().c_str());
					std::cout << ss.str();
				}
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

		//float temp = rotation.eps;
		//rotation.eps = 0.0f;

		// B3 precast
		//int precast = blm.get_cast_time(2);
		//blm.timeline.time = -precast;
		//blm.mp_timer.time += precast;
		//blm.dot_timer.time += precast;
		//blm.use_action(2);
		//blm.step();

		while (blm.timeline.time < 60000)
			rotation.step();

		//rotation.eps = temp;
	}
}
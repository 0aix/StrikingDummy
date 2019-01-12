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
		const int NUM_STEPS_PER_EPOCH = 10000;
		const int NUM_STEPS_PER_EPISODE_MAX = 500;
		const int NUM_STEPS_PER_EPISODE = 20;
		const int NUM_BATCHES_PER_EPOCH = 50;
		const int CAPACITY = 1000000;
		const int BATCH_SIZE = 1000;
		const float WINDOW = 3000.0f;
		const float WINDOW_MAX = 20000.0f; // .. 60000?
		const float WINDOW_GROWTH = 0.1f;
		const float EPS_DECAY = 0.9995f;
		const float EPS_MIN = 0.25f;
		const float STEPS_GROWTH = 0.15f;
		const float STEPS_MAX = NUM_STEPS_PER_EPISODE_MAX;

		std::mt19937 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
		std::uniform_int_distribution<int> range(0, CAPACITY - 1);
		std::uniform_real_distribution<float> unif(0.0f, 1.0f);
		std::vector<int> indices(BATCH_SIZE);
		std::vector<int> actions(BATCH_SIZE);
		std::vector<float> rewards(BATCH_SIZE);

		Transition* memory = new Transition[CAPACITY];
		int m_index = 0;
		int m_size = 0;

		auto indices_gen = [&]() 
		{
			return range(rng);
		};

		BlackMage& blm = (BlackMage&)job;

		// Initialize model
		model.init(BATCH_SIZE);

		float nu = 0.01f; // sigmoid can use a larger learning rate
		float eps = 1.0f;
		float window = WINDOW;
		float steps_per_episode = NUM_STEPS_PER_EPISODE;
		float avg_dps = 0.0f;
		float beta = 0.9f;
		int epoch_offset = 0;

		for (int epoch = 0; epoch < NUM_EPOCHS; epoch++)
		{
			rotation.eps = eps;

			int num_episodes = NUM_STEPS_PER_EPOCH / steps_per_episode;
			//for (int episode = 0; episode < NUM_EPISODES_PER_EPOCH; episode++)
			for (int episode = 0; episode < num_episodes; episode++)
			{
				blm.reset();
				for (int step = 0; step < (int)steps_per_episode; step++)
					rotation.step();
				std::vector<Transition>* history = (std::vector<Transition>*)blm.get_history();
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

			if (m_size == CAPACITY)
			{
				// batch train a bunch
				for (int batch = 0; batch < NUM_BATCHES_PER_EPOCH; batch++)
				{
					std::generate(indices.begin(), indices.end(), indices_gen);

					// compute Q1
					for (int i = 0; i < BATCH_SIZE; i++)
						//memcpy(model.X0.col(i).data(), &memory[indices[i]].t1, sizeof(State));
						memcpy(&model.X0[i * 46], &memory[indices[i]].t1, sizeof(State));

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
							int index = *iter;
							if (q[index] > max_q)
								max_q = q[index];
						}
						rewards[i] = (1.0f / window) * (0.01f * t.reward + (window - t.dt) * max_q);
						actions[i] = t.action;
					}

					// compute Q0
					for (int i = 0; i < BATCH_SIZE; i++)
						//memcpy(model.X0.col(i).data(), &memory[indices[i]].t0, sizeof(State));
						memcpy(&model.X0[i * 46], &memory[indices[i]].t0, sizeof(State));

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
				//window *= WINDOW_GROWTH;
				window += WINDOW_GROWTH;
				if (window > WINDOW_MAX)
					window = WINDOW_MAX;
				//steps_per_episode *= STEPS_GROWTH;
				steps_per_episode += STEPS_GROWTH;
				if (steps_per_episode > STEPS_MAX)
					steps_per_episode = STEPS_MAX;
				//if (epoch % 1 == 0)
				{
					rotation.eps = 0.0f;
					test();

					float dps = 0.1f * blm.total_damage / blm.timeline.time;

					avg_dps = 0.9f * avg_dps + 0.1f * dps;
					float est_dps = avg_dps / (1.0f - beta);
					beta *= 0.9f;

					int _epoch = epoch - epoch_offset;

					std::stringstream ss;
					ss << "epoch: " << _epoch << ", eps: " << eps << ", window: " << window << ", steps: " << steps_per_episode << ", avg dps: " << est_dps << ", " << "dps: " << dps << ", fouls: " << blm.foul_count << ", f4s: " << blm.f4_count << ", b4s: " << blm.b4_count << ", t3s: " << blm.t3_count << std::endl;

					Logger::log(ss.str().c_str());
					std::cout << ss.str();

					if (_epoch % 1000 == 0)
					{
						std::stringstream filename;
						filename << "Weights\\weights-" << _epoch << std::flush;
						model.save(filename.str().c_str());
					}
				}
			}
			else
				epoch_offset++;
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

		while (blm.timeline.time < 60000)
			rotation.step();
	}
}
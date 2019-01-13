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
		const int NUM_STEPS_PER_EPISODE_MAX = 100;
		const int NUM_STEPS_PER_EPISODE = 100;
		const int NUM_BATCHES_PER_EPOCH = 50;
		const int CAPACITY = 1000000;
		const int BATCH_SIZE = 1000;
		const float WINDOW = 4000.0f;
		const float WINDOW_MAX = 18000.0f;
		const float WINDOW_GROWTH = 0.0f;
		const float EPS_DECAY = 0.9995f;
		const float EPS_MIN = 0.25f;
		const float STEPS_GROWTH = 0.1f;
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

		// Initialize model
		int state_size = job.get_state_size();
		int num_actions = job.get_num_actions();
		model.init(state_size, num_actions, BATCH_SIZE);
		//model.load("Weights\\BLM-46-64-64-15-20000s");

		float nu = 0.01f;
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
			for (int episode = 0; episode < num_episodes; episode++)
			{
				job.reset();
				for (int step = 0; step < (int)steps_per_episode; step++)
					rotation.step();
				for (int i = 0; i < (int)steps_per_episode; i++)
				{
					memory[m_index] = std::move(job.history[i]);
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
						memcpy(&model.X0[i * state_size], &memory[indices[i]].t1, sizeof(float) * state_size);

					//MatrixXf& Q1 = model.batch_compute();
					float* Q1 = model.batch_compute();

					// calculate rewards
					for (int i = 0; i < BATCH_SIZE; i++)
					{
						Transition& t = memory[indices[i]];
						//float* q = Q1.col(i).data();
						float* q = &Q1[i * num_actions];
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
						memcpy(&model.X0[i * state_size], &memory[indices[i]].t0, sizeof(float) * state_size);

					model.batch_compute();

					// calculate target
					//model.target = model.Xk;
					memcpy(model.target, model.X3, sizeof(float) * num_actions * BATCH_SIZE);
					for (int i = 0; i < BATCH_SIZE; i++)
						//model.target.col(i)(actions[i]) = rewards[i];
						model.target[i * num_actions + actions[i]] = rewards[i];

					// train
					model.train(nu);
				}
				eps *= EPS_DECAY;
				if (eps < EPS_MIN)
					eps = EPS_MIN;
				window += WINDOW_GROWTH;
				if (window > WINDOW_MAX)
					window = WINDOW_MAX;
				steps_per_episode += STEPS_GROWTH;
				if (steps_per_episode > STEPS_MAX)
					steps_per_episode = STEPS_MAX;

				rotation.eps = 0.0f;
				test();

				float dps = 0.1f * job.total_damage / job.timeline.time;

				avg_dps = 0.9f * avg_dps + 0.1f * dps;
				float est_dps = avg_dps / (1.0f - beta);
				beta *= 0.9f;

				int _epoch = epoch - epoch_offset;

				std::stringstream ss;
				//ss << "epoch: " << _epoch << ", eps: " << eps << ", window: " << window << ", steps: " << steps_per_episode << ", avg dps: " << est_dps << ", " << "dps: " << dps << ", fouls: " << blm.foul_count << ", f4s: " << blm.f4_count << ", b4s: " << blm.b4_count << ", t3s: " << blm.t3_count << std::endl;
				ss << "epoch: " << _epoch << ", eps: " << eps << ", window: " << window << ", steps: " << steps_per_episode << ", avg dps: " << est_dps << ", " << "dps: " << dps << std::endl;

				Logger::log(ss.str().c_str());
				std::cout << ss.str();

				if (_epoch % 1000 == 0)
				{
					std::stringstream filename;
					filename << "Weights\\monk-" << _epoch << std::flush;
					model.save(filename.str().c_str());
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
		job.reset();
		while (job.timeline.time < 4000)
			rotation.step();
	}

	std::string blm_actions[] =
	{
		"NONE",
		"B1", "B3", "B4", "F1", "F3", "F4", "T3", "FOUL",
		"SWIFT", "TRIPLE", "SHARP", "LEYLINES", "CONVERT", "ENOCHIAN"
	};

	void TrainingDummy::trace()
	{
		Logger::open();

		std::cout.precision(4);

		BlackMage& blm = (BlackMage&)job;
		blm.reset();

		// B3 precast
		int precast = blm.get_cast_time(2) + 10;
		//blm.use_action(11);
		//blm.step();
		blm.use_action(2);
		blm.step();

		Logger::log("=============\n");

		model.init(blm.get_state_size(), blm.get_num_actions(), 1);
		model.load("Weights\\weights");

		rotation.eps = 0.0f;

		while (blm.timeline.time < 60000)
			rotation.step();

		int length = blm.history.size() - 1;
		for (int i = 0; i < length; i++)
		{
			Transition& t = blm.history[i];
			std::stringstream ss;
			if (t.action == 5 && t.t0[22] == 1.0f)
				ss << t.t0[0] * 15480.0f << " F3p";
			else if (t.action == 7)
			{
				if (t.t0[24] == 1.0f)
					ss << t.t0[0] * 15480.0f << " T3p at " << t.t0[27] * 24 << "s left on dot";
				else
					ss << t.t0[0] * 15480.0f << " T3 at " << t.t0[27] * 24 << "s left on dot";
			}
			else if (t.action != 0)
				ss << t.t0[0] * 15480.0f << " " << blm_actions[t.action];
			if (t.action != 0)
			{
				if (t.t0[24] == 1.0f)
					ss << " (T3p w/ " << t.t0[25] * 18 << "s)";
				ss << std::endl;
			}
			Logger::log(ss.str().c_str());
		}

		std::stringstream ss;
		ss << "DPS: " << 100.0f / (blm.timeline.time - precast) * blm.total_damage << "\n=============" << std::endl;
		Logger::log(ss.str().c_str());

		Logger::close();
	}
}
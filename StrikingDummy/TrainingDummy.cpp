#include "TrainingDummy.h"
#include "Samurai.h"
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
		const int NUM_STEPS_PER_EPISODE = 10000;
		const int NUM_BATCHES_PER_EPOCH = 50;
		const int CAPACITY = 1000000;
		const int BATCH_SIZE = 10000;
		const float WINDOW = 60000.0f;
		const float EPS_DECAY = 0.999f;
		const float EPS_START = 1.0f;
		const float EPS_MIN = 0.10f;
		const float OUTPUT_LOWER = 18.000f;
		const float OUTPUT_UPPER = 21.000f;
		const float OUTPUT_RANGE = OUTPUT_UPPER - OUTPUT_LOWER;

		std::stringstream zz;
		zz << "lower: " << OUTPUT_LOWER << ", upper: " << OUTPUT_UPPER << std::endl;

		Logger::log(zz.str().c_str());
		std::cout << zz.str();

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
		model.init(state_size, num_actions, BATCH_SIZE, false);
		model.load("Weights\\weights");

		Samurai& sam = (Samurai&)job;

		float nu = 0.001f;
		float eps = EPS_START;
		float exp = 0.0f;
		float steps_per_episode = NUM_STEPS_PER_EPISODE;
		float avg_dps = 0.0f;
		float est_dps = 0.0f;
		float beta = 0.9f;
		int epoch_offset = 0;

		for (int epoch = 0; epoch < NUM_EPOCHS; epoch++)
		{
			rotation.reset(eps, exp);

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
						memcpy(&model.X0[i * state_size], &memory[indices[i]].t1, sizeof(float) * state_size);

					float* Q1 = model.batch_compute();

					// calculate rewards
					for (int i = 0; i < BATCH_SIZE; i++)
					{
						Transition& t = memory[indices[i]];
						float* q = &Q1[i * num_actions];
						float max_q = q[t.actions[0]];
						auto cend = t.actions.cend();
						for (auto iter = t.actions.cbegin() + 1; iter != cend; iter++)
						{
							int index = *iter;
							if (q[index] > max_q)
								max_q = q[index];
						}
						max_q = OUTPUT_LOWER + OUTPUT_RANGE * max_q;
						rewards[i] = (1.0f / OUTPUT_RANGE) * ((1.0f / WINDOW) * (t.reward + (WINDOW - t.dt) * max_q) - OUTPUT_LOWER);
						actions[i] = t.action;
					}

					// compute Q0
					for (int i = 0; i < BATCH_SIZE; i++)
						memcpy(&model.X0[i * state_size], &memory[indices[i]].t0, sizeof(float) * state_size);

					model.batch_compute();

					// calculate target
					memcpy(model.target, model.X3, sizeof(float) * num_actions * BATCH_SIZE);
					for (int i = 0; i < BATCH_SIZE; i++)
						model.target[i * num_actions + actions[i]] = rewards[i];

					// train
					model.train(nu);
				}
				model.copyToHost();

				// adjust parameters
				eps *= EPS_DECAY;
				if (eps < EPS_MIN)
					eps = EPS_MIN;

				// test model
				test();

				float dps = 0.1f * job.total_damage / job.timeline.time;

				avg_dps = 0.9f * avg_dps + 0.1f * dps;
				est_dps = avg_dps / (1.0f - beta);

				int _epoch = epoch - epoch_offset;

				// test model
				if (_epoch % 50 == 0)
				{
					test();

					float dps = job.total_damage / job.timeline.time;
					//avg_dps = 0.9f * avg_dps + 0.1f * (0.1f * job.total_damage / job.timeline.time);
					//est_dps = avg_dps / (1.0f - beta);
					//beta *= 0.9f;

					std::stringstream ss;
					ss << "epoch: " << _epoch << ", eps: " << eps << ", window: " << WINDOW << ", steps: " << steps_per_episode << ", dps: " << dps << ", midares: " << sam.midare_count << ", tsubames: " << sam.tsubame_count << ", higanbanas: " << sam.higanbana_count << ", kaitens: " << sam.kaiten_count << ", shintens: " << sam.shinten_count << ", seneis: " << sam.senei_count << ", hagakures: " << sam.hagakure_count << ", shohas: " << sam.shoha_count << ", pots: " << sam.pot_count << std::endl;
					Logger::log(ss.str().c_str());
					std::cout << ss.str();

					if (_epoch % 500 == 0)
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
		rotation.reset(0.0f, 0.0f);
		job.reset();
		while (job.timeline.time < 600000)
			rotation.step();
	}

	void TrainingDummy::trace()
	{
		Logger::open();

		std::cout.precision(4);

		Samurai& sam = (Samurai&)job;
		sam.reset();

		Logger::log("=============\n");

		model.init(sam.get_state_size(), sam.get_num_actions(), 1, false);
		model.load("Weights\\weights");

		rotation.eps = 0.0f;

		while (sam.timeline.time < 7 * 24 * 3600000)
		//while (sam.timeline.time < 600000)
			rotation.step();

		std::stringstream ss;
		ss << "DPS: " << 1000.0f / sam.timeline.time * sam.total_damage << "\n=============" << std::endl;
		Logger::log(ss.str().c_str());
		
		int length = sam.history.size() - 1;
		if (length > 10000)
			length = 10000;
		int time = 0;
		for (int i = 0; i < length; i++)
		{
			Transition& t = sam.history[i];
			int hours = time / 3600000;
			int minutes = (time / 60000) % 60;
			int seconds = (time / 1000) % 60;
			int centiseconds = lround(time % 1000) / 10;
			if (t.action != 0)
			{
				std::stringstream ss;
				ss << "[";
				if (hours < 10)
					ss << "0";
				ss << hours << ":";
				if (minutes < 10)
					ss << "0";
				ss << minutes << ":";
				if (seconds < 10)
					ss << "0";
				ss << seconds << ".";
				if (centiseconds < 10)
					ss << "0";
				ss << centiseconds << "] ";
				ss << sam.get_action_name(t.action) << " " << t.t0[0] * 100.0f << std::endl;
				//ss << t.t0[0] * 100.0f << " " << sam.get_action_name(t.action) << std::endl;
				Logger::log(ss.str().c_str());
			}
			time += t.dt;
		}
		Logger::close();
	}
}
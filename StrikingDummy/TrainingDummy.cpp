#include "TrainingDummy.h"
#include "Summoner.h"
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
		const int NUM_STEPS_PER_EPOCH = 20000;
		const int NUM_STEPS_PER_EPISODE = 4000;
		const int NUM_BATCHES_PER_EPOCH = 50;
		const int CAPACITY = 2000000;
		const int BATCH_SIZE = 10000;
		const float WINDOW = 60000.0f;
		const float EPS_DECAY = 0.999f;
		const float EPS_START = 1.0f;
		const float EPS_MIN = 0.15f;
		const float OUTPUT_LOWER = 157.75f;
		const float OUTPUT_UPPER = 162.75f;
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

		Summoner& smn = (Summoner&)job;

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

				std::stringstream ss;
				ss << "epoch: " << _epoch << ", eps: " << eps << ", window: " << WINDOW << ", steps: " << steps_per_episode << ", avg dps: " << est_dps << ", " << "dps: " << dps << ", time: " << 0.01f * job.timeline.time << "s" << std::endl;

				beta *= 0.9f;

				Logger::log(ss.str().c_str());
				std::cout << ss.str();

				if (_epoch % 1000 == 0)
				{
					std::stringstream filename;
					filename << "Weights\\weights-" << _epoch << std::flush;
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
		rotation.reset(0.0f, 0.0f);
		job.reset();
		//while (job.timeline.time < 60000)
		for (int i = 0; i < 2000; i++)
			rotation.step();
	}

	void TrainingDummy::trace()
	{
		Logger::open();

		std::cout.precision(4);

		Summoner& smn = (Summoner&)job;
		smn.reset();

		Logger::log("=============\n");

		model.init(smn.get_state_size(), smn.get_num_actions(), 1, false);
		model.load("Weights\\weights");

		rotation.eps = 0.0f;

		while (smn.timeline.time < 7 * 24 * 360000)
		//while (smn.timeline.time < 120000)
			rotation.step();

		std::stringstream ss;
		ss << "DPS: " << 100.0f / smn.timeline.time * smn.total_damage << "\n";
		ss << "Dot uptime: " << 100.0f / smn.timeline.time * smn.total_dot_time << "%\n=============" << std::endl;
		Logger::log(ss.str().c_str());
		
		int length = smn.history.size() - 1;
		if (length > 10000)
			length = 10000;
		int time = 0;
		for (int i = 0; i < length; i++)
		{
			Transition& t = smn.history[i];
			if (t.action != 0)
			{
				int hours = time / 360000;
				int minutes = (time / 6000) % 60;
				int seconds = (time / 100) % 60;
				int centiseconds = time % 100;
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
				ss << centiseconds << "] " << smn.get_action_name(t.action) << std::endl;;
				Logger::log(ss.str().c_str());
			}
			time += t.dt;
		}
		Logger::close();
	}
	/*
	void TrainingDummy::metrics()
	{
		Logger::open();

		std::cout.precision(2);

		BlackMage& blm = (BlackMage&)job;
		blm.reset();
		blm.metrics_enabled = true;

		model.init(blm.get_state_size(), blm.get_num_actions(), 1, false);
		model.load("Weights\\weights");

		rotation.eps = 0.0f;

		while (blm.timeline.time < 24 * 360000)
			rotation.step();

		std::vector<int>* dists[] = { &blm.t3_dist, &blm.t3p_dist, &blm.swift_dist, &blm.triple_dist, &blm.sharp_dist, &blm.ll_dist, &blm.mf_dist };
		std::stringstream ss;
		for (int i = 0; i < 7; i++)
		{
			for (int t : *dists[i])
				ss << 0.01 * t << ",";
			ss << std::endl;
		}
		Logger::log(ss.str().c_str());
		Logger::close();
	}

	void TrainingDummy::dist(int seconds, int times)
	{
		Logger::open();

		std::cout.precision(2);

		BlackMage& blm = (BlackMage&)job;

		model.init(blm.get_state_size(), blm.get_num_actions(), 1, false);
		model.load("Weights\\weights");

		rotation.eps = 0.0f;

		int time = seconds * 100;

		std::stringstream ss;

		for (int i = 0; i < times; i++)
		{
			blm.reset();
			while (blm.timeline.time < time)
				rotation.step();
			ss << 100.0f / blm.timeline.time * blm.total_damage << "\n";
		}

		Logger::log(ss.str().c_str());
		Logger::close();
	}
	*/
}
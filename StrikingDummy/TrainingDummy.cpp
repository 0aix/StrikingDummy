#include "TrainingDummy.h"
#include "BlackMage.h"
#include "Logger.h"
#include <chrono>
#include <iostream>
#include <random>
#include <algorithm>
#include <execution>
#include <future>

namespace StrikingDummy
{
	const int NUM_EPOCHS = 1000000;
	const int NUM_STEPS_PER_EPOCH = 20000;
	const int NUM_STEPS_PER_EPISODE = 2500;
	const int NUM_EPISODES = NUM_STEPS_PER_EPOCH / NUM_STEPS_PER_EPISODE;
	const int CAPACITY = 1000000;
	const int NUM_INDICES = CAPACITY / NUM_STEPS_PER_EPOCH;
	const int BATCH_SIZE = 10000;
	const int NUM_BATCHES_PER_EPOCH = CAPACITY / BATCH_SIZE;
	const float WINDOW = 600000.0f;
	const float EPS_DECAY = 0.999f;
	const float EPS_START = 1.00f;
	const float EPS_MIN = 0.01f;
	const float NU_DECAY = 0.9999f;
	const float NU_START = 0.00001f; //0.00001f
	const float NU_MIN = 0.000001f;
	const float OUTPUT_LOWER = 112.000f;
	const float OUTPUT_UPPER = 123.000f;
	const float OUTPUT_RANGE = OUTPUT_UPPER - OUTPUT_LOWER;
	const double BEST_THRESHOLD_TO_SAVE = 100.000;

	void TrainingDummy::train()
	{
		std::cout.precision(4);

		long long start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();

		std::stringstream zz;
		zz << "lower: " << OUTPUT_LOWER << ", upper: " << OUTPUT_UPPER << std::endl;

		Logger::log(zz.str().c_str());
		std::cout << zz.str();

		int m_index = 0;

		// Initialize model
		int state_size = job.get_state_size();
		int num_actions = job.get_num_actions();
		model.init(BATCH_SIZE, CAPACITY);
		model.load("Weights\\weights");

		float* state_memory = new float[state_size * NUM_STEPS_PER_EPOCH];
		unsigned char* action_memory = new unsigned char[num_actions * NUM_STEPS_PER_EPOCH];
		float* reward_memory = new float[2 * NUM_STEPS_PER_EPOCH];
		int* move_memory = new int[NUM_STEPS_PER_EPOCH];

		BlackMage& blm = (BlackMage&)job;

		float nu = NU_START;
		float eps = EPS_START;
		//float eps = EPS_MIN;
		float exp = 0.0f;
		float steps_per_episode = NUM_STEPS_PER_EPISODE;
		float avg_dps = 0.0f;
		float est_dps = 0.0f;
		float beta = 0.9f;
		int epoch_offset = 0;

		long long generate_time = 0;
		long long total_count = 0;

		long long copy_time = 0;
		long long copy_count = 0;

		long long time_a;
		long long time_b;

		std::future<bool> best_future;

		std::vector<BlackMage> jobs(NUM_EPISODES, blm);
		std::vector<ModelRotation> rotations;
		std::vector<int> ints(NUM_EPISODES);
		std::iota(ints.begin(), ints.end(), 0);
		std::for_each(ints.begin(), ints.end(), [&](int& i) { rotations.emplace_back(jobs[i], model, i * 1111); });

		auto rotato = [&](int idx)
		{
			ModelRotation& r = rotations[idx];
			r.reset(eps, exp);
			r.job.reset();
			for (int step = 0; step < NUM_STEPS_PER_EPISODE; step++)
				r.step();
			int c = idx * NUM_STEPS_PER_EPISODE;
			for (int i = 0; i < NUM_STEPS_PER_EPISODE; i++)
			{
				Transition& t = r.job.history[i];
				memcpy(&state_memory[(c + i) * state_size], t.t0, state_size * sizeof(float));
				// actions size can't actually be num_actions
				if (t.actions.size() >= num_actions)
				{
					std::cout << "more actions than num_actions" << std::endl;
					throw 0;
				}
				int j;
				for (j = 0; j < t.actions.size(); j++)
					action_memory[(c + i) * num_actions + j] = t.actions[j];
				action_memory[(c + i) * num_actions + j] = num_actions;
				reward_memory[(c + i) * 2] = (t.reward - t.dt * OUTPUT_LOWER) / OUTPUT_RANGE / WINDOW;
				reward_memory[(c + i) * 2 + 1] = 1.0f - t.dt / WINDOW;
				move_memory[c + i] = t.action;
			}
		};

		for (int i = 0; i < NUM_INDICES; i++)
		{
			std::for_each(std::execution::par_unseq, ints.begin(), ints.end(), rotato);
			model.copyMemory(i * NUM_STEPS_PER_EPOCH, state_memory, action_memory, reward_memory, move_memory, NUM_STEPS_PER_EPOCH);
		}

		for (int epoch = 0; epoch < NUM_EPOCHS; epoch++)
		{
			time_a = std::chrono::high_resolution_clock::now().time_since_epoch().count();

			auto future = std::async(std::launch::async, [&] { std::for_each(std::execution::par_unseq, ints.begin(), ints.end(), rotato); });

			time_b = std::chrono::high_resolution_clock::now().time_since_epoch().count();

			generate_time += time_b - time_a;
			total_count++;

			time_a = std::chrono::high_resolution_clock::now().time_since_epoch().count();

			for (int batch = 0; batch < NUM_BATCHES_PER_EPOCH; batch++)
				model.batch_train(nu, batch);

			future.wait();

			model.copyToHost();

			model.copyMemory(m_index * NUM_STEPS_PER_EPOCH, state_memory, action_memory, reward_memory, move_memory, NUM_STEPS_PER_EPOCH);

			if (++m_index == NUM_INDICES)
				m_index = 0;

			time_b = std::chrono::high_resolution_clock::now().time_since_epoch().count();

			copy_time += time_b - time_a;
			copy_count++;

			// adjust parameters
			eps *= EPS_DECAY;
			if (eps < EPS_MIN)
				eps = EPS_MIN;

			//nu *= NU_DECAY;
			//if (nu < NU_MIN)
			//	nu = NU_MIN;

			// test model
			if (epoch % 50 == 0)
			{
				float q;
				float r;
				int s;
				test(q, r, s);

				q = OUTPUT_LOWER + (OUTPUT_UPPER - OUTPUT_LOWER) / (1.0f + expf(-q));
				r = OUTPUT_LOWER + (OUTPUT_UPPER - OUTPUT_LOWER) / (1.0f + expf(-r));

				double dps = job.total_damage / job.timeline.time;

				std::stringstream ss;
				ss << "epoch: " << epoch << ", eps: " << eps << ", window: " << WINDOW << ", steps: " << steps_per_episode << ", test steps: " << s << ", " << "dps: " << dps << ", guess: " << q << ", error: " << dps - q << ", end_guess: " << r << ", tornados: " << blm.tornado_count << std::endl;
				ss << "20000 rotation steps ms: " << generate_time / 1000000.0 / total_count << ", epoch ms: " << copy_time / 1000000.0 / copy_count << std::endl;

				generate_time = 0;
				total_count = 0;
				copy_time = 0;
				copy_count = 0;

				Logger::log(ss.str().c_str());
				std::cout << ss.str();

				if (epoch % 10000 == 0)
				{
					std::stringstream filename;
					filename << "Weights\\weights-" << epoch << std::flush;
					model.save(filename.str().c_str());
				}
			}

			// best model
			if (!best_future.valid() || best_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
			{
				if (best_future.valid() && best_future.get())
				{
					std::stringstream ss;
					ss << "best epoch: " << best_epoch << ", ms: " << best_time / 1000000.0 << ", best mean: " << best_mean << std::endl;
					Logger::log(ss.str().c_str());
					std::cout << ss.str();
				}
				job.reset();
				best_epoch = epoch;
				best_model.copyWeights(model);
				best_future = std::async(std::launch::async, &TrainingDummy::best, this);
			}
		}

		delete[] state_memory;
		delete[] action_memory;
		delete[] reward_memory;
		delete[] move_memory;

		long long end_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();

		std::cout << "running time: " << (end_time - start_time) / 1000000000.0 << " seconds" << std::endl;

		Logger::close();
	}

	void TrainingDummy::test(float& q, float& r, int& s)
	{
		rotation.reset(0.0f, 0.0f);
		job.reset();
		rotation.step();
		q = rotation.stored_max_weight;
		int num_steps = 0;
		while (job.timeline.time < 600000)
		{
			//while (job.timeline.time < 510000)
			rotation.step();
			num_steps++;
		}
		r = rotation.stored_max_weight;
		s = num_steps;
	}

	bool TrainingDummy::best()
	{
		long long time_a = std::chrono::high_resolution_clock::now().time_since_epoch().count();

		BlackMage blm = (BlackMage&)job;
		ModelRotation rotation(blm, best_model);
		std::vector<double> dps;

		for (int gauge_tick = 100; gauge_tick <= 1000; gauge_tick += 100)
		{
			blm.reset(gauge_tick);
			while (blm.timeline.time < 6000000) // 100 minutes
			//while (blm.timeline.time < 510000)
				rotation.step();
			dps.push_back(blm.total_damage / blm.timeline.time);
		}

		bool update = false;
		double mean = std::accumulate(dps.begin(), dps.end(), 0.0f) / dps.size();
		if (mean > best_mean)
		{
			best_mean = mean;
			update = true;
			if (best_mean >= BEST_THRESHOLD_TO_SAVE)
			{
				std::stringstream filename;
				filename << "Weights\\weights-mean-" << best_epoch << std::flush;
				best_model.save(filename.str().c_str());
			}
		}

		long long time_b = std::chrono::high_resolution_clock::now().time_since_epoch().count();

		best_time = time_b - time_a;

		return update;
	}

	void TrainingDummy::trace()
	{
		Logger::open("trace");

		BlackMage& blm = (BlackMage&)job;
		blm.reset();

		Logger::log("=============\n");

		model.load("Weights\\weights");

		rotation.eps = 0.0f;
		while (blm.timeline.time < 7 * 24 * 3600000)
		//while (blm.timeline.time < 510000)
			rotation.step();

		std::stringstream ss;
		ss.setf(std::ios::fixed, std::ios::floatfield);
		ss.precision(2);
		ss << "DPS: " << 1000.0 / blm.timeline.time * blm.total_damage << "\n";
		ss << "Tornado % damage: " << 100.0 / blm.total_damage * blm.total_tornado_damage << "%\n=============" << std::endl;
		Logger::log(ss.str().c_str());

		int length = blm.history.size() - 1;
		if (length > 10000)
			length = 10000;
		int time = 0;
		float damage = blm.pre_damage;
		for (int i = 0; i < length; i++)
		{
			Transition& t = blm.history[i];
			int hours = time / 3600000;
			int minutes = (time / 60000) % 60;
			int seconds = (time / 1000) % 60;
			int centiseconds = lround(time % 1000) / 10;
			std::stringstream ss;
			ss.setf(std::ios::fixed, std::ios::floatfield);
			ss.precision(1);
			damage += t.reward;
			if (t.action != 0)
			{
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
				int gauge = lroundf(t.t0[0] * 130.0f);
				int sharp = lroundf(t.t0[1] + t.t0[2] + t.t0[3] + t.t0[4] + t.t0[5] + t.t0[6]);
				int chasing = lroundf(t.t0[19] + t.t0[20]);
				ss << gauge << "|" << sharp << "|" << chasing << " ";
				/*
				if (t.action == BlackMage::F1 && t.t0[24] == 1.0f)
					ss << "F1^";
				else if (t.action == BlackMage::F3 && t.t0[24] == 1.0f)
					ss << "F3p";
				else if (t.action == BlackMage::PARADOX && t.t0[2] == 1.0f && t.t0[24] == 1.0f)
					ss << "PARADOX^";
				else if (t.action == BlackMage::T5)
					ss << "T3 at " << lround(t.t0[27] * BlackMage::DOT_DURATION) / 1000.0f << "s left on dot";
				else if (t.action == BlackMage::XENO)
				{
					if (t.t0[14] == 1.0f)
						ss << "XENO***";
					else if (t.t0[13] == 1.0f)
						ss << "XENO**";
					else
						ss << "XENO*";
				}
				else if (t.action == BlackMage::TRIPLE)
				{
					if (t.t0[35] == 1.0f)
						ss << "TRIPLE**";
					else
						ss << "TRIPLE*";
				}
				else
					ss << blm.get_action_name(t.action);
				*/
				ss << blm.get_action_name(t.action);
				ss << " " << 1000.0f * damage / time;
				ss << std::endl;

				Logger::log(ss.str().c_str());
			}
			time += t.dt;
		}
		Logger::close();
	}
}
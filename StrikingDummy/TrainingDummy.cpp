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
	const int NUM_THREADS = 8;
	const int NUM_STEPS_PER_THREAD = NUM_STEPS_PER_EPOCH / NUM_THREADS;
	const int NUM_STEPS_PER_EPISODE = 2500;
	const int NUM_EPISODES = NUM_STEPS_PER_EPOCH / NUM_STEPS_PER_EPISODE;
	const int CAPACITY = 1000000;
	const int NUM_INDICES = CAPACITY / NUM_STEPS_PER_EPOCH;
	const int BATCH_SIZE = 10000;
	const int NUM_BATCHES = CAPACITY / BATCH_SIZE;
	const int NUM_BATCHES_PER_EPOCH = NUM_BATCHES;
	const float WINDOW = 600000.0f;
	const float EPS_DECAY = 0.999f;
	const float EPS_START = 1.0f;
	const float EPS_MIN = 0.005f;
	const float OUTPUT_LOWER = 9.050f;
	const float OUTPUT_UPPER = 9.300f;
	const float OUTPUT_RANGE = OUTPUT_UPPER - OUTPUT_LOWER;
	const double BEST_THRESHOLD_TO_SAVE = 9.190;

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

		float nu = 0.00001f;
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

		std::vector<BlackMage> jobs(NUM_THREADS, blm);
		std::vector<ModelRotation> rotations;
		std::vector<int> ints(NUM_THREADS);
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

			std::priority_queue<std::pair<float, int>, std::vector<std::pair<float, int>>, std::greater<std::pair<float, int>>> pq;

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

			// test model
			if (epoch % 50 == 0)
			{
				float q = test();

				q = OUTPUT_LOWER + (OUTPUT_UPPER - OUTPUT_LOWER) / (1.0f + expf(-q));

				double dps = job.total_damage / job.timeline.time;

				std::stringstream ss;
				ss << "epoch: " << epoch << ", eps: " << eps << ", window: " << WINDOW << ", steps: " << steps_per_episode << ", " << "dps: " << dps << ", guess: " << q << ", error: " << dps - q << ", xenos: " << blm.xeno_count << ", f1s: " << blm.f1_count << ", f4s: " << blm.f4_count << ", b3s: " << blm.b3_count << ", b4s: " << blm.b4_count << ", t3s: " << blm.t3_count << ", transposes: " << blm.transpose_count << ", despairs: " << blm.despair_count << ", lucids: " << blm.lucid_count << ", pots: " << blm.pot_count << std::endl;
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

	float TrainingDummy::test()
	{
		rotation.reset(0.0f, 0.0f);
		job.reset();
		rotation.step();
		float q = rotation.stored_max_weight;
		while (job.timeline.time < 600000)
			rotation.step();
		return q;
	}

	bool TrainingDummy::best()
	{
		long long time_a = std::chrono::high_resolution_clock::now().time_since_epoch().count();

		BlackMage blm = (BlackMage&)job;
		ModelRotation rotation(blm, best_model);
		std::vector<double> dps;

		for (int mp_tick = 100; mp_tick <= 3000; mp_tick += 100)
		{
			for (int lucid_tick = 100; lucid_tick <= 3000; lucid_tick += 100)
			{
				blm.reset(mp_tick, lucid_tick, 0);
				while (blm.timeline.time < 6000000) // 100 minutes
					rotation.step();
				dps.push_back(blm.total_damage / blm.timeline.time);
			}
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
			rotation.step();

		std::stringstream ss;
		ss.setf(std::ios::fixed, std::ios::floatfield);
		ss.precision(2);
		ss << "DPS: " << 1000.0 / blm.timeline.time * blm.total_damage << "\n";
		ss << "T3 uptime: " << 100.0 / blm.timeline.time * blm.total_dot_time << "%\n";
		ss << "F4 % damage: " << 100.0 / blm.total_damage * blm.total_f4_damage << "%\n";
		ss << "Desp % damage: " << 100.0 / blm.total_damage * blm.total_desp_damage << "%\n";
		ss << "Xeno % damage: " << 100.0 / blm.total_damage * blm.total_xeno_damage << "%\n";
		ss << "T3 % damage: " << 100.0 / blm.total_damage * blm.total_t3_damage << "%\n";
		ss << "Dot % damage: " << 100.0 / blm.total_damage * blm.total_dot_damage << "%\n=============" << std::endl;
		Logger::log(ss.str().c_str());

		int length = blm.history.size() - 1;
		if (length > 10000)
			length = 10000;
		int time = 0;
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
				ss << lroundf(t.t0[0] * 10000.0f) << " ";
				if (t.action == BlackMage::F1 && t.t0[24] == 1.0f)
					ss << "F1^";
				else if (t.action == BlackMage::F3 && t.t0[24] == 1.0f)
					ss << "F3p";
				else if (t.action == BlackMage::PARADOX && t.t0[2] == 1.0f && t.t0[24] == 1.0f)
					ss << "PARADOX^";
				else if (t.action == BlackMage::T3)
				{
					if (t.t0[26] == 1.0f)
						ss << "T3p at " << lround(t.t0[29] * BlackMage::DOT_DURATION) / 1000.0f << "s left on dot";
					else
						ss << "T3 at " << lround(t.t0[29] * BlackMage::DOT_DURATION) / 1000.0f << "s left on dot";
				}
				else if (t.action == BlackMage::XENO)
				{
					if (t.t0[14] == 1.0f)
						ss << "XENO**";
					else
						ss << "XENO*";
				}
				else if (t.action == BlackMage::TRIPLE)
				{
					if (t.t0[37] == 1.0f)
						ss << "TRIPLE**";
					else
						ss << "TRIPLE*";
				}
				else if (t.action == BlackMage::SHARP)
				{
					if (t.t0[40] == 1.0f)
						ss << "SHARP**";
					else
						ss << "SHARP*";
				}
				else
					ss << blm.get_action_name(t.action);

				if (t.t0[26] == 1.0f)
					ss << " (T3p w/ " << lround(t.t0[27] * BlackMage::TC_DURATION) / 1000.0f << "s)";

				ss << std::endl;

				Logger::log(ss.str().c_str());
			}
			time += t.dt;
		}
		Logger::close();
	}

	void TrainingDummy::montecarlo()
	{
		Logger::open("monte-carlo");

		BlackMage& blm = (BlackMage&)job;
		blm.reset();

		BlackMage temp(blm);
		temp.reset();

		ModelRotation rotation(temp, model);

		Logger::log("=============\n");

		model.load("Weights\\weights");

		MCRotation mc(job, rotation);

		while (blm.timeline.time < 1200000)
			mc.step();

		std::stringstream ss;
		ss.setf(std::ios::fixed, std::ios::floatfield);
		ss.precision(2);
		ss << "DPS: " << 1000.0 / blm.timeline.time * blm.total_damage << "\n";
		ss << "T3 uptime: " << 100.0 / blm.timeline.time * blm.total_dot_time << "%\n";
		ss << "F4 % damage: " << 100.0 / blm.total_damage * blm.total_f4_damage << "%\n";
		ss << "Desp % damage: " << 100.0 / blm.total_damage * blm.total_desp_damage << "%\n";
		ss << "Xeno % damage: " << 100.0 / blm.total_damage * blm.total_xeno_damage << "%\n";
		ss << "T3 % damage: " << 100.0 / blm.total_damage * blm.total_t3_damage << "%\n";
		ss << "Dot % damage: " << 100.0 / blm.total_damage * blm.total_dot_damage << "%\n=============" << std::endl;
		Logger::log(ss.str().c_str());

		int length = blm.history.size() - 1;
		if (length > 10000)
			length = 10000;
		int time = 0;
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
				ss << lroundf(t.t0[0] * 10000.0f) << " ";
				if (t.action == BlackMage::F1 && t.t0[24] == 1.0f)
					ss << "F1^";
				else if (t.action == BlackMage::F3 && t.t0[24] == 1.0f)
					ss << "F3p";
				else if (t.action == BlackMage::PARADOX && t.t0[2] == 1.0f && t.t0[24] == 1.0f)
					ss << "PARADOX^";
				else if (t.action == BlackMage::T3)
				{
					if (t.t0[26] == 1.0f)
						ss << "T3p at " << lround(t.t0[29] * BlackMage::DOT_DURATION) / 1000.0f << "s left on dot";
					else
						ss << "T3 at " << lround(t.t0[29] * BlackMage::DOT_DURATION) / 1000.0f << "s left on dot";
				}
				else if (t.action == BlackMage::XENO)
				{
					if (t.t0[14] == 1.0f)
						ss << "XENO**";
					else
						ss << "XENO*";
				}
				else if (t.action == BlackMage::TRIPLE)
				{
					if (t.t0[37] == 1.0f)
						ss << "TRIPLE**";
					else
						ss << "TRIPLE*";
				}
				else if (t.action == BlackMage::SHARP)
				{
					if (t.t0[40] == 1.0f)
						ss << "SHARP**";
					else
						ss << "SHARP*";
				}
				else
					ss << blm.get_action_name(t.action);

				int temp = mc.history[i];
				if (t.action != temp)
				{
					ss << " [[";
					if (temp == BlackMage::F1 && t.t0[24] == 1.0f)
						ss << "F1^";
					else if (temp == BlackMage::F3 && t.t0[24] == 1.0f)
						ss << "F3p";
					else if (temp == BlackMage::PARADOX && t.t0[2] == 1.0f && t.t0[24] == 1.0f)
						ss << "PARADOX^";
					else if (temp == BlackMage::T3)
					{
						if (t.t0[26] == 1.0f)
							ss << "T3p at " << lround(t.t0[29] * BlackMage::DOT_DURATION) / 1000.0f << "s left on dot";
						else
							ss << "T3 at " << lround(t.t0[29] * BlackMage::DOT_DURATION) / 1000.0f << "s left on dot";
					}
					else if (temp == BlackMage::XENO)
					{
						if (t.t0[14] == 1.0f)
							ss << "XENO**";
						else
							ss << "XENO*";
					}
					else if (temp == BlackMage::TRIPLE)
					{
						if (t.t0[37] == 1.0f)
							ss << "TRIPLE**";
						else
							ss << "TRIPLE*";
					}
					else if (temp == BlackMage::SHARP)
					{
						if (t.t0[40] == 1.0f)
							ss << "SHARP**";
						else
							ss << "SHARP*";
					}
					else
						ss << blm.get_action_name(temp);
					ss << "]]";
				}

				if (t.t0[26] == 1.0f)
					ss << " (T3p w/ " << lround(t.t0[27] * BlackMage::TC_DURATION) / 1000.0f << "s)";

				ss << std::endl;

				Logger::log(ss.str().c_str());
			}
			time += t.dt;
		}
		Logger::close();
	}

	void TrainingDummy::metrics()
	{
		Logger::open("metrics");

		std::cout.precision(2);

		BlackMage& blm = (BlackMage&)job;
		blm.reset();
		blm.dist_metrics_enabled = true;

		model.load("Weights\\weights");

		rotation.eps = 0.0f;

		//while (blm.timeline.time < 24 * 3600000)
		while (blm.timeline.time < 7 * 24 * 3600000)
			rotation.step();
		/*
		std::vector<int>* dists[] = { &blm.t3_dist, &blm.t3p_dist, &blm.swift_dist, &blm.triple_dist, &blm.sharp_dist, &blm.ll_dist, &blm.mf_dist };
		std::stringstream ss;
		for (int i = 0; i < 7; i++)
		{
			for (int t : *dists[i])
				ss << 0.001 * t << ",";
			ss << std::endl;
		}
		*/
		// get Avg # of refreshes and Average HRC for t3
		std::stringstream ss;
		ss << "# of T3Ps: " << blm.t3p_dist.size() << "\n" << "# of hardcast T3s: " << blm.t3_dist.size() << "\n";
		ss << "Average # of refreshes: " << (float)blm.t3p_dist.size() / blm.t3_dist.size() << "\n";
		ss << "Average HRC: " << 0.001f * blm.total_dot_time / blm.t3_dist.size() << std::endl;

		Logger::log(ss.str().c_str());
		Logger::close();
	}

	void TrainingDummy::dist(int seconds, int samples)
	{
		Logger::open("dist");

		std::cout.precision(2);

		BlackMage& blm = (BlackMage&)job;

		model.load("Weights\\weights");

		rotation.eps = 0.0f;

		int time = seconds * 1000;

		std::stringstream ss;

		for (int i = 0; i < samples; i++)
		{
			blm.reset();
			while (blm.timeline.time < time)
				rotation.step();
			int j = blm.history.size() - 2;
			assert(j >= 0);
			// additional minute to "end" rotation
			while (blm.timeline.time < time + 60000)
			{
				Transition& t = blm.history[j];
				if (t.t0[2] == 1.0f && t.t1[1] == 1.0f)
				{
					blm.timeline.time -= t.dt;
					blm.total_damage -= t.reward;
					break;
				}
				rotation.step();
				j++;
			}
			// time and dps
			// still slightly off if for example, instant -> ogcd vs instant -> nothing
			ss << 0.001f * blm.timeline.time << "\t" << 1000.0f / blm.timeline.time * blm.total_damage << "\n";
		}

		Logger::log(ss.str().c_str());
		Logger::close();
	}

	void TrainingDummy::study(int mode)
	{
		Logger::open("study");

		std::cout.precision(4);

		BlackMage& blm = (BlackMage&)job;
		blm.reset();

		model.load("Weights\\weights");

		rotation.eps = 0.0f;

		std::unordered_map<std::string, int> lines_map;
		int total_rotations = 0;
		const int TOTAL_ROTATIONS = 1000000;

		std::cout << "Running until " << TOTAL_ROTATIONS << " total rotations\n=============" << std::endl;

		while (total_rotations < TOTAL_ROTATIONS)
		{
			while (blm.timeline.time < 7 * 24 * 3600000)
				rotation.step();

			std::vector<int> points;

			int length = blm.history.size() - 1;
			for (int i = 0; i < length - 1; i++)
			{
				Transition& t = blm.history[i];
				switch (mode)
				{
				case 0:
					if (t.t0[2] == 1.0f && t.t1[1] == 1.0f)
						points.push_back(i);
					break;
				case 1: // 1 for UI; 2 for AF
				case 2:
					if (t.t0[mode] != 1.0f && t.t1[mode] == 1.0f)
					{
						points.push_back(i);
						for (i = i + 1; i < length - 1; i++)
						{
							if (blm.history[i].t1[mode] == 1.0f)
								continue;
							points.push_back(i);
							break;
						}
					}
					break;
				}
			}
			int stride = (mode == 0) ? 1 : 2;
			length = points.size() - 1;
			for (int i = 0; i < length; i += stride)
			{
				std::stringstream ss;
				for (int j = points[i]; j <= points[i + 1]; j++)
				{
					Transition& t = blm.history[j];
					if (t.action == BlackMage::F1 && t.t0[24] == 1.0f)
						ss << "F1^ ";
					else if (t.action == BlackMage::F3 && t.t0[24] == 1.0f)
						ss << "F3p ";
					else if (t.action == BlackMage::T3)
						ss << "T3/p ";
					else if (t.action != 0 && t.action != BlackMage::SWIFT && t.action != BlackMage::TRIPLE && t.action != BlackMage::SHARP && t.action != BlackMage::LEYLINES && t.action < BlackMage::AMPLIFIER)
					{
						// Not NONE, SWIFT, TRIPLE, SHARP, LEYLINES, AMPLIFIER, LUCID, WAIT_FOR_MP, TINCTURE, or F3P_OFF
						if ((t.action != BlackMage::XENO && t.action != BlackMage::MANAFONT && t.action != BlackMage::TRANSPOSE && (t.t0[16] > 0.0f || t.t0[20] > 0.0f)) || (t.action == BlackMage::PARADOX && t.t0[1] == 1.0f))
							ss << blm.get_action_name(t.action) << "* ";
						else
							ss << blm.get_action_name(t.action) << " ";
					}
				}
				lines_map[ss.str()]++;
			}
			blm.reset();

			total_rotations += length / stride;
			std::cout << "Total rotations: " << total_rotations << std::endl;
		}

		std::vector<std::pair<std::string, int>> lines(lines_map.begin(), lines_map.end());
		std::sort(lines.begin(), lines.end(), [](std::pair<std::string, int>& a, std::pair<std::string, int>& b) { return a.second > b.second; });

		int sum = 0;
		int length = 0;
		for (int i = 0; i < lines.size(); i++)
		{
			sum += lines[i].second;
			length++;
			if ((float)sum / total_rotations > 0.95f)
				break;
		}

		Logger::log("=============\n");

		std::stringstream ss;
		ss << "Total rotation count: " << total_rotations << std::endl;
		ss << "Unique rotation count: " << lines.size() << std::endl;
		ss << "=============" << std::endl;
		ss << length << " unique rotations listed below account for " << 100.0f * sum / total_rotations << "% of rotations logged.\n";

		Logger::log(ss.str().c_str());

		for (int i = 0; i < length; i++)
		{
			std::stringstream ss;
			ss << i + 1 << ") " << 100.0f * lines[i].second / total_rotations << "%: " << lines[i].first << std::endl;
			Logger::log(ss.str().c_str());
		}

		Logger::close();
	}

	void TrainingDummy::mp_offset()
	{
		/*
		Logger::open("mp_offset");

		BlackMage& blm = (BlackMage&)job;
		blm.reset();

		model.load("Weights\\weights");

		ModelComputeInput mci = model.getModelComputeInput();

		memcpy(mci.m_x0.data(), job.get_state(), sizeof(float) * job.get_state_size());

		std::vector<float> dps;

		for (int i = 1; i <= 3000; i++)
		{
			mci.m_x0.data()[56] = i / 3000.0f;
			float q = model.compute(mci)[BlackMage::ENOCHIAN];
			q = OUTPUT_LOWER + OUTPUT_RANGE / (1.0f + expf(-q));
			dps.push_back(q);
		}

		std::stringstream ss;
		ss.precision(12);

		for (int i = 0; i < dps.size(); i++)
			ss << i + 1 << "," << dps[i] << "\n";

		Logger::log(ss.str().c_str());

		Logger::close();
		*/
	}
}
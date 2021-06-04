#include "TrainingDummy.h"
#include "BlackMage.h"
#include "Logger.h"
#include <chrono>
#include <iostream>
#include <random>
#include <algorithm>
#include <execution>

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
		std::cout.precision(4);

		long long start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();

		const int NUM_EPOCHS = 1000000;
		const int NUM_STEPS_PER_EPOCH = 10000;
		const int NUM_STEPS_PER_EPISODE = 2500;
		const int NUM_EPISODES = NUM_STEPS_PER_EPOCH / NUM_STEPS_PER_EPISODE;
		const int NUM_BATCHES_PER_EPOCH = 50;
		const int CAPACITY = 1000000;
		const int BATCH_SIZE = 10000;
		const float WINDOW = 600000.0f;
		const float EPS_DECAY = 0.999f;
		const float EPS_START = 1.0f;
		const float EPS_MIN = 0.10f;
		const float OUTPUT_LOWER = 24.400f;
		const float OUTPUT_UPPER = 25.070f;
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

		//Transition* memory = new Transition[CAPACITY];

		int m_index = 0;

		auto indices_gen = [&]()
		{
			return range(rng);
		};

		// Initialize model
		int state_size = job.get_state_size();
		int num_actions = job.get_num_actions();
		model.init(state_size, num_actions, BATCH_SIZE, false);
		model.load("Weights\\weights");




		float* state_memory = new float[state_size * NUM_STEPS_PER_EPOCH];
		bool* action_memory = new bool[num_actions * NUM_STEPS_PER_EPOCH];
		float* reward_memory = new float[2 * NUM_STEPS_PER_EPOCH];
		int* move_memory = new int[NUM_STEPS_PER_EPOCH];







		BlackMage& blm = (BlackMage&)job;

		float nu = 0.001f;
		float eps = EPS_START;
		//float eps = EPS_MIN;
		float exp = 0.0f;
		float steps_per_episode = NUM_STEPS_PER_EPISODE;
		float avg_dps = 0.0f;
		float est_dps = 0.0f;
		float beta = 0.9f;
		int epoch_offset = 0;

		long long generate_time = 0;
		long long copy_q1_time = 0;
		long long compute_q1_time = 0;
		long long calc_reward_time = 0;
		long long copy_q0_time = 0;
		long long compute_q0_time = 0;
		long long calc_target_time = 0;
		long long train_time = 0;
		long long total_count = 0;

		long long copy_time = 0;
		long long copy_count = 0;

		long long time_a;
		long long time_b;
		
		std::vector<BlackMage> jobs(4, blm);
		std::vector<ModelRotation> rotations =
		{
			{ jobs[0], model, 0 },
			{ jobs[1], model, 1 },
			{ jobs[2], model, 2 },
			{ jobs[3], model, 3 }
		};
		std::vector<int> ints = { 0, 1, 2, 3 };

		auto rotato = [&](int idx)
		{
			ModelRotation& r = rotations[idx];
			r.reset(eps, exp);
			r.job.reset();
			for (int step = 0; step < NUM_STEPS_PER_EPISODE; step++)
				r.step();
			//int c = (m_index * 4 + idx) * NUM_STEPS_PER_EPISODE;
			int c = idx * NUM_STEPS_PER_EPISODE;
			std::vector<Transition>& history = r.job.history;
			for (int i = 0; i < NUM_STEPS_PER_EPISODE; i++) {
				Transition& t = r.job.history[i];
				memcpy(&state_memory[(c + i) * 57], t.t0, 57 * sizeof(float));
				memset(&action_memory[(c + i) * 20], 0, sizeof(bool) * 20);
				for (int a : t.actions)
					action_memory[(c + i) * 20 + a] = true;
				reward_memory[(c + i) * 2] = (t.reward - t.dt * OUTPUT_LOWER) / OUTPUT_RANGE / WINDOW;
				reward_memory[(c + i) * 2 + 1] = 1.0f - t.dt / WINDOW;
				move_memory[c + i] = t.action;
				//memory[c + i] = std::move(r.job.history[i]);
			}
		};

		for (m_index = 0; m_index < 99; m_index++) {
			std::for_each(std::execution::par_unseq, ints.begin(), ints.end(), rotato);
			// copy memory over into large data reservoir hmm
			model.copyMemory(m_index * NUM_STEPS_PER_EPOCH, state_memory, action_memory, reward_memory, move_memory);
		}

		for (int epoch = 0; epoch < NUM_EPOCHS; epoch++)
		{
			time_a = std::chrono::high_resolution_clock::now().time_since_epoch().count();

			std::for_each(std::execution::par_unseq, ints.begin(), ints.end(), rotato);
			model.copyMemory(m_index * NUM_STEPS_PER_EPOCH, state_memory, action_memory, reward_memory, move_memory);

			if (++m_index == 100)
				m_index = 0;

			/*
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
			*/

			time_b = std::chrono::high_resolution_clock::now().time_since_epoch().count();

			generate_time += time_b - time_a;
			total_count++;

			time_a = std::chrono::high_resolution_clock::now().time_since_epoch().count();

			for (int batch = 0; batch < 100; batch++)
				model.batch_train(nu, batch * NUM_STEPS_PER_EPOCH);

			// batch train a bunch
			/*
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
			*/

			model.copyToHost();

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
				test();

				float dps = job.total_damage / job.timeline.time;
				//avg_dps = 0.9f * avg_dps + 0.1f * (0.1f * job.total_damage / job.timeline.time);
				//est_dps = avg_dps / (1.0f - beta);
				//beta *= 0.9f;

				std::stringstream ss;
				//ss << "epoch: " << _epoch << ", eps: " << eps << ", window: " << WINDOW << ", steps: " << steps_per_episode << ", avg dps: " << est_dps << ", " << "dps: " << dps << ", xenos: " << blm.xeno_count << ", f1s: " << blm.f1_count << ", f4s: " << blm.f4_count << ", b4s: " << blm.b4_count << ", t3s: " << blm.t3_count << ", transposes: " << blm.transpose_count << ", despairs: " << blm.despair_count << ", lucids: " << blm.lucid_count << ", pots: " << blm.pot_count << std::endl;
				ss << "epoch: " << epoch << ", eps: " << eps << ", window: " << WINDOW << ", steps: " << steps_per_episode << ", " << "dps: " << dps << ", xenos: " << blm.xeno_count << ", f1s: " << blm.f1_count << ", f4s: " << blm.f4_count << ", b4s: " << blm.b4_count << ", t3s: " << blm.t3_count << ", transposes: " << blm.transpose_count << ", despairs: " << blm.despair_count << ", lucids: " << blm.lucid_count << ", pots: " << blm.pot_count << std::endl;
				ss << "10000 rotation steps ms: " << generate_time / 1000000.0 / total_count << ", 50 batches ms: " << copy_time / 1000000.0 / copy_count << std::endl;
				//ss << "generate: " << generate_time / 1000000.0 / total_count << ", copy_q1: " << copy_q1_time / 1000000.0 / total_count << ", compute_q1: " << compute_q1_time / 1000000.0 / total_count << ", calc_reward: " << calc_reward_time / 1000000.0 / total_count << ", copy_q0: " << copy_q0_time / 1000000.0 / total_count << ", compute_q0: " << compute_q0_time / 1000000.0 / total_count << ", calc_target: " << calc_target_time / 1000000.0 / total_count << ", train: " << train_time / 1000000.0 / total_count << ", copy: " << copy_time / 1000000.0 / copy_count << std::endl;

				generate_time = 0; copy_q1_time = 0; compute_q1_time = 0; calc_reward_time = 0; copy_q0_time = 0; compute_q0_time = 0; calc_target_time = 0; train_time = 0; total_count = 0; copy_time = 0; copy_count = 0;

				Logger::log(ss.str().c_str());
				std::cout << ss.str();

				if (epoch % 500 == 0)
				{
					std::stringstream filename;
					filename << "Weights\\weights-" << epoch << std::flush;
					model.save(filename.str().c_str());
				}
			}
		}

		//delete[] memory;

		delete[] state_memory;
		delete[] action_memory;
		delete[] reward_memory;
		delete[] move_memory;

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
		Logger::open("trace");

		std::cout.precision(4);

		BlackMage& blm = (BlackMage&)job;
		blm.reset();

		Logger::log("=============\n");

		model.init(blm.get_state_size(), blm.get_num_actions(), 1, false);
		model.load("Weights\\weights");

		rotation.eps = 0.0f;

		while (blm.timeline.time < 7 * 24 * 3600000)
			rotation.step();

		std::stringstream ss;
		ss << "DPS: " << 1000.0f / blm.timeline.time * blm.total_damage << "\n";
		ss << "T3 uptime: " << 100.0f / blm.timeline.time * blm.total_dot_time << "%\n";
		ss << "F4 % damage: " << 100.0f / blm.total_damage * blm.total_f4_damage << "%\n";
		ss << "Desp % damage: " << 100.0f / blm.total_damage * blm.total_desp_damage << "%\n";
		ss << "Xeno % damage: " << 100.0f / blm.total_damage * blm.total_xeno_damage << "%\n";
		ss << "T3 % damage: " << 100.0f / blm.total_damage * blm.total_t3_damage << "%\n";
		ss << "Dot % damage: " << 100.0f / blm.total_damage * blm.total_dot_damage << "%\n=============" << std::endl;
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
			}
			if (t.action == 5 && t.t0[21] == 1.0f)
				ss << t.t0[0] * 10000.0f << " F3p";
			else if (t.action == 7)
			{
				if (t.t0[23] == 1.0f)
					ss << t.t0[0] * 10000.0f << " T3p at " << lround(t.t0[26] * 2400.0f) / 100.0f << "s left on dot";
				else
					ss << t.t0[0] * 10000.0f << " T3 at " << lround(t.t0[26] * 2400.0f) / 100.0f << "s left on dot";
			}
			else if (t.action == 8)
			{
				if (t.t0[11] == 1.0f)
					ss << t.t0[0] * 10000.0f << " XENO**";
				else
					ss << t.t0[0] * 10000.0f << " XENO*";
			}
			else if (t.action != 0)
				ss << t.t0[0] * 10000.0f << " " << blm.get_action_name(t.action);
			if (t.action != 0)
			{
				if (t.t0[23] == 1.0f)
					ss << " (T3p w/ " << lround(t.t0[24] * 1800.0f) / 100.0f << "s)";
				ss << std::endl;
			}
			Logger::log(ss.str().c_str());
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
		blm.metrics_enabled = true;

		model.init(blm.get_state_size(), blm.get_num_actions(), 1, false);
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

		model.init(blm.get_state_size(), blm.get_num_actions(), 1, false);
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

	void TrainingDummy::study()
	{
		Logger::open("study");

		std::cout.precision(4);

		BlackMage& blm = (BlackMage&)job;
		blm.reset();

		model.init(blm.get_state_size(), blm.get_num_actions(), 1, false);
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
				if (t.t0[2] == 1.0f && t.t1[1] == 1.0f)
					points.push_back(i);
			}

			length = points.size() - 1;
			for (int i = 0; i < length; i++)
			{
				std::stringstream ss;
				for (int j = points[i]; j <= points[i + 1]; j++)
				{
					Transition& t = blm.history[j];
					if (t.action == 5 && t.t0[21] == 1.0f)
						ss << "F3p ";
					else if (t.action == 7)
					{
						ss << "T3/p ";
						//if (t.t0[23] == 1.0f)
						//	ss << "T3p ";
						//else
						//	ss << "T3 ";
					}
					else if (t.action != 0 && t.action != 10 && t.action != 11 && t.action != 12 && t.action != 13 && t.action < 17)
					{
						// Not NONE, SWIFT, TRIPLE, SHARP, LEYLINES, LUCID, WAIT_FOR_MP, or TINCTURE
						if (t.action != 8 && t.action != 14 && t.action != 16 && (t.t0[13] > 0.0f || t.t0[17] > 0.0f))
							ss << blm.get_action_name(t.action) << "* ";
						else
							ss << blm.get_action_name(t.action) << " ";
					}
				}
				lines_map[ss.str()]++;
			}
			blm.reset();

			total_rotations += length;
			std::cout << "Total rotations: " << total_rotations << std::endl;
		}

		std::vector<std::pair<std::string, int>> lines(lines_map.begin(), lines_map.end());
		std::sort(lines.begin(), lines.end(), [](std::pair<std::string, int>& a, std::pair<std::string, int>& b) { return a.second > b.second; });

		//int length;// = lines.size();
		//for (length = 0; length < lines.size(); length++)
		//	if (100.0f * lines[length].second / total_rotations < 0.01f)
		//		break;
		//if (length > 200)
		//	length = 200;

		int sum = 0;
		int length = 0;
		//for (int i = 0; i < length; i++)
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
		//for (int i = 0; i < lines.size(); i++)
		{
			std::stringstream ss;
			//float percent = 100.0f * lines[i].second / total_rotations;
			//if (percent < 0.01f)
			//	break;
			ss << i + 1 << ") " << 100.0f * lines[i].second / total_rotations << "%: " << lines[i].first << std::endl;
			//ss << i + 1 << ") " << percent << "%: " << lines[i].first << std::endl;
			Logger::log(ss.str().c_str());
		}

		for (int i = lines.size() - 100; i < lines.size(); i++)
		{
			std::stringstream ss;
			ss << i + 1 << ") " << 100.0f * lines[i].second / total_rotations << "%: " << lines[i].first << std::endl;
			Logger::log(ss.str().c_str());
		}

		Logger::close();
	}
}
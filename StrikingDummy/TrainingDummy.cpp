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
		std::cout.precision(4);

		long long start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();

		const int NUM_EPOCHS = 1000000;
		const int NUM_STEPS_PER_EPOCH = 10000;
		const int NUM_STEPS_PER_EPISODE = 2500;
		const int NUM_BATCHES_PER_EPOCH = 50;
		const int CAPACITY = 1000000;
		const int BATCH_SIZE = 10000;
		const float WINDOW = 600000.0f;
		const float EPS_DECAY = 0.999f;
		const float EPS_START = 1.0f;
		const float EPS_MIN = 0.10f;
		const float OUTPUT_LOWER = 20.100f;
		const float OUTPUT_UPPER = 20.650f;
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

		BlackMage& blm = (BlackMage&)job;

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
					//ss << "epoch: " << _epoch << ", eps: " << eps << ", window: " << WINDOW << ", steps: " << steps_per_episode << ", avg dps: " << est_dps << ", " << "dps: " << dps << ", xenos: " << blm.xeno_count << ", f1s: " << blm.f1_count << ", f4s: " << blm.f4_count << ", b4s: " << blm.b4_count << ", t3s: " << blm.t3_count << ", transposes: " << blm.transpose_count << ", despairs: " << blm.despair_count << ", lucids: " << blm.lucid_count << ", pots: " << blm.pot_count << std::endl;
					ss << "epoch: " << _epoch << ", eps: " << eps << ", window: " << WINDOW << ", steps: " << steps_per_episode << ", " << "dps: " << dps << ", xenos: " << blm.xeno_count << ", f1s: " << blm.f1_count << ", f4s: " << blm.f4_count << ", b4s: " << blm.b4_count << ", t3s: " << blm.t3_count << ", transposes: " << blm.transpose_count << ", despairs: " << blm.despair_count << ", lucids: " << blm.lucid_count << ", pots: " << blm.pot_count << std::endl;
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
		Logger::open();

		std::cout.precision(2);

		BlackMage& blm = (BlackMage&)job;
		blm.reset();
		blm.metrics_enabled = true;

		model.init(blm.get_state_size(), blm.get_num_actions(), 1, false);
		model.load("Weights\\weights");

		rotation.eps = 0.0f;

		while (blm.timeline.time < 24 * 3600000)
			rotation.step();

		std::vector<int>* dists[] = { &blm.t3_dist, &blm.t3p_dist, &blm.swift_dist, &blm.triple_dist, &blm.sharp_dist, &blm.ll_dist, &blm.mf_dist };
		std::stringstream ss;
		for (int i = 0; i < 7; i++)
		{
			for (int t : *dists[i])
				ss << 0.001 * t << ",";
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

		int time = seconds * 1000;

		std::stringstream ss;

		for (int i = 0; i < times; i++)
		{
			blm.reset();
			while (blm.timeline.time < time)
				rotation.step();
			ss << 1000.0f / blm.timeline.time * blm.total_damage << "\n";
		}

		Logger::log(ss.str().c_str());
		Logger::close();
	}

	void TrainingDummy::study()
	{
		Logger::open();

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
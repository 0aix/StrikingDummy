#include "TrainingDummy.h"
#include "BlackMage.h"
#include "Mimu.h"
#include "Samurai.h"
#include "Machinist.h"
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
		const int NUM_STEPS_PER_EPISODE_MAX = 5000;
		const int NUM_STEPS_PER_EPISODE = NUM_STEPS_PER_EPISODE_MAX;
		const int NUM_BATCHES_PER_EPOCH = 50;
		const int CAPACITY = 1000000;
		const int BATCH_SIZE = 20000;
		const float WINDOW_MAX = 120000.0f;
		const float WINDOW = WINDOW_MAX;
		const float WINDOW_GROWTH = 0.1f;
		const float EPS_DECAY = 0.9995f;
		const float EPS_MIN = 0.01f;
		const float STEPS_GROWTH = 0.1f;
		const float STEPS_MAX = NUM_STEPS_PER_EPISODE_MAX;
		const float ADJUST = 0.00005f;
		const float ADJUST_MIN = 4.0f;

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
		Mimu& mimu = (Mimu&)job;
		Samurai& sam = (Samurai&)job;

		//float nu = 0.001f;
		float nu = 0.0001f;
		float eps = EPS_MIN;
		float exp = 0.0f;
		float window = WINDOW;
		float steps_per_episode = NUM_STEPS_PER_EPISODE;
		float avg_dps = 0.0f;
		float est_dps = 0.0f;
		float beta = 0.9f;
		int epoch_offset = 0;
		float adjust = 4.0f;
		float max_dps = 0.0f;
		bool adjusted = true;
		int max_i = 0;

		// WEIGHT DISTRIBUTION LEARNING

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
				float L = 87.0f - 0.5f * adjust;

				// batch train a bunch
				for (int batch = 0; batch < NUM_BATCHES_PER_EPOCH; batch++)
				{
					std::generate(indices.begin(), indices.end(), indices_gen);

					// compute Q1
					for (int i = 0; i < BATCH_SIZE; i++)
					{
						//memcpy(model.X0.col(i).data(), &memory[indices[i]].t1, sizeof(State));
						memcpy(&model.X0[i * state_size], &memory[indices[i]].t1, sizeof(float) * state_size);
					}

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
						if (adjusted)
						{
							max_q = L + adjust * max_q;
							rewards[i] = (1.0f / adjust) * ((1.0f / window) * (t.reward + (window - t.dt) * max_q) - L);
						}
						else
						{
							max_q = 100.0f * max_q;
							rewards[i] = 0.01f * ((1.0f / window) * (t.reward + (window - t.dt) * max_q));
						}
						//max_q = L + adjust * max_q;
						//rewards[i] = (1.0f / adjust) * ((1.0f / window) * (t.reward + (window - t.dt) * max_q) - L);

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
				model.copyToHost();

				// adjust parameters
				eps *= EPS_DECAY;
				if (eps < EPS_MIN)
					eps = EPS_MIN;
				window += WINDOW_GROWTH;
				if (window > WINDOW_MAX)
					window = WINDOW_MAX;
				steps_per_episode += STEPS_GROWTH;
				if (steps_per_episode > STEPS_MAX)
					steps_per_episode = STEPS_MAX;
				adjust -= ADJUST;
				if (adjust < ADJUST_MIN)
					adjust = ADJUST_MIN;

				// test model
				test();

				float dps = 0.1f * job.total_damage / job.timeline.time;

				avg_dps = 0.9f * avg_dps + 0.1f * dps;
				est_dps = avg_dps / (1.0f - beta);

				int _epoch = epoch - epoch_offset;

				std::stringstream ss;
				ss << "epoch: " << _epoch << ", eps: " << eps << ", window: " << window << ", steps: " << steps_per_episode << ", avg dps: " << est_dps << ", " << "dps: " << dps << ", fouls: " << blm.foul_count << ", f4s: " << blm.f4_count << ", b4s: " << blm.b4_count << ", t3s: " << blm.t3_count << ", transposes: " << blm.transpose_count << ", flares: " << blm.flare_count << std::endl;
				//ss << "epoch: " << _epoch << ", eps: " << eps << ", window: " << window << ", steps: " << steps_per_episode << ", avg dps: " << est_dps << ", " << "dps: " << dps << ", tks: " << mimu.tk_count << std::endl;
				//ss << "epoch: " << _epoch << ", eps: " << eps << ", window: " << window << ", steps: " << steps_per_episode << ", avg dps: " << est_dps << ", " << "dps: " << dps << ", midares: " << sam.midare_count << ", kaitens: " << sam.kaiten_count << ", shintens: " << sam.shinten_count << ", gurens: " << sam.guren_count << std::endl;
				//ss << "epoch: " << _epoch << ", eps: " << eps << ", window: " << window << ", steps: " << steps_per_episode << ", avg dps: " << est_dps << ", " << "dps: " << dps << std::endl;

				beta *= 0.9f;

				Logger::log(ss.str().c_str());
				std::cout << ss.str();

				if (_epoch % 1000 == 0)
				{
					std::stringstream filename;
					filename << "Weights\\weights-" << _epoch << std::flush;
					model.save(filename.str().c_str());
				}
				if (dps > max_dps)
				{
					max_dps = dps;
					max_i++;
					std::stringstream filename;
					filename << "Weights\\max-" << max_i << std::flush;
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
		while (job.timeline.time < 60000)
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

		//while (blm.timeline.time < 7 * 24 * 360000)
		while (blm.timeline.time < 180000)
			rotation.step();

		std::stringstream zz;
		zz << "DPS: " << 100.0f / blm.timeline.time * blm.total_damage << "\n=============" << std::endl;
		Logger::log(zz.str().c_str());
		
		int length = blm.history.size() - 1;
		//if (length > 1000)
		//	length = 1000;
		int time = 0;
		for (int i = 0; i < length; i++)
		{
			Transition& t = blm.history[i];
			int hours = time / 360000;
			int minutes = (time / 6000) % 60;
			int seconds = (time / 100) % 60;
			int centiseconds = time % 100;
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
			if (t.action == 5 && t.t0[20] == 1.0f)
				ss << t.t0[0] * 15480.0f << " F3p";
			else if (t.action == 7)
			{
				if (t.t0[22] == 1.0f)
					ss << t.t0[0] * 15480.0f << " T3p at " << t.t0[25] * 24 << "s left on dot";
				else
					ss << t.t0[0] * 15480.0f << " T3 at " << t.t0[25] * 24 << "s left on dot";
			}
			else if (t.action != 0)
				ss << t.t0[0] * 15480.0f << " " << blm.get_action_name(t.action);
			if (t.action != 0)
			{
				if (t.t0[22] == 1.0f)
					ss << " (T3p w/ " << t.t0[23] * 18 << "s)";
				ss << std::endl;
			}
			Logger::log(ss.str().c_str());
			time += t.dt;
		}
		Logger::close();
	}

	std::string mimu_actions[] =
	{
		"NONE",
		"BOOTSHINE", "TRUESTRIKE", "SNAPPUNCH", "DRAGONKICK", "TWINSNAKES", "DEMOLISH",
		"FISTSOFWIND", "FISTSOFFIRE",
		"INTERNALRELEASE", "PERFECTBALANCE", "BROTHERHOOD", "STEELPEAK", "HOWLINGFIST", "FORBIDDENCHAKRA", "ELIXIRFIELD", "TORNADOKICK",
		"RIDDLEOFWIND", "RIDDLEOFFIRE", "WINDTACKLE", "FIRETACKLE", "WAIT"
	};

	void TrainingDummy::trace_mimu()
	{
		Logger::open();

		std::cout.precision(4);

		Mimu& mimu = (Mimu&)job;
		mimu.reset();

		Logger::log("=============\n");

		model.init(mimu.get_state_size(), mimu.get_num_actions(), 1, false);
		model.load("Weights\\weights");

		rotation.eps = 0.0f;

		while (mimu.timeline.time < 7 * 24 * 360000)
			rotation.step();

		int length = mimu.history.size() - 1;
		for (int i = 0; i < length; i++)
		{
			Transition& t = mimu.history[i];
			if (t.action != 0)
			{
				std::stringstream ss;
				ss << mimu_actions[t.action] << std::endl;
				Logger::log(ss.str().c_str());
			}
		}

		std::stringstream ss;
		ss << "DPS: " << 100.0f / mimu.timeline.time * mimu.total_damage << "\n=============" << std::endl;
		Logger::log(ss.str().c_str());

		Logger::close();
	}

	std::string sam_actions[] =
	{
		"NONE",
		"HAKAZE", "JINPU", "SHIFU", "GEKKO", "KASHA", "YUKIKAZE",
		"HIGANBANA", "TENKA", "MIDARE", "MEIKYO", "KAITEN", "SHINTEN", "GUREN", "HAGAKURE"
	};

	void TrainingDummy::trace_sam()
	{
		Logger::open();

		std::cout.precision(4);

		Samurai& sam = (Samurai&)job;
		sam.reset();

		Logger::log("=============\n");

		model.init(sam.get_state_size(), sam.get_num_actions(), 1, false);
		model.load("Weights\\weights");

		rotation.eps = 0.0f;

		while (sam.timeline.time < 7 * 24 * 360000)
			rotation.step();

		std::stringstream zz;
		zz << "DPS: " << 100.0f / sam.timeline.time * sam.total_damage << "\n=============" << std::endl;
		Logger::log(zz.str().c_str());

		int length = sam.history.size() - 1;
		if (length > 1000)
			length = 1000;
		int time = 0;
		for (int i = 0; i < length; i++)
		{
			int hours = time / 360000;
			int minutes = (time / 6000) % 60;
			int seconds = (time / 100) % 60;
			int centiseconds = time % 100;

			Transition& t = sam.history[i];
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
				ss << sam_actions[t.action] << " " << t.t0[0] * 100.0f << std::endl;
				Logger::log(ss.str().c_str());
			}
			time += t.dt;
		}

		Logger::close();
	}

	void TrainingDummy::trace_mch()
	{
		Logger::open();

		std::cout.precision(4);

		Machinist& mch = (Machinist&)job;
		mch.reset();

		Logger::log("=============\n");

		model.init(mch.get_state_size(), mch.get_num_actions(), 1, false);
		model.load("Weights\\weights");

		rotation.eps = 0.0f;

		while (mch.timeline.time < 7 * 24 * 360000)
		//while (mch.timeline.time < 60000)
			rotation.step();

		std::stringstream zz;
		zz << "DPS: " << 100.0f / mch.timeline.time * mch.total_damage << "\n=============" << std::endl;
		Logger::log(zz.str().c_str());

		int length = mch.history.size() - 1;
		if (length > 1000)
			length = 1000;
		int time = 0;
		for (int i = 0; i < length; i++)
		{
			int hours = time / 360000;
			int minutes = (time / 6000) % 60;
			int seconds = (time / 100) % 60;
			int centiseconds = time % 100;

			Transition& t = mch.history[i];
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
				ss << mch.get_action_name(t.action) << " " << t.t0[0] * 100.0f << std::endl;
				Logger::log(ss.str().c_str());
			}
			time += t.dt;
		}

		Logger::close();
	}
}
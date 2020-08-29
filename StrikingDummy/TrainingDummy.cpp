#include "TrainingDummy.h"
#include "Cube.h"
#include "Solver.h"
#include "Logger.h"
#include <chrono>
#include <iostream>
#include <random>

namespace StrikingDummy
{
	TrainingDummy::TrainingDummy(Job& job) : job(job), rotation(job, model), solver(model)
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
		const int NUM_STEPS_PER_EPISODE = 20;
		const int NUM_BATCHES_PER_EPOCH = 50;
		const int CAPACITY = 1000000;
		const int BATCH_SIZE = 10000;
		const float EPS_DECAY = 0.999f;
		const float EPS_START = 1.0f;
		const float EPS_MIN = 0.10f;
		const float OUTPUT_LOWER = 0.0f;
		const float OUTPUT_UPPER = 30.0f;
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

		Cube& cube = (Cube&)job;

		float nu = 0.001f;
		float eps = EPS_START;
		float exp = 0.0f;
		int epoch_offset = 0;

		for (int epoch = 0; epoch < NUM_EPOCHS; epoch++)
		{
			rotation.reset(eps, exp);

			int num_episodes = NUM_STEPS_PER_EPOCH / NUM_STEPS_PER_EPISODE;
			for (int episode = 0; episode < num_episodes; episode++)
			{
				job.reset();
				for (int step = 0; step < (int)NUM_STEPS_PER_EPISODE; step++)
					rotation.step();
				for (int i = 0; i < (int)NUM_STEPS_PER_EPISODE; i++)
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
						if (t.c0 || t.c1)
							rewards[i] = 0.0f;
						//if (t.c0)
						//	rewards[i] = 0.0f;
						//else if (t.c1)
						//	rewards[i] = 1.0f / OUTPUT_RANGE;
						else
						{
							float* q = &Q1[i * num_actions];
							float min_q = q[0];
							for (int j = 1; j < 12; j++)
								if (q[j] < min_q)
									min_q = q[j];
							rewards[i] = min_q + 1.0f / OUTPUT_RANGE;
						}
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
					//test();
					job.reset();
					for (int i = 0; i < 12; i++)
						rotation.step();
					int moves = solver.solve(cube.data, cube.encoded);

					std::stringstream ss;
					ss << "epoch: " << _epoch << ", # of moves: " << moves << std::endl;
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

	int TrainingDummy::test()
	{
		// basically attempt to solve in 30 moves
		Cube& cube = (Cube&)job;
		job.reset();
		for (int i = 0; i < 8; i++)
			rotation.step();
		return solver.solve(cube.data, cube.encoded);
	}

	void TrainingDummy::trace()
	{
		Logger::open();

		std::cout.precision(4);

		Cube& cube = (Cube&)job;
		cube.reset();

		Logger::log("=============\n");

		model.init(cube.get_state_size(), cube.get_num_actions(), 1, false);
		model.load("Weights\\weights");

		rotation.eps = 0.0f;

		rotation.step();

		Logger::close();
	}
}
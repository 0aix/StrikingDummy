#pragma once
#include <Eigen/Core>
#include <vector>
using namespace Eigen;

namespace StrikingDummy
{
	struct Layer
	{
		int num_units;
	};

	struct ModelParams
	{
		int num_inputs;
		int num_outputs;
		std::vector<Layer> layers;
		int batch_size;
	};

	struct ModelComputeInput
	{
		MatrixXf m_x0;
		MatrixXf m_x1;
		MatrixXf m_x2;
		MatrixXf m_x3;
	};

	struct Model
	{
		float* _state = NULL;
		unsigned char* _action = NULL;
		float* _reward = NULL;
		int* _move = NULL;

		float* x0 = NULL;
		float* x3 = NULL;
		float* X0 = NULL;
		float* X3 = NULL;
		float* target = NULL;

		float* _x0 = NULL;
		float* _x1 = NULL;
		float* _x2 = NULL;
		float* _x3 = NULL;
		float* _X0 = NULL;
		float* _X1 = NULL;
		float* _X2 = NULL;
		float* _X3 = NULL;
		float* _target = NULL;

		float* _W1 = NULL;
		float* _W2 = NULL;
		float* _W3 = NULL;
		float* _b1 = NULL;
		float* _b2 = NULL;
		float* _b3 = NULL;
		float* _d1 = NULL;
		float* _d2 = NULL;
		float* _d3 = NULL;
		float* _dLdW1 = NULL;
		float* _dLdW2 = NULL;
		float* _dLdW3 = NULL;
		float* _dLdb1 = NULL;
		float* _dLdb2 = NULL;
		float* _dLdb3 = NULL;
		float* _ones = NULL;

		MatrixXf m_x0;
		MatrixXf m_x1;
		MatrixXf m_x2;
		MatrixXf m_x3;
		MatrixXf m_W1;
		MatrixXf m_W2;
		MatrixXf m_W3;
		MatrixXf m_b1;
		MatrixXf m_b2;
		MatrixXf m_b3;
		
		int input_size;
		int output_size;
		int batch_size = 0;
		int total_size;

		Model(int input_size, int output_size);
		~Model();

		void init(int batch_size);

		ModelComputeInput getModelComputeInput();

		float* compute();
		float* compute(ModelComputeInput& input);
		float* batch_compute();

		void train(float nu);
		void copyToHost();

		void batch_train(float nu, int offset);

		void copyMemory(int offset, float* state_memory, unsigned char* action_memory, float* reward_memory, int* move_memory, int size);

		void load(const char* filename);
		void save(const char* filename);
	};
}
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
		float* _state2 = NULL;
		bool* _action = NULL;
		float* _reward = NULL;
		int* _move = NULL;
		unsigned int* _indices = NULL;
		float* _X4 = NULL;

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

		float* _dLdW1m = NULL;
		float* _dLdW2m = NULL;
		float* _dLdW3m = NULL;
		float* _dLdb1m = NULL;
		float* _dLdb2m = NULL;
		float* _dLdb3m = NULL;
		float* _dLdW1v = NULL;
		float* _dLdW2v = NULL;
		float* _dLdW3v = NULL;
		float* _dLdb1v = NULL;
		float* _dLdb2v = NULL;
		float* _dLdb3v = NULL;
		float* _temp = NULL;

		float* __X0 = NULL;
		float* __X1 = NULL;
		float* __X2 = NULL;
		float* __W2 = NULL;
		float* __W3 = NULL;

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

		static constexpr float BETA1 = 0.85f;
		static constexpr float BETA2 = 0.85f;
		static constexpr float EPSILON = 0.00000001f;
		
		int input_size = 0;
		int output_size = 0;
		int batch_size = 0;
		float beta1 = BETA1;
		float beta2 = BETA2;
		bool adam = false;

		//Model(ModelParams& params);
		~Model();

		void init(int input_size, int output_size, int batch_size, bool adam);

		ModelComputeInput getModelComputeInput();

		float* compute();
		float* compute(ModelComputeInput& input);
		float* batch_compute();

		void train(float nu);
		void copyToHost();

		void batch_train(float nu, int offset);

		void copyMemory(int offset, float* state_memory, float* state2_memory, bool* action_memory, float* reward_memory, int* move_memory);

		void load(const char* filename);
		void save(const char* filename);
	};
}
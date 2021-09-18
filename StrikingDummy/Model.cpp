#include "Model.h"
#include "CUDA.cuh"
#include <fstream>
#include <iostream>

namespace StrikingDummy
{
	const int INNER_1 = 128;
	const int INNER_2 = 128;

	Model::Model(int input_size, int output_size) : input_size(input_size), output_size(output_size)
	{
		m_W1 = MatrixXf(INNER_1, input_size);
		m_W2 = MatrixXf(INNER_2, INNER_1);
		m_W3 = MatrixXf(output_size, INNER_2);
		m_b1 = MatrixXf::Zero(INNER_1, 1);
		m_b2 = MatrixXf::Zero(INNER_2, 1);
		m_b3 = MatrixXf::Zero(output_size, 1);
	}

	Model::~Model()
	{
		cudaSafeFree((void**)&_state);
		cudaSafeFree((void**)&_action);
		cudaSafeFree((void**)&_reward);
		cudaSafeFree((void**)&_move);

		matrixFree(&_X0);
		matrixFree(&_X1);
		matrixFree(&_X2);
		matrixFree(&_X3);
		matrixFree(&_W1);
		matrixFree(&_d1);
		matrixFree(&_d2);
		matrixFree(&_d3);
		matrixFree(&_dLdW1);
		matrixFree(&_ones);
		matrixFree(&_dLdW1mv);
	}

	void Model::init(int batch_size, int capacity)
	{
		cudaInitialize();

		this->batch_size = batch_size;
		this->total_size = INNER_1 * input_size + INNER_1 + INNER_2 * INNER_1 + INNER_2 + output_size * INNER_2 + output_size;

		cudaSafeMalloc((void**)&_state, input_size * 4 * capacity);
		cudaSafeMalloc((void**)&_action, output_size * capacity);
		cudaSafeMalloc((void**)&_reward, 2 * 4 * capacity);
		cudaSafeMalloc((void**)&_move, 4 * capacity);

		matrixInitialize(&_X0, input_size, batch_size);
		matrixInitialize(&_X1, INNER_1, batch_size);
		matrixInitialize(&_X2, INNER_2, batch_size);
		matrixInitialize(&_X3, output_size, batch_size);

		cudaSafeMalloc((void**)&_W1, sizeof(float) * total_size);

		_b1 = _W1 + INNER_1 * input_size;
		_W2 = _b1 + INNER_1;
		_b2 = _W2 + INNER_2 * INNER_1;
		_W3 = _b2 + INNER_2;
		_b3 = _W3 + output_size * INNER_2;

		matrixInitialize(_W1, INNER_1, input_size, sqrtf(96.0f / (INNER_1 + input_size)));
		matrixInitialize(_W2, INNER_2, INNER_1, sqrtf(96.0f / (INNER_2 + INNER_1)));
		matrixInitialize(_W3, output_size, INNER_2, sqrtf(96.0f / (output_size + INNER_2)));
		matrixInitialize(_b1, INNER_1, 1, 0.0f);
		matrixInitialize(_b2, INNER_2, 1, 0.0f);
		matrixInitialize(_b3, output_size, 1, 0.0f);

		matrixInitialize(&_d1, INNER_1, batch_size);
		matrixInitialize(&_d2, INNER_2, batch_size);
		matrixInitialize(&_d3, output_size, batch_size, 0.0f);

		cudaSafeMalloc((void**)&_dLdW1, sizeof(float) * total_size);

		_dLdb1 = _dLdW1 + INNER_1 * input_size;
		_dLdW2 = _dLdb1 + INNER_1;
		_dLdb2 = _dLdW2 + INNER_2 * INNER_1;
		_dLdW3 = _dLdb2 + INNER_2;
		_dLdb3 = _dLdW3 + output_size * INNER_2;

		arrayInitialize(&_ones, batch_size, 1.0f);

		arrayInitialize(&_dLdW1mv, 2 * total_size, 0.0f);

		arrayCopyToHost(m_W1.data(), _W1, INNER_1 * input_size);
		arrayCopyToHost(m_W2.data(), _W2, INNER_2 * INNER_1);
		arrayCopyToHost(m_W3.data(), _W3, output_size * INNER_2);

		cudaSafeDeviceSynchronize();
	}

	ModelComputeInput Model::getModelComputeInput() {
		return
		{
			MatrixXf(input_size, 1),
			MatrixXf(INNER_1, 1),
			MatrixXf(INNER_2, 1),
			MatrixXf(output_size, 1)
		};
	}

	float sigmoid(float x)
	{
		return 1.0f / (1.0f + expf(-x));
	}

	float* Model::compute(ModelComputeInput& input)
	{
		input.m_x1 = (m_W1 * input.m_x0 + m_b1).unaryExpr(&sigmoid);
		input.m_x2 = (m_W2 * input.m_x1 + m_b2).unaryExpr(&sigmoid);
		input.m_x3 = (m_W3 * input.m_x2 + m_b3);
		return input.m_x3.data();
	}

	void Model::copyToHost()
	{
		arrayCopyToHost(m_W1.data(), _W1, INNER_1 * input_size);
		arrayCopyToHost(m_W2.data(), _W2, INNER_2 * INNER_1);
		arrayCopyToHost(m_W3.data(), _W3, output_size * INNER_2);
		arrayCopyToHost(m_b1.data(), _b1, INNER_1);
		arrayCopyToHost(m_b2.data(), _b2, INNER_2);
		arrayCopyToHost(m_b3.data(), _b3, output_size);
	}

	void Model::batch_train(float nu, int index)
	{
		int offset = index * batch_size;

		// compute Q0/Q1
		matrixMultiply(_X1, _W1, INNER_1, input_size, _state + input_size * offset, input_size, batch_size);
		arrayAddRepSigmoid(_X1, _X1, _b1, INNER_1, batch_size);

		matrixMultiply(_X2, _W2, INNER_2, INNER_1, _X1, INNER_1, batch_size);
		arrayAddRepSigmoid(_X2, _X2, _b2, INNER_2, batch_size);

		matrixMultiply(_X3, _W3, output_size, INNER_2, _X2, INNER_2, batch_size);
		arrayAddRepSigmoid(_X3, _X3, _b3, output_size, batch_size);

		// create _d3 directly
		potato(_d3, _X3, _action + output_size * offset, _reward + 2 * offset, _move + offset, batch_size);

		//
		matrixMultiplyTranspose(_dLdW3, _d3, output_size, batch_size, _X2, batch_size, INNER_2);
		matrixMultiply(_dLdb3, _d3, output_size, batch_size, _ones, batch_size, 1);

		//
		matrixTransposeMultiply(_d2, _W3, INNER_2, output_size, _d3, output_size, batch_size);
		arrayMultiplyDerivSigmoid(_d2, _d2, _X2, INNER_2 * batch_size);

		// reset _d3
		unpotato(_d3, _move + offset, batch_size);

		//
		matrixMultiplyTranspose(_dLdW2, _d2, INNER_2, batch_size, _X1, batch_size, INNER_1);
		matrixMultiply(_dLdb2, _d2, INNER_2, batch_size, _ones, batch_size, 1);

		//
		matrixTransposeMultiply(_d1, _W2, INNER_1, INNER_2, _d2, INNER_2, batch_size);
		arrayMultiplyDerivSigmoid(_d1, _d1, _X1, INNER_1 * batch_size);

		//
		matrixMultiplyTranspose(_dLdW1, _d1, INNER_1, batch_size, _state + input_size * offset, batch_size, input_size);
		matrixMultiply(_dLdb1, _d1, INNER_1, batch_size, _ones, batch_size, 1);

		//
		adamStep(_dLdW1, _dLdW1mv, BETA1, BETA2, total_size);

		nu *= sqrtf(1.0f - beta2) / (1.0f - beta1);

		beta1 *= BETA1;
		beta2 *= BETA2;

		//
		arrayStep(_W1, _dLdW1, -nu, total_size);
	}

	void Model::copyMemory(int offset, float* state_memory, unsigned char* action_memory, float* reward_memory, int* move_memory, int size)
	{
		cudaMemcpyAsync(_state + input_size * offset, state_memory, input_size * 4 * size, cudaMemcpyHostToDevice);
		cudaMemcpyAsync(_action + output_size * offset, action_memory, output_size * size, cudaMemcpyHostToDevice);
		cudaMemcpyAsync(_reward + 2 * offset, reward_memory, 2 * 4 * size, cudaMemcpyHostToDevice);
		cudaMemcpy(_move + offset, move_memory, 4 * size, cudaMemcpyHostToDevice);
	}

	void Model::copyWeights(Model& other)
	{
		m_W1 = other.m_W1;
		m_W2 = other.m_W2;
		m_W3 = other.m_W3;
		m_b1 = other.m_b1;
		m_b2 = other.m_b2;
		m_b3 = other.m_b3;
	}

	void Model::load(const char* filename)
	{
		std::fstream fs(filename, std::fstream::in | std::fstream::binary);
		if (fs.is_open())
		{
			fs.read((char*)m_W1.data(), INNER_1 * input_size * sizeof(float));
			fs.read((char*)m_W2.data(), INNER_2 * INNER_1 * sizeof(float));
			fs.read((char*)m_W3.data(), output_size * INNER_2 * sizeof(float));
			fs.read((char*)m_b1.data(), INNER_1 * sizeof(float));
			fs.read((char*)m_b2.data(), INNER_2 * sizeof(float));
			fs.read((char*)m_b3.data(), output_size * sizeof(float));
			fs.close();

			if (batch_size)
			{
				arrayCopyToDevice(_W1, m_W1.data(), INNER_1 * input_size);
				arrayCopyToDevice(_W2, m_W2.data(), INNER_2 * INNER_1);
				arrayCopyToDevice(_W3, m_W3.data(), output_size * INNER_2);
				arrayCopyToDevice(_b1, m_b1.data(), INNER_1);
				arrayCopyToDevice(_b2, m_b2.data(), INNER_2);
				arrayCopyToDevice(_b3, m_b3.data(), output_size);
			}
		}
	}

	void Model::save(const char* filename)
	{
		std::fstream fs;
		fs.open(filename, std::fstream::out | std::fstream::binary);
		fs.write((const char*)m_W1.data(), INNER_1 * input_size * sizeof(float));
		fs.write((const char*)m_W2.data(), INNER_2 * INNER_1 * sizeof(float));
		fs.write((const char*)m_W3.data(), output_size * INNER_2 * sizeof(float));
		fs.write((const char*)m_b1.data(), INNER_1 * sizeof(float));
		fs.write((const char*)m_b2.data(), INNER_2 * sizeof(float));
		fs.write((const char*)m_b3.data(), output_size * sizeof(float));
		fs.flush();
		fs.close();
	}
}
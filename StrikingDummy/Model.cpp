#include "Model.h"
#include "CUDA.cuh"
#include <fstream>
#include <iostream>

namespace StrikingDummy
{
	const int INNER_1 = 128;
	const int INNER_2 = 128;

	Model::~Model()
	{
		delete[] x0;
		delete[] x3;
		delete[] X0;
		delete[] X3;
		delete[] target;

		matrixFree(&_x0);
		matrixFree(&_x1);
		matrixFree(&_x2);
		matrixFree(&_x3);
		matrixFree(&_X0);
		matrixFree(&_X1);
		matrixFree(&_X2);
		matrixFree(&_X3);
		matrixFree(&_target);
		matrixFree(&_W1);
		matrixFree(&_W2);
		matrixFree(&_W3);
		matrixFree(&_b1);
		matrixFree(&_b2);
		matrixFree(&_b3);
		matrixFree(&_d1);
		matrixFree(&_d2);
		matrixFree(&_d3);
		matrixFree(&_dLdW1);
		matrixFree(&_dLdW2);
		matrixFree(&_dLdW3);
		matrixFree(&_dLdb1);
		matrixFree(&_dLdb2);
		matrixFree(&_dLdb3);
		matrixFree(&_ones);

		matrixFree(&_dLdW1m);
		matrixFree(&_dLdW2m);
		matrixFree(&_dLdW3m);
		matrixFree(&_dLdb1m);
		matrixFree(&_dLdb2m);
		matrixFree(&_dLdb3m);
		matrixFree(&_dLdW1v);
		matrixFree(&_dLdW2v);
		matrixFree(&_dLdW3v);
		matrixFree(&_dLdb1v);
		matrixFree(&_dLdb2v);
		matrixFree(&_dLdb3v);
		matrixFree(&_temp);

		matrixFree(&__X0);
		matrixFree(&__X1);
		matrixFree(&__X2);
		matrixFree(&__W2);
		matrixFree(&__W3);
	}

	void Model::init(int input_size, int output_size, int batch_size, bool adam)
	{
		cudaInitialize();

		this->input_size = input_size;
		this->output_size = output_size;
		this->batch_size = batch_size;
		this->adam = adam;

		x0 = new float[input_size];
		x3 = new float[output_size];
		X0 = new float[input_size * batch_size];
		X3 = new float[output_size * batch_size];
		target = new float[output_size * batch_size];

		matrixInitialize(&_x0, input_size, 1);
		matrixInitialize(&_x1, INNER_1, 1);
		matrixInitialize(&_x2, INNER_2, 1);
		matrixInitialize(&_x3, output_size, 1);
		matrixInitialize(&_X0, input_size, batch_size);
		matrixInitialize(&_X1, INNER_1, batch_size);
		matrixInitialize(&_X2, INNER_2, batch_size);
		matrixInitialize(&_X3, output_size, batch_size);
		matrixInitialize(&_target, output_size, batch_size);
		matrixInitialize(&_W1, INNER_1, input_size, sqrtf(96.0f / (INNER_1 + input_size)));
		matrixInitialize(&_W2, INNER_2, INNER_1, sqrtf(96.0f / (INNER_2 + INNER_1)));
		matrixInitialize(&_W3, output_size, INNER_2, sqrtf(96.0f / (output_size + INNER_2)));
		matrixInitialize(&_b1, INNER_1, 1, 0.0f);
		matrixInitialize(&_b2, INNER_2, 1, 0.0f);
		matrixInitialize(&_b3, output_size, 1, 0.0f);
		matrixInitialize(&_d1, INNER_1, batch_size);
		matrixInitialize(&_d2, INNER_2, batch_size);
		matrixInitialize(&_d3, output_size, batch_size);
		matrixInitialize(&_dLdW1, INNER_1, input_size);
		matrixInitialize(&_dLdW2, INNER_2, INNER_1);
		matrixInitialize(&_dLdW3, output_size, INNER_2);
		matrixInitialize(&_dLdb1, INNER_1, 1);
		matrixInitialize(&_dLdb2, INNER_2, 1);
		matrixInitialize(&_dLdb3, output_size, 1);
		matrixInitialize(&_ones, batch_size, 1);
		arrayAdd(_ones, _ones, 1.0f, batch_size);

		matrixInitialize(&_dLdW1m, INNER_1, input_size, 0.0f);
		matrixInitialize(&_dLdW2m, INNER_2, INNER_1, 0.0f);
		matrixInitialize(&_dLdW3m, output_size, INNER_2, 0.0f);
		matrixInitialize(&_dLdb1m, INNER_1, 1, 0.0f);
		matrixInitialize(&_dLdb2m, INNER_2, 1, 0.0f);
		matrixInitialize(&_dLdb3m, output_size, 1, 0.0f);
		matrixInitialize(&_dLdW1v, INNER_1, input_size, 0.0f);
		matrixInitialize(&_dLdW2v, INNER_2, INNER_1, 0.0f);
		matrixInitialize(&_dLdW3v, output_size, INNER_2, 0.0f);
		matrixInitialize(&_dLdb1v, INNER_1, 1, 0.0f);
		matrixInitialize(&_dLdb2v, INNER_2, 1, 0.0f);
		matrixInitialize(&_dLdb3v, output_size, 1, 0.0f);
		matrixInitialize(&_temp, INNER_1, INNER_2);

		matrixInitialize(&__X0, batch_size, input_size);
		matrixInitialize(&__X1, batch_size, INNER_1);
		matrixInitialize(&__X2, batch_size, INNER_2);
		matrixInitialize(&__W2, INNER_1, INNER_2);
		matrixInitialize(&__W3, INNER_2, output_size);

		m_x0 = MatrixXf(input_size, 1);
		m_x1 = MatrixXf(INNER_1, 1);
		m_x2 = MatrixXf(INNER_2, 1);
		m_x3 = MatrixXf(output_size, 1);
		m_W1 = MatrixXf(INNER_1, input_size);
		m_W2 = MatrixXf(INNER_2, INNER_1);
		m_W3 = MatrixXf(output_size, INNER_2);
		m_b1 = MatrixXf::Zero(INNER_1, 1);
		m_b2 = MatrixXf::Zero(INNER_2, 1);
		m_b3 = MatrixXf::Zero(output_size, 1);

		arrayCopyToHost(m_W1.data(), _W1, INNER_1 * input_size);
		arrayCopyToHost(m_W2.data(), _W2, INNER_2 * INNER_1);
		arrayCopyToHost(m_W3.data(), _W3, output_size * INNER_2);
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

	float* Model::compute()
	{
		m_x1 = (m_W1 * m_x0 + m_b1).unaryExpr(&sigmoid);
		m_x2 = (m_W2 * m_x1 + m_b2).unaryExpr(&sigmoid);
		m_x3 = (m_W3 * m_x2 + m_b3);
		return m_x3.data();
	}

	float* Model::compute(ModelComputeInput& input)
	{
		input.m_x1 = (m_W1 * input.m_x0 + m_b1).unaryExpr(&sigmoid);
		input.m_x2 = (m_W2 * input.m_x1 + m_b2).unaryExpr(&sigmoid);
		input.m_x3 = (m_W3 * input.m_x2 + m_b3);
		return input.m_x3.data();
	}

	float* Model::batch_compute()
	{
		arrayCopyToDevice(_X0, X0, input_size * batch_size);

		matrixMultiply(_X1, _W1, INNER_1, input_size, _X0, input_size, batch_size);
		arrayAddRep(_X1, _X1, _b1, INNER_1, batch_size);
		arraySigmoid(_X1, _X1, INNER_1 * batch_size);

		matrixMultiply(_X2, _W2, INNER_2, INNER_1, _X1, INNER_1, batch_size);
		arrayAddRep(_X2, _X2, _b2, INNER_2, batch_size);
		arraySigmoid(_X2, _X2, INNER_2 * batch_size);

		matrixMultiply(_X3, _W3, output_size, INNER_2, _X2, INNER_2, batch_size);
		arrayAddRep(_X3, _X3, _b3, output_size, batch_size);
		arraySigmoid(_X3, _X3, output_size * batch_size);

		arrayCopyToHost(X3, _X3, output_size * batch_size);
		return X3;
	}

	void Model::train(float nu)
	{
		arrayCopyToDevice(_target, target, output_size * batch_size);

		// d3 = (Xk - target).cwiseProduct(Xk.unaryExpr(&dsigmoid));
		arraySubtract(_target, _X3, _target, output_size * batch_size);

		// MULTIPLY

		arrayDerivSigmoid(_X3, _X3, output_size * batch_size);

		arrayMultiply(_d3, _target, _X3, output_size * batch_size);

		// 
		//matrixTranspose(__X2, _X2, INNER_2, batch_size);
		//matrixMultiply(_dLdW3, _d3, output_size, batch_size, __X2, batch_size, INNER_2);
		matrixMultiplyTranspose(_dLdW3, _d3, output_size, batch_size, _X2, batch_size, INNER_2);

		matrixMultiply(_dLdb3, _d3, output_size, batch_size, _ones, batch_size, 1);
		
		//
		matrixTranspose(__W3, _W3, output_size, INNER_2);

		matrixMultiply(_d2, __W3, INNER_2, output_size, _d3, output_size, batch_size);

		arrayDerivSigmoid(_X2, _X2, INNER_2 * batch_size);

		arrayMultiply(_d2, _d2, _X2, INNER_2 * batch_size);

		//
		//matrixTranspose(__X1, _X1, INNER_1, batch_size);
		//matrixMultiply(_dLdW2, _d2, INNER_2, batch_size, __X1, batch_size, INNER_1);
		matrixMultiplyTranspose(_dLdW2, _d2, INNER_2, batch_size, _X1, batch_size, INNER_1);

		matrixMultiply(_dLdb2, _d2, INNER_2, batch_size, _ones, batch_size, 1);

		//
		matrixTranspose(__W2, _W2, INNER_2, INNER_1);

		matrixMultiply(_d1, __W2, INNER_1, INNER_2, _d2, INNER_2, batch_size);

		arrayDerivSigmoid(_X1, _X1, INNER_1 * batch_size);

		arrayMultiply(_d1, _d1, _X1, INNER_1 * batch_size);

		//
		//matrixTranspose(__X0, _X0, input_size, batch_size);
		//matrixMultiply(_dLdW1, _d1, INNER_1, batch_size, __X0, batch_size, input_size);
		matrixMultiplyTranspose(_dLdW1, _d1, INNER_1, batch_size, _X0, batch_size, input_size);

		matrixMultiply(_dLdb1, _d1, INNER_1, batch_size, _ones, batch_size, 1);

		//

		// ADAM
		/*
		if (adam)
		{
			// W3
			arrayMultiply(_temp, _dLdW3, 1.0f - BETA1, output_size * INNER_2);
			arrayMultiply(_dLdW3m, _dLdW3m, BETA1, output_size * INNER_2);
			arrayAdd(_dLdW3m, _dLdW3m, _temp, output_size * INNER_2);

			arrayMultiply(_temp, _dLdW3, _dLdW3, output_size * INNER_2);
			arrayMultiply(_temp, _temp, 1.0f - BETA2, output_size * INNER_2);
			arrayMultiply(_dLdW3v, _dLdW3v, BETA2, output_size * INNER_2);
			arrayAdd(_dLdW3v, _dLdW3v, _temp, output_size * INNER_2);

			arrayMultiply(_dLdW3, _dLdW3m, 1.0f / (1.0f - beta1), output_size * INNER_2);
			arrayMultiply(_temp, _dLdW3v, 1.0f / (1.0f - beta2), output_size * INNER_2);
			arraySqrt(_temp, _dLdW3v, output_size * INNER_2);
			arrayAdd(_temp, _temp, EPSILON, output_size * INNER_2);
			arrayDivide(_dLdW3, _dLdW3, _temp, output_size * INNER_2);

			// W2
			arrayMultiply(_temp, _dLdW2, 1.0f - BETA1, INNER_2 * INNER_1);
			arrayMultiply(_dLdW2m, _dLdW2m, BETA1, INNER_2 * INNER_1);
			arrayAdd(_dLdW2m, _dLdW2m, _temp, INNER_2 * INNER_1);

			arrayMultiply(_temp, _dLdW2, _dLdW2, INNER_2 * INNER_1);
			arrayMultiply(_temp, _temp, 1.0f - BETA2, INNER_2 * INNER_1);
			arrayMultiply(_dLdW2v, _dLdW2v, BETA2, INNER_2 * INNER_1);
			arrayAdd(_dLdW2v, _dLdW2v, _temp, INNER_2 * INNER_1);

			arrayMultiply(_dLdW2, _dLdW2m, 1.0f / (1.0f - beta1), INNER_2 * INNER_1);
			arrayMultiply(_temp, _dLdW2v, 1.0f / (1.0f - beta2), INNER_2 * INNER_1);
			arraySqrt(_temp, _dLdW2v, INNER_2 * INNER_1);
			arrayAdd(_temp, _temp, EPSILON, INNER_2 * INNER_1);
			arrayDivide(_dLdW2, _dLdW2, _temp, INNER_2 * INNER_1);

			// W1
			arrayMultiply(_temp, _dLdW1, 1.0f - BETA1, INNER_1 * input_size);
			arrayMultiply(_dLdW1m, _dLdW1m, BETA1, INNER_1 * input_size);
			arrayAdd(_dLdW1m, _dLdW1m, _temp, INNER_1 * input_size);

			arrayMultiply(_temp, _dLdW1, _dLdW1, INNER_1 * input_size);
			arrayMultiply(_temp, _temp, 1.0f - BETA2, INNER_1 * input_size);
			arrayMultiply(_dLdW1v, _dLdW1v, BETA2, INNER_1 * input_size);
			arrayAdd(_dLdW1v, _dLdW1v, _temp, INNER_1 * input_size);

			arrayMultiply(_dLdW1, _dLdW1m, 1.0f / (1.0f - beta1), INNER_1 * input_size);
			arrayMultiply(_temp, _dLdW1v, 1.0f / (1.0f - beta2), INNER_1 * input_size);
			arraySqrt(_temp, _dLdW1v, INNER_1 * input_size);
			arrayAdd(_temp, _temp, EPSILON, INNER_1 * input_size);
			arrayDivide(_dLdW1, _dLdW1, _temp, INNER_1 * input_size);

			// b3
			arrayMultiply(_temp, _dLdb3, 1.0f - BETA1, output_size);
			arrayMultiply(_dLdb3m, _dLdb3m, BETA1, output_size);
			arrayAdd(_dLdb3m, _dLdb3m, _temp, output_size);

			arrayMultiply(_temp, _dLdb3, _dLdb3, output_size);
			arrayMultiply(_temp, _temp, 1.0f - BETA2, output_size);
			arrayMultiply(_dLdb3v, _dLdb3v, BETA2, output_size);
			arrayAdd(_dLdb3v, _dLdb3v, _temp, output_size);

			arrayMultiply(_dLdb3, _dLdb3m, 1.0f / (1.0f - beta1), output_size);
			arrayMultiply(_temp, _dLdb3v, 1.0f / (1.0f - beta2), output_size);
			arraySqrt(_temp, _dLdb3v, output_size);
			arrayAdd(_temp, _temp, EPSILON, output_size);
			arrayDivide(_dLdb3, _dLdb3, _temp, output_size);

			// b2
			arrayMultiply(_temp, _dLdb2, 1.0f - BETA1, INNER_2);
			arrayMultiply(_dLdb2m, _dLdb2m, BETA1, INNER_2);
			arrayAdd(_dLdb2m, _dLdb2m, _temp, INNER_2);

			arrayMultiply(_temp, _dLdb2, _dLdb2, INNER_2);
			arrayMultiply(_temp, _temp, 1.0f - BETA2, INNER_2);
			arrayMultiply(_dLdb2v, _dLdb2v, BETA2, INNER_2);
			arrayAdd(_dLdb2v, _dLdb2v, _temp, INNER_2);

			arrayMultiply(_dLdb2, _dLdb2m, 1.0f / (1.0f - beta1), INNER_2);
			arrayMultiply(_temp, _dLdb2v, 1.0f / (1.0f - beta2), INNER_2);
			arraySqrt(_temp, _dLdb2v, INNER_2);
			arrayAdd(_temp, _temp, EPSILON, INNER_2);
			arrayDivide(_dLdb2, _dLdb2, _temp, INNER_2);

			// b1
			arrayMultiply(_temp, _dLdb1, 1.0f - BETA1, INNER_1);
			arrayMultiply(_dLdb1m, _dLdb1m, BETA1, INNER_1);
			arrayAdd(_dLdb1m, _dLdb1m, _temp, INNER_1);

			arrayMultiply(_temp, _dLdb1, _dLdb1, INNER_1);
			arrayMultiply(_temp, _temp, 1.0f - BETA2, INNER_1);
			arrayMultiply(_dLdb1v, _dLdb1v, BETA2, INNER_1);
			arrayAdd(_dLdb1v, _dLdb1v, _temp, INNER_1);

			arrayMultiply(_dLdb1, _dLdb1m, 1.0f / (1.0f - beta1), INNER_1);
			arrayMultiply(_temp, _dLdb1v, 1.0f / (1.0f - beta2), INNER_1);
			arraySqrt(_temp, _dLdb1v, INNER_1);
			arrayAdd(_temp, _temp, EPSILON, INNER_1);
			arrayDivide(_dLdb1, _dLdb1, _temp, INNER_1);

			beta1 *= BETA1;
			beta2 *= BETA2;
		}
		*/

		//
		arrayMultiply(_dLdW3, _dLdW3, nu, output_size * INNER_2);
		
		arrayMultiply(_dLdb3, _dLdb3, nu, output_size);
		
		arrayMultiply(_dLdW2, _dLdW2, nu, INNER_2 * INNER_1);
		
		arrayMultiply(_dLdb2, _dLdb2, nu, INNER_2);
		
		arrayMultiply(_dLdW1, _dLdW1, nu, INNER_1 * input_size);
		
		arrayMultiply(_dLdb1, _dLdb1, nu, INNER_1);

		arraySubtract(_W3, _W3, _dLdW3, output_size * INNER_2);

		arraySubtract(_b3, _b3, _dLdb3, output_size);

		arraySubtract(_W2, _W2, _dLdW2, INNER_2 * INNER_1);

		arraySubtract(_b2, _b2, _dLdb2, INNER_2);

		arraySubtract(_W1, _W1, _dLdW1, INNER_1 * input_size);

		arraySubtract(_b1, _b1, _dLdb1, INNER_1);
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

	void Model::load(const char* filename)
	{
		std::fstream fs;
		fs.open(filename, std::fstream::in | std::fstream::binary);
		if (fs.is_open())
		{
			fs.read((char*)m_W1.data(), INNER_1 * input_size * sizeof(float));
			fs.read((char*)m_W2.data(), INNER_2 * INNER_1 * sizeof(float));
			fs.read((char*)m_W3.data(), output_size * INNER_2 * sizeof(float));
			fs.read((char*)m_b1.data(), INNER_1 * sizeof(float));
			fs.read((char*)m_b2.data(), INNER_2 * sizeof(float));
			fs.read((char*)m_b3.data(), output_size * sizeof(float));
			fs.close();

			arrayCopyToDevice(_W1, m_W1.data(), INNER_1 * input_size);
			arrayCopyToDevice(_W2, m_W2.data(), INNER_2 * INNER_1);
			arrayCopyToDevice(_W3, m_W3.data(), output_size * INNER_2);
			arrayCopyToDevice(_b1, m_b1.data(), INNER_1);
			arrayCopyToDevice(_b2, m_b2.data(), INNER_2);
			arrayCopyToDevice(_b3, m_b3.data(), output_size);
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
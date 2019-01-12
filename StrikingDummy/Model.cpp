#include "Model.h"
#include "CUDA.cuh"
#include <fstream>

namespace StrikingDummy
{
	const int NUM_IN = 46;
	const int NUM_OUT = 15;
	const int INNER_1 = 64;
	const int INNER_2 = 64;

	float sigmoid(float x)
	{
		return 1.0f / (1.0f + exp(-x));
	}

	//Model::Model(ModelParams& params)
	void Model::init(int batch_size)
	{
		cudaInitialize();

		this->batch_size = batch_size;

		x0 = new float[NUM_IN];
		x3 = new float[NUM_OUT];
		X0 = new float[NUM_IN * batch_size];
		X3 = new float[NUM_OUT * batch_size];
		target = new float[NUM_OUT * batch_size];

		matrixInitialize(&_x0, NUM_IN, 1);
		matrixInitialize(&_x1, INNER_1, 1);
		matrixInitialize(&_x2, INNER_2, 1);
		matrixInitialize(&_x3, NUM_OUT, 1);
		matrixInitialize(&_X0, NUM_IN, batch_size);
		matrixInitialize(&_X1, INNER_1, batch_size);
		matrixInitialize(&_X2, INNER_2, batch_size);
		matrixInitialize(&_X3, NUM_OUT, batch_size);
		matrixInitialize(&_target, NUM_OUT, batch_size);
		matrixInitialize(&_W1, INNER_1, NUM_IN, sqrtf(96.0f / (INNER_1 + NUM_IN)));
		matrixInitialize(&_W2, INNER_2, INNER_1, sqrtf(96.0f / (INNER_2 + INNER_1)));
		matrixInitialize(&_W3, NUM_OUT, INNER_2, sqrtf(96.0f / (NUM_OUT + INNER_2)));
		matrixInitialize(&_b1, INNER_1, 1, 0.0f);
		matrixInitialize(&_b2, INNER_2, 1, 0.0f);
		matrixInitialize(&_b3, NUM_OUT, 1, 0.0f);
		matrixInitialize(&_d1, INNER_1, batch_size);
		matrixInitialize(&_d2, INNER_2, batch_size);
		matrixInitialize(&_d3, NUM_OUT, batch_size);
		matrixInitialize(&_dLdW1, INNER_1, NUM_IN);
		matrixInitialize(&_dLdW2, INNER_2, INNER_1);
		matrixInitialize(&_dLdW3, NUM_OUT, INNER_2);
		matrixInitialize(&_dLdb1, INNER_1, 1);
		matrixInitialize(&_dLdb2, INNER_2, 1);
		matrixInitialize(&_dLdb3, NUM_OUT, 1);
		matrixInitialize(&_ones, batch_size, 1);
		arrayAdd(_ones, _ones, 1.0f, batch_size);

		matrixInitialize(&__X0, batch_size, NUM_IN);
		matrixInitialize(&__X1, batch_size, INNER_1);
		matrixInitialize(&__X2, batch_size, INNER_2);
		matrixInitialize(&__W2, INNER_1, INNER_2);
		matrixInitialize(&__W3, INNER_2, NUM_OUT);

		m_x0 = MatrixXf(NUM_IN, 1);
		m_x1 = MatrixXf(INNER_1, 1);
		m_x2 = MatrixXf(INNER_2, 1);
		m_x3 = MatrixXf(NUM_OUT, 1);
		m_W1 = MatrixXf(INNER_1, NUM_IN);
		m_W2 = MatrixXf(INNER_2, INNER_1);
		m_W3 = MatrixXf(NUM_OUT, INNER_2);
		m_b1 = MatrixXf::Zero(INNER_1, 1);
		m_b2 = MatrixXf::Zero(INNER_2, 1);
		m_b3 = MatrixXf::Zero(NUM_OUT, 1);

		arrayCopyToHost(m_W1.data(), _W1, INNER_1 * NUM_IN);
		arrayCopyToHost(m_W2.data(), _W2, INNER_2 * INNER_1);
		arrayCopyToHost(m_W3.data(), _W3, NUM_OUT * INNER_2);
	}

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

		matrixFree(&__X0);
		matrixFree(&__X1);
		matrixFree(&__X2);
		matrixFree(&__W2);
		matrixFree(&__W3);
	}

	float* Model::compute()
	{
		/*
		arrayCopyToDevice(_x0, x0, NUM_IN);

		matrixMultiply(_x1, _W1, INNER_1, NUM_IN, _x0, NUM_IN, 1);
		arrayAdd(_x1, _x1, _b1, INNER_1);
		arraySigmoid(_x1, _x1, INNER_1);

		matrixMultiply(_x2, _W2, INNER_2, INNER_1, _x1, INNER_1, 1);
		arrayAdd(_x2, _x2, _b2, INNER_2);
		arraySigmoid(_x2, _x2, INNER_2);

		matrixMultiply(_x3, _W3, NUM_OUT, INNER_2, _x2, INNER_2, 1);
		arrayAdd(_x3, _x3, _b3, NUM_OUT);
		//arraySigmoid(_x3, NUM_OUT);

		arrayCopyToHost(x3, _x3, NUM_OUT);
		return x3;
		*/
		m_x1 = (m_W1 * m_x0 + m_b1).unaryExpr(&sigmoid);
		m_x2 = (m_W2 * m_x1 + m_b2).unaryExpr(&sigmoid);
		//m_x1 = (m_W1 * m_x0 + m_b1).cwiseMax(0.0f);
		//m_x2 = (m_W2 * m_x1 + m_b2).cwiseMax(0.0f);
		m_x3 = (m_W3 * m_x2 + m_b3);
		return m_x3.data();
	}

	float* Model::batch_compute()
	{
		arrayCopyToDevice(_X0, X0, NUM_IN * batch_size);

		matrixMultiply(_X1, _W1, INNER_1, NUM_IN, _X0, NUM_IN, batch_size);
		arrayAddRep(_X1, _X1, _b1, INNER_1, batch_size);
		arraySigmoid(_X1, _X1, INNER_1 * batch_size);
		//arrayReLU(_X1, _X1, INNER_1 * batch_size);

		matrixMultiply(_X2, _W2, INNER_2, INNER_1, _X1, INNER_1, batch_size);
		arrayAddRep(_X2, _X2, _b2, INNER_2, batch_size);
		arraySigmoid(_X2, _X2, INNER_2 * batch_size);
		//arrayReLU(_X2, _X2, INNER_2 * batch_size);

		matrixMultiply(_X3, _W3, NUM_OUT, INNER_2, _X2, INNER_2, batch_size);
		arrayAddRep(_X3, _X3, _b3, NUM_OUT, batch_size);
		arraySigmoid(_X3, _X3, NUM_OUT * batch_size);

		arrayCopyToHost(X3, _X3, NUM_OUT * batch_size);
		return X3;
	}

	void Model::train(float nu)
	{
		arrayCopyToDevice(_target, target, NUM_OUT * batch_size);

		// d3 = (Xk - target).cwiseProduct(Xk.unaryExpr(&dsigmoid));
		arraySubtract(_target, _X3, _target, NUM_OUT * batch_size);

		arrayDerivSigmoid(_X3, _X3, NUM_OUT * batch_size);

		arrayMultiply(_d3, _target, _X3, NUM_OUT * batch_size);

		// 
		matrixTranspose(__X2, _X2, INNER_2, batch_size);

		matrixMultiply(_dLdW3, _d3, NUM_OUT, batch_size, __X2, batch_size, INNER_2);

		matrixMultiply(_dLdb3, _d3, NUM_OUT, batch_size, _ones, batch_size, 1);

		//
		matrixTranspose(__W3, _W3, NUM_OUT, INNER_2);

		matrixMultiply(_d2, __W3, INNER_2, NUM_OUT, _d3, NUM_OUT, batch_size);

		arrayDerivSigmoid(_X2, _X2, INNER_2 * batch_size);
		//arrayDerivReLU(_X2, _X2, INNER_2 * batch_size);

		arrayMultiply(_d2, _d2, _X2, INNER_2 * batch_size);

		//
		matrixTranspose(__X1, _X1, INNER_1, batch_size);

		matrixMultiply(_dLdW2, _d2, INNER_2, batch_size, __X1, batch_size, INNER_1);

		matrixMultiply(_dLdb2, _d2, INNER_2, batch_size, _ones, batch_size, 1);

		//
		matrixTranspose(__W2, _W2, INNER_2, INNER_1);

		matrixMultiply(_d1, __W2, INNER_1, INNER_2, _d2, INNER_2, batch_size);

		arrayDerivSigmoid(_X1, _X1, INNER_1 * batch_size);
		//arrayDerivReLU(_X1, _X1, INNER_1 * batch_size);

		arrayMultiply(_d1, _d1, _X1, INNER_1 * batch_size);

		//
		matrixTranspose(__X0, _X0, NUM_IN, batch_size);

		matrixMultiply(_dLdW1, _d1, INNER_1, batch_size, __X0, batch_size, NUM_IN);

		matrixMultiply(_dLdb1, _d1, INNER_1, batch_size, _ones, batch_size, 1);

		//
		arrayMultiply(_dLdW3, _dLdW3, nu, NUM_OUT * INNER_2);
		
		arrayMultiply(_dLdb3, _dLdb3, nu, NUM_OUT);
		
		arrayMultiply(_dLdW2, _dLdW2, nu, INNER_2 * INNER_1);
		
		arrayMultiply(_dLdb2, _dLdb2, nu, INNER_2);
		
		arrayMultiply(_dLdW1, _dLdW1, nu, INNER_1 * INNER_2);
		
		arrayMultiply(_dLdb1, _dLdb1, nu, INNER_1);

		arraySubtract(_W3, _W3, _dLdW3, NUM_OUT * INNER_2);

		arraySubtract(_b3, _b3, _dLdb3, NUM_OUT);

		arraySubtract(_W2, _W2, _dLdW2, INNER_2 * INNER_1);

		arraySubtract(_b2, _b2, _dLdb2, INNER_2);

		arraySubtract(_W1, _W1, _dLdW1, INNER_1 * INNER_2);

		arraySubtract(_b1, _b1, _dLdb1, INNER_1);

		arrayCopyToHost(m_W1.data(), _W1, INNER_1 * NUM_IN);
		arrayCopyToHost(m_W2.data(), _W2, INNER_2 * INNER_1);
		arrayCopyToHost(m_W3.data(), _W3, NUM_OUT * INNER_2);
		arrayCopyToHost(m_b1.data(), _b1, INNER_1);
		arrayCopyToHost(m_b2.data(), _b2, INNER_2);
		arrayCopyToHost(m_b3.data(), _b3, NUM_OUT);
	}

	void Model::save(const char* filename)
	{
		std::fstream fs;
		fs.open(filename, std::fstream::out | std::fstream::binary);
		fs.write((const char*)m_W1.data(), INNER_1 * NUM_IN * sizeof(float));
		fs.write((const char*)m_W2.data(), INNER_2 * INNER_1 * sizeof(float));
		fs.write((const char*)m_W3.data(), NUM_OUT * INNER_2 * sizeof(float));
		fs.write((const char*)m_b1.data(), INNER_1 * sizeof(float));
		fs.write((const char*)m_b2.data(), INNER_2 * sizeof(float));
		fs.write((const char*)m_b3.data(), NUM_OUT * sizeof(float));
		fs.flush();
		fs.close();
	}

	// set input layer
	// add hidden layer
	// add hidden layer
	// add hidden layer
	// set output layer
	// build model?

	// 56 inputs -> 128 -> 15

	// initialize Q model

	// W1 is a 128 x 56 matrix
	// b1 is a 128 x 1 matrix
	// W2 is a 15 x 128 matrix
	// b2 is a 15 x 1 matrix
	/*
	MatrixXf W1;
	MatrixXf b1;
	MatrixXf W2;
	MatrixXf b2;
	MatrixXf W3;
	MatrixXf b3;
	MatrixXf x1;
	MatrixXf x2;
	MatrixXf dLdW1;
	MatrixXf dLdb1;
	MatrixXf dLdW2;
	MatrixXf dLdb2;
	MatrixXf dLdW3;
	MatrixXf dLdb3;

	// initialize intermediate matrices -> noalias()
	MatrixXf X1;
	MatrixXf X2;
	//MatrixXf QR;
	//MatrixXf R;
	//MatrixXf Q;
	//MatrixXf dQdX2;
	//MatrixXf dLdX2;
	MatrixXf d1;
	MatrixXf d2;
	MatrixXf d3;
	MatrixXf ones_row;
	MatrixXf ones_col;
	MatrixXf ones_X2;
	MatrixXf ones_X3;

	float sigmoid(float x)
	{
		return 1.0f / (1.0f + exp(-x));
	}

	float dsigmoid(float x)
	{
		return x * (1.0f - x);
	}

	void Model::init(int batch_size)
	{
		// sigmoid - rt96
		// relu - rt12
		W1 = MatrixXf::Random(INNER_1, NUM_IN) * (sqrtf(96.0f / (INNER_1 + NUM_IN)));
		b1 = MatrixXf::Zero(INNER_1, 1);
		W2 = MatrixXf::Random(INNER_2, INNER_1) * (sqrtf(96.0f / (INNER_2 + INNER_1)));
		b2 = MatrixXf::Zero(INNER_2, 1);
		W3 = MatrixXf::Random(NUM_OUT, INNER_2) * (sqrtf(96.0f / (NUM_OUT + INNER_2)));
		b3 = MatrixXf::Zero(NUM_OUT, 1);
		x0 = MatrixXf::Zero(NUM_IN, 1);
		x1 = MatrixXf::Zero(INNER_1, 1);
		x2 = MatrixXf::Zero(INNER_2, 1);
		xk = MatrixXf::Zero(NUM_OUT, 1);
		dLdW1 = MatrixXf::Zero(INNER_1, NUM_IN);
		dLdb1 = MatrixXf::Zero(INNER_1, 1);
		dLdW2 = MatrixXf::Zero(INNER_2, INNER_1);
		dLdb2 = MatrixXf::Zero(INNER_2, 1);
		dLdW3 = MatrixXf::Zero(NUM_OUT, INNER_2);
		dLdb3 = MatrixXf::Zero(NUM_OUT, 1);
		X0 = MatrixXf::Zero(NUM_IN, batch_size);
		X1 = MatrixXf::Zero(INNER_1, batch_size);
		X2 = MatrixXf::Zero(INNER_2, batch_size);
		Xk = MatrixXf::Zero(NUM_OUT, batch_size);
		//QR = MatrixXf::Zero(1, batch_size);
		//R = MatrixXf::Zero(1, batch_size);
		//Q = MatrixXf::Zero(1, batch_size);
		//dQdX2 = MatrixXf::Zero(NUM_OUT, batch_size);
		//dLdX2 = MatrixXf::Zero(NUM_OUT, batch_size);
		d1 = MatrixXf::Zero(INNER_1, batch_size);
		d2 = MatrixXf::Zero(INNER_2, batch_size);
		d3 = MatrixXf::Zero(NUM_OUT, batch_size);
		//dX1 = MatrixXf::Zero(INNER_1, batch_size);
		//dX2 = MatrixXf::Zero(NUM_OUT, batch_size);
		ones_row = MatrixXf::Ones(1, batch_size);
		ones_col = MatrixXf::Ones(batch_size, 1);
		ones_X2 = MatrixXf::Ones(INNER_2, 1);
		ones_X3 = MatrixXf::Ones(NUM_OUT, 1);

		target = MatrixXf::Zero(NUM_OUT, batch_size);


		//std::fstream fs;
		//
		//fs.open("Weights\\W1.CEM", std::fstream::in | std::fstream::binary);
		//fs.read((char*)W1.data(), INNER_1 * NUM_IN * 4);
		//fs.close();
		//
		//if (W1.hasNaN())
		//	throw 0;
		//
		//fs.open("Weights\\b1.CEM", std::fstream::in | std::fstream::binary);
		//fs.read((char*)b1.data(), INNER_1 * 4);
		//fs.close();
		//
		//if (b1.hasNaN())
		//	throw 0;
		//
		//fs.open("Weights\\W2.CEM", std::fstream::in | std::fstream::binary);
		//fs.read((char*)W2.data(), INNER_2 * INNER_1 * 4);
		//fs.close();
		//
		//if (W2.hasNaN())
		//	throw 0;
		//
		//fs.open("Weights\\b2.CEM", std::fstream::in | std::fstream::binary);
		//fs.read((char*)b2.data(), INNER_2 * 4);
		//fs.close();
		//
		//if (b2.hasNaN())
		//	throw 0;
		//
		//fs.open("Weights\\W3.CEM", std::fstream::in | std::fstream::binary);
		//fs.read((char*)W3.data(), NUM_OUT * INNER_2 * 4);
		//fs.close();
		//
		//if (W3.hasNaN())
		//	throw 0;
		//
		//fs.open("Weights\\b3.CEM", std::fstream::in | std::fstream::binary);
		//fs.read((char*)b3.data(), NUM_OUT * 4);
		//fs.close();
		//
		//if (b3.hasNaN())
		//	throw 0;

		//std::stringstream ss;
		//ss << "Weights\\weights-" << start_time;
		//fs.open(ss.str().c_str(), std::fstream::out | std::fstream::binary);
		//fs.write((const char*)W1.data(), INNER * 56 * 8);
		//fs.write((const char*)b1.data(), INNER * 1 * 8);
		//fs.write((const char*)W2.data(), 15 * INNER * 8);
		//fs.write((const char*)b2.data(), 15 * 1 * 8);
		//fs.flush();
		//fs.close();
	}

	MatrixXf& Model::compute()
	{
		//x1 = (W1 * x0 + b1).cwiseMax(0.0f);
		//x1 = (W1 * x0 + b1).unaryExpr(&leaky);
		//x1 = (W1 * x0 + b1).unaryExpr(&sigmoid);
		
		//x1 = (W1 * x0 + b1).cwiseMax(0.0f);
		//x2 = (W2 * x1 + b2).cwiseMax(0.0f);
		x1 = (W1 * x0 + b1).unaryExpr(&sigmoid);
		x2 = (W2 * x1 + b2).unaryExpr(&sigmoid);
		xk = (W3 * x2 + b3);// .unaryExpr(&sigmoid);
		return xk;
	}

	MatrixXf& Model::batch_compute()
	{
		//X1 = (W1 * X0 + b1 * ones_row).cwiseMax(0.0f);
		//X1 = (W1 * X0 + b1 * ones_row).unaryExpr(&leaky);
		//X1 = (W1 * X0 + b1 * ones_row).unaryExpr(&sigmoid);
		
		//X1 = (W1 * X0 + b1 * ones_row).cwiseMax(0.0f);
		//X2 = (W2 * X1 + b2 * ones_row).cwiseMax(0.0f);
		X1 = (W1 * X0 + b1 * ones_row).unaryExpr(&sigmoid);
		X2 = (W2 * X1 + b2 * ones_row).unaryExpr(&sigmoid);
		Xk = (W3 * X2 + b3 * ones_row).unaryExpr(&sigmoid);
		return Xk;
	}

	void Model::train(float nu)
	{
		// Calculate gradient stuff

		// dL/dQ -> 1 x N
		// Q - QR;

		// dQ/dX2 -> 15 x N
		//dQdX2.setZero();
		//for (int i = 0; i < BATCH_SIZE; i++)
		//	dQdX2.col(i)(action_indices[i]) = 1.0;

		// dL/dX2 -> 15 x N
		//dLdX2 = (ones_X2 * (Q - QR)).cwiseProduct(dQdX2);
		//dLdX2 = (ones_X2 * (Q - QR)).cwiseProduct(dQdX2);

		// d2 = dL/dX2 . f'(S2)
		d3 = (Xk - target).cwiseProduct(Xk.unaryExpr(&dsigmoid));

		// dL/dW2 = d2 * X1^T
		dLdW3 = d3 * X2.transpose();

		// dL/db2 = d2 * (N x 1)
		dLdb3 = d3 * ones_col;

		// d1 = (W2^T * d2) . f'(S1)
		
		//d1 = (W2.transpose() * d2).cwiseProduct(X1.cwiseSign());
		//d1 = (W2.transpose() * d2).cwiseProduct(X1.unaryExpr(&dleaky));
		//d1 = (W2.transpose() * d2).cwiseProduct(X1.unaryExpr(&dsigmoid));

		//d2 = (W3.transpose() * d3).cwiseProduct(X2.cwiseSign());
		d2 = (W3.transpose() * d3).cwiseProduct(X2.unaryExpr(&dsigmoid));

		// dL/dW1 = d1 * X0^T
		dLdW2 = d2 * X1.transpose();

		// dL/db1 = d1 * (N x 1)
		dLdb2 = d2 * ones_col;

		//////////

		//d1 = (W2.transpose() * d2).cwiseProduct(X1.cwiseSign());
		d1 = (W2.transpose() * d2).cwiseProduct(X1.unaryExpr(&dsigmoid));

		// dL/dW1 = d1 * X0^T
		dLdW1 = d1 * X0.transpose();

		// dL/db1 = d1 * (N x 1)
		dLdb1 = d1 * ones_col;

		// W2 -= nu * dL/dW2
		W3 = W3 - (nu * dLdW3);

		// b2 -= nu * dL/db2
		b3 = b3 - (nu * dLdb3);

		// W2 -= nu * dL/dW2
		W2 = W2 - (nu * dLdW2);

		// b2 -= nu * dL/db2
		b2 = b2 - (nu * dLdb2);

		// W1 -= nu * dL/dW1
		W1 = W1 - (nu * dLdW1);

		// b1 -= nu * dL/db1
		b1 = b1 - (nu * dLdb1);

		//std::cout << W2.col(0)(0) << std::endl;
		//if (std::isnan(W2.col(0)(0)))
		//	throw 0;
	}
	*/
}
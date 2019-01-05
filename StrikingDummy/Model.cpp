#include "Model.h"
#include <iostream>
#include <fstream>

namespace StrikingDummy
{
	const int NUM_IN = 47;
	const int NUM_OUT = 15;
	const int INNER_1 = 32;
	const int INNER_2 = 32;

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
		//return 1.0f / (1.0f + abs(x));
		//return x / (1.0f + abs(x));
		return 1.0f / (1.0f + exp(-x));
	}

	float dsigmoid(float x)
	{
		return x * (1.0f - x);
	}

	float leaky(float x)
	{
		if (x <= 0.0f)
			return 0.0f;
		if (x >= 1.0f)
			return 1.0f;
		return x;
	}

	float dleaky(float x)
	{
		if (x <= 0.0f)
			return 0.0f;
		if (x >= 1.0f)
			return 0.0f;
		return 1.0f;
	}

	void Model::init(int batch_size)
	{
		// sigmoid - rt96
		// relu - rt12
		W1 = MatrixXf::Random(INNER_1, NUM_IN) * (sqrtf(12.0f / (INNER_1 + NUM_IN)));
		b1 = MatrixXf::Zero(INNER_1, 1);
		W2 = MatrixXf::Random(INNER_2, INNER_1) * (sqrtf(12.0f / (INNER_2 + INNER_1)));
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
		
		x1 = (W1 * x0 + b1).cwiseMax(0.0f);
		x2 = (W2 * x1 + b2).cwiseMax(0.0f);
		//x1 = (W1 * x0 + b1).unaryExpr(&sigmoid);
		//x2 = (W2 * x1 + b2).unaryExpr(&sigmoid);
		xk = (W3 * x2 + b3);// .unaryExpr(&sigmoid);
		return xk;
	}

	MatrixXf& Model::batch_compute()
	{
		//X1 = (W1 * X0 + b1 * ones_row).cwiseMax(0.0f);
		//X1 = (W1 * X0 + b1 * ones_row).unaryExpr(&leaky);
		//X1 = (W1 * X0 + b1 * ones_row).unaryExpr(&sigmoid);
		
		X1 = (W1 * X0 + b1 * ones_row).cwiseMax(0.0f);
		X2 = (W2 * X1 + b2 * ones_row).cwiseMax(0.0f);
		//X1 = (W1 * X0 + b1 * ones_row).unaryExpr(&sigmoid);
		//X2 = (W2 * X1 + b2 * ones_row).unaryExpr(&sigmoid);
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

		d2 = (W3.transpose() * d3).cwiseProduct(X2.cwiseSign());
		//d2 = (W3.transpose() * d3).cwiseProduct(X2.unaryExpr(&dsigmoid));

		// dL/dW1 = d1 * X0^T
		dLdW2 = d2 * X1.transpose();

		// dL/db1 = d1 * (N x 1)
		dLdb2 = d2 * ones_col;

		//////////

		d1 = (W2.transpose() * d2).cwiseProduct(X1.cwiseSign());
		//d1 = (W2.transpose() * d2).cwiseProduct(X1.unaryExpr(&dsigmoid));

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
		if (std::isnan(W2.col(0)(0)))
			throw 0;
	}
}
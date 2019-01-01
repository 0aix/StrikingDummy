#include "Model.h"
#include <iostream>

namespace StrikingDummy
{
	const int NUM_IN = 47;
	const int NUM_OUT = 15;
	const int INNER_1 = 30;

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
	MatrixXf x1;
	MatrixXf dLdW1;
	MatrixXf dLdb1;
	MatrixXf dLdW2;
	MatrixXf dLdb2;

	// initialize intermediate matrices -> noalias()
	MatrixXf X1;
	MatrixXf QR;
	MatrixXf R;
	MatrixXf Q;
	MatrixXf dQdX2;
	MatrixXf dLdX2;
	MatrixXf d1;
	MatrixXf d2;
	MatrixXf dX1;
	MatrixXf dX2;
	MatrixXf ones_row;
	MatrixXf ones_col;
	MatrixXf ones_X2;

	float sigmoid(float x)
	{
		return 1.0f / (1.0f + abs(x));
		//return 1.0f / (1.0f + exp(-x));
	}

	float dsigmoid(float x)
	{
		return x * (1.0f - x);
	}

	void Model::init(int batch_size)
	{
		W1 = MatrixXf::Random(INNER_1, NUM_IN) * (sqrtf(6.0f / (INNER_1 + NUM_IN)));
		b1 = MatrixXf::Zero(INNER_1, 1);
		W2 = MatrixXf::Random(NUM_OUT, INNER_1) * (sqrtf(6.0f / (NUM_OUT + INNER_1)));
		b2 = MatrixXf::Zero(NUM_OUT, 1);
		x0 = MatrixXf::Zero(NUM_IN, 1);
		x1 = MatrixXf::Zero(INNER_1, 1);
		xk = MatrixXf::Zero(NUM_OUT, 1);
		dLdW1 = MatrixXf::Zero(INNER_1, NUM_IN);
		dLdb1 = MatrixXf::Zero(INNER_1, 1);
		dLdW2 = MatrixXf::Zero(NUM_OUT, INNER_1);
		dLdb2 = MatrixXf::Zero(NUM_OUT, 1);
		X0 = MatrixXf::Zero(NUM_IN, batch_size);
		X1 = MatrixXf::Zero(INNER_1, batch_size);
		Xk = MatrixXf::Zero(NUM_OUT, batch_size);
		QR = MatrixXf::Zero(1, batch_size);
		R = MatrixXf::Zero(1, batch_size);
		Q = MatrixXf::Zero(1, batch_size);
		dQdX2 = MatrixXf::Zero(NUM_OUT, batch_size);
		dLdX2 = MatrixXf::Zero(NUM_OUT, batch_size);
		d1 = MatrixXf::Zero(INNER_1, batch_size);
		d2 = MatrixXf::Zero(NUM_OUT, batch_size);
		dX1 = MatrixXf::Zero(INNER_1, batch_size);
		dX2 = MatrixXf::Zero(NUM_OUT, batch_size);
		ones_row = MatrixXf::Ones(1, batch_size);
		ones_col = MatrixXf::Ones(batch_size, 1);
		ones_X2 = MatrixXf::Ones(NUM_OUT, 1);

		target = MatrixXf::Zero(NUM_OUT, batch_size);
	}

	MatrixXf& Model::compute()
	{
		//x1 = (W1 * x0 + b1).cwiseMax(0.0f);
		x1 = (W1 * x0 + b1).unaryExpr(&sigmoid);
		xk = (W2 * x1 + b2);
		return xk;
	}

	MatrixXf& Model::batch_compute()
	{
		//X1 = (W1 * X0 + b1 * ones_row).cwiseMax(0.0f);
		X1 = (W1 * X0 + b1 * ones_row).unaryExpr(&sigmoid);
		Xk = (W2 * X1 + b2 * ones_row);
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
		d2 = Xk - target;

		// dL/dW2 = d2 * X1^T
		dLdW2 = d2 * X1.transpose();

		// dL/db2 = d2 * (N x 1)
		dLdb2 = d2 * ones_col;

		// d1 = (W2^T * d2) . f'(S1)
		
		//d1 = (W2.transpose() * d2).cwiseProduct(X1.cwiseSign());
		d1 = (W2.transpose() * d2).cwiseProduct(X1.unaryExpr(&dsigmoid));

		// dL/dW1 = d1 * X0^T
		dLdW1 = d1 * X0.transpose();

		// dL/db1 = d1 * (N x 1)
		dLdb1 = d1 * ones_col;

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
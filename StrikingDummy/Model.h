#pragma once
#include <Eigen/Core>
using namespace Eigen;

namespace StrikingDummy
{
	struct Model
	{
		MatrixXf x0; // single input
		MatrixXf xk; // single output
		MatrixXf X0; // batch input
		MatrixXf Xk; // batch output
		MatrixXf target; // batch target

		void init(int batch_size);

		MatrixXf& compute();
		MatrixXf& batch_compute();

		void train(float nu);
	};
}
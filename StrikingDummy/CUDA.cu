#include "CUDA.cuh"
#include <iostream>
#include <chrono>
#include <math.h>
#include <curand.h>

int blockSize = 0;
bool cuda_init = false;

void cudaSafeDeviceSynchronize()
{
	cudaError_t cudaStatus = cudaDeviceSynchronize();
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "cudaDeviceSynchronize failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
}

void cudaInitialize()
{
	if (!cuda_init)
	{
		cudaError_t cudaStatus = cudaSetDevice(0);
		if (cudaStatus != cudaSuccess)
		{
			std::cerr << "cudaSetDevice failed: " << cudaGetErrorString(cudaStatus) << std::endl;
			throw 0;
		}

		cudaDeviceProp prop;
		cudaStatus = cudaGetDeviceProperties(&prop, 0);
		if (cudaStatus != cudaSuccess)
		{
			std::cerr << "cudaGetDeviceProperties failed: " << cudaGetErrorString(cudaStatus) << std::endl;
			throw 0;
		}
		blockSize = prop.maxThreadsPerBlock;
		if (blockSize != 1024)
			throw 0;

		//cudaMemset(actions, 0, sizeof(actions));

		cuda_init = true;
	}
}

void cudaSafeMalloc(void** A, int n)
{
	cudaError_t cudaStatus = cudaMalloc(A, n);
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "cudaMalloc failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
}

void cudaSafeFree(void** A)
{
	if (A)
	{
		cudaFree(*A);
		*A = NULL;
	}
}

void cudaCopyToDevice(void* _A, void* A, int n)
{
	cudaError_t cudaStatus = cudaMemcpy(_A, A, n, cudaMemcpyHostToDevice);
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "cudaMemcpy failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
}

__global__ void _matrixInitialize(float* A, int n, float r)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n)
		A[i] = 2.0f * (A[i] - 0.5f) * r;
}

__global__ void _matrixMultiply(float* C, float* A, int n0, int m0, float* B, int n1, int m1)
{
	int i = blockIdx.y * blockDim.y + threadIdx.y;
	int j = blockIdx.x * blockDim.x + threadIdx.x;

	__shared__ float _A[32][32];
	__shared__ float _B[32][32];

	float sum = 0.0f;
	bool compute = i < n0 && j < m1;
	int Z = (m0 + 31) / 32;
	int K = (m0 + 31) % 32 + 1;

	for (int z = 0; z < Z; ++z)
	{
		int Ay = i;
		int Ax = threadIdx.x + z * 32;
		int By = threadIdx.y + z * 32;
		int Bx = j;
		if (Ay < n0 && Ax < m0)
			_A[threadIdx.y][threadIdx.x] = A[Ay + Ax * n0];
		if (By < n1 && Bx < m1)
			_B[threadIdx.y][threadIdx.x] = B[By + Bx * n1];

		__syncthreads();

		if (compute)
		{
			if (z == Z - 1)
			{
				for (int k = 0; k < K; ++k)
					sum += _A[threadIdx.y][k] * _B[k][threadIdx.x];
			}
			else
			{
#pragma unroll
				for (int k = 0; k < 32; ++k)
					sum += _A[threadIdx.y][k] * _B[k][threadIdx.x];
			}
		}

		__syncthreads();
	}
	if (compute)
		C[j * n0 + i] = sum;
}

__global__ void _matrixMultiplyTranspose(float* C, float* A, int n0, int m0, float* B, int n1, int m1)
{
	int i = blockIdx.y * blockDim.y + threadIdx.y;
	int j = blockIdx.x * blockDim.x + threadIdx.x;

	__shared__ float _A[32][32];
	__shared__ float _B[32][32];

	float sum = 0.0f;
	bool compute = i < n0 && j < m1;
	int Z = (m0 + 31) / 32;
	int K = (m0 + 31) % 32 + 1;

	for (int z = 0; z < Z; ++z)
	{
		int Ay = i;
		int Ax = threadIdx.x + z * 32;
		int By = threadIdx.y + z * 32;
		int Bx = j;
		if (Ay < n0 && Ax < m0)
			_A[threadIdx.y][threadIdx.x] = A[Ay + Ax * n0];
		if (By < n1 && Bx < m1)
			_B[threadIdx.y][threadIdx.x] = B[By * m1 + Bx];

		__syncthreads();

		if (compute)
		{
			if (z == Z - 1)
			{
				for (int k = 0; k < K; ++k)
					sum += _A[threadIdx.y][k] * _B[k][threadIdx.x];
			}
			else
			{
#pragma unroll
				for (int k = 0; k < 32; ++k)
					sum += _A[threadIdx.y][k] * _B[k][threadIdx.x];
			}
		}

		__syncthreads();
	}
	if (compute)
		C[j * n0 + i] = sum;
}

__global__ void _matrixTranspose(float* B, float* A, int n, int m)
{
	__shared__ float tile[32][33];

	int i = blockIdx.y * 32 + threadIdx.y;
	int j = blockIdx.x * 32 + threadIdx.x;
	if (i < n && j < m)
		tile[threadIdx.y][threadIdx.x] = A[i * m + j];

	__syncthreads();

	i = blockIdx.x * 32 + threadIdx.y;
	j = blockIdx.y * 32 + threadIdx.x;
	if (i < m && j < n)
		B[i * n + j] = tile[threadIdx.x][threadIdx.y];
	/*
	int i = blockIdx.y * blockDim.y + threadIdx.y;
	int j = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n && j < m)
		B[i * m + j] = A[j * n + i];
	*/
}

void matrixInitialize(float** A, int n, int m)
{
	cudaError_t cudaStatus = cudaMalloc(A, sizeof(float) * n * m);
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "cudaMalloc failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
}

void matrixInitialize(float** A, int n, int m, float r)
{
	matrixInitialize(A, n, m);

	curandGenerator_t gen;
	
	curandStatus_t status = curandCreateGenerator(&gen, CURAND_RNG_PSEUDO_MT19937);
	if (status != CURAND_STATUS_SUCCESS)
	{
		std::cerr << "curandCreateGenerator failed" << std::endl;
		throw 0;
	}

	status = curandSetPseudoRandomGeneratorSeed(gen, std::chrono::high_resolution_clock::now().time_since_epoch().count());
	if (status != CURAND_STATUS_SUCCESS)
	{
		std::cerr << "curandSetPseudoRandomGeneratorSeed failed" << std::endl;
		throw 0;
	}

	status = curandGenerateUniform(gen, *A, (size_t)n * (size_t)m);
	if (status != CURAND_STATUS_SUCCESS)
	{
		std::cerr << "curandGenerateUniform failed" << std::endl;
		throw 0;
	}

	status = curandDestroyGenerator(gen);
	if (status != CURAND_STATUS_SUCCESS)
	{
		std::cerr << "curandDestroyGenerator failed" << std::endl;
		throw 0;
	}

	int numBlocks = (n * m + blockSize - 1) / blockSize;
	_matrixInitialize<<<numBlocks, blockSize>>>(*A, n * m, r);

	cudaError_t cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "_matrixInitialize failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
	//cudaSafeDeviceSynchronize();
}

void matrixFree(float** A)
{
	if (A)
	{
		cudaFree(*A);
		*A = NULL;
	}
}

void matrixMultiply(float* C, float* A, int n0, int m0, float* B, int n1, int m1)
{
	if (m0 != n1)
		throw 0;

	dim3 numThreads(32, 32);
	dim3 numBlocks((m1 + 31) / 32, (n0 + 31) / 32);

	_matrixMultiply<<<numBlocks, numThreads>>>(C, A, n0, m0, B, n1, m1);

	cudaError_t cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "_matrixMultiply failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
	//cudaSafeDeviceSynchronize();
}

void matrixMultiplyTranspose(float* C, float* A, int n0, int m0, float* B, int n1, int m1)
{
	if (m0 != n1)
		throw 0;

	dim3 numThreads(32, 32);
	dim3 numBlocks((m1 + 31) / 32, (n0 + 31) / 32);

	_matrixMultiplyTranspose<<<numBlocks, numThreads>>>(C, A, n0, m0, B, n1, m1);

	cudaError_t cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "_matrixMultiply failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
	//cudaSafeDeviceSynchronize();
}

void matrixTranspose(float* B, float* A, int n, int m)
{
	dim3 numThreads(32, 32);
	dim3 numBlocks((m + 31) / 32, (n + 31) / 32);

	_matrixTranspose<<<numBlocks, numThreads>>>(B, A, n, m);

	cudaError_t cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "_matrixTranspose failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
	//cudaSafeDeviceSynchronize();
}

__global__ void _arrayAdd(float* C, float* A, float* B, int n)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n)
		C[i] = A[i] + B[i];
}

__global__ void _arrayAddScalar(float* C, float* A, float b, int n)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n)
		C[i] = A[i] + b;
}

__global__ void _arrayAddRep(float* C, float* A, float* B, int n, int width)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n)
		C[i] = A[i] + B[i % width];
}

__global__ void _arraySubtract(float* C, float* A, float* B, int n)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n)
		C[i] = A[i] - B[i];
}

__global__ void _arrayMultiply(float* C, float* A, float* B, int n)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n)
		C[i] = A[i] * B[i];
}

__global__ void _arrayMultiplyScalar(float* B, float* A, float b, int n)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n)
		B[i] = A[i] * b;
}

__global__ void _arrayDivide(float* C, float* A, float* B, int n)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n)
		C[i] = A[i] / B[i];
}

__global__ void _arraySigmoid(float* B, float* A, int n)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n)
		B[i] = 1.0f / (1.0f + expf(-A[i]));
}

__global__ void _arrayDerivSigmoid(float* B, float* A, int n)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n)
		B[i] = A[i] * (1.0f - A[i]);
}

__global__ void _arrayReLU(float* B, float* A, int n)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n)
		B[i] = fmaxf(0.0f, A[i]);
}

__global__ void _arrayDerivReLU(float* B, float* A, int n)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n)
		B[i] = A[i] > 0.0f ? 1.0f : 0.0f;
}

__global__ void _arraySqrt(float* B, float* A, int n)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n)
		B[i] = sqrtf(A[i]);
}

void arrayCopyToDevice(float* _A, float* A, int n)
{
	cudaError_t cudaStatus = cudaMemcpy(_A, A, sizeof(float) * n, cudaMemcpyHostToDevice);
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "cudaMemcpy failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
}

void arrayCopyToHost(float* A, float* _A, int n)
{
	cudaError_t cudaStatus = cudaMemcpy(A, _A, sizeof(float) * n, cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "cudaMemcpy failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
}

void arrayAdd(float* C, float* A, float* B, int n)
{
	int numBlocks = (n + blockSize - 1) / blockSize;
	_arrayAdd<<<numBlocks, blockSize>>>(C, A, B, n);

	cudaError_t cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "_arrayAdd failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
	//cudaSafeDeviceSynchronize();
}

void arrayAdd(float* C, float* A, float b, int n)
{
	int numBlocks = (n + blockSize - 1) / blockSize;
	_arrayAddScalar<<<numBlocks, blockSize>>>(C, A, b, n);

	cudaError_t cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "_arrayAddScalar failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
	//cudaSafeDeviceSynchronize();
}

void arrayAddRep(float* C, float* A, float* B, int n, int m)
{
	int numBlocks = (n * m + blockSize - 1) / blockSize;
	_arrayAddRep<<<numBlocks, blockSize>>>(C, A, B, n * m, n);

	cudaError_t cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "_arrayAddRep failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
	//cudaSafeDeviceSynchronize();
}

void arraySubtract(float* C, float* A, float* B, int n)
{
	int numBlocks = (n + blockSize - 1) / blockSize;
	_arraySubtract<<<numBlocks, blockSize>>>(C, A, B, n);

	cudaError_t cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "_arraySubtract failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
	//cudaSafeDeviceSynchronize();
}

void arrayMultiply(float* C, float* A, float* B, int n)
{
	int numBlocks = (n + blockSize - 1) / blockSize;
	_arrayMultiply<<<numBlocks, blockSize>>>(C, A, B, n);

	cudaError_t cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "_arrayMultiply failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
	//cudaSafeDeviceSynchronize();
}

void arrayMultiply(float* C, float* A, float b, int n)
{
	int numBlocks = (n + blockSize - 1) / blockSize;
	_arrayMultiplyScalar<<<numBlocks, blockSize>>>(C, A, b, n);

	cudaError_t cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "_arrayMultiplyScalar failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
	//cudaSafeDeviceSynchronize();
}

void arrayDivide(float* C, float* A, float* B, int n)
{
	int numBlocks = (n + blockSize - 1) / blockSize;
	_arrayDivide<<<numBlocks, blockSize>>>(C, A, B, n);

	cudaError_t cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "_arrayDivide failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
	//cudaSafeDeviceSynchronize();
}

void arraySigmoid(float* B, float* A, int n)
{
	int numBlocks = (n + blockSize - 1) / blockSize;
	_arraySigmoid<<<numBlocks, blockSize>>>(B, A, n);

	cudaError_t cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "_arraySigmoid failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
	//cudaSafeDeviceSynchronize();
}

void arrayDerivSigmoid(float* B, float* A, int n)
{
	int numBlocks = (n + blockSize - 1) / blockSize;
	_arrayDerivSigmoid<<<numBlocks, blockSize>>>(B, A, n);

	cudaError_t cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "_arrayDerivSigmoid failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
	//cudaSafeDeviceSynchronize();
}

void arrayReLU(float* B, float* A, int n)
{
	int numBlocks = (n + blockSize - 1) / blockSize;
	_arrayReLU<<<numBlocks, blockSize>>>(B, A, n);

	cudaError_t cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "_arrayReLU failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
	//cudaSafeDeviceSynchronize();
}

void arrayDerivReLU(float* B, float* A, int n)
{
	int numBlocks = (n + blockSize - 1) / blockSize;
	_arrayDerivReLU<<<numBlocks, blockSize>>>(B, A, n);

	cudaError_t cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "_arrayDerivReLU failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
	//cudaSafeDeviceSynchronize();
}

void arraySqrt(float* B, float* A, int n)
{
	int numBlocks = (n + blockSize - 1) / blockSize;
	_arraySqrt<<<numBlocks, blockSize>>>(B, A, n);

	cudaError_t cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "_arraySqrt failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
	//cudaSafeDeviceSynchronize();
}

__global__ void _unpotato(float* A, int* B)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if ((i + 1) % 2500)
		A[i * 20 + B[i]] = 0.0f;
}

__global__ void _potato(float* A, float* B, bool* C, float* D, int* E)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if ((i + 1) % 2500)
	{
		// _d3, _X3, _action + 20 * offset, _reward + 2 * offset
		int j;
		float q = 0.0f;
#pragma unroll
		for (int k = 0; k < 20; ++k)
		{
			if (C[i * 20 + k] && B[(i + 1) * 20 + k] > q)
			{
				j = k;
				q = B[(i + 1) * 20 + k];
			}
		}
		float x = B[i * 20 + E[i]];
		A[i * 20 + E[i]] = (x - (D[i * 2] + D[i * 2 + 1] * q)) * x * (1.0f - x);
	}
}

void unpotato(float* A, int* B)
{
	_unpotato<<<10000 / 1000, 1000>>>(A, B);

	cudaError_t cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "_unpotato failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
}

void potato(float* A, float* B, bool* C, float* D, int* E)
{
	_potato<<<10000 / 1000, 1000>>>(A, B, C, D, E);

	cudaError_t cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "_potato failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
}
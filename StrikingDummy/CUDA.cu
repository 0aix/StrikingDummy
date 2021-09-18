#include "CUDA.cuh"
#include <iostream>
#include <chrono>
#include <math.h>
#include <curand.h>
#include <cublas_v2.h>

#define NUM_ACTIONS 21

int blockSize = 0;
bool cuda_init = false;
curandGenerator_t gen;
cublasHandle_t handle;

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

		cublasStatus_t cublasStatus = cublasCreate(&handle);
		if (cublasStatus != CUBLAS_STATUS_SUCCESS)
		{
			std::cerr << "cublasCreate failed" << std::endl;
			throw 0;
		}

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

	curandStatus_t status = curandGenerateUniform(gen, *A, (size_t)n * (size_t)m);
	if (status != CURAND_STATUS_SUCCESS)
	{
		std::cerr << "curandGenerateUniform failed" << std::endl;
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

void matrixInitialize(float* A, int n, int m, float r)
{
	curandStatus_t status = curandGenerateUniform(gen, A, (size_t)n * (size_t)m);
	if (status != CURAND_STATUS_SUCCESS)
	{
		std::cerr << "curandGenerateUniform failed" << std::endl;
		throw 0;
	}

	int numBlocks = (n * m + blockSize - 1) / blockSize;
	_matrixInitialize<<<numBlocks, blockSize>>>(A, n * m, r);

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

	float a = 1.0f;
	float b = 0.0f;

	cublasSgemm(handle, CUBLAS_OP_N, CUBLAS_OP_N, n0, m1, m0, &a, A, n0, B, n1, &b, C, n0);
}

void matrixMultiplyTranspose(float* C, float* A, int n0, int m0, float* B, int n1, int m1)
{
	if (m0 != n1)
		throw 0;

	float a = 1.0f;
	float b = 0.0f;

	cublasSgemm(handle, CUBLAS_OP_N, CUBLAS_OP_T, n0, m1, m0, &a, A, n0, B, m1, &b, C, n0);
}

void matrixTransposeMultiply(float* C, float* A, int n0, int m0, float* B, int n1, int m1)
{
	if (m0 != n1)
		throw 0;

	float a = 1.0f;
	float b = 0.0f;

	cublasSgemm(handle, CUBLAS_OP_T, CUBLAS_OP_N, n0, m1, m0, &a, A, m0, B, n1, &b, C, n0);
}

__global__ void _arrayInitialize(float* A, int n, float r)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n)
		A[i] = r;
}

void arrayInitialize(float** A, int n, float r)
{
	cudaError_t cudaStatus = cudaMalloc(A, sizeof(float) * n);
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "cudaMalloc failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}

	int numBlocks = (n + blockSize - 1) / blockSize;
	_arrayInitialize<<<numBlocks, blockSize>>>(*A, n, r);
}

__global__ void _arrayAddRep(float* C, float* A, float* B, int n, int width)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n)
		C[i] = A[i] + B[i % width];
}

__global__ void _arrayAddRepSigmoid(float* C, float* A, float* B, int n, int width)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n)
		C[i] = 1.0f / (1.0f + expf(-(A[i] + B[i % width])));
}

__global__ void _arrayMultiplyDerivSigmoid(float* C, float* A, float* B, int n)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n)
		C[i] = A[i] * B[i] * (1.0f - B[i]);
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
	//cudaError_t cudaStatus = cudaMemcpy(A, _A, sizeof(float) * n, cudaMemcpyDeviceToHost);
	cudaError_t cudaStatus = cudaMemcpyAsync(A, _A, sizeof(float) * n, cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess)
	{
		std::cerr << "cudaMemcpy failed: " << cudaGetErrorString(cudaStatus) << std::endl;
		throw 0;
	}
}

void arrayAddRep(float* C, float* A, float* B, int n, int m)
{
	int numBlocks = (n * m + blockSize - 1) / blockSize;
	_arrayAddRep<<<numBlocks, blockSize>>>(C, A, B, n * m, n);
}

void arrayAddRepSigmoid(float* C, float* A, float* B, int n, int m)
{
	int numBlocks = (n * m + blockSize - 1) / blockSize;
	_arrayAddRepSigmoid<<<numBlocks, blockSize>>>(C, A, B, n * m, n);
}

void arrayMultiplyDerivSigmoid(float* C, float* A, float* B, int n)
{
	int numBlocks = (n + blockSize - 1) / blockSize;
	_arrayMultiplyDerivSigmoid<<<numBlocks, blockSize>>>(C, A, B, n);
}

void arrayAbsSum(float* A, int n, float* out)
{
	cublasSasum(handle, n, A, 1, out);
}

void arrayStep(float* B, float* A, float nu, int n)
{
	cublasSaxpy(handle, n, &nu, A, 1, B, 1);
}

__global__ void _adamStep(float* A, float* B, float BETA1, float BETA2, int n)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	float dldw = A[i];
	float dldwm = (1.0f - BETA1) * dldw + BETA1 * B[i * 2];
	float dldwv = (1.0f - BETA2) * dldw * dldw + BETA2 * B[i * 2 + 1];
	B[i * 2] = dldwm;
	B[i * 2 + 1] = dldwv;
	A[i] = dldwm / (sqrtf(dldwv) + 1e-8);
}

void adamStep(float* A, float* B, float BETA1, float BETA2, int n)
{
	int numBlocks = (n + blockSize - 1) / blockSize;
	_adamStep<<<numBlocks, blockSize>>>(A, B, BETA1, BETA2, n);
}

__global__ void _potato(float* A, float* B, unsigned char* C, float* D, int* E)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if ((i + 1) % 2500)
	{
		// _d3, _X3, _action + NUM_ACTIONS * offset, _reward + 2 * offset
		unsigned int m = i * NUM_ACTIONS;
		unsigned int n = m + NUM_ACTIONS;
		float q = 0.0f;
		int k = 0;
		unsigned char a;
		while ((a = C[m + k++]) < NUM_ACTIONS)
			if (B[n + a] > q)
				q = B[n + a];
		float x = B[m + E[i]];
		A[m + E[i]] = (x - (D[i * 2] + D[i * 2 + 1] * q)) * x * (1.0f - x);
	}
}

__global__ void _unpotato(float* A, int* B)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if ((i + 1) % 2500)
		A[i * NUM_ACTIONS + B[i]] = 0.0f;
}

void potato(float* A, float* B, unsigned char* C, float* D, int* E, int n)
{
	if (n % 1000)
		throw 0;
	_potato<<<n / 1000, 1000>>>(A, B, C, D, E);
}

void unpotato(float* A, int* B, int n)
{
	if (n % 1000)
		throw 0;
	_unpotato<<<n / 1000, 1000>>>(A, B);
}
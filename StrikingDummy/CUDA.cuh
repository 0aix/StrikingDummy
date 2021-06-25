#pragma once
#include <cuda_runtime.h>
#include "device_launch_parameters.h"

void cudaSafeDeviceSynchronize();

void cudaInitialize();

void cudaSafeMalloc(void** A, int n);

void cudaSafeFree(void** A);

void cudaCopyToDevice(void* _A, void* A, int n);

void matrixInitialize(float** A, int n, int m);

void matrixInitialize(float** A, int n, int m, float r);

void matrixInitialize(float* A, int n, int m, float r);

void matrixFree(float** A);

void matrixMultiply(float* C, float* A, int n0, int m0, float* B, int n1, int m1);

void matrixMultiplyTranspose(float* C, float* A, int n0, int m0, float* B, int n1, int m1);

void matrixTransposeMultiply(float* C, float* A, int n0, int m0, float* B, int n1, int m1);

void arrayInitialize(float** A, int n, float r);

void arrayCopyToDevice(float* _A, float* A, int n);

void arrayCopyToHost(float* A, float* _A, int n);

void arrayAddRepSigmoid(float* C, float* A, float* B, int n, int m);

void arrayMultiplyDerivSigmoid(float* C, float* A, float* B, int n);

void arrayStep(float* B, float* A, float nu, int n);

void unpotato(float* A, int* B, int n);

void potato(float* A, float* B, unsigned char* C, float* D, int* E, int n);

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

void matrixFree(float** A);

void matrixMultiply(float* C, float* A, int n0, int m0, float* B, int n1, int m1);

void matrixMultiplyTranspose(float* C, float* A, int n0, int m0, float* B, int n1, int m1);

void matrixTranspose(float* B, float* A, int n, int m);

void arrayCopyToDevice(float* _A, float* A, int n);

void arrayCopyToHost(float* A, float* _A, int n);

void arrayAdd(float* C, float* A, float* B, int n);

void arrayAdd(float* C, float* A, float b, int n);

void arrayAddRep(float* C, float* A, float* B, int n, int m);

void arraySubtract(float* C, float* A, float* B, int n);

void arrayMultiply(float* C, float* A, float* B, int n);

void arrayMultiply(float* C, float* A, float b, int n);

void arrayDivide(float* C, float* A, float* B, int n);

void arraySigmoid(float* B, float* A, int n);

void arrayDerivSigmoid(float* B, float* A, int n);

void arrayReLU(float* B, float* A, int n);

void arrayDerivReLU(float* B, float* A, int n);

void arraySqrt(float* B, float* A, int n);

void unpotato(float* A, int* B);

void potato(float* A, float* B, bool* C, float* D, int* E);
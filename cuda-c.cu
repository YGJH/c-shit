#include <cuda_runtime.h>
#include <iostream>

// CUDA 核函數，負責在 GPU 上執行數據處理
__global__ void kernelMultiply(int* data, int size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        data[idx] *= 2;
    }
}

int main() {
    const int size = 1024;
    const int bytes = size * sizeof(int);
    
    // 主機內存分配並初始化數據
    int* h_data = new int[size];
    for (int i = 0; i < size; ++i) {
        h_data[i] = i;
    }

    // 設備內存分配
    int* d_data;
    cudaError_t err = cudaMalloc((void**)&d_data, bytes);
    if (err != cudaSuccess) {
        std::cerr << "cudaMalloc failed: " << cudaGetErrorString(err) << std::endl;
        return -1;
    }

    // 將數據從主機複製到設備
    cudaMemcpy(d_data, h_data, bytes, cudaMemcpyHostToDevice);

    // 定義核函數的執行配置
    int blockSize = 256;
    int gridSize = (size + blockSize - 1) / blockSize;
    kernelMultiply<<<gridSize, blockSize>>>(d_data, size);

    // 同步等待 GPU 完成任務
    cudaDeviceSynchronize();

    // 將結果從設備複製回主機
    cudaMemcpy(h_data, d_data, bytes, cudaMemcpyDeviceToHost);

    // 驗證結果（此處僅作簡單輸出）
    for (int i = 0; i < 10; ++i) {
        std::cout << "h_data[" << i << "] = " << h_data[i] << std::endl;
    }

    // 清理資源
    cudaFree(d_data);
    delete[] h_data;
    
    return 0;
}

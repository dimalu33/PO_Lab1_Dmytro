#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>

struct ExecutionResult {
    int numThreads;
    int matrixSize;
    double executionTime;
};

std::vector<std::vector<double>> generateRandomMatrix(int size) {
    std::vector<std::vector<double>> matrix(size, std::vector<double>(size));
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(1.0, 100.0);

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            matrix[i][j] = dis(gen);
        }
    }
    return matrix;
}

std::vector<std::vector<double>> processMatrixSequential(const std::vector<std::vector<double>>& originalMatrix) {
    int size = originalMatrix.size();
    std::vector<std::vector<double>> resultMatrix = originalMatrix;

    for (int i = 0; i < size; i++) {
        double maxElement = resultMatrix[i][0];
        int maxIndex = 0;
        for (int j = 1; j < size; j++) {
            if (resultMatrix[i][j] > maxElement) {
                maxElement = resultMatrix[i][j];
                maxIndex = j;
            }
        }

        if (maxIndex != i) {
            std::swap(resultMatrix[i][maxIndex], resultMatrix[i][i]);
        }
    }

    return resultMatrix;
}

void processRowsRange(const std::vector<std::vector<double>>& originalMatrix,
                      std::vector<std::vector<double>>& resultMatrix,
                      int startRow, int endRow) {
    for (int i = startRow; i < endRow; i++) {
        resultMatrix[i] = originalMatrix[i];

        double maxElement = resultMatrix[i][0];
        int maxIndex = 0;
        for (int j = 1; j < originalMatrix.size(); j++) {
            if (resultMatrix[i][j] > maxElement) {
                maxElement = resultMatrix[i][j];
                maxIndex = j;
            }
        }

        if (maxIndex != i) {
            std::swap(resultMatrix[i][maxIndex], resultMatrix[i][i]);
        }
    }
}

std::vector<std::vector<double>> processMatrixParallel(const std::vector<std::vector<double>>& originalMatrix, int numThreads) {
    int size = originalMatrix.size();
    std::vector<std::vector<double>> resultMatrix(size, std::vector<double>(size));
    std::vector<std::thread> threads;

    int rowsPerThread = size / numThreads;
    int remainingRows = size % numThreads;

    int startRow = 0;
    for (int i = 0; i < numThreads; i++) {
        int threadRows = rowsPerThread + (i < remainingRows ? 1 : 0);
        int endRow = startRow + threadRows;

        threads.push_back(std::thread(processRowsRange,
                                      std::ref(originalMatrix),
                                      std::ref(resultMatrix),
                                      startRow, endRow));
        startRow = endRow;
    }

    for (auto& thread : threads) {
        thread.join();
    }

    return resultMatrix;
}

double measureExecutionTime(const std::vector<std::vector<double>>& matrix, bool isParallel, int numThreads = 1) {
    auto start = std::chrono::high_resolution_clock::now();

    if (isParallel) {
        processMatrixParallel(matrix, numThreads);
    } else {
        processMatrixSequential(matrix);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;

    return duration.count();
}

int main() {
    int maxHardwareThreads = std::thread::hardware_concurrency();
    int physicalCores = maxHardwareThreads / 2;

    std::cout << "System information:" << std::endl;
    std::cout << "Total logical cores: " << maxHardwareThreads << std::endl;
    std::cout << "Estimated physical cores: " << physicalCores << std::endl;
    std::cout << std::endl;

    std::vector<int> matrixSizes = {100, 500, 1000, 2000};

    std::vector<int> threadCounts = {
            physicalCores / 2,
            physicalCores,
            maxHardwareThreads,
            maxHardwareThreads * 2,
            maxHardwareThreads * 4,
            maxHardwareThreads * 8,
            maxHardwareThreads * 16
    };

    std::vector<ExecutionResult> results;

    for (int size : matrixSizes) {
        std::cout << "Testing matrix size: " << size << "x" << size << std::endl;

        auto matrix = generateRandomMatrix(size);

        double seqTime = measureExecutionTime(matrix, false);
        results.push_back({1, size, seqTime});
        std::cout << "Sequential execution time: " << seqTime << " ms" << std::endl;

        for (int threads : threadCounts) {
            double parTime = measureExecutionTime(matrix, true, threads);
            results.push_back({threads, size, parTime});
            std::cout << "Parallel execution time (" << threads << " threads): " << parTime << " ms" << std::endl;
        }

        std::cout << std::endl;
    }

    return 0;
}
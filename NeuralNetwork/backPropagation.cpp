#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <random>
// Sigmoid 函數
double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

// Sigmoid 函數的導數
double sigmoid_derivative(double sigmoid_output) {
    return sigmoid_output * (1.0 - sigmoid_output);
}

class Perceptron {
public:
    Perceptron(int input_size) : weights(input_size), bias(0.0), learning_rate(0.1) {
        std::mt19937 rng{ std::random_device{}() }; // 亂數生成器
        // 初始化權重為隨機小數
        std::uniform_real_distribution<double> dist(-1.0, 1.0);
        for (auto& weight : weights) {
            weight = static_cast<double>(dist(rng)) / 1.0;
        }
    }

    // 前向傳播
    double forward(const std::vector<double>& inputs) {
        double sum = bias;
        for (size_t i = 0; i < inputs.size(); ++i) {
            sum += weights[i] * inputs[i];
        }
        output = sigmoid(sum);
        return output;
    }

    // 反向傳播
    void backward(const std::vector<double>& inputs, double target) {
        double error = target - output;
        double delta = error * sigmoid_derivative(output);

        // 更新權重和偏置
        for (size_t i = 0; i < weights.size(); ++i) {
            weights[i] += learning_rate * delta * inputs[i];
        }
        bias += learning_rate * delta;
    }

private:
    std::vector<double> weights;
    double bias;
    double learning_rate;
    double output;
};

int main() {
    // 假設我們有兩個輸入的感知器
    Perceptron perceptron(100);

    // 訓練資料：邏輯與（AND）操作
    std::vector<std::vector<double>> training_inputs = {
        {0.0, 0.0},
        {0.0, 1.0},
        {1.0, 0.0},
        {1.0, 1.0}
    };
    std::vector<double> training_outputs = { 0.0, 0.0, 0.0, 1.0 };

    // 訓練迭代
    for (int epoch = 0; epoch < 100000; ++epoch) {
        for (size_t i = 0; i < training_inputs.size(); ++i) {
            perceptron.forward(training_inputs[i]);
            perceptron.backward(training_inputs[i], training_outputs[i]);
        }
    }

    // 測試
    for (const auto& inputs : training_inputs) {
        double output = perceptron.forward(inputs);
        std::cout << inputs[0] << " AND " << inputs[1] << " = " << output << std::endl;
    }

    return 0;
}

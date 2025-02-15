#include <vector>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <random>
class NeuralNetwork {
public:
    NeuralNetwork(const std::vector<int>& layers);
    std::vector<double> forward(const std::vector<double>& input);
    void backward(const std::vector<double>& target);
    void updateWeights(double learning_rate);

private:
    std::vector<int> layers;
    std::vector<std::vector<std::vector<double>>> weights;
    std::vector<std::vector<double>> biases;
    std::vector<std::vector<double>> activations;
    std::vector<std::vector<double>> deltas;

    double sigmoid(double x);
    double sigmoidDerivative(double x);
};

NeuralNetwork::NeuralNetwork(const std::vector<int>& layers) : layers(layers) {
    std::mt19937 rng{ std::random_device{}() };
    const int MaxRand = 1000;
    activations.push_back( std::vector<double>(layers[0]));
    std::uniform_real_distribution<double> dist(-MaxRand, MaxRand);
    double stddev;
    for (size_t i = 1; i < layers.size(); ++i) {
        weights.push_back(std::vector<std::vector<double>>(layers[i], std::vector<double>(layers[i - 1])));
        biases.push_back(std::vector<double>(layers[i]));
        activations.push_back(std::vector<double>(layers[i]));
        deltas.push_back(std::vector<double>(layers[i]));
        
        stddev = sqrt(2.0 / layers[i - 1]); // He Initialization
        for (int j = 0; j < layers[i]; ++j) {
            for (int k = 0; k < layers[i - 1]; ++k) {
                weights[i - 1][j][k] = stddev * static_cast<double>(dist(rng)) / MaxRand;
            }
            biases[i - 1][j] = static_cast<double>(dist(rng)) / MaxRand;
        }
    }
}

std::vector<double> NeuralNetwork::forward(const std::vector<double>& input) {
    activations[0] = input;
    for (size_t i = 1; i < layers.size(); ++i) {
        for (int j = 0; j < layers[i]; ++j) {
            double z = biases[i - 1][j];
            for (int k = 0; k < layers[i - 1]; ++k) {
                z += weights[i - 1][j][k] * activations[i - 1][k];
            }
            activations[i][j] = sigmoid(z);
        }
    }
    return activations.back();
}

void NeuralNetwork::backward(const std::vector<double>& target) {
    size_t last = layers.size() - 1;
    for (int i = 0; i < layers[last]; ++i) {
        double a = activations[last][i];
        deltas[last - 1][i] = (a - target[i]) * sigmoidDerivative(a);
    }

    for (int i = layers.size() - 2; i > 0; --i) {
        for (int j = 0; j < layers[i]; ++j) {
            double delta_sum = 0.0;
            for (int k = 0; k < layers[i + 1]; ++k) {
                delta_sum += weights[i][k][j] * deltas[i][k];
            }
            deltas[i - 1][j] = delta_sum * sigmoidDerivative(activations[i][j]);
        }
    }
}

void NeuralNetwork::updateWeights(double learning_rate) {
    for (size_t i = 1; i < layers.size(); ++i) {
        for (int j = 0; j < layers[i]; ++j) {
            for (int k = 0; k < layers[i - 1]; ++k) {
                weights[i - 1][j][k] -= learning_rate * deltas[i - 1][j] * activations[i - 1][k];
            }
            biases[i - 1][j] -= learning_rate * deltas[i - 1][j];
        }
    }
}

double NeuralNetwork::sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

double NeuralNetwork::sigmoidDerivative(double x) {
    return x * (1.0 - x);
}

int main() {
    std::vector<int> layers = {3, 5, 4, 2};
    NeuralNetwork nn(layers);

    std::vector<std::vector<double>> inputs = {
        {0.1, 0.2, 0.3},
        {0.4, 0.5, 0.6},
        {0.7, 0.8, 0.9}
    };
    std::vector<std::vector<double>> targets ;


    double learning_rate = 0.1;
    int epochs = 1000;

    for (int epoch = 0; epoch < epochs; ++epoch) {
        for (size_t i = 0; i < inputs.size(); ++i) {
            nn.forward(inputs[i]);
            nn.backward(targets[i]);
            nn.updateWeights(learning_rate);
        }
    }

    // std::vector<double> test_input = {0.4, 0.5, 0.6};
    // std::vector<double> output = nn.forward(test_input);
    // std::cout << "Output: ";
    // for (double val : output) {
    //     std::cout << val << " ";
    // }
    return 0;
}

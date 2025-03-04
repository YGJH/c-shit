#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

using namespace std;

// 激活函數：Sigmoid
double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

// Sigmoid 的導數
double sigmoidDerivative(double x) {
    double s = sigmoid(x);
    return s * (1 - s);
}

class DNN {
public:
    int input_size;
    int hidden_size;
    int output_size;
    // 權重與偏置（分別對於輸入-隱藏層和隱藏層-輸出層）
    vector<vector<double>> weights_input_hidden;
    vector<double> bias_hidden;
    vector<vector<double>> weights_hidden_output;
    vector<double> bias_output;

    DNN(int in, int hidden, int out)
        : input_size(in), hidden_size(hidden), output_size(out) {
        // 隨機初始化權重與偏置（範圍在 -1 到 1）
        srand(static_cast<unsigned>(time(0)));
        
        weights_input_hidden.resize(input_size, vector<double>(hidden_size));
        bias_hidden.resize(hidden_size);
        for (int i = 0; i < input_size; i++) {
            for (int j = 0; j < hidden_size; j++) {
                weights_input_hidden[i][j] = ((double)rand() / RAND_MAX) * 2 - 1;
            }
        }
        for (int j = 0; j < hidden_size; j++) {
            bias_hidden[j] = ((double)rand() / RAND_MAX) * 2 - 1;
        }
        
        weights_hidden_output.resize(hidden_size, vector<double>(output_size));
        bias_output.resize(output_size);
        for (int i = 0; i < hidden_size; i++) {
            for (int j = 0; j < output_size; j++) {
                weights_hidden_output[i][j] = ((double)rand() / RAND_MAX) * 2 - 1;
            }
        }
        for (int j = 0; j < output_size; j++) {
            bias_output[j] = ((double)rand() / RAND_MAX) * 2 - 1;
        }
    }

    // 前向傳播函數
    vector<double> forward(const vector<double>& input) {
        // 隱藏層輸出計算
        vector<double> hidden(hidden_size, 0.0);
        for (int j = 0; j < hidden_size; j++) {
            double sum = bias_hidden[j];
            for (int i = 0; i < input_size; i++) {
                sum += input[i] * weights_input_hidden[i][j];
            }
            hidden[j] = sigmoid(sum);
        }
        // 輸出層計算
        vector<double> output(output_size, 0.0);
        for (int k = 0; k < output_size; k++) {
            double sum = bias_output[k];
            for (int j = 0; j < hidden_size; j++) {
                sum += hidden[j] * weights_hidden_output[j][k];
            }
            output[k] = sigmoid(sum);
        }
        return output;
    }

    // 單個樣本的訓練，使用簡單的梯度下降進行反向傳播
    void train(const vector<double>& input, const vector<double>& target, double learning_rate) {
        // --- 前向傳播 ---
        vector<double> hidden(hidden_size, 0.0);
        vector<double> hidden_input(hidden_size, 0.0);  // 保存隱藏層加權和
        for (int j = 0; j < hidden_size; j++) {
            double sum = bias_hidden[j];
            for (int i = 0; i < input_size; i++) {
                sum += input[i] * weights_input_hidden[i][j];
            }
            hidden_input[j] = sum;
            hidden[j] = sigmoid(sum);
        }
        
        vector<double> output(output_size, 0.0);
        vector<double> output_input(output_size, 0.0);  // 保存輸出層加權和
        for (int k = 0; k < output_size; k++) {
            double sum = bias_output[k];
            for (int j = 0; j < hidden_size; j++) {
                sum += hidden[j] * weights_hidden_output[j][k];
            }
            output_input[k] = sum;
            output[k] = sigmoid(sum);
        }
        
        // --- 反向傳播 ---
        // 輸出層誤差與梯度
        vector<double> output_error(output_size, 0.0);
        for (int k = 0; k < output_size; k++) {
            double error = target[k] - output[k];
            output_error[k] = error * sigmoidDerivative(output_input[k]);
        }
        
        // 隱藏層誤差與梯度
        vector<double> hidden_error(hidden_size, 0.0);
        for (int j = 0; j < hidden_size; j++) {
            double error = 0.0;
            for (int k = 0; k < output_size; k++) {
                error += output_error[k] * weights_hidden_output[j][k];
            }
            hidden_error[j] = error * sigmoidDerivative(hidden_input[j]);
        }
        
        // 更新隱藏層到輸出層的權重和偏置
        for (int j = 0; j < hidden_size; j++) {
            for (int k = 0; k < output_size; k++) {
                weights_hidden_output[j][k] += learning_rate * output_error[k] * hidden[j];
            }
        }
        for (int k = 0; k < output_size; k++) {
            bias_output[k] += learning_rate * output_error[k];
        }
        
        // 更新輸入層到隱藏層的權重和偏置
        for (int i = 0; i < input_size; i++) {
            for (int j = 0; j < hidden_size; j++) {
                weights_input_hidden[i][j] += learning_rate * hidden_error[j] * input[i];
            }
        }
        for (int j = 0; j < hidden_size; j++) {
            bias_hidden[j] += learning_rate * hidden_error[j];
        }
    }
};

int main() {
    // 以 XOR 問題作為示例：輸入為兩個二元數字，輸出應該為 XOR 結果
    DNN dnn(2, 2, 1);
    vector<vector<double>> inputs = { {0, 0}, {0, 1}, {1, 0}, {1, 1} };
    vector<vector<double>> targets = { {0}, {1}, {1}, {0} };

    int epochs = 10000;
    double learning_rate = 0.1;
    for (int epoch = 0; epoch < epochs; epoch++) {
        for (size_t i = 0; i < inputs.size(); i++) {
            dnn.train(inputs[i], targets[i], learning_rate);
        }
        if (epoch % 1000 == 0) {
            cout << "Epoch: " << epoch << endl;
        }
    }

    // 測試訓練後的網絡
    for (size_t i = 0; i < inputs.size(); i++) {
        vector<double> output = dnn.forward(inputs[i]);
        cout << "Input: " << inputs[i][0] << ", " << inputs[i][1] 
             << " -> Output: " << output[0] << endl;
    }
    return 0;
}

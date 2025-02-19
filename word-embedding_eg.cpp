#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>

using namespace std;

// 參數設定
const int EMBEDDING_DIM = 5;       // 嵌入向量的維度
const double LEARNING_RATE = 0.025;
const int WINDOW_SIZE = 2;         // 上下文窗口大小
const int NEGATIVE_SAMPLES = 5;    // 負採樣數量
const int EPOCHS = 10;             // 訓練迭代次數

// Sigmoid 函數
double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

// 計算兩個向量的內積
double dotProduct(const vector<double>& v1, const vector<double>& v2) {
    double dot = 0.0;
    for (int i = 0; i < v1.size(); i++) {
        dot += v1[i] * v2[i];
    }
    return dot;
}

// 產生 [min, max] 區間內的隨機浮點數
double randomDouble(double min, double max) {
    return min + (max - min) * (rand() / (RAND_MAX + 1.0));
}

int main() {
    srand((unsigned int) time(0));

    // 簡單語料庫，每個字串代表一個句子
    vector<string> corpus ;
    ifstream fin("embeddings.txt");
    if (!fin) {
        cerr << "Failed to open file embeddings.txt" << endl;
        return 1;
    }
    string line;
    while(getline(fin, line)){
        corpus.push_back(line);
    }
    fin.close();
    // 建立字彙表與句子的數值表示
    unordered_map<string, int> word2id;
    vector<string> id2word;
    vector<vector<int>> sentences;
    int wordIndex = 0;

    for (const auto& line : corpus) {
        istringstream iss(line);
        string word;
        vector<int> sentence;
        while (iss >> word) {
            string wword;
            for(int i = 0 ; i < word.length() ; i++) {
                if(word[i] >= 'A' && word[i] <= 'Z') {
                    wword.push_back(word[i] - 'A' + 'a');
                }
                else if(word[i] >= 'a' && word[i] <= 'z') {
                    wword.push_back(word[i]);
                }
                else {
                    continue;
                }
            }
            if(wword.length() == 0) {
                continue;
            }
            if (word2id.find(wword) == word2id.end()) {
                word2id[wword] = wordIndex;
                id2word.push_back(wword);
                wordIndex++;
            }
            sentence.push_back(word2id[wword]);
        }
        sentences.push_back(sentence);
    }
    int vocabSize = word2id.size();
    cout << "Vocabulary size: " << vocabSize << endl;

    // 初始化輸入與輸出嵌入矩陣
    vector<vector<double>> inputEmbeddings(vocabSize, vector<double>(EMBEDDING_DIM));
    vector<vector<double>> outputEmbeddings(vocabSize, vector<double>(EMBEDDING_DIM));
    double initRange = 0.5 / EMBEDDING_DIM;
    for (int i = 0; i < vocabSize; i++) {
        for (int d = 0; d < EMBEDDING_DIM; d++) {
            inputEmbeddings[i][d] = randomDouble(-initRange, initRange);
            outputEmbeddings[i][d] = randomDouble(-initRange, initRange);
        }
    }

    // 訓練迴圈
    for (int epoch = 0; epoch < EPOCHS; epoch++) {
        // 對每個句子
        for (const auto& sentence : sentences) {
            int sentenceLength = sentence.size();
            // 對句子中的每個中心單詞
            for (int pos = 0; pos < sentenceLength; pos++) {
                int centerWord = sentence[pos];
                // 決定上下文窗口範圍
                int start = max(0, pos - WINDOW_SIZE);
                int end = min(sentenceLength, pos + WINDOW_SIZE + 1);
                // 對窗口內的每個上下文單詞
                for (int ctx = start; ctx < end; ctx++) {
                    if (ctx == pos) continue; // 跳過中心單詞本身
                    int contextWord = sentence[ctx];

                    // 正樣本更新
                    double dot = dotProduct(inputEmbeddings[centerWord], outputEmbeddings[contextWord]);
                    double pred = sigmoid(dot);
                    double grad = LEARNING_RATE * (1 - pred); // 真實標籤為 1
                    for (int d = 0; d < EMBEDDING_DIM; d++) {
                        double inputTemp = inputEmbeddings[centerWord][d];
                        inputEmbeddings[centerWord][d] += grad * outputEmbeddings[contextWord][d];
                        outputEmbeddings[contextWord][d] += grad * inputTemp;
                    }

                    // 負樣本更新
                    for (int n = 0; n < NEGATIVE_SAMPLES; n++) {
                        // 隨機選取一個負樣本（避免與正樣本重複）
                        int negativeWord;
                        do {
                            negativeWord = rand() % vocabSize;
                        } while (negativeWord == contextWord);

                        double dotNeg = dotProduct(inputEmbeddings[centerWord], outputEmbeddings[negativeWord]);
                        double predNeg = sigmoid(dotNeg);
                        double gradNeg = LEARNING_RATE * (0 - predNeg); // 負樣本的標籤為 0
                        for (int d = 0; d < EMBEDDING_DIM; d++) {
                            double inputTemp = inputEmbeddings[centerWord][d];
                            inputEmbeddings[centerWord][d] += gradNeg * outputEmbeddings[negativeWord][d];
                            outputEmbeddings[negativeWord][d] += gradNeg * inputTemp;
                        }
                    }
                }
            }
        }
        cout << "Epoch " << epoch + 1 << " completed." << endl;
    }

    // 印出每個單詞的嵌入向量（使用輸入嵌入）
    cout << "\nWord Embeddings:" << endl;
    for (int i = 0; i < vocabSize; i++) {
        cout << id2word[i] << ": ";
        for (double val : inputEmbeddings[i]) {
            cout << val << " ";
        }
        cout << endl;
    }

    return 0;
}

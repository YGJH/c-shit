#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <locale>

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

// 簡單的中文或英文斷詞函數
// 若句子中含有空白，則以空白分割；否則每個字當作一個詞
vector<wstring> segmentSentence(const wstring &sentence) {
    vector<wstring> tokens;
    if (sentence.find(L' ') != wstring::npos) {
        // 句子中含有空白，以空白作為分隔符
        wistringstream wiss(sentence);
        wstring token;
        while (wiss >> token) {
            tokens.push_back(token);
        }
    } else {
        // 沒有空白，將每個中文字視為一個詞
        for (wchar_t ch : sentence) {
            // 忽略空白或標點（可依需求擴充）
            if (!iswspace(ch))
                tokens.push_back(wstring(1, ch));
        }
    }
    return tokens;
}

int main() {
    // 設定 locale（根據系統環境設定合適的 locale）
    locale::global(locale("zh_TW.UTF-8"));
    wcout.imbue(locale());

    srand((unsigned int) time(0));

    // 簡單語料庫，包含中文句子與已斷詞的英文句子皆可
    vector<wstring> corpus = {
        L"今天天氣很好",             // 中文句子（未斷詞，每個字視為詞）
        L"我愛自然語言處理",         // 中文句子
        L"this is a sample sentence", // 英文句子（以空白分割）
        L"another example for embedding"  // 英文句子
    };

    // 建立字彙表與句子的數值表示
    unordered_map<wstring, int> word2id;
    vector<wstring> id2word;
    vector<vector<int>> sentences;
    int wordIndex = 0;

    for (const auto& line : corpus) {
        vector<wstring> tokens = segmentSentence(line);
        vector<int> sentence;
        for (const auto& word : tokens) {
            if (word2id.find(word) == word2id.end()) {
                word2id[word] = wordIndex;
                id2word.push_back(word);
                wordIndex++;
            }
            sentence.push_back(word2id[word]);
        }
        sentences.push_back(sentence);
    }
    int vocabSize = word2id.size();
    wcout << L"Vocabulary size: " << vocabSize << L"\n";

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
            // 對句子中的每個中心詞
            for (int pos = 0; pos < sentenceLength; pos++) {
                int centerWord = sentence[pos];
                // 決定上下文窗口範圍
                int start = max(0, pos - WINDOW_SIZE);
                int end = min(sentenceLength, pos + WINDOW_SIZE + 1);
                // 對窗口內的每個上下文詞
                for (int ctx = start; ctx < end; ctx++) {
                    if (ctx == pos) continue; // 跳過中心詞本身
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
        wcout << L"Epoch " << epoch + 1 << L" completed.\n";
    }

    // 印出每個詞的嵌入向量（使用輸入嵌入）
    wcout << L"\nWord Embeddings:\n";
    for (int i = 0; i < vocabSize; i++) {
        wcout << id2word[i] << L": ";
        for (double val : inputEmbeddings[i]) {
            wcout << val << L" ";
        }
        wcout << L"\n";
    }

    return 0;
}

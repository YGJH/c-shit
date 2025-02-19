#include <iostream>
#include <vector>
#include <complex>
#include <cmath>
using namespace std;

using cd = complex<double>;
const double PI = acos(-1.0);

// FFT（Cooley-Tukey 算法）
// 如果 invert 為 false，計算 DFT；如果為 true，計算逆 DFT
void fft(vector<cd>& a, bool invert) {
    int n = a.size();
    if (n == 1)
        return;

    vector<cd> a0(n / 2), a1(n / 2);
    for (int i = 0; i < n / 2; i++) {
        a0[i] = a[i*2];
        a1[i] = a[i*2 + 1];
    }
    fft(a0, invert);
    fft(a1, invert);

    double ang = 2 * PI / n * (invert ? -1 : 1);
    cd w(1), wn(cos(ang), sin(ang));
    for (int i = 0; i < n / 2; i++) {
        a[i] = a0[i] + w * a1[i];
        a[i + n/2] = a0[i] - w * a1[i];
        if (invert) {
            a[i] /= 2;
            a[i + n/2] /= 2;
        }
        w *= wn;
    }
}

// 多項式乘法：傳入兩個多項式係數，返回乘積多項式的係數
vector<int> multiply(const vector<int>& a, const vector<int>& b) {
    vector<cd> fa(a.begin(), a.end()), fb(b.begin(), b.end());
    int n = 1;
    // 找到足夠容納乘積結果的最小 2 的冪次
    while (n < a.size() + b.size() - 1)
        n <<= 1;
    fa.resize(n);
    fb.resize(n);

    // 進行 FFT
    fft(fa, false);
    fft(fb, false);
    cout << "fa: " << endl;
    for (int i = 0; i < n; i++) {
        cout << fa[i] << " ";
    }
    cout << endl;
    cout << "fb: " << endl;
    for (int i = 0; i < n; i++) {
        cout << fb[i] << " ";
    }
    cout << endl;
    
    // 逐點相乘
    for (int i = 0; i < n; i++)
        fa[i] *= fb[i];
    // 反 FFT
    fft(fa, true);

    // 四捨五入得到整數係數
    vector<int> result(n);
    for (int i = 0; i < n; i++)
        result[i] = round(fa[i].real());
    // 裁剪掉可能多餘的 0
    while (!result.empty() && result.back() == 0)
        result.pop_back();
    return result;
}

int main() {
    // 例子：計算多項式 (2 + 3x + x^2) 和 (1 + 2x)
    vector<int> poly1 = {2, 3, 1}; // 代表 2 + 3x + x^2
    vector<int> poly2 = {1, 2};    // 代表 1 + 2x

    vector<int> prod = multiply(poly1, poly2);
    
    cout << "多項式乘積的係數為：" << endl;
    for (int coeff : prod)
        cout << coeff << " ";
    cout << endl;
    
    // 輸出結果應為：2, 7, 7, 2
    // 對應於 2 + 7x + 7x^2 + 2x^3
    return 0;
}

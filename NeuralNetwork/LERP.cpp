#include <iostream>

double linearInterpolation(double x0, double y0, double x1, double y1, double x) {
    return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
}

int main() {
    double x0 = 1, y0 = 2;
    double x1 = 3, y1 = 10;
    double x = 2; // 要插值的 x
    double y = linearInterpolation(x0, y0, x1, y1, x);
    
    std::cout << "插值結果 y = " << y << std::endl; // 預期結果: y = 6
    return 0;
}
#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // 讀取圖片
    cv::Mat image = cv::imread("image.jpg", cv::IMREAD_COLOR);
    if (image.empty()) {
        std::cerr << "Error: could not open image.jpg" << std::endl;
        return 1;
    }

    // 建立與原圖相同大小與型態的 noise 矩陣，使用均值0、標準差30的高斯分布
    cv::Mat noise(image.size(), image.type());
    cv::randn(noise, 0 , 300);

    // 加上 noise (OpenCV 的矩陣加法會自動飽和運算，不會溢出)
    cv::Mat noisyImage = image + noise;

    // 儲存結果
    if (!cv::imwrite("image2.jpg", noisyImage)) {
        std::cerr << "Error: could not write image2.jpg" << std::endl;
        return 1;
    }
    
    std::cout << "Added noise and saved to image2.jpg" << std::endl;
    return 0;
}
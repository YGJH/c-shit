// g++ -std=c++23 -o multidownload multidownload.cpp -lcurl -pthread
#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <sstream>
#include <cstdio>
#include <cmath>
#include <mutex>
#include <algorithm>
#include <locale>
#include <codecvt>
#include <cctype>
#include <string>
#include <atomic>
#include <chrono>

using namespace std::chrono;

// Mutex for thread-safe output.
std::mutex cout_mutex;

// Global atomic counter of downloaded bytes.
std::atomic<long> downloaded_bytes(0);

// Write callback: write received data to a file and update progress.
size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    FILE* fp = static_cast<FILE*>(userdata);
    size_t written = fwrite(ptr, size, nmemb, fp);
    // Update downloaded_bytes (number of bytes = written * size)
    downloaded_bytes.fetch_add(written * size);
    return written;
}

// Header callback to capture and parse the "Content-Range" header.
size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
    size_t total_bytes = size * nitems;
    std::string header_line(buffer, total_bytes);
    std::string lower_line = header_line;
    std::transform(lower_line.begin(), lower_line.end(), lower_line.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    std::string prefix = "content-range:";
    if (lower_line.find(prefix) != std::string::npos) {
        size_t slashPos = header_line.find('/');
        if (slashPos != std::string::npos) {
            std::string total_str = header_line.substr(slashPos + 1);
            total_str.erase(total_str.find_last_not_of(" \r\n") + 1);
            try {
                long* fileSizePtr = static_cast<long*>(userdata);
                *fileSizePtr = std::stol(total_str);
            } catch (...) {
                // conversion error: do nothing
            }
        }
    }
    return total_bytes;
}

// Function to get file size.
// First try a HEAD request; if that fails, try GET with range "0-0".
long get_file_size(const std::string& url) {
    long file_size = -1;
    {
        CURL* curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            // Use HEAD request.
            curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            CURLcode res = curl_easy_perform(curl);
            if (res == CURLE_OK) {
                double cl = 0;
                if (curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl) == CURLE_OK &&
                    !std::isnan(cl) && cl > 0)
                {
                    file_size = static_cast<long>(cl);
                }
            }
            curl_easy_cleanup(curl);
        }
    }
    // If file_size is still invalid, try GET with range "0-0" and use header_callback.
    if (file_size <= 0) {
        file_size = -1;
        CURL* curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_RANGE, "0-0");
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, &file_size);
            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cerr << "GET with Range error: " << curl_easy_strerror(res) << std::endl;
            }
            curl_easy_cleanup(curl);
        }
    }
    return file_size;
}

// Download a file segment [start, end] into part_filename.
void download_segment(const std::string& url, long start, long end, const std::string& part_filename) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "Failed to initialize curl for " << part_filename << std::endl;
        return;
    }
    FILE* fp = fopen(part_filename.c_str(), "wb");
    if (!fp) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "Cannot open file " << part_filename << " for writing." << std::endl;
        curl_easy_cleanup(curl);
        return;
    }
    std::stringstream rangeStream;
    rangeStream << start << "-" << end;
    std::string rangeStr = rangeStream.str();
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_RANGE, rangeStr.c_str());
    
    CURLcode res = curl_easy_perform(curl);
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        if (res != CURLE_OK)
            std::cerr << "Error downloading range " << rangeStr << ": " 
                      << curl_easy_strerror(res) << std::endl;
    }
    fclose(fp);
    curl_easy_cleanup(curl);
}

// Extract file name from URL.
std::string get_filename_from_url(const std::string& url) {
    size_t pos = url.find_last_of('/');
    if (pos == std::string::npos)
        return "downloaded_file";
    std::string fname = url.substr(pos + 1);
    if(fname.empty())
        return "downloaded_file";
    return fname;
}

// Single-threaded whole file download.
void download_whole(const std::string& url, const std::string& final_filename) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize curl for whole file download." << std::endl;
        return;
    }
    FILE* fp = fopen(final_filename.c_str(), "wb");
    if (!fp) {
        std::cerr << "Cannot open output file " << final_filename << " for writing." << std::endl;
        curl_easy_cleanup(curl);
        return;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        std::cerr << "Whole file download error: " << curl_easy_strerror(res) << std::endl;
    fclose(fp);
    curl_easy_cleanup(curl);
}

int main(int argc, char* argv[]) {
    // 設定全域 locale，避免亂碼。這裡使用 "C.UTF-8" (若無效則捕獲例外)
    try {
        std::locale::global(std::locale("C.UTF-8"));
    } catch (const std::runtime_error& e) {
        std::cerr << "Failed to set locale (C.UTF-8): " << e.what() << std::endl;
        std::locale::global(std::locale("C"));
    }
    
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <URL>" << std::endl;
        return 1;
    }
    std::string url = argv[1];

    // Initialize libcurl globally.
    curl_global_init(CURL_GLOBAL_ALL);

    // 先取得總檔案大小（利用 HEAD / GET 方式）。
    long total_size = get_file_size(url);
    if (total_size <= 0) {
        std::cerr << "Unable to get valid file size. Starting single thread download..." << std::endl;
        std::string final_filename = get_filename_from_url(url);
        download_whole(url, final_filename);
        curl_global_cleanup();
        return 0;
    }
    std::cout << "File size: " << total_size << " bytes" << std::endl;

    // 決定使用的執行緒數，若無法偵測則至少使用 1 個。
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0)
        num_threads = 1;
    std::cout << "Using " << num_threads << " threads." << std::endl;

    long segment_size = total_size / num_threads;
    std::vector<std::thread> threads;
    std::vector<std::string> part_files;

    // 記錄下載開始時間。
    auto start_time = steady_clock::now();

    // 啟動一個進度列執行緒。
    std::thread progress_thread([&]() {
        while (downloaded_bytes.load() < total_size) {
            double progress = (double)downloaded_bytes.load() / total_size * 100;
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "\rProgress: " << static_cast<int>(progress) << "%";
                std::cout.flush();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        // 確保進度列顯示 100%
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "\rProgress: 100%" << std::endl;
        }
    });

    // Spawn threads to download each segment.
    for (unsigned int i = 0; i < num_threads; i++) {
        long seg_start = i * segment_size;
        long seg_end = (i == num_threads - 1) ? (total_size - 1) : (seg_start + segment_size - 1);
        std::string part_filename = "part_" + std::to_string(i) + ".tmp";
        part_files.push_back(part_filename);
        threads.emplace_back(download_segment, url, seg_start, seg_end, part_filename);
    }

    // Wait for all download threads to complete.
    for (auto& t : threads)
        t.join();

    // 等待進度列執行緒結束。
    progress_thread.join();

    // 合併所有區段成最終檔案。
    std::string final_filename = get_filename_from_url(url);
    std::ofstream output(final_filename, std::ios::binary);
    if (!output) {
        std::cerr << "Failed to create output file " << final_filename << std::endl;
        curl_global_cleanup();
        return 1;
    }
    char buffer[8192];
    for (const auto& part : part_files) {
        std::ifstream input(part, std::ios::binary);
        if (!input) {
            std::cerr << "Failed to open temporary file " << part << std::endl;
            continue;
        }
        while (input.read(buffer, sizeof(buffer)))
            output.write(buffer, input.gcount());
        output.write(buffer, input.gcount());
        input.close();
        std::remove(part.c_str());
    }
    output.close();

    // 計算總花費時間。
    auto end_time = steady_clock::now();
    duration<double> elapsed = end_time - start_time;
    std::cout << "Download complete: " << final_filename << std::endl;
    std::cout << "Total time: " << elapsed.count() << " seconds" << std::endl;

    curl_global_cleanup();
    return 0;
}

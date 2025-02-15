// g++ extractFile.cpp -o extractFile -lzip -pthread -std=c++23
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <zip.h>
#include <thread>

void extract_range(const char* zip_filename, zip_int64_t start, zip_int64_t end) {
    int err = 0;
    zip* archive = zip_open(zip_filename, ZIP_RDONLY, &err);
    if (!archive) {
        std::cerr << "Thread failed to open zip file: " << zip_filename << std::endl;
        return;
    }
    for (zip_int64_t i = start; i < end; i++) {
        const char* name = zip_get_name(archive, i, 0);
        if (!name) continue;
        std::string entry_name(name);
        if (entry_name.empty()) continue;  // 忽略空檔名
        
        std::filesystem::path file_path(entry_name);
        
        // 如果 entry 為目錄（名稱最後有 '/' 判斷）
        if (!entry_name.empty() && entry_name.back() == '/') {
            try {
                std::filesystem::create_directories(file_path);
            } catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "建立目錄失敗: " << file_path << "，原因: " << e.what() << std::endl;
            }
            continue;
        }
        else {
            // 取得父目錄路徑，若不為空且不等於目前目錄，再建立父目錄
            std::filesystem::path parent = file_path.parent_path();
            if (!parent.empty() && parent.string() != ".") {
                try {
                    std::filesystem::create_directories(parent);
                } catch (const std::filesystem::filesystem_error& e) {
                    std::cerr << "建立目錄失敗: " << parent << "，原因: " << e.what() << std::endl;
                }
            }
        }
        
        // 開啟壓縮檔內的檔案
        zip_file* zf = zip_fopen_index(archive, i, 0);
        if (!zf) {
            std::cerr << "Failed to open entry: " << entry_name << std::endl;
            continue;
        }
        // 建立輸出檔案
        std::ofstream out_file(entry_name, std::ios::binary);
        if (!out_file) {
            std::cerr << "Could not create file: " << entry_name << std::endl;
            zip_fclose(zf);
            continue;
        }
        // 每次讀取 4096 bytes
        std::vector<char> buffer((std::size_t)(1<<12));
        zip_int64_t n = 0;
        while ((n = zip_fread(zf, buffer.data(), buffer.size())) > 0) {
            out_file.write(buffer.data(), n);
        }
        zip_fclose(zf);
        out_file.close();
        // std::cout << "Extracted: " << entry_name << std::endl;
    }
    zip_close(archive);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <zip_file>" << std::endl;
        return 1;
    }

    const char* zip_filename = argv[1];
    int err = 0;
    // 為取得總檔案數量，不用多緒，先開啟一次
    zip* archive = zip_open(zip_filename, ZIP_RDONLY, &err);
    if (!archive) {
        std::cerr << "Failed to open zip file: " << zip_filename << std::endl;
        return 1;
    }
    zip_int64_t num_entries = zip_get_num_entries(archive, 0);
    zip_close(archive);

    unsigned int num_threads = std::thread::hardware_concurrency();
    if(num_threads == 0) num_threads = 4; // 若偵測不到，預設為 4 個執行緒

    std::vector<std::thread> threads;
    zip_int64_t chunk = num_entries / num_threads;
    zip_int64_t remainder = num_entries % num_threads;
    zip_int64_t start = 0;
    for (unsigned int i = 0; i < num_threads; i++) {
        zip_int64_t end = start + chunk + (i < remainder ? 1 : 0);
        threads.push_back(std::thread(extract_range, zip_filename, start, end));
        start = end;
    }

    for (auto &t : threads) {
        t.join();
    }
    return 0;
}
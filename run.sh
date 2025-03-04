#!/bin/bash

# 編譯程式
g++ -std=c++17 -O2 o3Download.cpp -o o3Download -lboost_system -lssl -lcrypto -lpthread
g++ -std=c++23 dwn.cpp -o dwn -lcurl -pthread

# 更新測試檔案列表：
declare -a test_files=(
    "nbg1-speed.hetzner.com|/10GB.bin|10GB.bin"
    "nbg1-speed.hetzner.com|/100MB.bin|100MB.bin"
    "nbg1-speed.hetzner.com|/1GB.bin|1GB.bin"
)

# 測試函數
test_download() {
    # 對於 o3Download，分割 URL 成 host 和 path
    IFS='|' read -r host path filename <<< "$1"
    local program=$2
    local segments=$3
    
    echo "測試 $program - https://$host$path"
    local start_time=$(date +%s.%N)
    
    case $program in
        "o3Download")
            ./o3Download "https://$host$path"
            ;;
        "dwn")
            ./dwn "https://$host$path" "test_$filename"
            ;;
    esac
    
    local end_time=$(date +%s.%N)
    local elapsed=$(echo "$end_time - $start_time" | bc)
    echo "執行時間: $elapsed 秒"
    echo "-------------------"
    
    # 檢查檔案是否成功下載
    if [ -f "test_$filename" ]; then
        echo "下載成功：test_$filename"
        local filesize=$(stat -f%z "test_$filename" 2>/dev/null || stat -c%s "test_$filename" 2>/dev/null)
        echo "檔案大小：$filesize bytes"
        rm -f "test_$filename"
    else
        echo "下載失敗：test_$filename"
    fi
}

# 執行測試
echo "開始效能測試..."
echo "==================="

for url_info in "${test_files[@]}"; do
    # 測試 o3Download 使用不同的分段數
    for segments in 1 4 8; do
        test_download "$url_info" "o3Download" "$segments"
    done
    
    # 測試 dwn
    test_download "$url_info" "dwn" ""
done

echo "測試完成"
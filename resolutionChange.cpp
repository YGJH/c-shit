#include <windows.h>
#include <iostream>

int main() {
    // 取得目前 Console 輸出 Code Page
    UINT outputCP = GetConsoleOutputCP();
    std::cout << "目前 Console 的輸出 Code Page 為: " << outputCP << std::endl;

    // 如果不是 UTF-8，則強制設定為 CP65001 (UTF-8)
    if (outputCP != CP_UTF8) {
        if (SetConsoleOutputCP(CP_UTF8)) {
            std::cout << "成功將 Console Code Page 設定為 UTF-8 (65001)" << std::endl;
        } else {
            std::cerr << "設定 Console Code Page 為 UTF-8 失敗" << std::endl;
        }
    }

    // 重新取得 Code Page 以確認設定成果 
    outputCP = GetConsoleOutputCP();
    std::cout << "目前 Console 的輸出 Code Page 為: " << outputCP << std::endl;

    // 取得目前顯示設定
    DEVMODE dm = {0};
    dm.dmSize = sizeof(dm);
    if (!EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm)) {
        std::cerr << "無法取得目前的顯示設定。" << std::endl;
        return 1;
    }

    std::cout << "目前解析度: " << dm.dmPelsWidth << "x" << dm.dmPelsHeight << std::endl;

    // 設定目標解析度 2560x1440
    dm.dmPelsWidth = 2560;
    dm.dmPelsHeight = 1440;
    dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

    // 改變顯示設定
    LONG status = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
    if (status == DISP_CHANGE_SUCCESSFUL) {
        std::cout << "成功變更解析度至 2560x1440" << std::endl;
    } else {
        std::cerr << "變更解析度失敗，錯誤碼: " << status << std::endl;
    }
    return 0;
}
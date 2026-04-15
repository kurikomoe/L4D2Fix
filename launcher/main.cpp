// #pragma comment(linker, "/subsystem:\"Windows\" /entry:\"mainCRTStartup\"")
#include <windows.h>
#include <string>
#include <filesystem>
#include <shellapi.h>

#include <detours/detours.h>
#include <inipp.h>

#include "vars.h"

namespace fs = std::filesystem;

const LPCSTR dll_path = "kpatch.dll";
std::wstring gameName = L"left4dead2.exe";

void init_cfg() {
    LoadIni();
    if (cfg::Redirect::enable) {
        gameName = cfg::Redirect::target;
    } else {
        gameName = L"left4dead2.exe";
    }
}

template <typename T, typename... Args>
void debugPrint(T fmt, Args... args) {
    std::wstring errMsg = std::wstring(L"[L4D2Fix] ") + std::vformat(fmt, std::make_wformat_args(args...));
    OutputDebugStringW(errMsg.c_str());
}

int WINAPI wWinMain(
    _In_ HINSTANCE /*hInstance*/,
    _In_opt_ HINSTANCE /*hPrevInstance*/,
    _In_ LPWSTR lpwCmdLine,
    _In_ int /*nShowCmd*/) {
    init_cfg();

    WCHAR buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);

    // Change the working directory to the directory containing the DLL.
    fs::path selfPath(buffer);
    fs::path workingPath = selfPath.parent_path();
    debugPrint(L"working_path {}\n", workingPath.c_str());

    LPCWSTR targetExe = gameName.c_str();
    debugPrint(L"target_exe_path {}\n", targetExe);

    if (selfPath.filename() == gameName) {
        debugPrint(L"Avoid launching self, exiting.\n");
        MessageBoxW(
            NULL,
            L"警告：配置错误，补丁尝试启动的目标为自身，请检查配置！\n"
            L"为防止卡死系统 L4D2Fix 将强制退出。\n"
            L"Fatal：Invalid config or setup！\n"
            L"The patch is trying to launch itself, please check the config!\n"
            L"Forcing exit to prevent system freeze.",
            L"L4D2Fix", MB_OK|MB_SYSTEMMODAL|MB_ICONERROR);
        ExitProcess(0);
    }

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    si.cb = sizeof(si);

    DWORD dwFlags = CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED;

    SetLastError(0);
    if (TRUE != DetourCreateProcessWithDllExW(
        targetExe,
        lpwCmdLine,
        nullptr,
        nullptr,
        TRUE,
        dwFlags,
        nullptr,
        nullptr,
        &si,
        &pi,
        dll_path,
        nullptr
    )) {
        auto dwError = GetLastError();
        debugPrint(L"DetourCreateProcessWithDllEx failed with error {}\n", dwError);

        ExitProcess(9009);
    }

    ResumeThread(pi.hThread);

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return 0;
}

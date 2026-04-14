//#pragma comment(linker, "/subsystem:\"Windows\" /entry:\"mainCRTStartup\"")
#include <windows.h>
#include <string>
#include <filesystem>
#include <fstream>
#include <shellapi.h>
#include <locale>
#include <codecvt>

#include <detours/detours.h>
#include <inipp.h>

#include "vars.h"

namespace fs = std::filesystem;

bool debug = false;
std::wstring game_name = L"left4dead2.exe";

void init_cfg() {
    LoadIni();
    if (cfg::Redirect::enable) {
        game_name = cfg::Redirect::target;
    } else {
        game_name = L"left4dead2.exe";
    }
}

template<typename T, typename ... Args>
void debugPrint(T fmt, Args... args) {
    std::wstring errMsg = std::wstring(L"[L4D2Fix] ") + std::vformat(fmt, std::make_wformat_args(args...));
    OutputDebugStringW(errMsg.c_str());
}

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpwCmdLine,
    _In_ int nShowCmd
) {
    std::wstring errMsg;
    init_cfg();

    WCHAR working_path[MAX_PATH];
    GetModuleFileNameW(nullptr, working_path, MAX_PATH);

    // Change the working directory to the directory containing the DLL.
    fs::path path(working_path);
    debugPrint(L"working_path {}\n", working_path);

    LPCSTR dll_path = "kpatch.dll";

    LPCWSTR target_exe_path = game_name.c_str();
    debugPrint(L"target_exe_path {}\n", target_exe_path);

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    si.cb = sizeof(si);

    DWORD dwFlags = CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED;

    SetLastError(0);
    if (TRUE != DetourCreateProcessWithDllExW(
            target_exe_path,
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
            nullptr)) {
        auto dwError = GetLastError();
        debugPrint(L"DetourCreateProcessWithDllEx failed with error {}\n", dwError);

        ExitProcess(9009);
    }

    ResumeThread(pi.hThread);

    WaitForSingleObject(pi.hProcess, INFINITE);

    return 0;
}

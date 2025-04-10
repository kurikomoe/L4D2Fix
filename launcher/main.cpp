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

namespace fs = std::filesystem;

// const wchar_t* game_name = L"left4dead2.exe";
bool debug = false;
std::wstring game_name = L"left4dead2.exe";

void init_cfg() {
    std::ifstream is("kpatch.ini");
    inipp::Ini<char> ini;

    ini.parse(is);
    inipp::extract(ini.sections["System"]["debug"], debug);
    std::string tmp_name;
    inipp::extract(ini.sections["System"]["target"], tmp_name);
    game_name = std::wstring(tmp_name.begin(), tmp_name.end());
}

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpwCmdLine,
    _In_ int nShowCmd
) {
    init_cfg();

    WCHAR working_path[MAX_PATH];
    GetModuleFileNameW(nullptr, working_path, MAX_PATH);

    // Change the working directory to the directory containing the DLL.
    fs::path path(working_path);
    SetCurrentDirectoryW(path.parent_path().wstring().c_str());

    LPCSTR dll_path = "kpatch.dll";

    LPCWSTR target_exe_path = game_name.c_str();

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
        printf("DetourCreateProcessWithDllEx failed with error %ld\n", dwError);

        ExitProcess(9009);
    }

    ResumeThread(pi.hThread);

    WaitForSingleObject(pi.hProcess, INFINITE);

    return 0;
}

#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <windows.h>

#include <cstdint>
#include <cstddef>
#include <iostream>
#include <filesystem>
#include <fstream>

#include <algorithm>
#include <detours/detours.h>
#include <MinHook.h>
#include <inipp.h>

#include "helper.hpp"

#include "vars.h"

#include "hooks_indices.h"
#include "hooks_dvb.h"


void InitConsole() {
    AllocConsole();
    FILE *dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
}

void Logging() {
    if (cfg::System::debug) {
        InitConsole();
    }
    // Get this module path
    WCHAR thisModulePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(thisModule, thisModulePath, MAX_PATH);
    sThisModulePath = thisModulePath;
    sThisModulePath = sThisModulePath.remove_filename();

    // Get game name and exe path
    WCHAR exePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(baseModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();
    sExePath = sExePath.remove_filename();

    // spdlog initialisation
    try {
        auto logger = spdlog::basic_logger_mt(sFixName, sLogFile, true);
        spdlog::set_default_logger(logger);

        auto start_time = std::chrono::system_clock::now();

        if (cfg::System::debug) {
            spdlog::set_level(spdlog::level::debug);
        }

        spdlog::flush_on(spdlog::level::debug);
        spdlog::info("----------");
        spdlog::info("Date: {}", std::chrono::system_clock::to_time_t(start_time));
        spdlog::info("{} v{} loaded.", sFixName.c_str(), sFixVer.c_str());
        spdlog::info("----------");
        spdlog::info("Log file: {}", sThisModulePath.string() + sLogFile);
        spdlog::info("----------");

        // Log module details
        spdlog::info("Module Name: {0:s}", sExeName.c_str());
        spdlog::info("Module Path: {0:s}", sExePath.string());
        spdlog::info("Module Address: 0x{0:x}", (uintptr_t)baseModule);
        spdlog::info("Module Timestamp: {0:d}", Memory::ModuleTimestamp(baseModule));
        spdlog::info("----------");
    } catch (const spdlog::spdlog_ex &ex) {
        InitConsole();
    }
}


DWORD __stdcall Main(void*) {
    LoadIni();
    Logging();

    if (cfg::System::debug) {
        MessageBoxW(NULL, L"警告：你已开启 debug 输出。", L"L4D2 Fix", MB_OK|MB_SYSTEMMODAL|MB_ICONWARNING);
    }

    std::filesystem::path startup_check = L"success.txt";

    if (!std::filesystem::exists(startup_check)) {
        MessageBoxW(
            NULL,
            L"这是一个启动测试，用于检验补丁是否正常运行。\n请注意由于修改内存，请不要进 VAC 服，后果自负。\n本弹窗仅首次启动出现，后续运行情况参见 L4D2Fix.log 日志文件。\n\n关注B站 5050 直播间，谢谢喵 by KurikoMoe!",
            L"L4D2 Fix", MB_OK);
    }

    std::wstring cmdline = GetCommandLineW();
    spdlog::info(L"Startup commandline: {}", cmdline.c_str());

    std::wstring dllName = L"shaderapidx9.dll";
    bool isVulkan = false;

    std::transform(cmdline.cbegin(), cmdline.cend(), cmdline.begin(), ::tolower);
    if (cmdline.find(L"-vulkan") != std::wstring::npos) {
        dllName = L"shaderapivk.dll";
        spdlog::info(L"Detected Vulkan: using {}", dllName);
        isVulkan = true;
    } else {
        // Preload d3d9.dll
        LoadLibraryW(L"d3d9.dll");
        spdlog::info(L"Detected DirectX9: using {}", dllName);
    }

    auto relPath = std::filesystem::path(L"bin") / dllName;
    auto absPath = std::filesystem::absolute(relPath);

    std::wcout << absPath << std::endl;
    spdlog::info(L"Loading Dll fromg {}", absPath.wstring());

    HMODULE hDll = LoadLibraryExW(absPath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);

    if (hDll == nullptr) {
        auto errCode = GetLastError();
        wchar_t buf[255];
        FormatMessageW(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            buf, (sizeof(buf) / sizeof(wchar_t)), NULL);
        std::wcout << errCode << std::endl;
        std::wcout << buf << std::endl;
        MessageBoxW(NULL, std::format(L"Failed to load {}!", dllName).c_str(), L"L4D2 Fix", MB_OK | MB_SYSTEMMODAL | MB_ICONSTOP);
        exit(-1);
    }

    // Not known

    auto ret = 0;
    ret += PatchIndices(hDll, dllName);
    ret += hooks_dvb(hDll, dllName);

    if (ret != 0) {
        MessageBoxW(NULL, L"未能正常应用补丁，patch 未生效，请联系开发者\n这只是一个警告，可能不影响游戏运行", L"L4D2 Fix", MB_OK);
    } else {
        std::fstream file(startup_check, std::ios::out);
        file << "补丁已生效，关闭启动提示，删除本文件可以重新启用" << std::endl;
    }

    return TRUE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    thisModule = hModule;

    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        Main(nullptr);
        // HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, CREATE_SUSPENDED, 0);
        // if (mainHandle) {
        //     SetThreadPriority(mainHandle, THREAD_PRIORITY_TIME_CRITICAL);
        //     ResumeThread(mainHandle);
        //     CloseHandle(mainHandle);
        // }
        break;
    }
    return TRUE;
}

// Detour needs at least on exported function
void __declspec(dllexport) kuriko_export() {}

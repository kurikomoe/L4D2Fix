#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <windows.h>
#include <libloaderapi.h>

#include <cstdint>
#include <cstddef>
#include <iostream>
#include <filesystem>

#include <algorithm>
#include <detours/detours.h>

#include "helper.hpp"

#ifdef NDEBUG
constexpr bool isDebug = false;
#else
constexpr bool isDebug = true;
#endif

#ifdef NTESTING
constexpr bool isTesting = false;
#else
constexpr bool isTesting = true;
#endif


HMODULE baseModule = GetModuleHandle(NULL);
HMODULE thisModule; // Fix DLL

// Version
std::string sFixName = "L4D2Fix";
std::string sFixVer = "1.1.0";
std::string sLogFile = sFixName + ".log";

// Logger
std::shared_ptr<spdlog::logger> logger;
std::filesystem::path sExePath;
std::string sExeName;
std::filesystem::path sThisModulePath;

void InitConsole() {
    AllocConsole();
    FILE *dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
}

void Logging() {
    if constexpr (isDebug) {
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
    Logging();

    if constexpr (isTesting) {
        MessageBoxW(NULL, L"警告：这是测试版补丁，设定为必定会炸，用于检测补丁是否正确应用。", L"L4D2 Fix", MB_OK|MB_SYSTEMMODAL|MB_ICONWARNING);
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

    // Tag, pattern, start_search_path, write_offset
    auto f = [&](const wchar_t* tag, const char* pattern, size_t offset, size_t w_offset) {
        std::uint8_t* ptr = Memory::PatternScan(hDll, pattern, offset);

        uint32_t new_buffer_size = 0x020000;
        if constexpr (isTesting) {
            new_buffer_size = 32;
        }

        if (ptr != nullptr) {
            Memory::Write((uintptr_t)(ptr + w_offset), new_buffer_size);
            spdlog::info(L"Modified the vertex max buffer {}: {} @ 0x{:0x}", dllName, tag, (intptr_t)(ptr) - (intptr_t)hDll);
            return ptr;
        }

        spdlog::error(L"Pattern scan failed for {}: {}", dllName, tag);
        return (uint8_t*)nullptr;
    };


    std::vector<void*> ptr_cache;
    std::string pattern;
    // CreateEmptyColorMesh

    pattern = "68 ?? ?? ?? ?? 68 00 80 00 00 6A 04";
    auto* ptr1 = f(
        L"CreateEmptyColorMesh",
        pattern.c_str(),
        0,
        6
    );
    ptr_cache.push_back(ptr1);

    pattern = "68 00 80 00 00 50 8B";
    auto* ptr1_1 = f(
        L"CreateEmptyColorMesh Later",
        pattern.c_str(),
        (size_t)ptr1,
        1
    );
    ptr_cache.push_back(ptr1_1);

    // CreateVertexIDBuffer
    pattern = "68 ?? ?? ?? ?? 68 00 80 00 00 6A 04";
    auto* ptr2 = f(
        L"CreateVertexIDBuffer",
        pattern.c_str(),
        (size_t)ptr1_1,
        6
    );
    ptr_cache.push_back(ptr2);

    pattern = "68 00 80 00 00 50 8B";
    auto* ptr2_1 = f(
        L"CreateVertexIDBuffer Later",
        pattern.c_str(),
        (size_t)ptr2,
        1
    );
    ptr_cache.push_back(ptr2_1);

    auto* ptr3 = f(
        L"Unknown",
        "0F B6 C0 50 68 00 80 00 00 53 8B CF E8 ?? ?? ?? ??",
        (size_t)ptr2,
        5
    );
    ptr_cache.push_back(ptr3);
    // Not known

    if (std::ranges::any_of(ptr_cache, [](void* ptr) { return ptr == nullptr; })) {
        MessageBoxW(NULL, L"未能正常应用补丁，patch 未生效，请联系开发者\n这只是一个警告，可能不影响游戏运行", L"L4D2 Fix", MB_OK);
    } else {
        std::fstream file(startup_check, std::ios::out);
        file << "补丁已生效，关闭启动提示，删除本文件可以重新启用" << std::endl;
    }

    return true;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    thisModule = hModule;

    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        Main(nullptr);
        break;
    }
    return TRUE;
}

// Detour needs at least on exported function
void __declspec(dllexport) kuriko_export() {}

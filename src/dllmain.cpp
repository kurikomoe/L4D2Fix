#include "stdafx.h"
#include "helper.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <mutex>

HMODULE baseModule = GetModuleHandle(NULL);
HMODULE thisModule; // Fix DLL

// Version
std::string sFixName = "L4D2Fix";
std::string sFixVer = "0.0.1";
std::string sLogFile = sFixName + ".log";

// Logger
std::shared_ptr<spdlog::logger> logger;
std::filesystem::path sExePath;
std::string sExeName;
std::filesystem::path sThisModulePath;

void Logging()
{
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
    {
        try {
            // Create 10MB truncated logger
            auto logger = spdlog::rotating_logger_mt(sFixName, sLogFile, 1048576 * 5, 3);
            spdlog::set_default_logger(logger);

            spdlog::flush_on(spdlog::level::debug);
            spdlog::info("----------");
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
        }
        catch (const spdlog::spdlog_ex& ex) {
            AllocConsole();
            FILE* dummy;
            freopen_s(&dummy, "CONOUT$", "w", stdout);
            std::cout << "Log initialisation failed: " << ex.what() << std::endl;
            FreeLibraryAndExitThread(baseModule, 1);
        }
    }
}

DWORD __stdcall Main(void*) {
    Logging();
    std::filesystem::path startup_check = L"success.txt";
    // MessageBoxW(NULL, L"警告：这是测试版补丁，设定为必定会炸，用于检测补丁是否正确应用。", L"L4D2 Fix", MB_OK);

    if (!std::filesystem::exists(startup_check)) {
        MessageBoxW(NULL, L"这是一个启动测试，用于检验补丁是否正常运行。\n请注意由于修改内存，请不要进 VAC 服，后果未知。\n本弹窗仅首次启动出现，后续运行情况参见 L4D2Fix.log 日志文件。\n\n关注B站 5050 直播间，谢谢喵 by KurikoMoe!", L"L4D2 Fix", MB_OK);
    }

    auto hDll = LoadLibraryW(L"shaderapidx9.dll");

    if (hDll == nullptr) {
        MessageBoxW(NULL, L"Failed to load shaderapidx9.dll!", L"L4D2 Fix", MB_OK);
    }

    auto f = [&](const char* tag, const char* pattern, size_t offset, size_t w_offset) {
        std::uint8_t* ptr = Memory::PatternScan(hDll, pattern);

        constexpr uint32_t new_buffer_size = 0x020000;
        // constexpr uint32_t new_buffer_size = 32;

        if (ptr != nullptr) {
            Memory::Write((uintptr_t)(ptr + w_offset), new_buffer_size);
            spdlog::info("Modified the vertex max buffer shaderapidx9.dll: {}", tag);
            return ptr;
        }

        spdlog::error("Pattern scan failed for shaderapidx9.dll: {}", tag);
        return (uint8_t*)nullptr;
    };

    // CreateEmptyColorMesh
    auto ptr1 = f("CreateEmptyColorMesh", "68 ?? ?? ?? ?? 68 00 80 00 00 6A 04", 0, 6);

    auto ptr1_1 = f("CreateEmptyColorMesh Later", "68 00 80 00 00 50 8B", (size_t)ptr1, 1);

    // CreateVertexIDBuffer
    auto ptr2 = f("CreateVertexIDBuffer", "68 ?? ?? ?? ?? 68 00 80 00 00 6A 04", (size_t)ptr1_1, 6);
    auto ptr2_1 = f("CreateVertexIDBuffer Later", "68 00 80 00 00 50 8B", (size_t)ptr2, 1);

    // Not known
    auto ptr3 = f("Unknown", "0F B6 C0 50 68 00 80 00 00 53 8B CF E8 ?? ?? ?? ??", (size_t)ptr1, 5);

    if (ptr1 == nullptr
        || ptr1_1 == nullptr
        || ptr2 == nullptr
        || ptr2_1 == nullptr
        || ptr3 == nullptr
    ) {
        MessageBoxW(NULL, L"未能正常应用补丁，patch 未生效，请联系开发者", L"L4D2 Fix", MB_OK);
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


void __declspec(dllexport) kuriko_export() {}

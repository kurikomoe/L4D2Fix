#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <cctype>
#include <algorithm>

#include "helper.hpp"

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

void Logging() {
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
        AllocConsole();
        FILE *dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "Log initialisation failed: " << ex.what() << std::endl;
        FreeLibraryAndExitThread(baseModule, 1);
    }
}

DWORD __stdcall Main(void*) {
    Logging();
    std::filesystem::path startup_check = L"success.txt";
    // MessageBoxW(NULL, L"警告：这是测试版补丁，设定为必定会炸，用于检测补丁是否正确应用。", L"L4D2 Fix", MB_OK|MB_SYSTEMMODAL|MB_ICONWARNING);

    if (!std::filesystem::exists(startup_check)) {
        MessageBoxW(
            NULL,
            L"这是一个启动测试，用于检验补丁是否正常运行。\n请注意由于修改内存，请不要进 VAC 服，后果自负。\n本弹窗仅首次启动出现，后续运行情况参见 L4D2Fix.log 日志文件。\n\n关注B站 5050 直播间，谢谢喵 by KurikoMoe!",
            L"L4D2 Fix", MB_OK);
    }

    std::wstring cmdline = GetCommandLineW();
    spdlog::info(L"Startup commandline: {}", cmdline.c_str());

    std::wstring dllName;
    std::transform(cmdline.cbegin(), cmdline.cend(), cmdline.begin(), ::tolower);

    if (cmdline.find(L"-vulkan") != std::wstring::npos) {
        dllName = L"shaderapivk.dll";
        spdlog::info(L"Detected Vulkan: using {}", dllName);
    } else {
        dllName = L"shaderapidx9.dll";
    }
    auto hDll = LoadLibraryW(dllName.c_str());

    if (hDll == nullptr) {
        MessageBoxW(NULL, std::format(L"Failed to load {}!", dllName).c_str(), L"L4D2 Fix", MB_OK|MB_SYSTEMMODAL|MB_ICONSTOP);
        exit(-1);
    }

    auto f = [&](const wchar_t* tag, const char* pattern, size_t offset, size_t w_offset) {
        std::uint8_t* ptr = Memory::PatternScan(hDll, pattern);

        constexpr uint32_t new_buffer_size = 0x020000;
        // constexpr uint32_t new_buffer_size = 32;

        if (ptr != nullptr) {
            Memory::Write((uintptr_t)(ptr + w_offset), new_buffer_size);
            spdlog::info(L"Modified the vertex max buffer {}: {}", dllName, tag);
            return ptr;
        }

        spdlog::error(L"Pattern scan failed for {}: {}", dllName, tag);
        return (uint8_t*)nullptr;
    };

    // CreateEmptyColorMesh
    auto ptr1 = f(L"CreateEmptyColorMesh", "68 ?? ?? ?? ?? 68 00 80 00 00 6A 04", 0, 6);

    auto ptr1_1 = f(L"CreateEmptyColorMesh Later", "68 00 80 00 00 50 8B", (size_t)ptr1, 1);

    // CreateVertexIDBuffer
    auto ptr2 = f(L"CreateVertexIDBuffer", "68 ?? ?? ?? ?? 68 00 80 00 00 6A 04", (size_t)ptr1_1, 6);
    auto ptr2_1 = f(L"CreateVertexIDBuffer Later", "68 00 80 00 00 50 8B", (size_t)ptr2, 1);

    // Not known
    auto ptr3 = f(L"Unknown", "0F B6 C0 50 68 00 80 00 00 53 8B CF E8 ?? ?? ?? ??", (size_t)ptr1, 5);

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
        break;
    }
    return TRUE;
}


void __declspec(dllexport) kuriko_export() {}

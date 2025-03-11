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
#include <MinHook.h>

#include "helper.hpp"

#ifdef NDEBUG
constexpr bool isDebug = false;
#else
constexpr bool isDebug = true;
#endif

#ifdef NTESTING
constexpr bool isTesting = false;
constexpr int VERTEX_BUFFER_SIZE = 0x020000;
#else
constexpr bool isTesting = true;
constexpr int VERTEX_BUFFER_SIZE = 32;
#endif


HMODULE baseModule = GetModuleHandle(NULL);
HMODULE thisModule; // Fix DLL

// Version
std::string sFixName = "L4D2Fix";
std::string sFixVer = "1.3.0";
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

        if (isDebug) {
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


using CVertexBufferCtorFn = void* (__thiscall*)(void*, int, int, int, int, int, int, char*, char, char);
CVertexBufferCtorFn oCVertexBufferCtorFn = nullptr;
CVertexBufferCtorFn tCVertexBufferCtorFn = nullptr;
intptr_t nCVertexBufferCtorFn = NULL;

void*__fastcall hCVertexBufferCtorFn(
    void* This,
    DWORD EDX,
    int a2,
    int a3,
    int a4,
    int a5,
    int count,
    int bufSize,
    char * source,
    char a6,
    char a7)
{
    // std::string msg = std::format(
    //     "This: {:x}, EDX: {:x}, a2: {:x}, a3: {}, a4: {}, a5: {}, count: {}, bufSize: {}, source: {}, a6: {}, a7: {}",
    //     (intptr_t)This,
    //     EDX,
    //     a2, a3, a4, a5,
    //     count, bufSize,
    //     source, (int)a6, (int)a7
    // );
    // spdlog::info(msg);
    // MessageBoxW(NULL, L"inside CVertexBufferCtorFn", L"", MB_OK);

    if (isDebug) {
        return tCVertexBufferCtorFn(This, a2, a3, a4, a5,
            count,
            32,
            source, a6, a7);
    }

    return tCVertexBufferCtorFn(This, a2, a3, a4, a5,
        std::max(1, count * bufSize / VERTEX_BUFFER_SIZE),
        VERTEX_BUFFER_SIZE,
        source, a6, a7);
}

void __declspec(naked) nakedCVertexBufferCtorFn() {
    __asm {
        call hCVertexBufferCtorFn
        jmp nCVertexBufferCtorFn
    };
}

using GetMaxToRenderFn = int (__thiscall*)(void*, DWORD*, char, int*, int*);
GetMaxToRenderFn oGetMaxToRender = nullptr;

int __fastcall hGetMaxToRender(
    void* This,
    DWORD EDX,
    DWORD* a2,
    char a3,
    int* p_bufsize,
    int* p_nCount
) {
    auto ret = oGetMaxToRender(This, a2, a3, p_bufsize, p_nCount);
    // spdlog::debug("p_bufSize: {} {:x}, p_nCount: {} {:x}", *p_bufsize, *p_bufsize, *p_nCount, *p_nCount);
    if (*p_bufsize == 32768)  *p_bufsize = VERTEX_BUFFER_SIZE;
    return ret;
}


bool fixDynamicVB(HMODULE hDll) {
    const char* pat = "0F B6 C8 8B 45 10 51 8B 4D FC 99 68 ?? ?? ?? ?? 68 00 80 00 00";
    size_t offset_imm = 0xe13b - 0xe12a;
    size_t offset_call = 0xe152 - 0xe12a;
    std::uint8_t* ptr = Memory::PatternScan(hDll, pat, 0);
    if (ptr == nullptr) {
        spdlog::info(L"Can not find the DynamicVB call");
        return false;
    }
    spdlog::info(L"Hook DynamicVB Call success");

    ptr += offset_call;
    spdlog::info(L"Modified the DynamicVB call @ 0x{:0x}", (intptr_t)ptr - (intptr_t)hDll);
    oCVertexBufferCtorFn = reinterpret_cast<CVertexBufferCtorFn>(ptr);
    nCVertexBufferCtorFn = reinterpret_cast<intptr_t>(ptr) + 0x5;

    // copy the target address, call is followed by an relative address
    const char* pat_GameCVertexBufferCtor = "55 8B EC 81 EC 08 01 00 00 A1 ?? ?? ?? ?? 33 C5 89 45 FC 53 56 57 8B 45 08 8B 55 10";
    std::uint8_t* ptr_GameCVertexBufferCtor = Memory::PatternScan(hDll, pat_GameCVertexBufferCtor, 0);
    if (ptr_GameCVertexBufferCtor == nullptr) {
        spdlog::info(L"Can not find the GameCVertexBufferCtor");
        return false;
    }
    tCVertexBufferCtorFn = reinterpret_cast<CVertexBufferCtorFn>(ptr_GameCVertexBufferCtor);
    spdlog::info(L"Hook GameCVertexBufferCtor success");

    const char* pat_GetMaxToRender = "55 8B EC 57 8B 7D 08 85 FF 75 ?? 8B 45 10 89 38";
    std::uint8_t* ptr_GetMaxToRender = Memory::PatternScan(hDll, pat_GetMaxToRender, 0);
    oGetMaxToRender = reinterpret_cast<GetMaxToRenderFn>(ptr_GetMaxToRender);
    if (oGetMaxToRender == nullptr) {
        spdlog::info(L"Can not find the GetMaxToRender");
        return false;
    }
    spdlog::info(L"Hook GetMaxToRender success");

    // Only for debugging
    // Memory::Write((uintptr_t)hDll + 0xbb67, (uint16_t)0x9090);

    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourAttach(&(PVOID&)oCVertexBufferCtorFn, nakedCVertexBufferCtorFn);

    DetourAttach(&(PVOID&)oGetMaxToRender, hGetMaxToRender);

    DetourTransactionCommit();

    spdlog::info(L"New DynamicVB call to 0x{:0x}", (intptr_t)oCVertexBufferCtorFn);

    return true;
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

        uint32_t new_buffer_size = VERTEX_BUFFER_SIZE;
        // uint32_t new_buffer_size = 0x020000;

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

    auto ret = fixDynamicVB(hDll);

    if (!ret || std::ranges::any_of(ptr_cache, [](void* ptr) { return ptr == nullptr; })) {
        MessageBoxW(NULL, L"未能正常应用补丁，patch 未生效，请联系开发者\n这只是一个警告，可能不影响游戏运行", L"L4D2 Fix", MB_OK);
    } else {
        std::fstream file(startup_check, std::ios::out);
        file << "补丁已生效，关闭启动提示，删除本文件可以重新启用" << std::endl;
    }

    return true;
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

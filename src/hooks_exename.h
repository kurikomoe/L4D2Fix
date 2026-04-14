#pragma once
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <windows.h>
#include <filesystem>

#include "vars.h"

namespace fs = std::filesystem;

decltype(&GetModuleFileNameA) oGetModuleFileNameAFn = nullptr;

DWORD __stdcall hGetModuleFileNameA(
    HMODULE hModule,
    LPSTR lpFilename,
    DWORD nSize
) {
    // MessageBoxA(NULL, "GetModuleFileNameA Hooked!", "Info", MB_OK);
    auto ret = oGetModuleFileNameAFn(hModule, lpFilename, nSize);
    if (!lpFilename)  return ret;

    void* callerAddress = _ReturnAddress();
    HMODULE callerModule = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)callerAddress, &callerModule);

    char moduleName[MAX_PATH];
    auto retCaller = oGetModuleFileNameAFn(callerModule, moduleName, MAX_PATH);

    const std::vector<std::string> moduleNameList = {
        "launcher.dll",
    };
    spdlog::info("GetModuleFileNameA called by module: {}", moduleName);
    for (auto& name : moduleNameList) {
        if (!strstr(moduleName, name.c_str())) {
            return ret;
        }
    }

    fs::path filePath(lpFilename);
    fs::path newFilePath(lpFilename);
    std::wstring exeName = filePath.filename().wstring();
    if (cfg::Redirect::enable && exeName == cfg::Redirect::target) {
        spdlog::info("GetModuleFileNameA called by module: {}", moduleName);
        std::string newExeName = std::string(
            cfg::Redirect::origin.begin(), cfg::Redirect::origin.end());
        newFilePath = newFilePath.replace_filename(newExeName);
        std::string newFilePathStr = newFilePath.string();
        memcpy_s(lpFilename, nSize, newFilePath.string().c_str(), newFilePathStr.size() + 1);
    }
    return ret;
}



void initExeNameHook() {
    oGetModuleFileNameAFn = &GetModuleFileNameA;
    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)oGetModuleFileNameAFn, hGetModuleFileNameA);
    DetourTransactionCommit();
}

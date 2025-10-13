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

    fs::path filePath(lpFilename);
    fs::path newFilePath(lpFilename);
    std::wstring exeName = filePath.filename().wstring();
    if (cfg::Redirect::enable && exeName == cfg::Redirect::target) {
        std::string newExeName = std::string(
            cfg::Redirect::origin.begin(), cfg::Redirect::origin.end());
        newFilePath = newFilePath.replace_filename(newExeName);
        std::string newFilePathStr = newFilePath.string();
        memcpy_s(lpFilename, nSize, newFilePath.string().c_str(), newFilePathStr.size() + 1);
        spdlog::info("GetModuleFileNameA hooked: \n{} => {}", filePath.string(), newFilePath.string());
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

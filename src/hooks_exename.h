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
        std::wstring newExeName = cfg::Redirect::origin;
        newFilePath = newFilePath.replace_filename(newExeName);
        std::wstring newFilePathStr = newFilePath.wstring();

        BOOL usedDefaultChar = FALSE;
        int requiredSize = WideCharToMultiByte(
            CP_ACP,
            WC_NO_BEST_FIT_CHARS,
            newFilePathStr.c_str(),
            -1,
            nullptr,
            0,
            "?",
            &usedDefaultChar
        );

        if (requiredSize <= 0) {
            spdlog::warn("WideCharToMultiByte size query failed, error={}", GetLastError());
            return ret;
        }

        std::string ansiPath(static_cast<size_t>(requiredSize), '\0');
        int convertedSize = WideCharToMultiByte(
            CP_ACP,
            WC_NO_BEST_FIT_CHARS,
            newFilePathStr.c_str(),
            -1,
            ansiPath.data(),
            requiredSize,
            "?",
            &usedDefaultChar
        );

        if (convertedSize <= 0) {
            spdlog::warn("WideCharToMultiByte conversion failed, error={}", GetLastError());
            return ret;
        }

        if (nSize == 0) {
            spdlog::warn("Cannot write redirected path: lpFilename buffer size is 0");
            return ret;
        }

        const size_t ansiLen = strnlen_s(ansiPath.c_str(), ansiPath.size());
        if (ansiLen + 1 > static_cast<size_t>(nSize)) {
            spdlog::warn(
                "Redirected path exceeds caller buffer: required={} bytes, capacity={} bytes; path will be truncated",
                ansiLen + 1,
                nSize
            );
        }

        size_t copyLen = ansiLen;
        const size_t maxWritable = static_cast<size_t>(nSize - 1);
        if (copyLen > maxWritable) {
            copyLen = maxWritable;
        }

        errno_t copyErr = memcpy_s(lpFilename, nSize, ansiPath.c_str(), copyLen);
        if (copyErr != 0) {
            spdlog::warn("memcpy_s failed while writing redirected path, errno={}", copyErr);
            return ret;
        }
        lpFilename[copyLen] = '\0';

        if (usedDefaultChar) {
            spdlog::warn("Path conversion used fallback characters; non-ANSI characters may be lost");
        }
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

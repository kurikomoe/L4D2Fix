#pragma once
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <windows.h>
#include <iostream>
#include <format>
#include <string>

#include <detours/detours.h>

#include "inipp.h"

#include "helper.hpp"
#include "vars.h"

// ---------------------------------------------------------------------------------

using CIndexBufferCtorFn = void* (__thiscall*)(void*, void*, int, char, char);
CIndexBufferCtorFn oCIndexBufferCtorFn = nullptr;

void*__fastcall hCIndexBufferCtorFn(
    void* This, DWORD EDX,
    void* pD3Device,
    int bufSize, char a6, char a7)
{
    if (cfg::IndexBuffer::debug) {
        std::string msg = std::format("CIndexBuffer bufSize: {}", bufSize);
        spdlog::info(msg);
    }

    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex)) {
        spdlog::info("Total physical memory: {} MB", statex.ullTotalPhys / (1024 * 1024));
        spdlog::info("Available physical memory: {} MB", statex.ullAvailPhys / (1024 * 1024));
        spdlog::info("Total virtual memory: {} MB", statex.ullTotalVirtual / (1024 * 1024));
        spdlog::info("Available virtual memory: {} MB", statex.ullAvailVirtual / (1024 * 1024));
        spdlog::info("====================");
    }


    // count *= 2;
    // if (count > 131072) count = 131072;
    // if (count > 131072) count = 131072;
    if (bufSize == 0x8000) bufSize = 0xF0000;
    // if (bufSize > 0x20000) bufSize = 0x20000;

    return oCIndexBufferCtorFn(This, pD3Device, bufSize, a6, a7);
}

int patchIndexBuffer(HMODULE hDll) {
    const char* pat = "55 8B EC 8A 45 10 24 01 02 C0 56 8B F1 8A 4D 14 8A 56 14 02 C0 80 E1 01 0A C1 02 C0 57 8B 7D 0C";
    std::uint8_t* ptr = Memory::PatternScan(hDll, pat, 0);
    if (ptr == nullptr) {
        spdlog::info(L"Can not find the CIndexBuffer::Ctor");
        return -1;
    }

    spdlog::info(L"Hook the CIndexBuffer::Ctor @ 0x{:0x}", (intptr_t)ptr - (intptr_t)hDll);
    oCIndexBufferCtorFn = reinterpret_cast<CIndexBufferCtorFn>(ptr);

    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)oCIndexBufferCtorFn, hCIndexBufferCtorFn);
    DetourTransactionCommit();

    return 0;
}

// ---------------------------------------------------------------------------------

int hooks_indexbuffer(HMODULE hDll, std::wstring dllName) {
    auto ret = 0;

    if (cfg::IndexBuffer::enable) {
        ret += patchIndexBuffer(hDll);
    }

    return ret;
}

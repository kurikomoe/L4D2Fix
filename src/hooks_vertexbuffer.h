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

namespace VertexBuffer {

using CVertexBufferCtorFn = void* (__thiscall*)(void*, void*, int, int, int, int, int, char*, char, char);
CVertexBufferCtorFn oCVertexBufferCtorFn = nullptr;

void*__fastcall hCVertexBufferCtorFn(
    void* This, DWORD EDX,
    void* pD3Device, int a3, int a4, int a5,
    int count, int bufSize,
    char * source, char a6, char a7)
{
    if (cfg::VertexBuffer::debug) {
        std::string msg = std::format("CVertexBuffer count: {}, bufSize: {}", count, bufSize);
        spdlog::info(msg);
    }

    if (bufSize == 0x8000) bufSize = 0xF0000;
    // if (bufSize > 0x20000) bufSize = 0x20000;

    return oCVertexBufferCtorFn(This, pD3Device, a3, a4, a5, count, bufSize, source, a6, a7);
}

int patchVertexBuffer(HMODULE hDll) {
    const char* pat = "55 8B EC 81 EC 08 01 00 00 A1 ?? ?? ?? ?? 33 C5 89 45 FC 53 56 57 8B 45 08";
    std::uint8_t* ptr = Memory::PatternScan(hDll, pat, 0);
    if (ptr == nullptr) {
        spdlog::info(L"Can not find the CVertexBuffer::Ctor");
        return -1;
    }

    spdlog::info(L"Hook the CVertexBuffer::Ctor @ 0x{:0x}", (intptr_t)ptr - (intptr_t)hDll);
    oCVertexBufferCtorFn = reinterpret_cast<CVertexBufferCtorFn>(ptr);

    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)oCVertexBufferCtorFn, hCVertexBufferCtorFn);
    DetourTransactionCommit();

    return 0;
}

// ---------------------------------------------------------------------------------

int hooks_vertexbuffer(HMODULE hDll, std::wstring dllName) {
    auto ret = 0;

    if (cfg::VertexBuffer::enable) {
        ret += patchVertexBuffer(hDll);
    }

    return ret;
}

}

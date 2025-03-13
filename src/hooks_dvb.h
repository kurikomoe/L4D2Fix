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

using CVertexBufferCtorFn = void* (__thiscall*)(void*, int, int, int, int, int, int, char*, char, char);
CVertexBufferCtorFn oCVertexBufferCtorFn = nullptr;
CVertexBufferCtorFn tCVertexBufferCtorFn = nullptr;
intptr_t nCVertexBufferCtorFn = NULL;

void*__fastcall hCVertexBufferCtorFn(
    void* This, DWORD EDX,
    int a2, int a3, int a4, int a5,
    int count, int bufSize,
    char * source, char a6, char a7)
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
    // auto new_count = std::max(1, count * bufSize / VBS);
    auto new_count = count;
    auto new_bufsize = cfg::DynamicVB::value;

    if (cfg::DynamicVB::debug) {
        spdlog::debug("alloc: {}, {}", new_count, new_bufsize);
    }

    return tCVertexBufferCtorFn(This, a2, a3, a4, a5,
        new_count,
        new_bufsize,
        source, a6, a7);

    // return tCVertexBufferCtorFn(This, a2, a3, a4, a5,
    //     count,
    //     bufSize,
    //     source, a6, a7);
}

void __declspec(naked) nakedCVertexBufferCtorFn() {
    __asm {
        call hCVertexBufferCtorFn
        jmp nCVertexBufferCtorFn
    };
}

int patchDynamicVB(HMODULE hDll) {
    const char* pat = "0F B6 C8 8B 45 10 51 8B 4D FC 99 68 ?? ?? ?? ?? 68 00 80 00 00";
    size_t offset_imm = 0xe13b - 0xe12a;
    size_t offset_call = 0xe152 - 0xe12a;
    std::uint8_t* ptr = Memory::PatternScan(hDll, pat, 0);
    if (ptr == nullptr) {
        spdlog::info(L"Can not find the DynamicVB call");
        return -1;
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
        return -1;
    }
    tCVertexBufferCtorFn = reinterpret_cast<CVertexBufferCtorFn>(ptr_GameCVertexBufferCtor);
    spdlog::info(L"Hook GameCVertexBufferCtor success");

    // Only for debugging
    // Memory::Write((uintptr_t)hDll + 0xbb67, (uint16_t)0x9090);

    spdlog::info(L"New DynamicVB call to 0x{:0x}", (intptr_t)oCVertexBufferCtorFn);

    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)oCVertexBufferCtorFn, nakedCVertexBufferCtorFn);
    DetourTransactionCommit();

    return 0;
}

// ---------------------------------------------------------------------------------

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

    // if (cfg::GetMaxToRender::debug) {
    //     if (p_bufsize != nullptr && p_nCount != nullptr) {
    //         spdlog::debug("p_bufSize: {} {:x}, p_nCount: {} {:x}", *p_bufsize, *p_bufsize, *p_nCount, *p_nCount);
    //     } else {
    //         spdlog::debug("invalid pointer, omitting, {}, {}", (intptr_t)p_bufsize, (intptr_t)p_nCount);
    //     }
    // }

    if (p_bufsize != nullptr && *p_bufsize == cfg::GetMaxToRender::old_value)  *p_bufsize = cfg::GetMaxToRender::new_value;
    return ret;
}

int patchGetMaxToRender(HMODULE hDll) {
    const char* pat_GetMaxToRender = "55 8B EC 57 8B 7D 08 85 FF 75 ?? 8B 45 10 89 38";
    std::uint8_t* ptr_GetMaxToRender = Memory::PatternScan(hDll, pat_GetMaxToRender, 0);
    oGetMaxToRender = reinterpret_cast<GetMaxToRenderFn>(ptr_GetMaxToRender);
    if (oGetMaxToRender == nullptr) {
        spdlog::info(L"Can not find the GetMaxToRender");
        return -1;
    }
    spdlog::info(L"Hook GetMaxToRender success");

    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)oGetMaxToRender, hGetMaxToRender);
    DetourTransactionCommit();

    return 0;
}

// ---------------------------------------------------------------------------------

int hooks_dvb(HMODULE hDll, std::wstring dllName) {
    auto ret = 0;

    if (cfg::DynamicVB::enable) {
        ret += patchDynamicVB(hDll);
    }

    if (cfg::GetMaxToRender::enable) {
        ret += patchGetMaxToRender(hDll);
    }

    return ret;
}

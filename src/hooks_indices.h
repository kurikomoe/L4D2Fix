#pragma once
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <string>
#include <ranges>

#include "helper.hpp"
#include "vars.h"

int PatchIndices(HMODULE hDll, std::wstring dllName) {
    if (!cfg::Indices::enable) return 0;

    // Tag, pattern, start_search_path, write_offset
    auto f = [&](const wchar_t* tag, const char* pattern, size_t offset, size_t w_offset) {
        std::uint8_t* ptr = Memory::PatternScan(hDll, pattern, offset);

        auto new_buffer_size = cfg::Indices::value;

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

    if (std::ranges::any_of(ptr_cache, [](void* ptr) { return ptr == nullptr; })) {
        return -1;
    }

    return 0;
}

#pragma once
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <windows.h>
#include <string>
#include <cstdlib>
#include <filesystem>

#include <inipp.h>

HMODULE baseModule = GetModuleHandle(NULL);
HMODULE thisModule; // Fix DLL

// Version
std::string sFixName = "L4D2Fix";
std::string sFixVer = "1.4.0-debug";
std::string sLogFile = sFixName + ".log";

// Logger
std::shared_ptr<spdlog::logger> logger;
std::filesystem::path sExePath;
std::string sExeName;
std::filesystem::path sThisModulePath;

// Ini Config
inipp::Ini<char> ini;

// Global config vars
namespace cfg {
namespace  System {
    bool debug;
    std::wstring target;
}

namespace Indices {
    bool enable;
    bool debug;
    int value;
}

namespace IndexBuffer {
    bool enable;
    bool debug;
    int value;
}

namespace VertexBuffer {
    bool enable;
    bool debug;
    int value;
}

namespace DynamicVB {
    bool enable;
    bool debug;
    int value;
}

namespace GetMaxToRender {
    bool enable;
    bool debug;
    int old_value;
    int new_value;
}
}

void LoadIni() {
    using namespace cfg;

    std::ifstream is("kpatch.ini");
    ini.parse(is);

    {
        using namespace System;
        inipp::extract(ini.sections["System"]["debug"], debug);
        std::string tmp_name;
        inipp::extract(ini.sections["System"]["target"], tmp_name);
        target = std::wstring(tmp_name.begin(), tmp_name.end());
    }

    {
        using namespace Indices;
        inipp::extract(ini.sections["Indices"]["enable"], enable);
        inipp::extract(ini.sections["Indices"]["debug"], debug);
        inipp::extract(ini.sections["Indices"]["value"], value);
    }

    // {
    //     using namespace IndexBuffer;
    //     inipp::extract(ini.sections["IndexBuffer"]["enable"], enable);
    //     inipp::extract(ini.sections["IndexBuffer"]["debug"], debug);
    //     inipp::extract(ini.sections["IndexBuffer"]["value"], value);
    // }

    // {
    //     using namespace VertexBuffer;
    //     inipp::extract(ini.sections["VertexBuffer"]["enable"], enable);
    //     inipp::extract(ini.sections["VertexBuffer"]["debug"], debug);
    //     inipp::extract(ini.sections["VertexBuffer"]["value"], value);
    // }

    {
        using namespace DynamicVB;
        inipp::extract(ini.sections["DynamicVB"]["enable"], enable);
        inipp::extract(ini.sections["DynamicVB"]["debug"], debug);
        inipp::extract(ini.sections["DynamicVB"]["value"], value);
    }

    {
        using namespace GetMaxToRender;
        inipp::extract(ini.sections["GetMaxToRender"]["enable"], enable);
        inipp::extract(ini.sections["GetMaxToRender"]["debug"], debug);
        inipp::extract(ini.sections["GetMaxToRender"]["old_value"], old_value);
        inipp::extract(ini.sections["GetMaxToRender"]["new_value"], new_value);
    }
}

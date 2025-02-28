-- add_requires("microsoft-proxy 2.4.0")
add_rules("plugin.compile_commands.autoupdate")
add_rules("mode.debug", "mode.release")

set_languages("cxx20", "c++20")

set_plat("windows")
set_arch("x86")  -- Use "x64" for 64-bit builds

add_requires("spdlog")
add_requires("vcpkg::detours")

if is_mode("release") then
    add_defines("NDEBUG", "NTESTING")
end

local name = "kpatch"
target(name)
    set_kind("shared")
    add_files("src/dllmain.cpp")
    add_packages("spdlog", "vcpkg::detours")
    add_links("user32", "gdi32")

local name = "left4dead2_fix"
target(name)
    set_kind("binary")
    add_files("launcher/main.cpp")
    add_files("assets/app.rc")
    add_links("user32", "gdi32")
    add_packages("vcpkg::detours")


rule("mode.release.testing")
    on_config(function (target)
        if is_mode("release.testing") then
            target:add_defines("NDEBUG")
        end
    end)

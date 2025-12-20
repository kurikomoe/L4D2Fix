MKDIR := if os() == "windows" { "mkdir" } else { "mkdir -p" }
RM_RF := if os() == "windows" { "rd /s /q" } else { "rm -rf" }

TEMPLATE_VER := "v1.0.0"
TARGET := "D:\\Games\\GamePlatforms\\SteamLibrary\\steamapps\\common\\Left 4 Dead 2"

XMAKE_TYPE := "release"
PREFIX := f"{{XMAKE_TYPE}}"

# use `just preset=x64-release config build` to override this
preset := "x86-release-msvc"
target := "main"

clean:
    {{RM_RF}} build

build:
    xmake f -m {{XMAKE_TYPE}} -y
    xmake

install:
    {{MKDIR}} "{{PREFIX}}"
    cp build/windows/x86/{{XMAKE_TYPE}}/left4dead2_fix.exe "{{PREFIX}}"
    cp build/windows/x86/{{XMAKE_TYPE}}/kpatch.dll "{{PREFIX}}"

distinstall:
    install
    cp USAGE.md "{{PREFIX}}/请读我.txt"
    cp "assets/left4dead2_fix - Shortcut.lnk" "{{PREFIX}}"
    cp kpatch.ini "{{PREFIX}}"

[confirm("Remove ALL files under {{PREFIX}} ?")]
uninstall:
    {{RM_RF}} {{PREFIX}}

copy: build
    cp build/windows/x86/release/kpatch.dll         "{{TARGET}}"
    cp build/windows/x86/release/left4dead2_fix.exe "{{TARGET}}"
    cp kpatch.ini                                   "{{TARGET}}"

copyrel: build
    cp build/windows/x86/release/kpatch.dll         "{{TARGET}}"
    cp build/windows/x86/release/left4dead2_fix.exe "{{TARGET}}"

release:
    rm release -rf
    mkdir -p release

    # release
    xmake f -m release -y
    xmake
    cp build/windows/x86/release/kpatch.dll         release/
    cp build/windows/x86/release/left4dead2_fix.exe release/
    cp assets/请读我.txt                            release/
    cp "assets/left4dead2_fix - Shortcut.lnk"       release/
    cp kpatch.ini                                   release/

@run:
    ./build/{{preset}}/bin/{{target}}

[linux]
init:
    bash ./scripts/init_linux.sh

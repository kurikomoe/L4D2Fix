TARGET := "D:\\Games\\GamePlatforms\\SteamLibrary\\steamapps\\common\\Left 4 Dead 2"

XMAKE_TYPE := "release"

# Just version >= 1.44.0
PREFIX := f"{{XMAKE_TYPE}}"

# use `just preset=x64-release config build` to override this
preset := "x86-release-msvc"
target := "main"

build:
    xmake f -m {{XMAKE_TYPE}} -y
    xmake

clean:
    rm -rf build
    rm -rf "{{PREFIX}}"

install:
    mkdir -p "{{PREFIX}}"
    cp build/windows/x86/{{XMAKE_TYPE}}/left4dead2_fix.exe "{{PREFIX}}"
    cp build/windows/x86/{{XMAKE_TYPE}}/kpatch.dll "{{PREFIX}}"

distinstall: install
    cp USAGE.md "{{PREFIX}}/请读我.txt"
    cp "assets/left4dead2_fix - Shortcut.lnk" "{{PREFIX}}"
    cp kpatch.ini "{{PREFIX}}"

[confirm("Remove ALL files under {{PREFIX}} ?")]
uninstall:
    rm -rf {{PREFIX}}

copyall: build
    cp build/windows/x86/release/kpatch.dll         "{{TARGET}}"
    cp build/windows/x86/release/left4dead2_fix.exe "{{TARGET}}"
    cp kpatch.ini                                   "{{TARGET}}"

copy: build
    cp build/windows/x86/release/kpatch.dll         "{{TARGET}}"
    cp build/windows/x86/release/left4dead2_fix.exe "{{TARGET}}"

release: clean build distinstall


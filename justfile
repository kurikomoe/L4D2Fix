TEMPLATE_VER := "v1.0.0"
TARGET := "D:\\Games\\GamePlatforms\\SteamLibrary\\steamapps\\common\\Left 4 Dead 2"

# use `just preset=x64-release config build` to override this
preset := "x86-release-msvc"
target := "main"

[linux]
init:
    bash ./scripts/init_linux.sh

clean:
    rm -rf build

build:
    xmake

copy: build
    cp build/windows/x86/release/kpatch.dll "{{TARGET}}"
    cp build/windows/x86/release/left4dead2_fix.exe "{{TARGET}}"

release:
    rm release -rf
    mkdir -p release/testing

    # release
    xmake f -m release
    xmake
    cp build/windows/x86/release/kpatch.dll         release/
    cp build/windows/x86/release/left4dead2_fix.exe release/
    cp assets/请读我.txt                             release/
    cp third/4gb_patch.exe                          release/

    # testing
    xmake f -m release.testing
    xmake
    cp build/windows/x86/release.testing/kpatch.dll         release/testing
    cp build/windows/x86/release.testing/left4dead2_fix.exe release/testing
    cp assets/请读我-自测补丁.txt                     release/testing
    cp third/4gb_patch.exe                          release/testing

@run:
    ./build/{{preset}}/bin/{{target}}

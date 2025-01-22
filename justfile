TEMPLATE_VER := "v1.0.0"
TARGET := "F:/Games/SteamLibrary/steamapps/common/Left 4 Dead 2"

# use `just preset=x64-release config build` to override this
preset := "x86-release-msvc"
target := "main"

[linux]
init:
    bash ./scripts/init_linux.sh

clean:
    rm -rf build

config:
    cmake --preset {{preset}}

build:
    cmake --build --preset {{preset}} --config Release

copy: build
    cd ./build/x86-release-msvc/bin/Release && \
        cp -f kpatch.dll "{{TARGET}}/kpatch.asi" && \
        cp -f fmt.dll "{{TARGET}}/"
    cp -f third/version.dll "{{TARGET}}/"

@run:
    ./build/{{preset}}/bin/{{target}}

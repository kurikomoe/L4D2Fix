#!nu

echo "init vcpkg"
git clone https://github.com/microsoft/vcpkg.git --depth 1
./vcpkg/bootstrap-vcpkg.bat

./vcpkg/vcpkg new --application

echo "Retarget from linux preset `x64-debug` to windows preset `x64-debug-msvc`"
open justfile | str replace r#'"x64-debug"'# r#'"x64-debug-msvc"'# | save justfile -f

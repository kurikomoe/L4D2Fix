#include <Windows.h>

#pragma region Proxy
struct version_dll {
	HMODULE dll;
	FARPROC oGetFileVersionInfoA;
	FARPROC oGetFileVersionInfoByHandle;
	FARPROC oGetFileVersionInfoExA;
	FARPROC oGetFileVersionInfoExW;
	FARPROC oGetFileVersionInfoSizeA;
	FARPROC oGetFileVersionInfoSizeExA;
	FARPROC oGetFileVersionInfoSizeExW;
	FARPROC oGetFileVersionInfoSizeW;
	FARPROC oGetFileVersionInfoW;
	FARPROC oVerFindFileA;
	FARPROC oVerFindFileW;
	FARPROC oVerInstallFileA;
	FARPROC oVerInstallFileW;
	FARPROC oVerLanguageNameA;
	FARPROC oVerLanguageNameW;
	FARPROC oVerQueryValueA;
	FARPROC oVerQueryValueW;
} version;

extern "C" {
	void fGetFileVersionInfoA() {  _asm jmp[version.oGetFileVersionInfoA] }
	void fGetFileVersionInfoByHandle() {  _asm jmp[version.oGetFileVersionInfoByHandle] }
	void fGetFileVersionInfoExA() {  _asm jmp[version.oGetFileVersionInfoExA] }
	void fGetFileVersionInfoExW() {  _asm jmp[version.oGetFileVersionInfoExW] }
	void fGetFileVersionInfoSizeA() {  _asm jmp[version.oGetFileVersionInfoSizeA] }
	void fGetFileVersionInfoSizeExA() {  _asm jmp[version.oGetFileVersionInfoSizeExA] }
	void fGetFileVersionInfoSizeExW() {  _asm jmp[version.oGetFileVersionInfoSizeExW] }
	void fGetFileVersionInfoSizeW() {  _asm jmp[version.oGetFileVersionInfoSizeW] }
	void fGetFileVersionInfoW() {  _asm jmp[version.oGetFileVersionInfoW] }
	void fVerFindFileA() {  _asm jmp[version.oVerFindFileA] }
	void fVerFindFileW() {  _asm jmp[version.oVerFindFileW] }
	void fVerInstallFileA() {  _asm jmp[version.oVerInstallFileA] }
	void fVerInstallFileW() {  _asm jmp[version.oVerInstallFileW] }
	void fVerLanguageNameA() {  _asm jmp[version.oVerLanguageNameA] }
	void fVerLanguageNameW() {  _asm jmp[version.oVerLanguageNameW] }
	void fVerQueryValueA() {  _asm jmp[version.oVerQueryValueA] }
	void fVerQueryValueW() {  _asm jmp[version.oVerQueryValueW] }
}

void setupFunctions() {
	version.oGetFileVersionInfoA = GetProcAddress(version.dll, "GetFileVersionInfoA");
	version.oGetFileVersionInfoByHandle = GetProcAddress(version.dll, "GetFileVersionInfoByHandle");
	version.oGetFileVersionInfoExA = GetProcAddress(version.dll, "GetFileVersionInfoExA");
	version.oGetFileVersionInfoExW = GetProcAddress(version.dll, "GetFileVersionInfoExW");
	version.oGetFileVersionInfoSizeA = GetProcAddress(version.dll, "GetFileVersionInfoSizeA");
	version.oGetFileVersionInfoSizeExA = GetProcAddress(version.dll, "GetFileVersionInfoSizeExA");
	version.oGetFileVersionInfoSizeExW = GetProcAddress(version.dll, "GetFileVersionInfoSizeExW");
	version.oGetFileVersionInfoSizeW = GetProcAddress(version.dll, "GetFileVersionInfoSizeW");
	version.oGetFileVersionInfoW = GetProcAddress(version.dll, "GetFileVersionInfoW");
	version.oVerFindFileA = GetProcAddress(version.dll, "VerFindFileA");
	version.oVerFindFileW = GetProcAddress(version.dll, "VerFindFileW");
	version.oVerInstallFileA = GetProcAddress(version.dll, "VerInstallFileA");
	version.oVerInstallFileW = GetProcAddress(version.dll, "VerInstallFileW");
	version.oVerLanguageNameA = GetProcAddress(version.dll, "VerLanguageNameA");
	version.oVerLanguageNameW = GetProcAddress(version.dll, "VerLanguageNameW");
	version.oVerQueryValueA = GetProcAddress(version.dll, "VerQueryValueA");
	version.oVerQueryValueW = GetProcAddress(version.dll, "VerQueryValueW");
}
#pragma endregion

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		char path[MAX_PATH];
		GetWindowsDirectory(path, sizeof(path));

		// Example: "\\System32\\version.dll"
		strcat_s(path, "\\add manual path\\version.dll");
		version.dll = LoadLibrary(path);
		setupFunctions();

		// Add here your code, I recommend you to create a thread
		break;
	case DLL_PROCESS_DETACH:
		FreeLibrary(version.dll);
		break;
	}
	return 1;
}

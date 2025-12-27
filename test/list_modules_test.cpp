// Source: https://learn.microsoft.com/en-us/windows/win32/psapi/enumerating-all-modules-for-a-process

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <psapi.h>

// To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS
// and compile with -DPSAPI_VERSION=1

int main(void) {
    HMODULE hMods[1024];
    HANDLE hProcess;
    DWORD cbNeeded;
    unsigned int i;

    // Get a handle to the process.

    hProcess = GetCurrentProcess();

    if(NULL == hProcess)
        return 1;

    // Get a list of all the modules in this process.

    if(EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        for(i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            TCHAR szModName[MAX_PATH];

            // Get the full path to the module's file.

            if(GetModuleFileNameEx(
                hProcess, hMods[i], szModName,
                sizeof(szModName) / sizeof(TCHAR)
            )) {
                // Print the module name and handle value.

                _tprintf(TEXT("\t%s (0x%08X)\n"), szModName, hMods[i]);
            }
        }
    }

    // Release the handle to the process.

    CloseHandle(hProcess);

    return 0;
}

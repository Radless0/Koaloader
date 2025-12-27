// Source: https://learn.microsoft.com/en-us/windows/win32/fileio/listing-the-files-in-a-directory

#define _UNICODE
#define UNICODE

#include <iostream>
#include <map>
#include <strsafe.h>
#include <tchar.h>
#include <Windows.h>

int _tmain(const int argc, TCHAR* argv[]) {
    LoadLibraryA("version.dll");

    WIN32_FIND_DATA ffd;
    LARGE_INTEGER filesize;
    TCHAR szDir[MAX_PATH];
    size_t length_of_arg;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    DWORD dwError = 0;

    // If the directory is not specified as a command-line argument,
    // print usage.

    if(argc != 2) {
        _tprintf(TEXT("\nUsage: %s <directory name>\n"), argv[0]);
        return (-1);
    }

    // Check that the input path plus 3 is not longer than MAX_PATH.
    // Three characters are for the "\*" plus NULL appended below.

    StringCchLength(argv[1], MAX_PATH, &length_of_arg);

    if(length_of_arg > (MAX_PATH - 3)) {
        _tprintf(TEXT("\nDirectory path is too long.\n"));
        return (-1);
    }

    _tprintf(TEXT("Target directory is %s\n"), argv[1]);

    // Prepare string for use with FindFile functions.  First, copy the
    // string to a buffer, then append '\*' to the directory name.

    StringCchCopy(szDir, MAX_PATH, argv[1]);
    StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

    // Find the first file in the directory.

    hFind = FindFirstFile(szDir, &ffd);

    if(INVALID_HANDLE_VALUE == hFind) {
        _tprintf(TEXT("FindFirstFile error"));
        return dwError;
    }

    // List all the files in the directory with some info about them.
    std::map<std::wstring, LONGLONG> found_files;

    do {
        if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            found_files[ffd.cFileName] = -1;
        } else {
            filesize.LowPart = ffd.nFileSizeLow;
            filesize.HighPart = ffd.nFileSizeHigh;

            found_files[{ffd.cFileName}] = filesize.QuadPart;
        }
    } while(FindNextFile(hFind, &ffd) != 0);

    dwError = GetLastError();
    if(dwError != ERROR_NO_MORE_FILES) {
        _tprintf(TEXT("FindFirstFile error"));
    }

    _tprintf(TEXT("Found files:\n"));
    FindClose(hFind);

    for(const auto& [filename, bytes] : found_files) {
        if(bytes >= 0) {
            _tprintf(TEXT("  %s   %lld bytes\n"), filename.c_str(), bytes);
        } else {
            _tprintf(TEXT("  %s   <DIR>\n"), filename.c_str());
        }
    }

    return dwError != ERROR_NO_MORE_FILES;
}

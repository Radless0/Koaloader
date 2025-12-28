#include <regex>

#include <koalabox/config.hpp>
#include <koalabox/hook.hpp>
#include <koalabox/logger.hpp>
#include <koalabox/path.hpp>
#include <koalabox/str.hpp>

#include "file_api.hpp"

#include "koaloader/koaloader.hpp"

namespace {
    namespace kb = koalabox;

    /**
     * @return map where key is file handle, and value is the initial search string
     */
    auto& get_tracked_file_handles() {
        static std::map<HANDLE, std::string> handles;
        return handles;
    }

    bool is_file_hidden(const std::string& filename) {
        for(const auto& pattern : koaloader::config.hide_files) {
            // TODO: Cache regex patterns
            if(std::regex_search(filename, std::regex(pattern, std::regex_constants::icase))) {
                return true;
            }
        }

        return false;
    }
}

#define ORIGINAL(FUNC) kb::hook::get_hooked_function(#FUNC, FUNC)

HANDLE WINAPI $FindFirstFileW(
    const LPCWSTR lpFileName,
    const LPWIN32_FIND_DATAW lpFindFileData
) {
    while(true) {
        auto* const handle = ORIGINAL(FindFirstFileW)(lpFileName, lpFindFileData);
        if(handle == INVALID_HANDLE_VALUE) {
            return handle;
        }

        auto base_path = kb::str::to_str(lpFileName);
        if(base_path.ends_with("*")) {
            base_path = base_path.substr(0, base_path.length() - 1);
        }
        get_tracked_file_handles()[handle] = base_path;

        const auto hiding = is_file_hidden(kb::str::to_str(lpFindFileData->cFileName));

        LOG_DEBUG(
            R"({} -> query: "{}", handle: {}, filename: "{}", hiding: {})",
            __func__,
            kb::str::to_str(lpFileName),
            reinterpret_cast<uintptr_t>(handle),
            kb::str::to_str(lpFindFileData->cFileName),
            hiding
        );

        if(hiding) {
            continue;
        }

        return handle;
    }
}

HANDLE WINAPI $FindFirstFileExW(
    const LPCWSTR lpFileName,
    const FINDEX_INFO_LEVELS fInfoLevelId,
    const LPWIN32_FIND_DATAW lpFindFileData,
    const FINDEX_SEARCH_OPS fSearchOp,
    const LPVOID lpSearchFilter,
    const DWORD dwAdditionalFlags
) {
    while(true) {
        auto* const handle = ORIGINAL(FindFirstFileExW)(
            lpFileName,
            fInfoLevelId,
            lpFindFileData,
            fSearchOp,
            lpSearchFilter,
            dwAdditionalFlags
        );

        if(handle == INVALID_HANDLE_VALUE) {
            return handle;
        }

        auto base_path = kb::str::to_str(lpFileName);
        if(base_path.ends_with("*")) {
            base_path = base_path.substr(0, base_path.length() - 1);
        }
        get_tracked_file_handles()[handle] = base_path;

        const auto hiding = is_file_hidden(kb::str::to_str(lpFindFileData->cFileName));

        LOG_DEBUG(
            "{} -> query: {}, handle: {}, filename: \"{}\", hiding: {}",
            __func__,
            kb::str::to_str(lpFileName),
            reinterpret_cast<uintptr_t>(handle),
            kb::str::to_str(lpFindFileData->cFileName),
            hiding
        );

        if(hiding) {
            continue;
        }

        return handle;
    }
}

BOOL WINAPI $FindNextFileW(
    HANDLE hFindFile,
    const LPWIN32_FIND_DATAW lpFindFileData
) {
    while(true) {
        const auto success = ORIGINAL(FindNextFileW)(hFindFile, lpFindFileData);

        if(success && get_tracked_file_handles().contains(hFindFile)) {
            const auto hiding = is_file_hidden(kb::str::to_str(lpFindFileData->cFileName));

            LOG_DEBUG(
                "{} -> handle: {}, filename: \"{}\", hiding: {}",
                __func__,
                reinterpret_cast<uintptr_t>(hFindFile),
                kb::str::to_str(lpFindFileData->cFileName),
                hiding
            );

            if(hiding) {
                continue;
            }
        }

        return success;
    }
}

BOOL WINAPI $FindClose(const HANDLE hFindFile) {
    const auto result = ORIGINAL(FindClose)(hFindFile);

    if(get_tracked_file_handles().contains(hFindFile)) {
        LOG_DEBUG(
            "{} -> handle: {}, result: {}",
            __func__, reinterpret_cast<uintptr_t>(hFindFile), static_cast<bool>(result)
        );
        get_tracked_file_handles().erase(hFindFile);
    }

    return result;
}

DWORD WINAPI $GetFileAttributesA(
    _In_ LPCSTR lpFileName
) {
    const auto file_name = std::string(lpFileName);
    const auto hiding = is_file_hidden(file_name);

    LOG_DEBUG("{} -> file_name: \"{}\", hiding: {}", __func__, file_name, hiding);

    if(hiding) {
        return INVALID_FILE_ATTRIBUTES;
    }

    const auto result = ORIGINAL(GetFileAttributesA)(
        lpFileName
    );

    return result;
}

DWORD WINAPI $GetFileAttributesW(
    _In_ LPCWSTR lpFileName
) {
    const auto file_name = kb::str::to_str(lpFileName);
    const auto hiding = is_file_hidden(file_name);

    LOG_DEBUG("{} -> file_name: \"{}\", hiding: {}", __func__, file_name, hiding);

    if(hiding) {
        return INVALID_FILE_ATTRIBUTES;
    }

    const auto result = ORIGINAL(GetFileAttributesW)(
        lpFileName
    );

    return result;
}

BOOL WINAPI $GetFileAttributesExA(
    const LPCSTR lpFileName,
    const GET_FILEEX_INFO_LEVELS fInfoLevelId,
    WIN32_FILE_ATTRIBUTE_DATA* lpFileInformation
) {
    const auto file_name = std::string(lpFileName);
    const auto hiding = is_file_hidden(file_name);

    LOG_DEBUG("{} -> file_name: \"{}\", hiding: {}", __func__, file_name, hiding);

    if(hiding) {
        *lpFileInformation = WIN32_FILE_ATTRIBUTE_DATA{};
        return FALSE;
    }

    const auto result = ORIGINAL(GetFileAttributesExA)(
        lpFileName,
        fInfoLevelId,
        lpFileInformation
    );

    return result;
}
BOOL WINAPI $GetFileAttributesExW(
    const LPCWSTR lpFileName,
    const GET_FILEEX_INFO_LEVELS fInfoLevelId,
    WIN32_FILE_ATTRIBUTE_DATA* lpFileInformation
) {
    const auto file_name = kb::str::to_str(lpFileName);
    const auto hiding = is_file_hidden(file_name);

    LOG_DEBUG("{} -> file_name: \"{}\", hiding: {}", __func__, file_name, hiding);

    if(hiding) {
        *lpFileInformation = WIN32_FILE_ATTRIBUTE_DATA{};
        return FALSE;
    }

    const auto result = ORIGINAL(GetFileAttributesExW)(
        lpFileName,
        fInfoLevelId,
        lpFileInformation
    );

    return result;
}

HANDLE WINAPI $CreateFileA(
    _In_ LPCSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile
) {
    // TODO: More robust checks

    const auto file_name = std::string(lpFileName);
    const auto hiding = is_file_hidden(file_name);

    LOG_DEBUG("{} -> file_name: \"{}\", hiding: {}", __func__, file_name, hiding);

    if(hiding) {
        return INVALID_HANDLE_VALUE;
    }

    const auto result = ORIGINAL(CreateFileA)(
        lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile
    );

    return result;
}

HANDLE WINAPI $CreateFileW(
    _In_ LPCWSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile
) {
    // TODO: More robust checks

    const auto file_name = kb::str::to_str(lpFileName);
    const auto hiding = is_file_hidden(file_name);

    LOG_DEBUG("{} -> file_name: \"{}\", hiding: {}", __func__, file_name, hiding);

    if(hiding) {
        return INVALID_HANDLE_VALUE;
    }

    auto* const result = ORIGINAL(CreateFileW)(
        lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile
    );

    return result;
}

namespace file_api {
    void hide_files() {
#define KL_HOOK(FUNC) kb::hook::detour( \
    reinterpret_cast<void*>(FUNC), \
    #FUNC, \
    reinterpret_cast<void*>($##FUNC) \
)
        LOG_INFO("Initializing file hider...");

        KL_HOOK(FindFirstFileW);
        KL_HOOK(FindFirstFileExW);
        KL_HOOK(FindNextFileW);
        KL_HOOK(FindClose);
        KL_HOOK(GetFileAttributesA);
        KL_HOOK(GetFileAttributesW);
        KL_HOOK(GetFileAttributesExA);
        KL_HOOK(GetFileAttributesExW);
        KL_HOOK(CreateFileA);
        KL_HOOK(CreateFileW);

        LOG_INFO("File hider initialized");
    }
}

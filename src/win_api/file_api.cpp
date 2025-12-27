#include <set>

#include <koalabox/config.hpp>
#include <koalabox/hook.hpp>
#include <koalabox/logger.hpp>
#include <koalabox/path.hpp>
#include <koalabox/str.hpp>

#include "file_api.hpp"
#include "koaloader/koaloader.hpp"

namespace {
    namespace kb = koalabox;
    namespace fs = std::filesystem;

    /**
     * @return map where key is file handle, and value is the initial search string
     */
    auto& get_tracked_file_handles() {
        static std::map<HANDLE, std::string> handles;
        return handles;
    }

    std::string normalize_path(const fs::path& abs_path) {
        const auto canonical_path_wstr = fs::weakly_canonical(abs_path).wstring();
        const auto canonical_path_str = kb::str::to_str(canonical_path_wstr);
        return kb::str::to_lower(canonical_path_str);
    }

    std::set<std::string>& get_hidden_files() {
        static std::set<std::string> hidden_files;

        static std::once_flag once_flag;
        std::call_once(
            once_flag, [] {
                for(const auto& extra_file : koaloader::config.hide_files) {
                    const auto abs_path = fs::absolute(extra_file);
                    const auto path_str = normalize_path(abs_path);
                    hidden_files.insert(path_str);
                }
            }
        );

        return hidden_files;
    }

    bool should_file_be_hidden(const HANDLE file_handle, const WIN32_FIND_DATAW* file) {
        const auto base_path = get_tracked_file_handles()[file_handle];
        const auto path = kb::path::from_str(base_path) / file->cFileName;
        const auto normalized_path_str = normalize_path(path);

        return get_hidden_files().contains(normalized_path_str);
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

        const auto hiding = should_file_be_hidden(handle, lpFindFileData);

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

        const auto hiding = should_file_be_hidden(handle, lpFindFileData);

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
            const auto hiding = should_file_be_hidden(hFindFile, lpFindFileData);

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

BOOL WINAPI $GetFileAttributesExW(
    const LPCWSTR lpFileName,
    const GET_FILEEX_INFO_LEVELS fInfoLevelId,
    WIN32_FILE_ATTRIBUTE_DATA* lpFileInformation
) {
    const auto result = ORIGINAL(GetFileAttributesExW)(
        lpFileName,
        fInfoLevelId,
        lpFileInformation
    );

    // TODO: More robust checks

    const auto file_name = kb::str::to_str(lpFileName);
    const auto hiding = koaloader::config.hide_files.contains(file_name);

    LOG_DEBUG("{} -> file_name: {}, hiding: {}", __func__, file_name, hiding);

    if(hiding) {
        return FALSE;
    }

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
        KL_HOOK(GetFileAttributesExW);

        LOG_INFO("File hider initialized");
    }
}

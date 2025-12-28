#include <regex>

#include <polyhook2/MemProtector.hpp>

#include "koalabox/lib.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/patcher.hpp"

#include "patcher/patcher.hpp"
#include "koaloader/koaloader.hpp"

namespace {
    namespace kb = koalabox;

    void patch_string(koaloader::Patch patch) {
        try {
            auto* const current_process_handle = kb::lib::get_exe_handle();
            const auto section = kb::lib::get_section_or_throw(current_process_handle, patch.section);

            const std::string_view section_str_view(static_cast<char*>(section.start_address), section.size);
            const std::regex regex_pattern(patch.pattern);

            std::match_results<std::string_view::const_iterator> m;
            if(std::regex_search(section_str_view.begin(), section_str_view.end(), m, regex_pattern)) {
                char* match_start = static_cast<char*>(section.start_address) + m.position(0);
                LOG_INFO(
                    R"({} -> Patching "{}" found at {:#x})",
                    __func__, patch.pattern, reinterpret_cast<uintptr_t>(match_start)
                );

                // Enable writing to the memory section
                static auto accessor = PLH::MemAccessor();
                const PLH::MemoryProtector protector(
                    reinterpret_cast<uint64_t>(match_start),
                    patch.replacement.size(),
                    PLH::ProtFlag::RWX,
                    accessor,
                    true
                );

                std::memcpy(match_start, patch.replacement.c_str(), patch.replacement.size());
            } else {
                LOG_WARN(
                    R"({} -> Pattern "{}" not found in section "{}")",
                    __func__, patch.pattern, patch.section
                );
            }
        } catch(const std::exception& e) {
            LOG_ERROR("{} -> Error: {}", __func__, e.what());
        }
    }
}

namespace patcher {
    void patch_strings() {
        if(koaloader::config.string_patches.empty()) {
            return;
        }

        LOG_INFO("Patching strings...");

        for(const auto& patch : koaloader::config.string_patches) {
            patch_string(patch);
        }

        LOG_INFO("Patching strings complete");
    }
}

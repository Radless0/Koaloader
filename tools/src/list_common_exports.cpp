#include <random>
#include <ranges>
#include <regex>
#include <set>

#include <glob/glob.h>

#include <koalabox/lib.hpp>
#include <koalabox/logger.hpp>
#include <koalabox/path.hpp>

namespace {
    namespace kb = koalabox;

    std::vector<std::string> parse_glob_input(const int argc, char* argv[]) {
        if(argc < 2) {
            throw std::runtime_error("No arguments provided.");
        }

        std::vector<std::string> paths;
        for(auto i = 1; i < argc; ++i) {
            paths.emplace_back(argv[i]);
        }
        return paths;
    }

    using symbol_name_t = std::string;
    using lib_path_t = std::filesystem::path;
    using symbol_to_lib_path_t = std::map<symbol_name_t, std::set<lib_path_t>>;

    auto construct_symbol_to_lib_path_map(const std::vector<std::string>& glob_strings) {
        symbol_to_lib_path_t result;

        for(const auto& glob_str : glob_strings) {
            const auto glob_paths = glob::glob(glob_str);

            for(const auto& lib_path : glob_paths) {
                const auto lib_handle = kb::lib::load_library_or_throw(lib_path);

                // Avoid streaming views over the export map which MSVC's ranges implementation
                // may not accept for this container. Materialize the export map into a local
                // variable and iterate using structured bindings.
                const auto export_map = kb::lib::get_export_map(lib_handle);
                for(const auto& [symbol, _] : export_map) {
                    auto& lib_set = result[symbol];
                    lib_set.insert(lib_path);
                }
            }
        }

        return result;
    }

    void print_common_exports(const symbol_to_lib_path_t& symbol_to_lib_path_map) {
        for(const auto& [symbol_name, lib_path_set] : symbol_to_lib_path_map) {
            if(lib_path_set.empty()) {
                LOG_ERROR("Invalid state: empty lib path set for symbol '{}':", symbol_name);
                DebugBreak();
                continue;
            }

            if(lib_path_set.size() > 1) {
                LOG_INFO("Common symbol '{}' found in {} libraries:", symbol_name, lib_path_set.size());
                for(const auto& lib_path : lib_path_set) {
                    LOG_INFO("    {}", kb::path::to_str(lib_path));
                }
            }
        }
    }
}

int main(const int argc, char* argv[]) { // NOLINT(*-use-internal-linkage)
    try {
        kb::logger::init_console_logger();

        const auto glob_strings = parse_glob_input(argc, argv);
        const auto symbol_to_lib_path_map = construct_symbol_to_lib_path_map(glob_strings);
        print_common_exports(symbol_to_lib_path_map);

        return EXIT_SUCCESS;
    } catch(const std::exception& e) {
        LOG_ERROR("Unhandled global exception: {}", e.what());
        return EXIT_FAILURE;
    }
}

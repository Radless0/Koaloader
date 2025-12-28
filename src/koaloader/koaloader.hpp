#pragma once

#include <set>
#include <string>

#include <nlohmann/json.hpp>

namespace koaloader {
    struct Module {
        std::string path;
        bool required = true;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Module, path, required)
    };

    struct Patch {
        std::string section;
        std::string pattern;
        std::string replacement;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Patch, section, pattern, replacement)
    };

    struct Config {
        bool logging = false;
        bool enabled = true;
        bool auto_load = true;
        std::vector<std::string> targets;
        std::vector<Module> modules;
        std::set<std::string> hide_files;
        std::vector<Patch> string_patches;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
            Config, logging, enabled, auto_load, targets, modules, hide_files, string_patches
        )
    };

    extern Config config;

    void init(HMODULE self_module);

    void shutdown();
}

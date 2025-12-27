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

    struct Config {
        bool logging = false;
        bool enabled = true;
        bool auto_load = true;
        std::vector<std::string> targets;
        std::vector<Module> modules;
        std::set<std::string> hide_files;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
            Config, logging, enabled, auto_load, targets, modules, hide_files
        )
    };

    extern Config config;

    void init(HMODULE self_module);

    void shutdown();

}

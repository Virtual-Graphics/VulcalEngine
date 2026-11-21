#pragma once
#include "base/path_utils.h"
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <yaml-cpp/yaml.h>

namespace editor {

struct BuildInfo {
    std::filesystem::path open_scene;
    std::filesystem::path build_dir;
};

struct ProjectInfo {
    std::string name;
    std::string description;
    std::string author;

    // relative paths to project file
    std::filesystem::path assets_path;
    std::filesystem::path last_edited_scene;
    std::optional<BuildInfo> build_info;
};

inline bool save_project_info(const std::filesystem::path& project_file, const ProjectInfo& info) {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << info.name;
    out << YAML::Key << "description" << YAML::Value << info.description;
    out << YAML::Key << "author" << YAML::Value << info.author;
    out << YAML::Key << "assets_path" << YAML::Value << base::path_to_utf8(info.assets_path);
    out << YAML::Key << "last_edited_scene" << YAML::Value << base::path_to_utf8(info.last_edited_scene);
    if (info.build_info) {
        YAML::Node build_info(YAML::NodeType::Map);
        build_info["open_scene"] = base::path_to_utf8(info.build_info->open_scene);
        if (!info.build_info->build_dir.empty()) build_info["build_dir"] = base::path_to_utf8(info.build_info->build_dir);
        out << YAML::Key << "build_info" << YAML::Value << build_info;
    }
    out << YAML::EndMap;

    std::ofstream fout(project_file);
    if (!fout.is_open()) {
        return false;
    }
    fout << out.c_str();
    fout.close();
    return static_cast<bool>(fout);
}

inline bool load_project_info(const std::filesystem::path& project_file, ProjectInfo& info) {
    try {
        std::ifstream input(project_file);
        if (!input) return false;
        YAML::Node node = YAML::Load(input);
        if (!node.IsMap()) {
            return false;
        }

        info.name              = node["name"].as<std::string>();
        info.description       = node["description"].as<std::string>();
        info.author            = node["author"].as<std::string>();
        info.assets_path       = base::path_from_utf8(node["assets_path"].as<std::string>());
        info.last_edited_scene = base::path_from_utf8(node["last_edited_scene"].as<std::string>());
        info.build_info.reset();
        if (const auto build_info = node["build_info"]) {
            if (!build_info.IsMap() || !build_info["open_scene"]) return false;
            BuildInfo loaded;
            loaded.open_scene = base::path_from_utf8(build_info["open_scene"].as<std::string>());
            if (build_info["build_dir"]) loaded.build_dir = base::path_from_utf8(build_info["build_dir"].as<std::string>());
            info.build_info = std::move(loaded);
        }
    } catch (const YAML::Exception&) {
        return false;
    }

    return true;
}

} // namespace editor

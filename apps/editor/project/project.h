#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <yaml-cpp/yaml.h>

namespace editor {

struct ProjectInfo {
    std::string name;
    std::string description;
    std::string author;

    // relative paths to project file
    std::filesystem::path assets_path;
    std::filesystem::path last_edited_scene;
};

inline bool save_project_info(const std::filesystem::path& project_file, const ProjectInfo& info) {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "name"              << YAML::Value << info.name;
    out << YAML::Key << "description"       << YAML::Value << info.description;
    out << YAML::Key << "author"            << YAML::Value << info.author;
    out << YAML::Key << "assets_path"       << YAML::Value << info.assets_path.generic_string();
    out << YAML::Key << "last_edited_scene" << YAML::Value << info.last_edited_scene.generic_string();
    out << YAML::EndMap;

    std::ofstream fout(project_file);
    if (!fout.is_open()) {
        return false;
    }
    fout << out.c_str();
    return true;
}

inline bool load_project_info(const std::filesystem::path& project_file, ProjectInfo& info) {
    try {
        YAML::Node node = YAML::LoadFile(project_file.string());
        if (!node.IsMap()) {
            return false;
        }

        info.name              = node["name"].as<std::string>();
        info.description       = node["description"].as<std::string>();
        info.author            = node["author"].as<std::string>();
        info.assets_path       = node["assets_path"].as<std::string>();
        info.last_edited_scene = node["last_edited_scene"].as<std::string>();
    } catch (const YAML::Exception&) {
        return false;
    }

    return true;
}

} // namespace editor

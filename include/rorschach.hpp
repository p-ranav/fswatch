#pragma once
#include <filesystem>
#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <unordered_map>
#include <regex>
#include <iostream>

class Rorschach {
public:
  bool running = true;
  // Root directory of the file watcher
  std::string path;
  // Periodicity of the file watcher, i.e., how often it checks to see if files have changed
  std::chrono::duration<int, std::milli> period;
  // Dictionary that maps files with their respective last_write_time timestmaps
  std::unordered_map<std::string, std::filesystem::file_time_type> file_last_write_time_map;
  // Regex of paths to ignore
  std::regex ignore_path;

  std::function<void (const std::string&)> on_path_created, on_path_modified, on_path_erased;

  Rorschach(const std::string& path, std::chrono::duration<int, std::milli> period, const std::regex& ignore_path) : 
    running(true), path(path), period(period), ignore_path(ignore_path), file_last_write_time_map({}) {}

  void watch() {

    // Build file map for user-specified path
    for (auto& file: std::filesystem::recursive_directory_iterator(path, std::filesystem::directory_options::skip_permission_denied)) {
      std::string filename = file.path().string();
      file_last_write_time_map[file.path().string()] = std::filesystem::last_write_time(file);
       std::smatch match;
      // Watch this file only if it is not in the ignore list
      if (std::regex_search(filename, match, ignore_path) && match.size() == 0)
        file_last_write_time_map[filename] = std::filesystem::last_write_time(file);
    }

    // Start watching files in path
    while(running) {
      std::this_thread::sleep_for(period);
      auto it = file_last_write_time_map.begin();

      // Check if the file was erased
      while (it != file_last_write_time_map.end()) {
        if (!std::filesystem::exists(it->first)) {
          if (on_path_erased) on_path_erased(it->first);
          it = file_last_write_time_map.erase(it);
        }
        else {
          it++;
        }
      }

      // Check if the file was created or modified
      for (auto& file : std::filesystem::recursive_directory_iterator(path)) {

        std::string filename = file.path().string();
        std::smatch match;
        // Watch this file only if it is not in the ignore list
        if (std::regex_search(filename, match, ignore_path) && match.size() == 0) {
          std::error_code ec;
          auto current_file_last_write_time = std::filesystem::last_write_time(file, ec);
          if (!contains(file.path().string())) {
            file_last_write_time_map[file.path().string()] = current_file_last_write_time;
            if (on_path_created) on_path_created(file.path().string());
          }
          else {
            if (file_last_write_time_map[file.path().string()] != current_file_last_write_time) {
              file_last_write_time_map[file.path().string()] = current_file_last_write_time;
              if (on_path_modified) on_path_modified(file.path().string());
            }
          }
        }
      }

    }
  }

private:
  bool contains(const std::string& file) {
    return file_last_write_time_map.find(file) != file_last_write_time_map.end();
  }

};
#pragma once
#include <filesystem>
#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <unordered_map>

class Rorschach {
public:
  bool running = true;
  // Root directory of the file watcher
  std::string path;
  // Periodicity of the file watcher, i.e., how often it checks to see if files have changed
  std::chrono::duration<int, std::milli> period;
  // Dictionary that maps files with their respective last_write_time timestmaps
  std::unordered_map<std::string, std::filesystem::file_time_type> file_last_write_time_map;

  std::function<void (const std::string&)> on_path_created, on_path_modified, on_path_erased;

  Rorschach(const std::string& path, std::chrono::duration<int, std::milli> period) : 
    running(true), path(path), period(period), file_last_write_time_map({}) {}

  void watch() {

    // Build file map for user-specified path
    for (auto& file: std::filesystem::recursive_directory_iterator(path)) {
      file_last_write_time_map[file.path().string()] = std::filesystem::last_write_time(file);
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
        auto current_file_last_write_time = std::filesystem::last_write_time(file);
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

private:
  bool contains(const std::string& file) {
    return file_last_write_time_map.find(file) != file_last_write_time_map.end();
  }

};
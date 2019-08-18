#pragma once
#include <filesystem>
#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <unordered_map>

namespace watch {

class FileWatcher {
public:
  bool running = true;
  // Root directory of the file watcher
  std::string path;
  // Periodicity of the file watcher, i.e., how often it checks to see if files have changed
  std::chrono::duration<int, std::milli> period;
  // Dictionary that maps files with their respective last_write_time timestmaps
  std::unordered_map<std::string, std::filesystem::file_time_type> file_last_write_time_map;

  std::function<void (const std::string&)> created, modified, erased;

  FileWatcher(const std::string& path, std::chrono::duration<int, std::milli> period) :
    path(path), period(period) {
    for (auto& file: std::filesystem::recursive_directory_iterator(path)) {
      file_last_write_time_map[file.path().string()] = std::filesystem::last_write_time(file);
    }
  }

  FileWatcher& configure() {
    return *this;
  }

  FileWatcher& on_created(const std::function<void (const std::string&)> &action) {
    created = action;
    return *this;
  }

  FileWatcher& on_modified(const std::function<void (const std::string&)> &action) {
    modified = action;
    return *this;
  }

  FileWatcher& on_erased(const std::function<void (const std::string&)> &action) {
    erased = action;
    return *this;
  }

  void start() {
    while(running) {
      std::this_thread::sleep_for(period);
      auto it = file_last_write_time_map.begin();
      while (it != file_last_write_time_map.end()) {
        if (!std::filesystem::exists(it->first)) {
          if (erased) erased(it->first);
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
          if (created) created(file.path().string());
        }
        else {
          if (file_last_write_time_map[file.path().string()] != current_file_last_write_time) {
            file_last_write_time_map[file.path().string()] = current_file_last_write_time;
            if (modified) modified(file.path().string());
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

} // namespace watch
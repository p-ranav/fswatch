#pragma once
#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <regex>
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>

enum class FileStatus { FILE_CREATED, FILE_MODIFIED, FILE_ERASED };

class Rorschach {
public:
  Rorschach(const std::string &path,
            std::chrono::duration<int, std::milli> period)
      : running_(true), path(expand(std::filesystem::path(path))),
        match_regex_provided(false), ignore_regex_provided(false),
        period(period), lwt_map_({}) {}

  void match(const std::regex &match) { 
    match_path = match; 
    match_regex_provided = true;
  }
  void ignore(const std::regex &ignore) { 
    ignore_path = ignore;
    ignore_regex_provided = true; 
  }

  void on(const FileStatus &event,
          const std::function<void(const std::filesystem::path &)> &action) {
    switch (event) {
    case FileStatus::FILE_CREATED:
      on_path_created = action;
      break;
    case FileStatus::FILE_MODIFIED:
      on_path_modified = action;
      break;
    case FileStatus::FILE_ERASED:
      on_path_erased = action;
      break;
    }
  }

  void watch() {

    // Build file map for user-specified path
    for (auto &file : std::filesystem::recursive_directory_iterator(
             std::filesystem::absolute(path),
             std::filesystem::directory_options::skip_permission_denied)) {
      if (!should_ignore(file)) {
        if (should_watch(file)) {
          std::error_code ec;
          lwt_map_[file.path().string()] =
              std::filesystem::last_write_time(file, ec);
        }
      }
    }

    // Start watching files in path
    while (running_) {
      std::this_thread::sleep_for(period);
      auto it = lwt_map_.begin();

      // Check if the file was erased
      while (it != lwt_map_.end()) {
        if (!std::filesystem::exists(it->first)) {
          if (on_path_erased)
            on_path_erased(std::filesystem::path(it->first));
          it = lwt_map_.erase(it);
        } else {
          it++;
        }
      }

      // Check if the file was created or modified
      for (auto &file : std::filesystem::recursive_directory_iterator(
               std::filesystem::absolute(path),
               std::filesystem::directory_options::skip_permission_denied)) {
        std::string filename = file.path().string();
        // Watch this file only if it is not in the ignore list
        if (!should_ignore(file)) {
          if (should_watch(file)) {
            std::error_code ec;
            auto current_file_last_write_time =
                std::filesystem::last_write_time(file, ec);
            if (!contains(file.path().string())) {
              lwt_map_[file.path().string()] = current_file_last_write_time;
              if (on_path_created)
                on_path_created(file.path());
            } else {
              if (lwt_map_[file.path().string()] !=
                  current_file_last_write_time) {
                lwt_map_[file.path().string()] = current_file_last_write_time;
                if (on_path_modified)
                  on_path_modified(file.path());
              }
            }
          }
        }
      }
    }
  }

private:
  // Root directory of the file watcher
  std::filesystem::path path;
  // Periodicity of the file watcher, i.e., how often it checks to see if files
  // have changed
  std::chrono::duration<int, std::milli> period;
  // Regex of paths to ignore
  std::regex match_path, ignore_path;
  // Is a match/ignore regex provided
  bool match_regex_provided, ignore_regex_provided;
  // Callback functions based on file status
  std::function<void(const std::filesystem::path &)> on_path_created,
      on_path_modified, on_path_erased;
  // Manage status of file watcher
  bool running_ = true;
  // Dictionary that maps files with their respective last_write_time timestmaps
  std::unordered_map<std::string, std::filesystem::file_time_type> lwt_map_;

  bool contains(const std::string &file) {
    return lwt_map_.find(file) != lwt_map_.end();
  }

  std::filesystem::path expand(std::filesystem::path in) {
    const char *home = getenv("HOME");
    if (!home)
      throw std::invalid_argument("HOME environment variable not set.");

    std::string result = in.c_str();
    if (result.length() > 0 && result[0] == '~') {
      result = std::string(home) + result.substr(1, result.size() - 1);
      return std::filesystem::path(result);
    }
    return in;
  }

  bool should_ignore(const std::filesystem::directory_entry &path) {
    if (!ignore_regex_provided) return false;
    std::string filename = path.path().string();
    if (!std::regex_search(filename, ignore_path))
      return false;
    else
      return true;
  }

  bool should_watch(const std::filesystem::directory_entry &path) {
    if (!match_regex_provided) return true;
    std::string filename = path.path().string();
    if (std::regex_search(filename, match_path))
      return true;
    else
      return false;
  }
};

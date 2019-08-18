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
  std::filesystem::path path;
  // Periodicity of the file watcher, i.e., how often it checks to see if files have changed
  std::chrono::duration<int, std::milli> period;
  // Regex of paths to ignore
  std::regex ignore_path;
  // Callback functions based on file status
  std::function<void (const std::filesystem::path&)> on_path_created, on_path_modified, on_path_erased;

  Rorschach(const std::string& path, std::chrono::duration<int, std::milli> period) : 
    running(true), path(expand(std::filesystem::path(path))), period(period), file_last_write_time_map({}) {}

  void ignore(std::regex ignore_path) {
    ignore_path = ignore_path;
  }

  bool should_ignore(const std::filesystem::directory_entry& path) {
      std::string filename = path.path().string();
      std::smatch match;
      if (!std::regex_search(filename, ignore_path)) {
        return false;
      }
      else {
        return true;
      }
  }

  void watch() {

    // Build file map for user-specified path
    for (auto& file: 
	   std::filesystem::recursive_directory_iterator(std::filesystem::absolute(path), 
							 std::filesystem::directory_options::skip_permission_denied)) {
      if (!should_ignore(file)) {
        std::error_code ec;
        file_last_write_time_map[file.path().string()] = std::filesystem::last_write_time(file, ec);
      }
    }

    // Start watching files in path
    while(running) {
      std::this_thread::sleep_for(period);
      auto it = file_last_write_time_map.begin();

      // Check if the file was erased
      while (it != file_last_write_time_map.end()) {
        if (!std::filesystem::exists(it->first)) {
          if (on_path_erased) on_path_erased(std::filesystem::path(it->first));
          it = file_last_write_time_map.erase(it);
        }
        else {
          it++;
        }
      }

      // Check if the file was created or modified
      for (auto& file: 
	     std::filesystem::recursive_directory_iterator(std::filesystem::absolute(path), 
							   std::filesystem::directory_options::skip_permission_denied)) {
        std::string filename = file.path().string();
        // Watch this file only if it is not in the ignore list
        if (!should_ignore(file)) {
          std::error_code ec;
          auto current_file_last_write_time = std::filesystem::last_write_time(file, ec);
          if (!contains(file.path().string())) {
            file_last_write_time_map[file.path().string()] = current_file_last_write_time;
            if (on_path_created) on_path_created(file.path());
          }
          else {
            if (file_last_write_time_map[file.path().string()] != current_file_last_write_time) {
              file_last_write_time_map[file.path().string()] = current_file_last_write_time;
              if (on_path_modified) on_path_modified(file.path());
            }
          }
        }
      }

    }
  }

private:
  // Dictionary that maps files with their respective last_write_time timestmaps
  std::unordered_map<std::string, std::filesystem::file_time_type> file_last_write_time_map;

  bool contains(const std::string& file) {
    return file_last_write_time_map.find(file) != file_last_write_time_map.end();
  }

  std::filesystem::path expand (std::filesystem::path in) {
    const char * home = getenv ("HOME");
    if (home == NULL) {
      std::cerr << "error: HOME variable not set." << std::endl;
      throw std::invalid_argument ("error: HOME environment variable not set.");
    }

    std::string s = in.c_str ();
    if (s[0] == '~') {
      s = std::string(home) + s.substr (1, s.size () - 1);
      return std::filesystem::path (s);
    } else {
      return in;
    }
  }

};

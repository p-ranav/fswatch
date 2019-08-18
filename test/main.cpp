#include <iostream>
#include <file_watcher.hpp>

int main() {
  auto watcher = FileWatcher("", std::chrono::milliseconds(500));
  watcher.skip_permission_denied();
  watcher.match(std::regex("bar.txt"));
  // Only match foo.txt or bar.csv
  // watcher.match(std::regex("foo.txt|bar.csv"));
  // Ignore .ini files
  // watcher.ignore(std::regex(".*\\.ini$")); 

  watcher.on(FileStatus::FILE_CREATED, [](auto &path) {
    std::cout << "Path created: " << path << std::endl;
  });

  watcher.on(FileStatus::FILE_MODIFIED, [](auto &path) {
    std::cout << "Path modified: " << path << std::endl;
  });

  watcher.on(FileStatus::FILE_ERASED, [](auto &path) {
    std::cout << "Path erased: " << path << std::endl;
  });

  try {
    watcher.start();
  } catch (std::filesystem::filesystem_error &error) {
    std::cout << error.what() << std::endl;
  }
}

#include <iostream>
#include <file_watcher.hpp>

int main() {
  auto watcher = FileWatcher("/home/pranav", std::chrono::milliseconds(500));
  // watcher.match(std::regex("bar.txt"));
  // Only match foo.txt or bar.csv
  // watcher.match(std::regex("foo.txt|bar.csv"));
  // Ignore .ini files
  // watcher.ignore(std::regex(".*\\.ini$")); 

  watcher.on(FileWatcher::Event::FILE_CREATED, [](auto &path) {
    std::cout << "Path created: " << path << std::endl;
  });

  watcher.on(FileWatcher::Event::FILE_MODIFIED, [](auto &path) {
    std::cout << "Path modified: " << path << std::endl;
  });

  watcher.on(FileWatcher::Event::FILE_ERASED, [](auto &path) {
    std::cout << "Path erased: " << path << std::endl;
  });

  try {
    watcher.start();
  } catch (std::filesystem::filesystem_error &error) {
    std::cout << error.what() << std::endl;
  }
}

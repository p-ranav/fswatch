#include <file_watcher.hpp>
#include <iostream>

int main() {
  auto watcher = FileWatcher("");

  watcher.on(FileWatcher::Event::DIR_CREATED, [](auto &path) {
    std::cout << "Directory created: " << path << std::endl;
  });

  watcher.on(FileWatcher::Event::FILE_CREATED, [&](auto &path) {
    std::cout << "File created: " << path << std::endl;
    watcher.stop();
  });

  watcher.on(FileWatcher::Event::FILE_MODIFIED, [](auto &path) {
    std::cout << "File modified: " << path << std::endl;
  });

  watcher.on(FileWatcher::Event::FILE_ERASED, [](auto &path) {
    std::cout << "File erased: " << path << std::endl;
  });

  try {
    watcher.start();
  } catch (std::filesystem::filesystem_error &error) {
    std::cout << error.what() << std::endl;
  }
}

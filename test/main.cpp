#include <fswatch.hpp>
#include <iostream>

int main() {
  auto watcher = fswatch(".");

  watcher.on(fswatch::Event::DIR_OPENED, [](auto& path) {
    std::cout << "Directory opened: " << path << std::endl;
  });

  watcher.on(fswatch::Event::FILE_OPENED, [](auto& path) {
    std::cout << "File opened: " << path << std::endl;
  });

  watcher.on(fswatch::Event::DIR_CREATED, [](auto &path) {
    std::cout << "Directory created: " << path << std::endl;
  });

  watcher.on(fswatch::Event::FILE_CREATED, [](auto &path) {
    std::cout << "File created: " << path << std::endl;
  });

  watcher.on(fswatch::Event::FILE_MODIFIED, [](auto &path) {
    std::cout << "File modified: " << path << std::endl;
  });

  watcher.on(fswatch::Event::FILE_ERASED, [](auto &path) {
    std::cout << "File erased: " << path << std::endl;
  });

  watcher.on(fswatch::Event::FILE_CLOSED, [](auto &path) {
    std::cout << "File closed: " << path << std::endl;
  });

  watcher.on(fswatch::Event::DIR_CLOSED, [](auto &path) {
    std::cout << "Directory closed: " << path << std::endl;
  });

  try {
    watcher.start();
  } catch (std::filesystem::filesystem_error &error) {
    std::cout << error.what() << std::endl;
  }
}

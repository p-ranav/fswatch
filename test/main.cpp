#include <fswatch.hpp>
#include <iostream>

int main() {
  auto watcher = fswatch(".");

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

  try {
    watcher.start();
  } catch (std::filesystem::filesystem_error &error) {
    std::cout << error.what() << std::endl;
  }
}

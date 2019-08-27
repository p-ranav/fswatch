#include <fswatch.hpp>
#include <iostream>

int main() {
  auto watcher = fswatch(".");

  watcher.on(fswatch::Event::FILE_CREATED, [](auto &object) {
    std::cout << "File created: " << object.path << std::endl;
  });

  watcher.on(fswatch::Event::FILE_MODIFIED, [](auto &object) {
    std::cout << "File modified: " << object.path << std::endl;
  });

  watcher.on(fswatch::Event::FILE_ERASED, [](auto &object) {
    std::cout << "File erased: " << object.path << std::endl;
  });

  try {
    watcher.start();
  } catch (std::filesystem::filesystem_error &error) {
    std::cout << error.what() << std::endl;
  }
}

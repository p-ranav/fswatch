#include <fswatch.hpp>
#include <iostream>

int main() {
  auto watcher = fswatch(".");

  watcher.on(fswatch::Event::FILE_CREATED, [](auto &event) {
    std::cout << "File created: " << event.path << std::endl;
  });

  watcher.on(fswatch::Event::FILE_MODIFIED, [](auto &event) {
    std::cout << "File modified: " << event.path << std::endl;
  });

  watcher.on(fswatch::Event::FILE_ERASED, [](auto &event) {
    std::cout << "File erased: " << event.path << std::endl;
  });

  try {
    watcher.start();
  } catch (std::filesystem::filesystem_error &error) {
    std::cout << error.what() << std::endl;
  }
}

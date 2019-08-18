#include <iostream>
#include <rorschach.hpp>

int main() {
  auto rorschach = Rorschach("", std::chrono::milliseconds(500));
  rorschach.ignore(std::regex(".*\\.ini$")); // Ignore .ini files

  rorschach.on(FileStatus::FILE_CREATED, [](auto &path) {
    std::cout << "Path created: " << path << std::endl;
  });

  rorschach.on(FileStatus::FILE_MODIFIED, [](auto &path) {
    std::cout << "Path modified: " << path << std::endl;
  });

  rorschach.on(FileStatus::FILE_ERASED, [](auto &path) {
    std::cout << "Path erased: " << path << std::endl;
  });

  try {
    rorschach.watch();
  } catch (std::filesystem::filesystem_error &error) {
    std::cout << error.what() << std::endl;
  }
}

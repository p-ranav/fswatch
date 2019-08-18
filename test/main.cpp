#include <iostream>
#include <rorschach.hpp>

int main() {
  auto rorschach = Rorschach("", std::chrono::milliseconds(500));
  // Only match foo.txt or bar.csv
  rorschach.match(std::regex("foo.txt|bar.csv"));
  // Ignore .ini files
  // rorschach.ignore(std::regex(".*\\.ini$")); 

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

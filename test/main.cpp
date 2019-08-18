#include <rorschach.hpp>
#include <iostream>

int main() {
  auto rorschach = Rorschach(".", std::chrono::milliseconds(500));

  rorschach.on_path_created = [](auto& path) { 
    std::cout << "Path created: " << path << std::endl; 
  };

  rorschach.on_path_modified = [](auto& path) {
    std::cout << "Path modified: " << path << std::endl;
  };

  rorschach.on_path_erased = [](auto& path) {
    std::cout << "Path erased: " << path << std::endl;
  };

  try {
    rorschach.watch();
  } catch (std::filesystem::filesystem_error& error) {
    std::cout << error.what() << std::endl;
  }

}

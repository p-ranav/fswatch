# file_watcher

## Highlights

* Single header file
* Requires C++17 and `std::filesystem`
* MIT License

## Quick Start

Simply include file_watcher.hpp and you're good to go.

```cpp
#include <file_watcher.hpp>
```

To start watching files, create a FileWatcher.

```cpp
auto watcher = FileWatcher("/opt", std::chrono::milliseconds(500));
watcher.skip_permission_denied();
```

This file watcher will observe /opt every 500 ms and skip permission denied errors. To add callbacks to file-related events, use the `file_watcher.on(...)` method like so:

```cpp
watcher.on(FileStatus::FILE_CREATED, [](auto &path) {
  std::cout << "Path created: " << path << std::endl;
});

watcher.on(FileStatus::FILE_MODIFIED, [](auto &path) {
  std::cout << "Path modified: " << path << std::endl;
});

watcher.on(FileStatus::FILE_ERASED, [](auto &path) {
  std::cout << "Path erased: " << path << std::endl;
});
```

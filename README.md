# file_watcher

## Highlights

* Single header file
* Requires C++17 and std::filesystem
* MIT License

## Quick Start

Simply include file_watcher.hpp and you're good to go.

```cpp
#include <file_watcher.hpp>
```

To start watching files, create a FileWatcher. This file watcher will check every 500 ms and skips permission denied errors. 

```cpp
auto watcher = FileWatcher("", std::chrono::milliseconds(500));
watcher.skip_permission_denied();
```

To add callbacks to file-related events, use the `file_watcher.on(...)` method like so:

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

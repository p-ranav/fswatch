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
To start watching files, create a FileWatcher and provide a directory to watch.

```cpp
auto watcher = FileWatcher("/opt");
```

This file watcher will observe /opt. 

## Register callbacks to events

To add callbacks to events, use the `file_watcher.on(...)` method like so:

```cpp
watcher.on(FileWatcher::Event::FILE_CREATED, [](auto &path) {
  std::cout << "Path created: " << path << std::endl;
});

watcher.on(FileWatcher::Event::FILE_MODIFIED, [](auto &path) {
  std::cout << "Path modified: " << path << std::endl;
});

watcher.on(FileWatcher::Event::FILE_ERASED, [](auto &path) {
  std::cout << "Path erased: " << path << std::endl;
});
```
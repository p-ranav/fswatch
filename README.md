# fswatch

## Highlights

* Single header file
* Requires C++17 and `std::filesystem`
* MIT License

## Quick Start

Simply include fswatch.hpp and you're good to go. 

```cpp
#include <fswatch.hpp>
```
To start watching files, create a FileWatcher and provide a directory to watch.

```cpp
auto watcher = fswatch("/opt");
try {
  watcher.start();
} catch (const std::runtime_error& error) {
  std::cout << error.what() << std::endl;
}
```

This file watcher will observe /opt. 

## Register callbacks to events

To add callbacks to events, use the `watcher.on(...)` method like so:

```cpp
watcher.on(fswatch::Event::FILE_CREATED, [](auto &path) {
  std::cout << "File created: " << path << std::endl;
});
```

fswatch works recursively on the directory being watched, i.e., fswatch is notified of all changes made to subdirectories in the path being watched. Currently, fswatch supports `FILE_CREATED`, `FILE_MODIFIED`, `FILE_ERASED`, `DIR_CREATED`, `DIR_MODIFIED`, and `DIR_ERASED` events.

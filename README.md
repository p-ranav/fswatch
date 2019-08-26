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
To start watching files, create a FileWatcher and provide a directory to watch. This file watcher will observe `/opt`. 

```cpp
auto watcher = fswatch("/opt");
try {
  watcher.start();
} catch (const std::runtime_error& error) {
  std::cout << error.what() << std::endl;
}
```

## Register callbacks to events

To add callbacks to events, use the `watcher.on(...)` method like so:

```cpp
watcher.on(fswatch::Event::FILE_CREATED, [](auto &path) {
  std::cout << "File created: " << path << std::endl;
});
```

fswatch works recursively on the directory being watched, i.e., fswatch is notified of all changes made to subdirectories in the path being watched. The following is a list of events that fswatch can handle:

| Event              | Description                                                |
|--------------------|------------------------------------------------------------|
| FILE_CREATED       | File created in watched directory                          |
| FILE_MODIFIED      | File modified in watched directory (e.g., write, truncate) |
| FILE_ERASED        | File deleted from watched directory                        |
| DIR_CREATED        | Directory created in watched directory                     |
| DIR_MODIFIED       | Directory modified in watched directory                    |
| DIR_ERASED         | Directory deleted from watched directory                   |

## Todo

1. Currently, fswatch only works in Linux. Need to suppport Win32 and OSX
2. Need to support more events, e.g.,`FILE_ACCESSED`, `FILE_CLOSED` etc.

<p align="center">
  <img height="80" src="https://i.imgur.com/YGfomu0.png" alt="fswatch"/>
</p>

## Highlights

* Single header file
* Requires C++17 and `std::filesystem`
* MIT License
* Based on [inotify](http://man7.org/linux/man-pages/man7/inotify.7.html) in Linux

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
watcher.on(fswatch::Event::FILE_CREATED, [](auto &object) {
  std::cout << "File created: " << object.path << std::endl;
});
```

You can register a single callback for multiple events like this:

```cpp
watcher.on( { fswatch::Event::FILE_OPENED, fswatch::Event::FILE_CLOSED }, [](auto &object) {
  if (object.event == fswatch::Event::FILE_OPENED)
    std::cout << "File opened: " << object.path << std::endl;
  else
    std::cout << "File closed: " << object.path << std::endl;
});
```

The following is a list of events that fswatch can handle:

| Event              | Description                                                   |
|--------------------|---------------------------------------------------------------|
| FILE_CREATED       | File created in watched directory                             |
| FILE_OPENED        | File opened in watched directory                              |
| FILE_MODIFIED      | File modified in watched directory (e.g., write, truncate)    |
| FILE_CLOSED        | File closed in watched directory                              |
| FILE_ERASED        | File deleted from watched directory                           |
| DIR_CREATED        | Directory created in watched directory                        |
| DIR_OPENED         | Directory opened in watched directory (e.g., when running ls) |
| DIR_MODIFIED       | Directory modified in watched directory                       |
| DIR_CLOSED         | Directory closed in watched directory                         |
| DIR_ERASED         | Directory deleted from watched directory                      |

## Todo

1. Currently, fswatch only works in Linux. Need to suppport Win32 and OSX

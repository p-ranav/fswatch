<p align="center">
  <img height="80" src="https://i.imgur.com/YGfomu0.png" alt="fswatch"/>
</p>

## Highlights

* Single header file
* Requires C++17 and `std::filesystem`
* MIT License
* For now, ONLY works in Linux - based on [inotify](http://man7.org/linux/man-pages/man7/inotify.7.html)

## Quick Start

Simply include fswatch.hpp and you're good to go. 

```cpp
#include <fswatch.hpp>
```
To start watching files, create an `fswatch` object and provide a variadic list of directories to watch. 

The constructor takes variadic arguments - Simply provide a list of directories to watch. This watcher will observe your home directory, `/opt`, `/tmp` and the current working directory. 

```cpp
auto watcher = fswatch("~", "/opt", "/tmp", ".");

try {
  watcher.start();
} catch (const std::runtime_error& error) {
  std::cout << error.what() << std::endl;
}
```

## Register callbacks to events

To add callbacks to events, use the `watcher.on(...)` method like so:

```cpp
watcher.on(fswatch::Event::FILE_CREATED, [](auto &event) {
  std::cout << "File created: " << event.path << std::endl;
});
```

You can register a single callback for multiple events like this:

```cpp
watcher.on({ fswatch::Event::FILE_OPENED, fswatch::Event::FILE_CLOSED },
  [](auto &event) {
    if (event.type == fswatch::Event::FILE_OPENED)
      std::cout << "File opened: " << event.path << std::endl;
    else
      std::cout << "File closed: " << event.path << std::endl;
});
```

Here are the list of events that fswatch can handle:

### File Events

| Event              | Description                                                   |
|--------------------|---------------------------------------------------------------|
| FILE_CREATED       | File created in watched directory                             |
| FILE_OPENED        | File opened in watched directory                              |
| FILE_MODIFIED      | File modified in watched directory (e.g., write, truncate)    |
| FILE_CLOSED        | File closed in watched directory                              |
| FILE_DELETED       | File deleted from watched directory                           |

### Directory Events

| Event              | Description                                                   |
|--------------------|---------------------------------------------------------------|
| DIR_CREATED        | Directory created in watched directory                        |
| DIR_OPENED         | Directory opened in watched directory (e.g., when running ls) |
| DIR_MODIFIED       | Directory modified in watched directory                       |
| DIR_CLOSED         | Directory closed in watched directory                         |
| DIR_DELETED        | Directory deleted from watched directory                      |

## Todo

1. Suppport Win32, FreeBSD, and OSX

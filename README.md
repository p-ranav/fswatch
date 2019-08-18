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

## Watch specific files

Maybe you don't want to watch every file in a directory but just specific files. You can use ```file_watcher.watch(...)``` and provide a regex of all files you want to watch. 

Let's say you only care about `foo.txt` in your home directory. You can configure the file_watcher to only watch this one file like so:

```cpp
auto watcher = FileWatcher("~", std::chrono::milliseconds(500));
watcher.skip_permission_denied();
watcher.match(std::regex("foo.txt"));
```

If you also want to watch `bar.log`, you can update the regex like so:

```cpp
watcher.match(std::regex("foo.txt|bar.log");
```

## Ignore file formats

Let's say you don't want to watch certain file formats, e.g., `.log`. You can use `file_watcher.ignore(...)` and provide a regex of all files you want to ignore. 

```cpp
watcher.ignore(std::regex(".*\\.log$")); // Ignore all log files
```

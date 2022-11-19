#pragma once
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <string>

#ifdef __linux__
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <unistd.h>
#define MAX_EVENTS 1024 /*Max. number of events to process at one go*/
#define LEN_NAME                                                               \
  16 /*Assuming that the length of the filename won't exceed 16 bytes*/
#define EVENT_SIZE (sizeof(struct inotify_event)) /*size of one event*/
#define EVENT_BUF_LEN                                                          \
  (MAX_EVENTS * (EVENT_SIZE + LEN_NAME)) /*buffer to store the data of         \
                                            events*/
#define WATCH_FLAGS (IN_CREATE | IN_MODIFY | IN_DELETE | IN_OPEN | IN_CLOSE)

// Keep going  while run == true, or, in other words, until user hits ctrl-c
static bool run = true;

void sig_callback([[maybe_unused]] int sig) { run = false; }

// Watch class keeps track of watch descriptors (wd), parent watch descriptors
// (pd), and names (from event->name). The class provides some helpers for
// inotify, primarily to enable recursive monitoring:
// 1. To add a watch (inotify_add_watch), a complete path is needed, but events
// only provide file/dir name with no path.
// 2. Delete events provide parent watch descriptor and file/dir name, but
// removing the watch (infotify_rm_watch) needs a wd.
//
class Watch {
  struct wd_elem {
    int pd;
    std::string name;
    bool operator()(const wd_elem &l, const wd_elem &r) const {
      return l.pd < r.pd ? true
                         : l.pd == r.pd && l.name < r.name ? true : false;
    }
  };
  std::map<int, wd_elem> watch;
  std::map<wd_elem, int, wd_elem> rwatch;

public:
  // Insert event information, used to create new watch, into Watch object.
  void insert(int pd, const std::string &name, int wd) {
    wd_elem elem = {pd, name};
    watch[wd] = elem;
    rwatch[elem] = wd;
  }
  // Erase watch specified by pd (parent watch descriptor) and name from watch
  // list. Returns full name (for display etc), and wd, which is required for
  // inotify_rm_watch.
  std::string erase(int pd, const std::string &name, int *wd) {
    wd_elem pelem = {pd, name};
    *wd = rwatch[pelem];
    rwatch.erase(pelem);
    const wd_elem &elem = watch[*wd];
    std::string dir = elem.name;
    watch.erase(*wd);
    return dir;
  }
  // Given a watch descriptor, return the full directory name as string.
  // Recurses up parent WDs to assemble name, an idea borrowed from Windows
  // change journals.
  std::string get(int wd) {
    const wd_elem &elem = watch[wd];
    return elem.pd == -1 ? elem.name : this->get(elem.pd) + "/" + elem.name;
  }
  // Given a parent wd and name (provided in IN_DELETE events), return the watch
  // descriptor. Main purpose is to help remove directories from watch list.
  int get(int pd, std::string name) {
    wd_elem elem = {pd, name};
    return rwatch[elem];
  }
  void cleanup(int fd) {
    for (std::map<int, wd_elem>::iterator wi = watch.begin(); wi != watch.end();
         wi++) {
      inotify_rm_watch(fd, wi->first);
      watch.erase(wi);
    }
    rwatch.clear();
  }
  void stats() {
    std::cout << "number of watches=" << watch.size()
              << " & reverse watches=" << rwatch.size() << std::endl;
  }
};
#endif

class fswatch {
public:
  enum class Event {
    FILE_CREATED,
    FILE_OPENED,
    FILE_MODIFIED,
    FILE_CLOSED,
    FILE_DELETED,
    DIR_CREATED,
    DIR_OPENED,
    DIR_MODIFIED,
    DIR_CLOSED,
    DIR_DELETED
  };

  struct EventInfo {
    Event type;
    std::filesystem::path path;
  };

  fswatch() {}

  fswatch(const std::string &directory) {
    append_to_path(directory);
  }

  template <class... T>
  fswatch(T... paths) {
    append_to_path(paths...);
  }

  void append_to_path(const std::string& path) {
    paths.push_back(expand(std::filesystem::path(path)));
    if (path.length() == 0) {
      paths[paths.size() - 1] = expand(std::filesystem::path("."));
    }
  }

  template <class... T2>
  void append_to_path(const std::string& head, T2... tail) {
    append_to_path(head);
    append_to_path(tail...);
  }

  void on(const Event &event,
          const std::function<void(const EventInfo &)> &action) {
    callbacks[event] = action;
  }

  void on(const std::vector<Event> &events,
          const std::function<void(const EventInfo &)> &action) {
    for (auto &event : events) {
      callbacks[event] = action;
    }
  }

#ifdef __linux__
  void stop() { run = false; }

  void start() {
    // std::map used to keep track of wd (watch descriptors) and directory names
    // As directory creation events arrive, they are added to the Watch map.
    // Directory delete events should be (but currently aren't in this sample)
    // handled the same way.
    Watch watch;

    // watch_set is used by select to wait until inotify returns some data to
    // be read using non-blocking read.
    fd_set watch_set;

    char buffer[EVENT_BUF_LEN];
    std::string current_dir, new_dir;
    int total_file_events = 0;
    int total_dir_events = 0;
    [[maybe_unused]] Event current_event;

    // Call sig_callback if user hits ctrl-c
    signal(SIGINT, sig_callback);

    // creating the INOTIFY instance
    // inotify_init1 not available with older kernels, consequently inotify
    // reads block. inotify_init1 allows directory events to complete
    // immediately, avoiding buffering delays. In practice, this significantly
    // improves monotiring of newly created subdirectories.
#ifdef IN_NONBLOCK
    int fd = inotify_init1(IN_NONBLOCK);
#else
    int fd = inotify_init();
#endif

    // checking for error
    if (fd < 0) {
      throw std::runtime_error("inotify_init failed");
    }

    // use select watch list for non-blocking inotify read
    FD_ZERO(&watch_set);
    FD_SET(fd, &watch_set);

    int wd;
    for (auto& path : paths) {
      auto path_string = path.string();
      const char *root = path_string.c_str();
      wd = inotify_add_watch(fd, root, WATCH_FLAGS);
      // add wd and directory name to Watch map
      watch.insert(-1, root, wd);
    }

    // Continue until run == false. See signal and sig_callback above.
    while (run) {
      // select waits until inotify has 1 or more events.
      // select syntax is beyond the scope of this sample but, don't worry, the
      // fd+1 is correct: select needs the the highest fd (+1) as the first
      // parameter.
      select(fd + 1, &watch_set, NULL, NULL, NULL);

      // Read event(s) from non-blocking inotify fd (non-blocking specified in
      // inotify_init1 above).
      int length = read(fd, buffer, EVENT_BUF_LEN);
      if (run && length < 0) {
        throw std::runtime_error("failed to read event(s) from inotify fd");
      }

      // Loop through event buffer
      for (int i = 0; i < length;) {
        struct inotify_event *event = (struct inotify_event *)&buffer[i];
        // Never actually seen this
        if (event->wd == -1) {
          throw std::runtime_error(
              "inotify IN_Q_OVERFLOW - Event queue overflowed");
        }
        // Never seen this either
        if (event->mask & IN_Q_OVERFLOW) {
          throw std::runtime_error(
              "inotify IN_Q_OVERFLOW - Event queue overflowed");
        }
        if (event->len) {
          if (event->mask & IN_IGNORED) {
            // Watch was removed explicitly (inotify_rm_watch) or automatically
            // (file was deleted, or filesystem was unmounted)
            throw std::runtime_error(
                "inotify IN_IGNORED - Watch was removed explicitly "
                "(inotify_rm_watch) or automatically (file was deleted, or "
                "filesystem was unmounted)");
          }
          if (event->mask & IN_CREATE) {
            current_dir = watch.get(event->wd);
            if (event->mask & IN_ISDIR) {
              new_dir = current_dir + "/" + event->name;
              wd = inotify_add_watch(fd, new_dir.c_str(), WATCH_FLAGS);
              watch.insert(event->wd, event->name, wd);
              total_dir_events++;
              run_callback(Event::DIR_CREATED, current_dir, event->name);
            } else {
              total_file_events++;
              run_callback(Event::FILE_CREATED, current_dir, event->name);
            }
          } else if (event->mask & IN_MODIFY) {
            if (event->mask & IN_ISDIR) {
              run_callback(Event::DIR_MODIFIED, current_dir, event->name);
            } else {
              run_callback(Event::FILE_MODIFIED, current_dir, event->name);
            }
          } else if (event->mask & IN_DELETE) {
            if (event->mask & IN_ISDIR) {
              // Directory was deleted
              new_dir = watch.erase(event->wd, event->name, &wd);
              inotify_rm_watch(fd, wd);
              total_dir_events--;
              run_callback(Event::DIR_DELETED, current_dir, event->name);
            } else {
              // File was deleted
              current_dir = watch.get(event->wd);
              total_file_events--;
              run_callback(Event::FILE_DELETED, current_dir, event->name);
            }
          } else if (event->mask & IN_OPEN) {
            current_dir = watch.get(event->wd);
            if (event->mask & IN_ISDIR) {
              // Directory was opened
              run_callback(Event::DIR_OPENED, current_dir, event->name);
            } else {
              // File was opened
              run_callback(Event::FILE_OPENED, current_dir, event->name);
            }
          } else if (event->mask & IN_CLOSE) {
            current_dir = watch.get(event->wd);
            if (event->mask & IN_ISDIR) {
              // Directory was closed
              run_callback(Event::DIR_CLOSED, current_dir, event->name);
            } else {
              // File was closed
              run_callback(Event::FILE_CLOSED, current_dir, event->name);
            }
          }
        }
        i += EVENT_SIZE + event->len;
      }
    }

    // Cleanup
    watch.cleanup(fd);
    close(fd);
    fflush(stdout);
  }
#endif

private:
  // Root directory of the file watcher
  std::vector<std::filesystem::path> paths;

  // Callback functions based on file status
  std::map<Event, std::function<void(const EventInfo &)>> callbacks;

  std::filesystem::path expand(std::filesystem::path in) {
    const char *home = getenv("HOME");
    if (!home)
      throw std::invalid_argument("HOME environment variable not set.");

    std::string result = in.c_str();
    if (result.length() > 0 && result[0] == '~') {
      result = std::string(home) + result.substr(1, result.size() - 1);
      return std::filesystem::path(result);
    }
    return in;
  }

  bool is_callback_registered(const Event &event) {
    return (callbacks.find(event) != callbacks.end());
  }

  void run_callback(const Event &event, const std::string &current_dir,
                    const std::string &filename) {
    if (is_callback_registered(event)) {
      callbacks[event](EventInfo{
          event, std::filesystem::path(current_dir + "/" + filename)});
    }
  }
};

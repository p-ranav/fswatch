#pragma once
#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <regex>
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>

#ifdef __linux__
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
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
#define WATCH_FLAGS (IN_CREATE | IN_MODIFY | IN_DELETE)

// Keep going  while run == true, or, in other words, until user hits ctrl-c
static bool run = true;

void sig_callback(int sig) { run = false; }

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

class FileWatcher {
public:
  enum class Event {
    FILE_CREATED,
    FILE_MODIFIED,
    FILE_ERASED,
    DIR_CREATED,
    DIR_MODIFIED,
    DIR_ERASED
  };

  FileWatcher(const std::string &path,
              std::chrono::duration<int, std::milli> period)
      : path(expand(std::filesystem::absolute(std::filesystem::path(path)))),
        match_regex_provided(false), ignore_regex_provided(false),
        period(period) {}

  void match(const std::regex &match) {
    match_path = match;
    match_regex_provided = true;
  }

  void ignore(const std::regex &ignore) {
    ignore_path = ignore;
    ignore_regex_provided = true;
  }

  void on(const Event &event,
          const std::function<void(const std::filesystem::path &)> &action) {
    callbacks[event] = action;
  }

#ifdef __linux__
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
      perror("inotify_init");
    }

    // use select watch list for non-blocking inotify read
    FD_ZERO(&watch_set);
    FD_SET(fd, &watch_set);

    const char *root = path.string().c_str();
    int wd = inotify_add_watch(fd, root, WATCH_FLAGS);

    // add wd and directory name to Watch map
    watch.insert(-1, root, wd);

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
      if (length < 0) {
        perror("read");
      }

      // Loop through event buffer
      for (int i = 0; i < length;) {
        struct inotify_event *event = (struct inotify_event *)&buffer[i];
        // Never actually seen this
        if (event->wd == -1) {
          printf("Overflow\n");
        }
        // Never seen this either
        if (event->mask & IN_Q_OVERFLOW) {
          printf("Overflow\n");
        }
        if (event->len) {
          if (event->mask & IN_IGNORED) {
            printf("IN_IGNORED\n");
          }
          if (event->mask & IN_CREATE) {
            current_dir = watch.get(event->wd);
            if (event->mask & IN_ISDIR) {
              new_dir = current_dir + "/" + event->name;
              wd = inotify_add_watch(fd, new_dir.c_str(), WATCH_FLAGS);
              watch.insert(event->wd, event->name, wd);
              total_dir_events++;
              if (is_callback_registered(Event::DIR_CREATED)) {
                callbacks[Event::DIR_CREATED](std::filesystem::path(current_dir + "/" + event->name));
              }
            } else {
              total_file_events++;
              if (is_callback_registered(Event::FILE_CREATED)) {
                callbacks[Event::FILE_CREATED](std::filesystem::path(current_dir + "/" + event->name));
              }
            }
          } else if (event->mask & IN_MODIFY) {
            if (event->mask & IN_ISDIR) {
              if (is_callback_registered(Event::DIR_MODIFIED)) {
                callbacks[Event::DIR_MODIFIED](std::filesystem::path(current_dir + "/" + event->name));
              }
            }
            else {
              if (is_callback_registered(Event::FILE_MODIFIED)) {
                callbacks[Event::FILE_MODIFIED](std::filesystem::path(current_dir + "/" + event->name));
              }              
            }
          } else if (event->mask & IN_DELETE) {
            if (event->mask & IN_ISDIR) {
              new_dir = watch.erase(event->wd, event->name, &wd);
              inotify_rm_watch(fd, wd);
              total_dir_events--;
              if (is_callback_registered(Event::DIR_ERASED)) {
                callbacks[Event::DIR_ERASED](std::filesystem::path(current_dir + "/" + event->name));
              }
            } else {
              current_dir = watch.get(event->wd);
              total_file_events--;
              if (is_callback_registered(Event::FILE_ERASED)) {
                callbacks[Event::FILE_ERASED](std::filesystem::path(current_dir + "/" + event->name));
              }
            }
          }
        }
        i += EVENT_SIZE + event->len;
      }
    }

    // Cleanup
    printf("cleaning up\n");
    std::cout << "total dir events = " << total_dir_events
              << ", total file events = " << total_file_events << std::endl;
    watch.stats();
    watch.cleanup(fd);
    watch.stats();
    close(fd);
    fflush(stdout);
  }
#endif

private:
  // Root directory of the file watcher
  std::filesystem::path path;
  // Periodicity of the file watcher, i.e., how often it checks to see if files
  // have changed
  std::chrono::duration<int, std::milli> period;
  // Regex of paths to ignore
  std::regex match_path, ignore_path;
  // Is a match/ignore regex provided
  bool match_regex_provided, ignore_regex_provided;

  // Callback functions based on file status
  std::map<Event, std::function<void(const std::filesystem::path &)>> callbacks;

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

  bool should_ignore(const std::filesystem::directory_entry &path) {
    if (!ignore_regex_provided)
      return false;
    std::string filename = path.path().string();
    if (!std::regex_search(filename, ignore_path))
      return false;
    else
      return true;
  }

  bool should_watch(const std::filesystem::directory_entry &path) {
    if (!match_regex_provided)
      return true;
    std::string filename = path.path().string();
    if (std::regex_search(filename, match_path))
      return true;
    else
      return false;
  }

  bool is_callback_registered(const Event& event) {
    return (callbacks.find(event) != callbacks.end());
  }
};

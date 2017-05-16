#ifndef H_BB_FILESYSTEM_MONITOR_H
#define H_BB_FILESYSTEM_MONITOR_H

#include "common.h"

#include <fcntl.h>
#include <sys/epoll.h>

/**
 * Types
 **/
struct bb_filesystem_event { }; // TODO

struct bb_filesystem_event_listener
{
    void (*on_event)(void*, struct bb_filesystem_event*);
    void* arg0;
};

struct bb_filesystem_monitor
{
    int inotify_fd;
    int epoll_fd;
    struct bb_filesystem_event_listener listener;
    char** excludes;
    size_t num_excludes;
    char const** watches;
    size_t num_watches;
};


/**
 * Functionality
 **/
void bb_filesystem_monitor_init(struct bb_filesystem_monitor*);
void bb_filesystem_monitor_include(struct bb_filesystem_monitor*, char const* path);
void bb_filesystem_monitor_exclude(struct bb_filesystem_monitor*, char const* path);
void bb_filesystem_monitor_set_listener(struct bb_filesystem_monitor*, struct bb_filesystem_event_listener);
void bb_filesystem_monitor_poll(struct bb_filesystem_monitor*);


/**
 * Implementation
 **/
void bb_filesystem_monitor_init(struct bb_filesystem_monitor* fm)
{
    BB_BZERO(fm);

    // non-blocking inotify fd
    BB_ASSERT((fm->inotify_fd = inotify_init()) > 0);
    int flags = fcntl(fm->inotify_fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    BB_ASSERT(0 == fcntl(fm->inotify_fd, F_SETFL, flags));

    // epoll fd
    BB_ASSERT((fm->epoll_fd = epoll_create(1)) > 0);
    struct epoll_event ev; ev.events = EPOLLIN; ev.data.fd = fm->inotify_fd;
    BB_ASSERT(0 == epoll_ctl(fm->epoll_fd, EPOLL_CTL_ADD, fm->inotify_fd, &ev));
}

void bb_filesystem_monitor_set_listener(struct bb_filesystem_monitor* fm, struct bb_filesystem_event_listener l)
{
    fm->listener = l;
}

void bb_filesystem_monitor_include(struct bb_filesystem_monitor* fm, char const* path)
{
    // register with inotify 
    BB_INFO("including '%s'", path);
    int const wd = inotify_add_watch(fm->inotify_fd, path, IN_CLOSE_WRITE);

    // save
    BB_ASSERT(wd >= 0);
    if ((size_t)wd >= fm->num_watches)
    {
      fm->num_watches = wd + 1;
      fm->watches     = realloc(fm->watches, fm->num_watches * sizeof(*fm->watches));
    }
    fm->watches[wd] = path;

    // recurse to subdirectories
    DIR* dir = opendir(path); if (!dir) { return; }
    struct dirent* de;
    char* const buffer = malloc(strlen(path) + 512);
    while ( (de = readdir(dir)) )
    {
        if (de->d_type != DT_DIR || bb_is_file_or_dir_hidden(de->d_name)) { continue; }
        sprintf(buffer, "%s/%s", path, de->d_name);
        bb_filesystem_monitor_include(fm, buffer);
    }
    free(buffer);
}

void bb_filesystem_monitor_exclude(struct bb_filesystem_monitor* fm, char const* path)
{
  fm->excludes = realloc(fm->excludes, (fm->num_excludes + 1) * sizeof(*fm->excludes));
  fm->excludes[fm->num_excludes++] = strdup(path);
  BB_INFO("excluding '%s'", path);
}

bool bb_filesystem_monitor_is_excluded(struct bb_filesystem_monitor* fm, char const* name)
{
  if (!fm->num_excludes) { return false; }
  size_t const namelen = strlen(name);
  for (size_t idx = 0; idx != fm->num_excludes; ++idx)
  {
    if (0 == strncmp(fm->excludes[idx], name, namelen)) { return true; }
  }
  return false;
}

void bb_filesystem_monitor_poll(struct bb_filesystem_monitor* fm)
{
    // epoll with timeout
    struct epoll_event ev;
    if (!epoll_wait(fm->epoll_fd, &ev, 1, 50)) { return; }

    // inotify
    char buf[2048]; char path[2048];
    int result = read(fm->inotify_fd, buf, sizeof(buf));
    BB_ASSERT(result > 0);
    char* const bufend = buf + result;
    char* head = buf;
    while (head < bufend)
    {
        struct inotify_event ev; memcpy(&ev, head, sizeof(ev));
        char* const name = head + sizeof(ev); name[ev.len] = '\0';
        sprintf(path, "%s/%s", fm->watches[ev.wd], name);
        if (ev.len && !bb_is_file_or_dir_hidden(name) && bb_is_source_file(name))
        {
            BB_INFO("'%s' updated", path);
            if (bb_filesystem_monitor_is_excluded(fm, path)) { BB_INFO("...found in excludes list, ignoring"); break; }
            struct bb_filesystem_event fs_event = {};
            (*fm->listener.on_event)(fm->listener.arg0, &fs_event);
            break;
        }
        head += sizeof(ev) + ev.len;
    }
}


#endif


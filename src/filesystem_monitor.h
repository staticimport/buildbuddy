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
};


/**
 * Functionality
 **/
void bb_filesystem_monitor_init(struct bb_filesystem_monitor*);
void bb_filesystem_monitor_add_directory(struct bb_filesystem_monitor*, char const* dir);
void bb_filesystem_monitor_set_listener(struct bb_filesystem_monitor*, struct bb_filesystem_event_listener);
void bb_filesystem_monitor_poll(struct bb_filesystem_monitor*);


/**
 * Implementation
 **/
void bb_filesystem_monitor_init(struct bb_filesystem_monitor* fm)
{
    BB_BZERO(&fm->listener);

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

void bb_filesystem_monitor_add_directory(struct bb_filesystem_monitor* fm, char const* dirname)
{
    // add directory
    BB_INFO("watching directory '%s'", dirname);
    BB_ASSERT(0 <= inotify_add_watch(fm->inotify_fd, dirname, IN_CLOSE_WRITE));

    // recurse to subdirectories
    DIR* dir = opendir(dirname);
    struct dirent* de;
    char* const buffer = malloc(strlen(dirname) + 512);
    while ( (de = readdir(dir)) )
    {
        if (de->d_type != DT_DIR || bb_is_file_or_dir_hidden(de->d_name)) { continue; }
        sprintf(buffer, "%s/%s", dirname, de->d_name);
        bb_filesystem_monitor_add_directory(fm, buffer);
    }
    free(buffer);
}

void bb_filesystem_monitor_poll(struct bb_filesystem_monitor* fm)
{
    // epoll with timeout
    struct epoll_event ev;
    if (!epoll_wait(fm->epoll_fd, &ev, 1, 50)) { return; }

    // inotify
    char buf[2048];
    int result = read(fm->inotify_fd, buf, sizeof(buf));
    BB_ASSERT(result > 0);
    char* const bufend = buf + result;
    char* head = buf;
    while (head < bufend)
    {
        struct inotify_event ev; memcpy(&ev, head, sizeof(ev));
        char* name = head + sizeof(ev);
        if (ev.len && !bb_is_file_or_dir_hidden(name) && bb_is_source_file(name))
        {
            BB_INFO("'%.*s' updated", (int)ev.len, name);
            struct bb_filesystem_event fs_event = {};
            (*fm->listener.on_event)(fm->listener.arg0, &fs_event);
            break;
        }
        head += sizeof(ev) + ev.len;
    }
}


#endif


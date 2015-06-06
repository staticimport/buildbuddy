#ifndef H_BB_FILESYSTEM_MONITOR_H
#define H_BB_FILESYSTEM_MONITOR_H

#include "common.h"

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
    struct bb_filesystem_event_listener listener;
};


/**
 * Functionality
 **/
void bb_filesystem_monitor_init(struct bb_filesystem_monitor*);
void bb_filesystem_monitor_add_directory(struct bb_filesystem_monitor*, char const* dir);
void bb_filesystem_monitor_set_listener(struct bb_filesystem_monitor*, struct bb_filesystem_event_listener);
void bb_filesystem_monitor_run(struct bb_filesystem_monitor*);


/**
 * Implementation
 **/
void bb_filesystem_monitor_init(struct bb_filesystem_monitor* fm)
{
    BB_BZERO(&fm->listener);
    fm->inotify_fd = inotify_init();
    BB_ASSERT(fm->inotify_fd > 0);
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

void bb_filesystem_monitor_run(struct bb_filesystem_monitor* fm)
{
    size_t const buflen = 64000;
    char* const buf = malloc(buflen);
    while (true)
    {
        int result = read(fm->inotify_fd, buf, buflen);
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
}


#endif


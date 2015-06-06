#ifndef H_BB_BUILDER_H
#define H_BB_BUILDER_H

#include "common.h"
#include "filesystem_monitor.h"

/**
 * Types
 **/
struct bb_builder
{
    char* command;
};


/**
 * Functionality
 **/
void bb_builder_init(struct bb_builder*, char const* command);
void bb_builder_on_filesystem_event(void*, struct bb_filesystem_event*);


/**
 * Implementation
 **/
void bb_builder_init(struct bb_builder* bdr, char const* command)
{
    bdr->command = strdup(command);
}

void bb_builder_on_filesystem_event(void* arg0, struct bb_filesystem_event* ev)
{
    struct bb_builder* const bdr = arg0; (void)ev;
    BB_INFO("executing '%s'", bdr->command);
    struct timeval before; gettimeofday(&before, NULL);
    int const rc = system(bdr->command);
    struct timeval after;  gettimeofday(&after,  NULL);
    double seconds = after.tv_sec - before.tv_sec; seconds += ((double)after.tv_usec - before.tv_usec) / 1e6;
    BB_INFO("'%s' completed with return code %d in %g seconds", bdr->command, rc, seconds);
}


#endif


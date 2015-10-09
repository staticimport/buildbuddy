#ifndef H_BB_BUILDER_H
#define H_BB_BUILDER_H

#include "common.h"
#include "filesystem_monitor.h"

#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * Types
 **/
struct bb_builder
{
    char* command;
    char** command_args;
    struct timeval build_timestamp;
    pid_t build_pid;
};


/**
 * Functionality
 **/
void bb_builder_init(struct bb_builder*, char const* command);
void bb_builder_start(struct bb_builder*);
bool bb_builder_try_complete(struct bb_builder*, int* status);
void bb_builder_kill(struct bb_builder*);


/**
 * Implementation
 **/
void bb_builder_update_line_count_(struct bb_builder*);

void bb_builder_init(struct bb_builder* bdr, char const* command)
{
    bdr->command = strdup(command);
    bdr->build_pid = 0;
    char* command_copy = strdup(command);
    char* args[1024];
    char* tok = strtok(command_copy, " ");
    size_t idx = 0;
    do
    {
        args[idx++] = strdup(tok);
    } while ( (tok = strtok(NULL, " ")) );
    bdr->command_args = malloc(sizeof(char*) * (idx + 1));
    memcpy(bdr->command_args, args, sizeof(char*) * idx);
    bdr->command_args[idx] = NULL;
    free(command_copy);
}

void bb_builder_start(struct bb_builder* bdr)
{
    BB_ASSERT(!bdr->build_pid);
    BB_INFO("executing '%s'", bdr->command);
    gettimeofday(&bdr->build_timestamp, NULL);
    bdr->build_pid = fork();
    if (!bdr->build_pid) { // child
        BB_ASSERT(!execvp(bdr->command_args[0], bdr->command_args));
    }
}

bool bb_builder_try_complete(struct bb_builder* bdr, int* status)
{
    *status = -1;
    if (!bdr->build_pid) { return false; }
    if (bdr->build_pid != waitpid(bdr->build_pid, status, WNOHANG)) { return false; }
    bdr->build_pid = 0;
    return true;
}

void bb_builder_kill(struct bb_builder* bdr)
{
    pid_t pid = bdr->build_pid;
    if (!pid) { return; }
    kill(pid, SIGINT);
    kill(pid, SIGTERM);
    waitpid(pid, NULL, 0);
    bdr->build_pid = 0;
    BB_INFO("aborted build (pid %u)", pid);
}

#endif


#pragma once

#include "common.h"

/* Types */
struct bb_task_config
{
  char*  name;
  char*  command;
  char** includes;
  size_t num_includes;
  char** excludes;
  size_t num_excludes;
};

struct bb_config
{
  struct bb_task_config* tasks;
  size_t                 num_tasks;
};

/* Functionality */
void bb_task_config_load(struct bb_config*, char const* filename);

struct bb_task_config* 
bb_task_config_find(struct bb_config*, char const* task_name);

/* Implementation */
struct bb_task_config* bb_task_config_find_or_create_task(struct bb_config* c, char const* name, size_t namelen)
{
  struct bb_task_config* task = NULL;

  // find
  for (size_t idx = 0; idx != c->num_tasks; ++idx)
  {
    task = &c->tasks[idx];
    if (strlen(task->name) == namelen && (0 == memcmp(task->name, name, namelen))) { return task; }
  }

  // create
  c->tasks   = realloc(c->tasks, (c->num_tasks + 1) * sizeof(*c->tasks));
  task       = &c->tasks[c->num_tasks++];
  BB_BZERO(task);
  task->name = strndup(name, namelen);
  return task;
}

void bb_task_config_fail(char const* line, char const* error)
{
  BB_DIE("Failed while parsing config '%s': %s", line, error); 
}

void bb_task_config_load(struct bb_config* c, char const* filename)
{
  BB_BZERO(c);

  FILE* file = fopen(filename, "r");
  BB_ASSERT(file != NULL, "Unable to open '%s'", filename);

  char* line = NULL; size_t capacity = 0; ssize_t bytes_read;
  while (0 <= (bytes_read = getline(&line, &capacity, file)))
  {
    size_t len = bb_trim_string(line); if (len == 0) { continue; }
    char* const orig_line = strdup(line);

    // identify task
    char* dot = strstr(line, "."); if (!dot) { return bb_task_config_fail(orig_line, "missing '.'"); }
    struct bb_task_config* const task = bb_task_config_find_or_create_task(c, line, dot - line);

    // key
    char* const key = dot + 1;
    char* space = key; while (isspace(*space) == 0) { ++space; }
    if (*space == '\n') { return bb_task_config_fail(orig_line, "no value found after key"); }
    *space = 0; // null-terminate key

    // value
    char* value = space + 1; while (isspace(*value)) { ++value; }
    if (*value == 0) { return bb_task_config_fail(orig_line, "end of line reached without value found"); }

    // populate task
    if (BB_STREQ(key, "command"))
    {
      if (task->command) { return bb_task_config_fail(orig_line, "multiple commands found for same key"); }
      task->command = strdup(value);
    }
    else if (BB_STREQ(key, "include"))
    {
      task->includes = realloc(task->includes, (task->num_includes + 1) * sizeof(*task->includes));
      task->includes[task->num_includes++] = strdup(value);
    }
    else if (BB_STREQ(key, "exclude"))
    {
      task->excludes = realloc(task->excludes, (task->num_excludes + 1) * sizeof(*task->excludes));
      task->excludes[task->num_excludes++] = strdup(value);
    }
    else { return bb_task_config_fail(orig_line, "invalid key"); }

    free(orig_line);
  }

  free(line);
  fclose(file);
  BB_INFO("config loaded");
}

struct bb_task_config* bb_task_config_find(struct bb_config* c, char const* task_name)
{
  for (size_t idx = 0; idx != c->num_tasks; ++idx)
  {
    struct bb_task_config* const task = &c->tasks[idx];
    if (BB_STREQ(task_name, task->name)) { return task; }
  }
  return NULL;
}

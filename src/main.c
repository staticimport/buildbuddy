
#include "builder.h"
#include "config.h"
#include "filesystem_monitor.h"

bool BUILD_STARTED = false;

void check_build_status(struct bb_builder*);
void on_filesystem_event(void*, struct bb_filesystem_event*);

int main(int argc, char** argv)
{
    // args
    if (argc < 2 || argc > 3) { BB_DIE("USAGE: %s <task name> [config filename (defaults to './buddyfile')]", *argv); }
    char const* const task_name       = argv[1];
    char const* const config_filename = (argc > 2) ? argv[2] : "./buddyfile";

    // load config
    struct bb_config config;
    bb_task_config_load(&config, config_filename);
    struct bb_task_config* const task_config = bb_task_config_find(&config, task_name);
    BB_ASSERT(task_config != NULL, "unable to find configured task with name '%s'", task_name);

    // filesystem monitor
    struct bb_filesystem_monitor fs_monitor; bb_filesystem_monitor_init(&fs_monitor);
    for (size_t idx = 0; idx != task_config->num_includes; ++idx) 
    { 
      bb_filesystem_monitor_include(&fs_monitor, task_config->includes[idx]);
    }
    for (size_t idx = 0; idx != task_config->num_excludes; ++idx)
    {
      bb_filesystem_monitor_exclude(&fs_monitor, task_config->excludes[idx]);
    }

    // builder
    struct bb_builder builder; bb_builder_init(&builder, task_config->command);
    struct bb_filesystem_event_listener listener = {
        .on_event = &on_filesystem_event,
        .arg0     = &builder
    };
    bb_filesystem_monitor_set_listener(&fs_monitor, listener);

    // go!
    while (true)
    {
        bb_filesystem_monitor_poll(&fs_monitor);
        check_build_status(&builder);
    }

    return 0;
}

void check_build_status(struct bb_builder* bdr)
{
    int status;
    if (!BUILD_STARTED || !bb_builder_try_complete(bdr, &status)) { return; }
    BB_INFO("build complete! (status=%d)", status);
    BUILD_STARTED = false;
}

void on_filesystem_event(void* arg0, struct bb_filesystem_event* event)
{
    struct bb_builder* const bdr = arg0; (void)event;
    bb_builder_kill(bdr);
    bb_builder_start(bdr);
    BUILD_STARTED = true;
}


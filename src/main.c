
#include "builder.h"
#include "filesystem_monitor.h"

bool BUILD_STARTED = false;

void check_build_status(struct bb_builder*);
void on_filesystem_event(void*, struct bb_filesystem_event*);

int main(int argc, char** argv)
{
    // args
    if (argc < 3) { fprintf(stderr, "USAGE: %s <build command> <src dirs...>\n", *argv); return 1; }
    char* const build_command = argv[1];
    char** const src_dirs = argv + 2;
    int const num_src_dirs = argc - 2;

    // init
    struct bb_filesystem_monitor fs_monitor; bb_filesystem_monitor_init(&fs_monitor);
    for (int idx = 0; idx != num_src_dirs; ++idx) 
    { 
        bb_filesystem_monitor_add_directory(&fs_monitor, src_dirs[idx]);
    }
    struct bb_builder builder; bb_builder_init(&builder, build_command);
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


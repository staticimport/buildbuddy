
#include "builder.h"
#include "filesystem_monitor.h"

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
        .on_event = &bb_builder_on_filesystem_event,
        .arg0     = &builder
    };
    bb_filesystem_monitor_set_listener(&fs_monitor, listener);

    // go!
    bb_filesystem_monitor_run(&fs_monitor);

    return 0;
}


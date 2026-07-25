#include <string.h>

#include "cli.h"
#include "sysinfo.h"
#include "modes/compute.h"
#include "modes/stub.h"

int main(int argc, char **argv) {
    cyclelab_options_t opts;
    int exit_now = 0, exit_code = 0;

    cli_parse(argc, argv, &opts, &exit_now, &exit_code);
    if (exit_now) {
        return exit_code;
    }

    if (strcmp(opts.mode, "compute") == 0) {
        cyclelab_hostinfo_t host;
        sysinfo_collect(&host);
        return compute_run(&opts, &host);
    }

    /* Every other name cli_parse() accepted is a known-but-unimplemented
     * mode (see cli.c's KNOWN_MODES) -- unknown names are already
     * rejected by cli_parse() before we get here. */
    return stub_run(opts.mode);
}

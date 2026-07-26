#include "rusage_util.h"

#include <sys/resource.h>

void rusage_get_context_switches(long *voluntary, long *involuntary) {
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) != 0) {
        *voluntary = -1;
        *involuntary = -1;
        return;
    }
    *voluntary = (long)ru.ru_nvcsw;
    *involuntary = (long)ru.ru_nivcsw;
}

#include "cc-common.h"
#include "history.h"

const char prog_name[] = "histtest";

int main(int argc, char **argv)
{
    time_t secs;
    hist_context *hc;

    if (chdir(default_dir) == 0) {
        time(&secs);
        if ((hc = hist_get(secs - 3600, secs, 30))) {
            hist_js_sens_out(hc, stdout, 0);
            hist_free(hc);
        }
        return 0;
    } else {
        log_syserr("unable to chdir to '%s'", default_dir);
        return 1;
    }
}

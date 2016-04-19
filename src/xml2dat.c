#include "cc-common.h"
#include "parsefile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char prog_name[] = "xml2csv";

static pf_status sample_cb(pf_context * ctx, pf_sample * smp)
{
    time_t ts;
    float value;

    FILE *fp = ctx->user_data;
    ts = smp->timestamp;
    fwrite(&ts, sizeof ts, 1, fp);
    value = smp->temp;
    fwrite(&value, sizeof value, 1, fp);
    putc(smp->sensor, fp);
    value = smp->watts;
    fwrite(&value, sizeof value, 1, fp);
    return PF_SUCCESS;
}

int main(int argc, char **argv)
{
    int status = 0;
    pf_context *pf;
    const char *arg, *ext;
    size_t len;
    char *dat;
    FILE *fp;

    if ((pf = pf_new())) {
        pf->sample_cb = sample_cb;
        while (--argc) {
            arg = *++argv;
            if ((ext = strrchr(arg, '.')) && strcmp(ext, ".xml") == 0)
                len = ext - arg;
            else
                len = strlen(arg);
            dat = alloca(len + 4);
            memcpy(dat, arg, len);
            strcpy(dat + len, ".dat");
            if ((fp = fopen(dat, "w"))) {
                pf->user_data = fp;
                if (pf_parse_file(pf, arg) == PF_FAIL)
                    status = 3;
                fclose(fp);
            } else {
                log_syserr("unable to open file '%s' for writing", dat);
                status = 2;
            }
        }
    } else
        status = 1;
    return status;
}

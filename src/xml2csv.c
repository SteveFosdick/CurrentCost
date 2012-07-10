#include "cc-common.h"
#include "parsefile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char prog_name[] = "xml2csv";

static pf_status sample_cb(pf_context *ctx, pf_sample *smp) {
    FILE *fp = ctx->user_data;
    fprintf(fp, "%s,%g,%d,%g\n", smp->timestamp, smp->temp, smp->sensor, smp->watts);
    return PF_SUCCESS;
}

int main(int argc, char **argv) {
    int status = 0;
    pf_context *pf;
    const char *arg, *ext;
    size_t len;
    char *csv;
    FILE *fp;

    if ((pf = pf_new())) {
	pf->sample_cb = sample_cb;
	while (--argc) {
	    arg = *++argv;
	    if (arg[0] == '-' && arg[1] == 'f')
		pf->file_cb = pf_parse_forward;
	    else if (arg[0] == '-' && arg[1] == 'b')
		pf->file_cb = pf_parse_backward;
	    else {
		if ((ext = strrchr(arg, '.')) && strcmp(ext, ".xml") == 0)
		    len = ext - arg;
		else
		    len = strlen(arg);
		csv = alloca(len + 4);
		memcpy(csv, arg, len);
		strcpy(csv + len, ".csv");
		if ((fp = fopen(csv, "w"))) {
		    pf->user_data = fp;
		    if (pf_parse_file(pf, arg) == PF_FAIL)
			status = 3;
		    fclose(fp);
		} else {
		    log_syserr("unable to open file '%s' for writing", csv);
		    status = 2;
		}
	    }
	}
    } else
	status = 1;
    return status;
}

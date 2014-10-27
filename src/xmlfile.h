#ifndef XMLFILE_INC
#define XMLFILE_INC

#include "parsefile.h"

extern pf_status xml_parse_forward(pf_context * ctx, void *data, size_t size);
extern pf_status xml_parse_backward(pf_context * ctx, void *data,
                                    size_t size);

#endif

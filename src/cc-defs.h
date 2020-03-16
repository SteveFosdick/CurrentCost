#ifndef CC_DEFS_H
#define CC_DEFS_H

#define MAX_SENSOR   10
#define ISO_DATE_LEN 21
#define MAX_LINE_LEN 1300

#ifndef DEFAULT_DIR
#define DEFAULT_DIR "/data/misc/CurrentCost/"
#endif

#ifndef BASE_URL
#define BASE_URL    "http://fosdick.slyip.net/cgi-bin/"
#endif

#define XML_FILE "cc-%Y-%m-%d.xml"
#define DATE_ISO "%Y-%m-%dT%H:%M:%SZ"

extern const char xml_file[];
extern const char date_iso[];

#endif

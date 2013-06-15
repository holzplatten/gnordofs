#ifndef _MISC_H_
#define _MISC_H_

#include <syslog.h>

#define DEBUG(...) syslog(LOG_LOCAL0 | LOG_DEBUG, __VA_ARGS__);
#define DEBUG_SHIT(...) syslog(LOG_LOCAL0 | LOG_DEBUG, __VA_ARGS__);
#define DEBUG_VERBOSE(...) //syslog(LOG_LOCAL0 | LOG_DEBUG, __VA_ARGS__);

unsigned long calculate_inode_count(unsigned long size);

#endif

#ifndef _LOG_H_
#define _LOG_H_

#include <syslog.h>

#ifndef unlikely
# define unlikely(x)	__builtin_expect((x),0)
#endif	/* unlikely */

#define CLNTLIB_LOGLEVEL LOG_WARNING

#if CLNTLIB_LOGLEVEL >=	LOG_DEBUG
#  define DEBUG(fmt, args...) syslog(LOG_ERR, fmt, ##args)
#  if CLNTLIB_LOGLEVEL > LOG_DEBUG
#	 define DEBUG_V(fmt, args...) syslog(LOG_ERR, fmt, ##args)
#  else
#	 define DEBUG_V(fmt, args...) do{}while(0)
#  endif
#else
#  define DEBUG(fmt, args...) do{}while(0)
#  define DEBUG_V(fmt, args...) do{}while(0)
#endif
#if CLNTLIB_LOGLEVEL >= LOG_WARNING
#  define WARNING(fmt, args...) syslog(LOG_WARNING, fmt, ##args)
#else
#  define WARNING(fmt, args...) do{}while(0)
#endif
#define ERR(fmt, args...) syslog(LOG_ERR, fmt, ##args)
#define	INFO(fmt, args...) syslog(LOG_INFO, fmt, ##args)

extern int __INIT_DEBUG;
#define DEBUG_IN(fmt, args...)						\
	do {								\
		DEBUG("Enter[%d] %s: "fmt, __INIT_DEBUG, __func__, args); \
		__INIT_DEBUG++;						\
	}while(0)

#define DEBUG_OUT(fmt, args...)						\
	do {								\
		__INIT_DEBUG--;						\
		DEBUG("Leave[%d] %s: "fmt, __INIT_DEBUG, __func__, args); \
	}while(0)

#endif	/* _LOG_H_ */


#include "daemon.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <unistd.h>

/* This is a hack nonworking implementation of daemon.
   In particular, it should return error codes correctly.
*/
int daemon(int nocd, int noclose)
{
	if (!nocd) chdir("/");
	setsid();
	if (!noclose) {
		freopen("/dev/null","r",stdin);
		freopen("/dev/null","w",stdout);
		freopen("/dev/null","w",stderr);
	}
	return 1;
}

#ifdef __cplusplus
}
#endif


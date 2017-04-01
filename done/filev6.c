#include <stdio.h>
#include "filev6.h"
#include "error.h"

int filev6_open(const struct unix_filesystem *u, uint16_t inr, struct filev6 *fv6)
{
	M_REQUIRE_NON_NULL(u);
	M_REQUIRE_NON_NULL(fv6);
	
	
}

int filev6_readblock(struct filev6 *fv6, void *buf)
{
	M_REQUIRE_NON_NULL(fv6);
	M_REQUIRE_NON_NULL(buf);
	
	
}

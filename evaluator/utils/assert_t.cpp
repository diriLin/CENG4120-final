#include "assert.h"
#include <cstdio>
#include <cstdlib>

void assert_t_(bool ans, const char* file, int line, bool abort) 
{
	if (!ans) {
    	fprintf(stderr, "Error at %s %d\n", file, line);
        if (abort) exit(0);
	}
}
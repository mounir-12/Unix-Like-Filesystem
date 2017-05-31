#include <stdlib.h>
#include <stdio.h>
#include "bmblock.h"

int main(void)
{
	struct bmblock_array *bm = bm_alloc(4, 131);
    bm_print(bm);
    printf("find_next() = %u\n", bm_find_next(bm));
    
    bm_set(bm, 4);
    bm_set(bm, 5);
    bm_set(bm, 6);
    bm_print(bm);
    printf("find_next() = %u\n", bm_find_next(bm));
    
    for(int i = 4; i <= 130; i += 3){
		bm_set(bm, i);
	}
	bm_print(bm);
    printf("find_next() = %u\n", bm_find_next(bm));
    
    for(int i = 5; i <= 130; i += 5){
		bm_clear(bm, i);
	}
	bm_print(bm);
    printf("find_next() = %u\n", bm_find_next(bm));
    
    free(bm);
    bm = NULL;
    
    return 0;
}

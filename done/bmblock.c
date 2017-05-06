#include <stdint.h>
#include <stdlib.h>
#include "inode.h"

struct bmblock_array *bm_alloc(uint64_t min, uint64_t max)
{
    int length = max - min + 1;
    if(length <= 0) {
        return NULL;
    }
    struct bmblock_array *a = NULL;
    a->bm = calloc(length, sizeof(uint64_t));
    if(a->bm != NULL) {
    a->length = size;
    a->cursor = 0;
    a->min = min;
    a->max = max;
} else {
    a = NULL
}
return a;
}

int bm_get(struct bmblock_array *bmblock_array, uint64_t x)
{

}

void bm_set(struct bmblock_array *bmblock_array, uint64_t x)
{

}

void bm_clear(struct bmblock_array *bmblock_array, uint64_t x)
{

}

int bm_find_next(struct bmblock_array *bmblock_array)
{

}

void bm_print(struct bmblock_array *bmblock_array)
{

}

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "inode.h"
#include "error.h"
#include "bmblock.h"

struct bmblock_array *bm_alloc(uint64_t min, uint64_t max)
{
    if(min > max) {
        return NULL;
    }
    size_t length = max - min + 1; // number of elements
    size_t nb = length/BITS_PER_VECTOR + (length % BITS_PER_VECTOR == 0 ? 0 : 1); // number of uint64_t to allocate to cover all elements
    const size_t N_MAX = (SIZE_MAX - sizeof(struct bmblock_array)) / sizeof(uint64_t) +1; // max number of uint64_t to allocate

    if(nb <= N_MAX) {
        struct bmblock_array *a = malloc(sizeof(struct bmblock_array) + (nb-1)*sizeof(uint64_t) );
        if(a != NULL) {
            a->length = nb;
            a->cursor = 0;
            a->min = min;
            a->max = max;
            return a;
        }
    }

    return NULL;
}

int bm_get(struct bmblock_array *bmblock_array, uint64_t x)
{
    M_REQUIRE_NON_NULL(bmblock_array);

    if(x > bmblock_array->max || x < bmblock_array->min) {
        return ERR_BAD_PARAMETER;
    }

    size_t index = (x - bmblock_array->min) / BITS_PER_VECTOR; // index of uint64_t of x whithin bm
    uint64_t bits = bmblock_array->bm[index]; // bits where bit x is contained
    size_t position = (x - bmblock_array->min) % BITS_PER_VECTOR; // position of x whitin bits
    int bit = ((UINT64_C(1) << position) & bits) >> position; // extract the bit
    
    return bit;
}

void bm_set(struct bmblock_array *bmblock_array, uint64_t x)
{
    if(bmblock_array != NULL) {
        if(x >= bmblock_array->min && x <= bmblock_array->max) { //value is in range
            size_t index = (x - bmblock_array->min) / BITS_PER_VECTOR; // index of uint64_t of x whithin bm
            uint64_t bits = bmblock_array->bm[index]; // bits where bit x is contained
            size_t position = (x - bmblock_array->min) % BITS_PER_VECTOR; // position of x whitin bits
            bits |= (UINT64_C(1) << position); // set the bit with an OR
            bmblock_array->bm[index] = bits; // save
        }
    }
}

void bm_clear(struct bmblock_array *bmblock_array, uint64_t x)
{
    if(bmblock_array != NULL) {
        if(x >= bmblock_array->min && x <= bmblock_array->max) { //value is in range
            size_t index = (x - bmblock_array->min) / BITS_PER_VECTOR; // index of uint64_t of x whithin bm
            uint64_t bits = bmblock_array->bm[index]; // bits where bit x is contained
            size_t position = (x - bmblock_array->min) % BITS_PER_VECTOR; // position of x whitin bits
            bits &= ~(UINT64_C(1) << position); // clear the bit with an AND
            bmblock_array->bm[index] = bits; // save
        }
    }
}

int bm_find_next(struct bmblock_array *bmblock_array)
{
    M_REQUIRE_NON_NULL(bmblock_array);
    
    bmblock_array->cursor = UINT64_C(0); // inital value

	uint64_t bits = UINT64_C(-1);
	
	do
	{
		bits = bmblock_array->bm[bmblock_array->cursor]; // read bits pointed by cursor
		if(bits == UINT64_C(-1)) // bits are all ones
		{
			bmblock_array->cursor += 1; // next 64 bits bloc
		}
	}while(bits == UINT64_C(-1) && bmblock_array->cursor < bmblock_array->length); // loop while 64 bits bloc not found
	
	if(bmblock_array->cursor == bmblock_array->length) // no free element (all ones)
	{
		return ERR_BITMAP_FULL;
	}
	uint64_t x = bmblock_array->cursor * BITS_PER_VECTOR + bmblock_array->min; // first element in the found bloc of 64 bits
	while(bm_get(bmblock_array, x) == 1)
	{
		x++; // next element
	}
    return x; // return first x with bit = 0
}

void bm_print(struct bmblock_array *bmblock_array)
{
    printf("**********BitMap Block START**********\n");
    if(bmblock_array == NULL) {
        printf("NULL ptr\n");
    } else {
		uint64_t min = bmblock_array->min; // min value
		uint64_t max = bmblock_array->max; // max value
        
        printf("length: %lu\n",bmblock_array->length);
        printf("min: %lu\n",min);
        printf("max: %lu\n",max);
        printf("cursor: %lu\n",bmblock_array->cursor);
        printf("content:\n");
        for(uint64_t i = 0; i < bmblock_array->length; i++) { //iterate on 64 bits blocs
            printf("%lu:",i);
            
            for(int j = 0; j < BITS_PER_VECTOR; j++) { //iterate the blocs bits
				uint64_t x = i*BITS_PER_VECTOR + j + min; // element number
                printf("%s%u", (j % 8 == 0) ? " " : "", bm_get(bmblock_array, x)); //get bit and print it
            }
            
            printf("\n");
        }
    }
    printf("**********BitMap Block END**********\n");
    fflush(stdout);
}

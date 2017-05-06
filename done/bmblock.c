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
    size_t nb = length/64 + (length % 64 == 0 ? 0 : 1); // number of uint64_t to allocate to cover all elements
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

    size_t index = (x - bmblock_array->min) / 64; // index of uint64_t of x whithin bm
    uint64_t bits = bmblock_array->bm[index]; // bits where bit x is contained
    size_t position = (x - bmblock_array->min) % 64; // position of x whitin bits
    int bit = ((UINT64_C(1) << position) & bits) >> position; // extract the bit
    
    return bit;
}

void bm_set(struct bmblock_array *bmblock_array, uint64_t x)
{
    if(bmblock_array != NULL) {
        if(x >= bmblock_array->min && x <= bmblock_array->max) { //value is in range
            size_t index = (x - bmblock_array->min) / 64; // index of uint64_t of x whithin bm
            uint64_t bits = bmblock_array->bm[index]; // bits where bit x is contained
            size_t position = (x - bmblock_array->min) % 64; // position of x whitin bits
            bits |= (UINT64_C(1) << position); // set the bit with an OR
            bmblock_array->bm[index] = bits; // save
        }
    }
}

void bm_clear(struct bmblock_array *bmblock_array, uint64_t x)
{
    if(bmblock_array != NULL) {
        if(x >= bmblock_array->min && x <= bmblock_array->max) { //value is in range
            size_t index = (x - bmblock_array->min) / 64; // index of uint64_t of x whithin bm
            uint64_t bits = bmblock_array->bm[index]; // bits where bit x is contained
            size_t position = (x - bmblock_array->min) % 64; // position of x whitin bits
            bits &= ~(UINT64_C(1) << position); // clear the bit with an AND
            bmblock_array->bm[index] = bits; // save
        }
    }
}

int bm_find_next(struct bmblock_array *bmblock_array)
{
    M_REQUIRE_NON_NULL(bmblock_array);
	
	if(bmblock_array->cursor >= bmblock_array->length)
	{
		bmblock_array->cursor = bmblock_array->length; // correctly set cursor
		return ERR_BITMAP_FULL; // return appropriate error code
	}
	
	uint64_t bits = UINT64_C(-1);
	
	do
	{
		bits = bmblock_array->bm[bmblock_array->cursor]; // read bits pointed by cursor
		if(bits == UINT64_C(-1)) // bits are all ones
		{
			bmblock_array->cursor += 1; // next 64 bits bloc
		}
	}while(bits == UINT64_C(-1) && bmblock_array->cursor < bmblock_array->length) // loop while 64 bits bloc not found
	
	if(bmblock_array->cursor == bmblock_array->length) // no free element (all ones)
	{
		return ERR_BITMAP_FULL;
	}
	uint64_t x = bmblock_array->cursor * 64 + bmblock_array->min; // first element in the found bloc of 64 bits
	while(bm_get(bmblock_array, x) == 1)
	{
		x++; // next element
	}
    return x; // return first x with bit = 0
}

void bm_print(struct bmblock_array *bmblock_array)
{
    printf("**********BitMap Block START**********");
    if(bmblock_array == NULL) {
        printf("NULL ptr\n");
    } else {
        printf("length: %lu\n",bmblock_array->length);
        printf("min: %lu\n",bmblock_array->min);
        printf("max: %lu\n",bmblock_array->max);
        printf("cursor: %lu\n",bmblock_array->cursor);
        printf("content:\n");
        for(uint64_t i = 0; i < bmblock_array->length; i++) { //iterate on 64 bit numbers
            printf("%lu:",i);
            for(uint64_t bit = 0; bit < sizeof(uint64_t); bit++) { //iterate the numbers' bits
                if(bit % 8 == 0) {//print a space before each byte
                    printf(" ");
                }
                uint64_t index = i * sizeof(uint64_t) + bit + bmblock_array->min; //number of the bit we want to print
                printf("%u", bm_get(bmblock_array, index)); //get bit and print it
            }
            printf("\n");
        }
    }
    printf("**********BitMap Block END**********");
    fflush(stdout);
}

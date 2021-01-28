#include <stdio.h>
#include <memory.h>
#include <unistd.h>

typedef struct header{
    unsigned int size;
    struct header *next;
} header_meta;


static header_meta base;           /* Zero sized block to get us started. */
static header_meta *freep = &base; /* Points to first free block of memory. */
static header_meta *usedp;         /* Points to first used block of memory. */

static void add_to_free_list(header_meta *bp)
{
    header_meta *p;

    for (p = freep; !(bp > p && bp < p->next); p = p->next)
        if (p >= p->next && (bp > p || bp < p->next))
            break;

    if (bp + bp->size == p->next) {
        bp->size += p->next->size;
        bp->next = p->next->next;
    } else
        bp->next = p->next;

    if (p + p->size == bp) {
        p->size += bp->size;
        p->next = bp->next;
    } else
        p->next = bp;

    freep = p;
}
#define MIN_ALLOC_SIZE 4096

static header_meta* morecore(int num_units)
{
    void *vp;
    header_meta *up;

    if (num_units > MIN_ALLOC_SIZE)
        num_units = MIN_ALLOC_SIZE / sizeof(header_meta);

    if ((vp = sbrk(num_units * sizeof(header_meta))) == (void *) -1)
        return NULL;

    up = (header_meta *) vp;
    up->size = num_units;
    add_to_free_list (up);
    return freep;
}

void main(int count, int* values){
    morecore(5);
}
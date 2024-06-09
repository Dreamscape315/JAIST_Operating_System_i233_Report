//
// Created by Akatsuki Sky on 6/8/2024.
//
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "getcmd.c"
#include "ssbanger.inc"


#define dprintf(x) printf x
typedef uint32_t bme_t;			    // Bitmap element type
#define	SSBLKS	40000
#define BMEBITS (sizeof(bme_t) * 8)	// # of bits in the bme_t

// Bitmap storage
bme_t bm[SSBLKS/BMEBITS];		    // Large enough to store SSBLKS bits

int bmfree;
// Macros to manipulate bits in the bitmap
#define	BM_ISCLR(n)	(!(bm[(n)/BMEBITS] & (1 << ((n) % BMEBITS))))
#define	BM_SET(n) bm[(n)/BMEBITS] |= (1 << ((n) % BMEBITS))
#define	BM_CLR(n) bm[(n)/BMEBITS] &= ~(1 << ((n) % BMEBITS))


void bmc_init() {
    memset(bm, 0, sizeof(bm));	// Mark all blocks as 'free'.
    bmfree = SSBLKS;            // initialize bmfree
}

int bmc_alloc(int nblks) {
    int head;   // search head
    int count;  // current contiguous free block count

    for (head = 0; head < SSBLKS; head++) { //Start of a new contiguous free blocks
        for (count = 0; head + count < SSBLKS; count++) {
            if(!BM_ISCLR(head+count))
                break;  //Test if requested block 'nblks' can be allocated starting at 'head'
        }
        if (count > nblks)
            break;
    }
    if (head < SSBLKS) {
        for (count = 0; count < nblks; count++) {
            BM_SET(head+count);
        }
        bmfree -= nblks; // update bmfree
        return head;
    }
    // When requested space is not available, return -1 to indicate an error
    printf("Memory allocation failed : out of memory.\n");
    return -1;
}

int bmc_free(int blkno, int nblks){
    for (int count = 0; count < nblks; count++) {
        BM_CLR(blkno+count);                               // clear the block
    }
    bmfree += nblks;                                       // update bmfree
    return 0;
}

void bmc_dump() {
    for (int count = 0; count < SSBLKS; count++) {
        putchar(BM_ISCLR(count) ? '0' : '1');
        if (count % 64 == 63) {                     //every 64 bits one line
            putchar('\n');
        } else if (count % 8 == 7) {                //every 8 bits one space
            putchar(' ');
        }
    }
}
void bmc_verify() {
    int allocated_blocks = 0;
    for (int count = 0; count < SSBLKS; count++) {
        if (!BM_ISCLR(count)) {
            allocated_blocks++;
        }
    }
    if (allocated_blocks != (SSBLKS - bmfree)) {
        printf("Memory allocation integrity has been compromised.\n");
    } else {
        printf("Memory allocation is valid.\n");
    }
}

void bmc_interactive() {
    int bn;
    char cmd;
    int param1, param2;
    int ic;

    for (;;) {
        fputs("bmc>", stdout); fflush(stdout);
        switch((ic = getcmd(&cmd, &param1, &param2))) { // get command
            case 0: // EOF
                goto out;

            case 1: // Command only
                if (cmd == 'd')
                    bmc_dump();
                else if (cmd == 'v')
                    bmc_verify();
                break;

            case 2: // Command and a parameter
                if (cmd == 'a') {
                    bn = bmc_alloc(param1);
                    printf("A %d %d\n", bn, param1);
                }
                else if (cmd == 'b') {
                    ssbanger(param1, bmc_alloc, bmc_free, NULL, NULL, NULL);
                }
                else{
                    printf("which one? 1");
                    fprintf(stderr, "bad command '%c' "
                                    "or invalid parameter(s)\n", cmd);
                }
                break;

            case 3: // Command and two parameters
                if (cmd == 'f') {
                    bmc_free(param1, param2);
                    printf("F %d\n", param1);
                }
                else if (cmd == 'b') {
                    ssbanger_mt(param1, param2, bmc_alloc, bmc_free, NULL, NULL, NULL);
                }
                else {
                    printf("which one? 2");
                    fprintf(stderr, "bad command '%c' "
                                    "or invalid parameter(s)\n", cmd);
                }
                break;

            case -1: // Error
                break;
        }
    }

    out:;
}

int main(void) {
    bmc_init();

#if 0
    bmc_interactive();
#else
    ssbanger(1000, bmc_alloc, bmc_free, NULL, NULL, NULL);
#endif

    bmc_dump();
    bmc_verify();
    return 0;
}
//sss 222
//ssbanger: total 1000 allocreq 774 success 774 fail 0
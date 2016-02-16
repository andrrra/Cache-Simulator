#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>

typedef enum {dm, fa} cache_map_t;
typedef enum {uc, sc} cache_org_t;
typedef enum {instruction, data} access_t;

typedef struct {
    uint32_t address;
    access_t accesstype;
} mem_access_t;

// Caches and counters for the statistics to be printed.
typedef struct {
    int tag;
    int valid;
} block;
block *c1, *c2;
int hits = 0;
int hitsD = 0;
int hitsI = 0;
int accesses = 0;
int accessesD = 0;
int accessesI = 0;

// Blockcounters used to count the occupied positions in the caches.
int blockcounter = 0;
int blockcounterD = 0;
int blockcounterI = 0;

// Variables with hold lenghts of different parts of the addresses.
uint32_t blockno;
uint32_t offsetsize = 6;
uint32_t indexsize = 0;
uint32_t tagsize;

// i used in for loops.
// ok used when checking for existing addresses in the caches (fully associative cache mapping);
// ok set to 1 when there is a hit and it is 0 otherwise.
// fifo used to find the position at which the first address was added into the cache, in order to replace it.
int i, ok, fifo = 0, fifoD = 0, fifoI = 0;


uint32_t cache_size;
uint32_t block_size = 64;
cache_map_t cache_mapping;
cache_org_t cache_org;


/* Reads a memory access from the trace file and returns
 * 1) access type (instruction or data access
 * 2) memory address
 */
mem_access_t read_transaction(FILE *ptr_file) {

    char buf[1000];
    char* token;
    char* string = buf;
    mem_access_t access;

    if (fgets(buf,1000, ptr_file)!=NULL) {

        /* Get the access type */
        token = strsep(&string, " \n");
        if (strcmp(token,"I") == 0) {
            access.accesstype = instruction;
        } else if (strcmp(token,"D") == 0) {
            access.accesstype = data;
        } else {
            printf("Unkown access type\n");
            exit(0);
        }

        /* Get the access type */
        token = strsep(&string, " \n");
        access.address = (uint32_t)strtol(token, NULL, 16);

        return access;
    }

    /* If there are no more entries in the file,
     * return an address 0 that will terminate the infinite loop in main
     */
    access.address = 0;
    return access;
}

/* Fully Associative cache mapping */
void fullyAssociative (mem_access_t a){

    uint32_t accessTag = a.address >> offsetsize;           //shift right the input address in order to remove the offset from it.
    ok = 0;

    if (cache_org == uc) {
            accesses++;                                     // increment the number of accesses.
            for (i = 0; i < blockcounter && ok == 0; i++) { // loop through the cache.
                    if (accessTag == c1[i].tag && c1[i].valid == 1) {  // check all tags in the cache for a match.
                            hits++;                         // if a match occurs inrement the hits number.
                            ok = 1;                         // if a match occurs, ok is set to 1 and the loop is exited.
                    }
            }
            if (ok == 0 && blockcounter < blockno) {        // if no hit happened and there still are unoccupied blocks in the cache
                    c1[blockcounter].tag = accessTag;       // add the accessTag to the cache.
                    c1[blockcounter].valid = 1;             // set valid to 1.
                    blockcounter++;                         // increment the number of occupied cache blocks.
            }
            else if (ok == 0 && blockcounter == blockno) {  // if no hit happened and the cache is full
                    c1[fifo].tag = accessTag;               // replace the first added tag with the current one.
                    c1[fifo].valid = 1;                     // set valid to 1
                    if (fifo == blockno - 1) fifo = 0;      // reset to first line
                        else (fifo)++;
            }
    }

    /*For a split cache, check which kind of accestype we are dealing with
    before doing the rest of the operations.*/
    else if (cache_org == sc && a.accesstype == data) {
            accessesD++;
            for (i = 0; i < blockcounterD && ok == 0; i++) {
                    if (accessTag == c1[i].tag && c1[i].valid == 1) {
                            hitsD++;
                            ok = 1;
                    }
            }
            if (ok == 0 && blockcounterD < blockno) {
                    c1[blockcounterD].tag = accessTag;
                    c1[blockcounterD].valid = 1;
                    blockcounterD++;
            }
            else if (ok == 0 && blockcounterD == blockno) {
                    c1[fifoD].tag = accessTag;
                    c1[fifoD].valid = 1;
                    if (fifoD == blockno - 1)
                            fifoD = 0;
                    else (fifoD)++;
            }
    }

    else if (cache_org == sc && a.accesstype == instruction) {
            accessesI++;
            for (i = 0; i < blockcounterI && ok == 0; i++) {
                    if (accessTag == c2[i].tag && c2[i].valid == 1) {
                            hitsI++;
                            ok = 1;
                    }
            }
            if (ok == 0 && blockcounterI < blockno) {
                    c2[blockcounterI].tag = accessTag;
                    c2[blockcounterI].valid = 1;
                    blockcounterI++;
            }
            else if (ok == 0 && blockcounterI == blockno) {
                    c2[fifoI].tag = accessTag;
                    c2[fifoI].valid = 1;
                    if (fifoI == blockno - 1)
                            fifoI = 0;
                    else (fifoI)++;
            }
    }
}

/* Directed Mapped cache mapping */
void directedMapping (mem_access_t a){

    uint32_t accessTag = (a.address) >> offsetsize; //shift right the input address in order to remove the offset from it.
    uint32_t index = accessTag & (blockno - 1);     //get the index from the address.
    accessTag = accessTag >> indexsize;             //get the tag by removing the index.

    /*Checks if the cache organization is unified or split. */
    if (cache_org == uc) {
            accesses++;                             //increment the number of accesses.
            if (c1[index].tag == accessTag && c1[index].valid == 1) //check if the cache has at index a tag equal to the access tag
                hits++;                             //if so, then increment the number or hits.
            else
                c1[index].tag = accessTag;
                c1[index].valid = 1;
        }

    /*For a split cache, check which kind of accestype we are dealing with
    before doing the rest of the operations.*/
    else if (cache_org == sc && a.accesstype == data) {
            accessesD++;
            if (c1[index].tag == accessTag && c1[index].valid == 1)
                hitsD++;
            else
                c1[index].tag = accessTag;
                c1[index].valid = 1;
        }
    else if (cache_org == sc && a.accesstype == instruction) {
            accessesI++;
            if (c2[index].tag == accessTag && c2[index].valid == 1)
                hitsI++;
            else
                c2[index].tag = accessTag;
                c2[index].valid = 1;
    }
}

int main(int argc, char** argv)
{
    /* Read command-line parameters and initialize:
     * cache_size, cache_mapping and cache_org variables
     */

    if ( argc != 4 ) { /* argc should be 2 for correct execution */
        printf("Usage: ./cache_sim [cache size: 128-4096] [cache mapping: dm|fa] [cache organization: uc|sc]\n");
        exit(0);
    } else  {
        /* argv[0] is program name, parameters start with argv[1] */

        /* Set cache size */
        cache_size = atoi(argv[1]);

        /* Set Cache Mapping */
        if (strcmp(argv[2], "dm") == 0) {
            cache_mapping = dm;
        } else if (strcmp(argv[2], "fa") == 0) {
            cache_mapping = fa;
        } else {
            printf("Unknown cache mapping\n");
            exit(0);
        }

        /* Set Cache Organization */
        if (strcmp(argv[3], "uc") == 0) {
            cache_org = uc;
        } else if (strcmp(argv[3], "sc") == 0) {
            cache_org = sc;
        } else {
            printf("Unknown cache organization\n");
            exit(0);
        }
    }

    if (cache_org == uc)
        blockno = cache_size/block_size;
    else if (cache_org == sc)
        blockno = cache_size/block_size/2;

    if (cache_mapping == dm)
        indexsize = log2(blockno);

    tagsize = 32 - offsetsize - indexsize;
    c1 = (block*) calloc (blockno, sizeof(block));
    if (cache_org == sc)
        c2 = (block*) calloc (blockno, sizeof(block));

    /* Open the file mem_trace.txt to read memory accesses */
    FILE *ptr_file;
    ptr_file =fopen("mem_trace.txt","r");
    if (!ptr_file) {
        printf("Unable to open the trace file\n");
        exit(1);
    }

    /* Loop until whole trace file has been read */
    mem_access_t access;
    while(1) {
        access = read_transaction(ptr_file);
        //If no transactions left, break out of loop
        if (access.address == 0)
            break;

	/* Do a cache access */
        if (cache_mapping == fa)
            fullyAssociative(access);
        else directedMapping(access);
    }

    /* Print the statistics */
    if (cache_org == uc) {
        printf("U.accesses: %d\n", accesses);
        printf("U.hits: %d\n", hits);
        printf("U.hit rate: %1.3f\n", (double) hits/accesses);
    }
    else if (cache_org == sc) {
        printf("I.accesses: %d\n", accessesI);
        printf("I.hits: %d\n", hitsI);
        printf("I.hit rate: %1.3f\n\n", (double) hitsI/accessesI);
        printf("D.accesses: %d\n", accessesD);
        printf("D.hits: %d\n", hitsD);
        printf("D.hit rate: %1.3f\n", (double) hitsD/accessesD);
    }

    /* Close the trace file */
    fclose(ptr_file);

    /* Free the caches */
    free (c1);
    if (cache_org == sc)
        free (c2);

}

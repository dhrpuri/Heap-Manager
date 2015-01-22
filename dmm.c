#include <stdio.h> //needed for size_t
#include <unistd.h> //needed for sbrk
#include <assert.h> //For asserts
#include "dmm.h"

typedef struct metadata {

    /* size_t is the return type of the sizeof operator. Since the size of
    * an object depends on the architecture and its implementation, size_t 
    * is used to represent the maximum size of any object in the particular
    * implementation. 
    * size contains the size of the data object or the amount of free
    * bytes 
    */
    size_t size;
    struct metadata* next;
    struct metadata* prev;
    bool free; 
 } metadata_t;

/* Blocklist maintains all the blocks (both free and allocated); blocklist is kept
 * always sorted to improve the efficiency of coalescing 
 */
 static metadata_t* blocklist = NULL;

 void* dmalloc(size_t numbytes) {

    if(blocklist == NULL) { //Initialize through sbrk call first time
        if(!dmalloc_init())
            return NULL;
    }
    assert(numbytes > 0);

    metadata_t* current = blocklist; //Start from the first block

    // Increment block until we find one that is free and large enough to fit numbytes
    while (current && !(current->free && current->size >= numbytes)) {
        current = current->next;
    }

    if (current==NULL){ //Check that current is a valid metadata_t*
        return NULL;
    }

    current->free = false; //Mark block as allocated

    if (current->size >= (numbytes + METADATA_T_ALIGNED)) { //Check if block size is large enough to split

        /* Create new free block in current's extra space
        */
        metadata_t* newblock = (metadata_t *)(((void*) current) + numbytes + METADATA_T_ALIGNED);
        newblock->free = true;
        newblock->size = current->size - numbytes - METADATA_T_ALIGNED;
        newblock->prev = current;
        newblock->next = current->next;
        if(newblock->next){
            newblock->next->prev = newblock;
        }
        
        /* Refine current block's data
        */
        current->size = numbytes;
        current->next = newblock;
    }

    return ((void*)current) + METADATA_T_ALIGNED; //Return pointer to the beginning of the newly allocated block
}


void dfree(void* ptr) {

    metadata_t* block_to_free = ((metadata_t*) ptr) - 1; //Find the block that we want to free from the pointer parameter

    if (block_to_free == NULL){ //Return if invalid block
        return;
    }

    block_to_free->free = true; //Free block

    //Coalesce after each free

    metadata_t* previous_block = block_to_free->prev;
    metadata_t* next_block = block_to_free->next;

    if (previous_block && previous_block->free){

        /* Combine previous and current block
        */
        previous_block->size += block_to_free->size + METADATA_T_ALIGNED;
        previous_block->next = next_block;

        if (next_block && next_block->free){

            /* Combine previous and next block
            */
            previous_block->size += next_block->size + METADATA_T_ALIGNED;
            previous_block->next = next_block->next;
        }
    }
    else if (next_block && next_block->free){

        /* Combine current and next block
        */
        block_to_free->size += next_block->size + METADATA_T_ALIGNED;
        block_to_free->next = next_block->next;
    }
}

bool dmalloc_init() {

    /* Two choices: 
    * 1. Append prologue and epilogue blocks to the start and the end of the blocklist
    * 2. Initialize blocklist pointers to NULL
    *
    * Note: We provide the code for 2. Using 1 will help you to tackle the
    * corner cases succinctly.
    */

    size_t max_bytes = ALIGN(MAX_HEAP_SIZE);
    blocklist = (metadata_t*) sbrk(max_bytes); // returns heap_region, which is initialized to blocklist
    /* Q: Why casting is used? i.e., why (void*)-1? */
    if (blocklist == (void *)-1)
        return false;
    blocklist->next = NULL;
    blocklist->prev = NULL;
    blocklist->size = max_bytes-METADATA_T_ALIGNED;
    blocklist->free = true; //Initial memory is all free

    return true;
}

/*Only for debugging purposes; can be turned off through -NDEBUG flag*/
void print_freelist() {
    metadata_t *blocklist_head = blocklist;
    while(blocklist_head != NULL) {
        DEBUG("\tblocklist Size:%zd, Head:%p, Prev:%p, Next:%p, Free:%d\t\n",blocklist_head->size,blocklist_head,blocklist_head->prev,blocklist_head->next,blocklist_head->free);
        blocklist_head = blocklist_head->next;
    }
    DEBUG("\n");
}

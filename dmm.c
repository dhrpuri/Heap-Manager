#include <stdio.h> //needed for size_t
#include <unistd.h> //needed for sbrk
#include <assert.h> //For asserts
#include "dmm.h"
/* You can improve the below metadata structure using the concepts from Bryant
 * and OHallaron book (chapter 9).
 */


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
	struct metadata* prev; //What's the use of prev pointer?
	bool free;
} metadata_t;

/* freelist maintains all the blocks which are not in use; freelist is kept
 * always sorted to improve the efficiency of coalescing 
 */

static metadata_t* freelist = NULL;

void* dmalloc(size_t numbytes) {
	
	if(freelist == NULL) { 			//Initialize through sbrk call first time
		if(!dmalloc_init())
			return NULL;
	}
	assert(numbytes > 0);


	/* We need a loop to traverse the freelist and find if any of the blocks have
     * been freed (by checking the metadata). If a free block is found, then we can
	 * malloc it, otherwise we call sbrk to move the heap pointer up.
	 */

metadata_t* current = freelist;
while (current && !(current->free && current->size >= numbytes)) {
    current = current->next;
}
if (current==NULL){
 return NULL;
}

	current->free = false;

metadata_t* newblock = current + numbytes + METADATA_T_ALIGNED;  
newblock->free = true;



newblock->size = current->size - numbytes - METADATA_T_ALIGNED;
current->size = numbytes;


newblock->next = current->next;
current->next = newblock;


if(newblock->next){
	newblock->next->prev = newblock;
}

newblock->prev = current;


return newblock + METADATA_T_ALIGNED;

}


void dfree(void* ptr) {
	
	printf("%d\n", ptr);
	metadata_t* block_to_free =  (metadata_t*) ptr;// - METADATA_T_ALIGNED;
	printf("%d\n", block_to_free);
	if (block_to_free){

	block_to_free->free = true;


//coallesing


	//metadata_t* previous_block = block_to_free->prev;
	//metadata_t* next_block = block_to_free->next;

	// 	if (previous_block){

// 	if (previous_block->free){
// 	printf("%s\n","we here" );
// 	previous_block->size = previous_block->size+ block_to_free->size + METADATA_T_ALIGNED;
// 	previous_block->next = next_block;

// 	if (next_block && next_block->free){
// 	previous_block->size = previous_block->size+ next_block->size + METADATA_T_ALIGNED;
// 	previous_block->next = next_block->next;
// 	}
// }
// }
// else if (next_block){
// 	if (next_block->free){
// 	block_to_free->size = block_to_free->size+ next_block->size + METADATA_T_ALIGNED;
// 	block_to_free->next = next_block->next;
// }
// }

	}

}

bool dmalloc_init() {

	/* Two choices: 
 	* 1. Append prologue and epilogue blocks to the start and the end of the freelist
 	* 2. Initialize freelist pointers to NULL
 	*
 	* Note: We provide the code for 2. Using 1 will help you to tackle the
 	* corner cases succinctly.
 	*/

	size_t max_bytes = ALIGN(MAX_HEAP_SIZE);
	freelist = (metadata_t*) sbrk(max_bytes); // returns heap_region, which is initialized to freelist
	/* Q: Why casting is used? i.e., why (void*)-1? */
	if (freelist == (void *)-1)
		return false;
	freelist->next = NULL;
	freelist->prev = NULL;
	freelist->size = max_bytes-METADATA_T_ALIGNED;
	freelist->free = true;
	return true;
}

/*Only for debugging purposes; can be turned off through -NDEBUG flag*/
void print_freelist() {
	metadata_t *freelist_head = freelist;
	while(freelist_head != NULL) {
		printf("\tFreelist Size:%zd, Head:%p, Prev:%p, Next:%p, Free:%d\t\n",freelist_head->size,freelist_head,freelist_head->prev,freelist_head->next,freelist_head->free);
		freelist_head = freelist_head->next;
	}
	DEBUG("\n");
}

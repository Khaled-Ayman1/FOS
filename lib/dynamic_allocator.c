/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
uint32 get_block_size(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->size ;
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
int8 is_free_block(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->is_free ;
}

//===========================================
// 3) ALLOCATE BLOCK BASED ON GIVEN STRATEGY:
//===========================================
void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockMetaData* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", blk->size, blk->is_free) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
bool is_initialized = 0;
//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{

	if (initSizeOfAllocatedSpace == 0 || (initSizeOfAllocatedSpace + sizeOfMetaData()) > DYN_ALLOC_MAX_SIZE)
		return ;
	is_initialized = 1;

	struct BlockMetaData *initAlloc = (struct BlockMetaData *) daStart;
	initAlloc->size = initSizeOfAllocatedSpace;
	initAlloc->is_free = 1;

	LIST_INIT(&BlockList);
	LIST_INSERT_HEAD(&BlockList, initAlloc);

	//TODO: [PROJECT'23.MS1 - #5] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator()
	//panic("initialize_dynamic_allocator is not implemented yet");
}

//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================


void *alloc_block_FF(uint32 size)
{
	//TODO: [PROJECT'23.MS1 - #6] [3] DYNAMIC ALLOCATOR - alloc_block_FF()

	if(size <= 0)
		return NULL;

	if(!is_initialized)
	{
		uint32 required_size = size +sizeOfMetaData();
		uint32 da_start = (uint32)sbrk(required_size);

		uint32 da_break = (uint32)sbrk(0);
		initialize_dynamic_allocator(da_start, da_break - da_start);
	}

	uint32 allocatedSize = size + sizeOfMetaData();
	struct BlockMetaData *block, *exStart, *allocated = NULL;

	exStart = LIST_LAST(&BlockList);
	void *heapLimit;


	LIST_FOREACH(block, &BlockList){

		if((block->size >= allocatedSize) && block->is_free == 1){

			uint32 block_size = block->size;

			allocated = block;
			allocated->size = allocatedSize;
			allocated->is_free = 0;


			if(block_size != allocatedSize) {

				if((block_size - allocatedSize) < sizeOfMetaData())

					allocated->size = block_size;

				else{

					struct BlockMetaData *newBlock = (void *) block + allocatedSize;
					LIST_INSERT_AFTER(&BlockList, block, newBlock );
					newBlock->size = block_size - allocatedSize;
					newBlock->is_free = 1;

				}
			}

			return ((void *) allocated + sizeOfMetaData());
		}
	}

	heapLimit = sbrk(0);

	if((uint32)heapLimit == ((uint32)exStart + exStart->size)){

		if(exStart->is_free == 0){

			//sbrk() calls multiple of pages, the following block handles free unused space

			uint32 exPages = allocatedSize / PAGE_SIZE; //Pages to be allocated

			if (allocatedSize % PAGE_SIZE != 0)
				exPages++;

			uint32 exSize = exPages * PAGE_SIZE;  //Total size >= allocatedSize

			heapLimit = sbrk(allocatedSize);

			if(heapLimit == (void *)-1)
				return NULL;

			allocated = heapLimit;
			allocated->size = allocatedSize;
			allocated->is_free = 0;

			LIST_INSERT_AFTER(&BlockList, exStart, allocated);

			//In case LIST_INSERT_AFTER fails uncomment the following:
			//LIST_INSERT_TAIL(&BlockList, allocated);

			if(exSize > allocatedSize){

				if((exSize - allocatedSize) < sizeOfMetaData())

					allocated->size = exSize;

				else{

					struct BlockMetaData *newBlock = (void *) allocated + allocatedSize;
					newBlock->size = exSize - allocatedSize;
					newBlock->is_free = 1;

					//In case LIST_INSERT_AFTER fails use Insert tail
					LIST_INSERT_AFTER(&BlockList, allocated, newBlock);
				}
			}

			return ((void *) allocated + sizeOfMetaData());
		}

		uint32 exAlloc = allocatedSize - (exStart->size); //allocated part of expansion
		uint32 exPages = exAlloc / PAGE_SIZE; //Pages to be allocated

		if (exAlloc % PAGE_SIZE != 0)
			exPages++;

		uint32 exSize = exPages * PAGE_SIZE;  //Total size >= (exAlloc)

		heapLimit = sbrk(exAlloc);

		if(heapLimit == (void *)-1)
			return NULL;

		allocated = exStart;
		allocated->size = allocatedSize;
		allocated->is_free = 0;

		if(exSize > exAlloc){

			if((exSize - exAlloc) < sizeOfMetaData())

				allocated->size = allocatedSize + exSize;

			else{

				struct BlockMetaData *newBlock = (void *) allocated + allocatedSize;
				newBlock->size = exSize - exAlloc;
				newBlock->is_free = 1;

				//In case LIST_INSERT_TAIL fails use Insert After
				LIST_INSERT_AFTER(&BlockList, allocated, newBlock);
			}
		}

		return ((void *) allocated + sizeOfMetaData());
	}


	return NULL;
}

//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	//TODO: [PROJECT'23.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF()
	//panic("alloc_block_BF is not implemented yet");

	if(size <= 0)
		return NULL;

	struct BlockMetaData *exStart = NULL;

	uint32 allocatedSize = size + sizeOfMetaData();
	uint32 minSize = 0;

	struct BlockMetaData *block, *allocated = NULL;

	void *heapLimit;
	int blockFound = 0;
	int initBlock = 1;

	LIST_FOREACH(block, &BlockList)
	{

		if((block->is_free == 1) && (block->size >= allocatedSize) && initBlock)
		{
			blockFound = 1;
			initBlock = 0;
			minSize = block->size;
			allocated = block;
			continue;
		}

		if(blockFound && block->is_free && block->size >= allocatedSize && block->size < minSize)
		{
			minSize = block->size;
			allocated = block;
		}

	}

	if(blockFound)
	{
		allocated->size = allocatedSize;
		allocated->is_free = 0;

		if(minSize != allocatedSize) {

			if((minSize - allocatedSize) < sizeOfMetaData())

				allocated->size = minSize;

			else{

				struct BlockMetaData *newBlock = (void *) allocated + allocatedSize;
				LIST_INSERT_AFTER(&BlockList, allocated, newBlock );
				newBlock->size = minSize - allocatedSize;
				newBlock->is_free = 1;
			}
		}

		return ((void *) allocated + sizeOfMetaData());
	}

	else{

		uint32 exPages = allocatedSize / PAGE_SIZE; //Pages to be allocated

		if (allocatedSize % PAGE_SIZE != 0)
			exPages++;

		uint32 exSize = exPages * PAGE_SIZE;  //Total size >= allocatedSize

		heapLimit = sbrk(allocatedSize);

		if(heapLimit == (void *)-1)
			return NULL;

		allocated = heapLimit;
		allocated->size = allocatedSize;
		allocated->is_free = 0;


		LIST_INSERT_AFTER(&BlockList, exStart, allocated);

		//In case LIST_INSERT_AFTER fails uncomment the following:
		//LIST_INSERT_TAIL(&BlockList, allocated);

		if(exSize > allocatedSize){

			if((exSize - allocatedSize) < sizeOfMetaData())

				allocated->size = exSize;

			else{

				struct BlockMetaData *newBlock = (void *) allocated + allocatedSize;
				newBlock->size = exSize - allocatedSize;
				newBlock->is_free = 1;

				//In case LIST_INSERT_AFTER fails use Insert tail
				LIST_INSERT_AFTER(&BlockList, allocated, newBlock);
			}
		}


		return ((void *) allocated + sizeOfMetaData());
	}

	return NULL;

}

//=========================================
// [6] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}

//===================================================
// [8] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//TODO: [PROJECT'23.MS1 - #7] [3] DYNAMIC ALLOCATOR - free_block()
	//panic("free_block is not implemented yet");

	if (va == NULL) return;

	struct BlockMetaData *block_to_free = (struct BlockMetaData *) (va - sizeOfMetaData());


	struct BlockMetaData *nextBlock = LIST_NEXT(block_to_free);
	struct BlockMetaData *prevBlock = LIST_PREV(block_to_free);


	//merge with the one after it if the next is free
	if (nextBlock && nextBlock->is_free==1) {


		block_to_free->size += nextBlock->size;

		//zeroING
		nextBlock->size = 0;
		nextBlock->is_free = 0;

		//remove the next free block, the current freed block will be the merge of both
		LIST_REMOVE(&BlockList, nextBlock);

	}


	//point to prev if it is free, can happen simultaneous with previous the IF that checks for next
	if (prevBlock && prevBlock->is_free==1) {

		prevBlock->size += block_to_free->size ;

		//zeroING
		block_to_free->size = 0;
		block_to_free->is_free = 0;

		//remove the current freed block as the previous free block is now the merge of both
		LIST_REMOVE(&BlockList, block_to_free);

		block_to_free = prevBlock;

	}

	block_to_free->is_free = 1;


}

//=========================================
// [4] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	//TODO: [PROJECT'23.MS1 - #8] [3] DYNAMIC ALLOCATOR - realloc_block_FF()
	//panic("realloc_block_FF is not implemented yet");

	struct BlockMetaData *currBlock = (struct BlockMetaData *) (va - sizeOfMetaData());
	struct BlockMetaData *nextBlock = LIST_NEXT(currBlock);
	struct BlockMetaData *prevBlock = LIST_PREV(currBlock);

	uint32 total_size = new_size + sizeOfMetaData();


	//The possible 5 cases: special, no split, split, need to allocate somewhere else, decreasing, we are at top so no nextBlock


	//CASE#0: special cases, if va is null or size is 0 or both

	if (va == NULL)
		return alloc_block_FF(new_size);
	else if (new_size == 0) {
		free_block(va);
		return NULL;
	}


	//CASE#1: size increasing and no splitting
	if (nextBlock->is_free == 1  &&  total_size == (currBlock->size + nextBlock->size)) {

		currBlock->size = total_size;

		//zeroING nextBlock as it is now completely merged with currBlock
		nextBlock->is_free = 0;
		nextBlock->size = 0;

		LIST_REMOVE(&BlockList, nextBlock);

		return (void*)currBlock + sizeOfMetaData();
	}

	//CASE#2: size increasing with splitting
	else if (nextBlock->is_free == 1  &&  total_size < (currBlock->size + nextBlock->size)) {

		uint32 nextBlockSize = nextBlock->size; //saving its size
		//zeroIng the old nextBlock
		nextBlock->is_free = 0;
		nextBlock->size = 0;
		//the new nextBlock
		nextBlock = (void*) nextBlock + (total_size - currBlock->size);
		nextBlock->is_free = 1;
		nextBlock->size = (currBlock->size + nextBlockSize) - total_size;

		currBlock->size = total_size;

		return (void*)currBlock + sizeOfMetaData();
	}

	//CASE#3: need more than what's available in current plus next
	else if (nextBlock->is_free == 1  &&  total_size > (currBlock->size + nextBlock->size)) {
		free_block(currBlock + sizeOfMetaData());
		return alloc_block_FF(new_size); //alloc takes the size without metadata, hence not passing total_size
	}


	//CASE#4: size decreasing case
	else if (total_size < currBlock->size) {

		//1st sub-case: there is a block after it and it is free
		if (nextBlock  &&  nextBlock->is_free == 1) {

			uint32 nextBlockSize = nextBlock->size; //saving its size

			//zeroIng old nextBlock
			nextBlock->is_free = 0;
			nextBlock->size = 0;

			//new nextBlock
			nextBlock = (void*)nextBlock - (currBlock->size - total_size);
			nextBlock->is_free = 1;
			nextBlock->size = nextBlockSize + (currBlock->size - total_size);

		}

		//2nd sub-case: there is either no block after it (it's last) or it is not free
		//create a new free one in either cases
		else {
			struct BlockMetaData *newFreeBlock = (void*)currBlock + total_size;
			newFreeBlock->is_free = 1;

			newFreeBlock->size = currBlock->size - total_size;
			LIST_INSERT_AFTER(&BlockList, currBlock, newFreeBlock);
		}


		currBlock->size = total_size;
		return (void*) currBlock + sizeOfMetaData();

	}


	//CASE#5: if we are at the top and there is no nextBlock or nextBlock is a zero sized block or the next Block is not free
	if (!nextBlock || (nextBlock && nextBlock->size==0) || (nextBlock && nextBlock->is_free==0)) {

		if (total_size == currBlock->size)
			return (void*)currBlock + sizeOfMetaData();
		else if (total_size > currBlock->size) {
			free_block(currBlock+sizeOfMetaData());
			return alloc_block_FF(new_size);
		}
		//the case of total_size being less than is covered in case#4 in its 2nd sub-case

	}


	return NULL;
}

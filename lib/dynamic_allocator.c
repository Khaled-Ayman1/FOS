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

//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{

	if (initSizeOfAllocatedSpace == 0 || (initSizeOfAllocatedSpace + sizeOfMetaData()) > DYN_ALLOC_MAX_SIZE)
		return ;

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

	if(size == 0)
		return NULL;

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

				struct BlockMetaData *newBlock = (void *) block + allocatedSize;
				LIST_INSERT_AFTER(&BlockList, block, newBlock );
				newBlock->size = block_size - allocatedSize;
				newBlock->is_free = 1;
			}

			return ((void *) allocated + sizeOfMetaData());
		}
	}

	heapLimit = sbrk(0);

	if((uint32)heapLimit == ((uint32)exStart + exStart->size)){

		if(exStart->is_free == 0){

			heapLimit = sbrk(allocatedSize);

			if(heapLimit == (void *)-1)
				return NULL;

			allocated = heapLimit;
			allocated->size = allocatedSize;
			allocated->is_free = 0;

			LIST_INSERT_TAIL(&BlockList, allocated);

			//In case LIST_INSERT_TAIL fails uncomment the following:
			//LIST_INSERT_AFTER(&BlockList, exStart, allocated);

			return ((void *) allocated + sizeOfMetaData());
		}

		heapLimit = sbrk((allocatedSize) - (exStart->size));

		if(heapLimit == (void *)-1)
			return NULL;

		allocated = exStart;
		allocated->size = allocatedSize;
		allocated->is_free = 0;

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
	panic("alloc_block_BF is not implemented yet");
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
	panic("realloc_block_FF is not implemented yet");
	return NULL;
}

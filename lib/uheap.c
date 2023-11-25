#include <inc/lib.h>

#define maxThreads 1024
void *thrd_addr[maxThreads] = {NULL};
int thrd_pages[maxThreads] = {0};
//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

int FirstTimeFlag = 1;
void InitializeUHeap()
{
	if(FirstTimeFlag)
	{
#if UHP_USE_BUDDY
		initialize_buddy();
		cprintf("BUDDY SYSTEM IS INITIALIZED\n");
#endif
		FirstTimeFlag = 0;
	}
}

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
void* sbrk(int increment)
{
	return (void*) sys_sbrk(increment);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================
void* malloc(uint32 size)
{

	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0 || size > DYN_ALLOC_MAX_SIZE) return NULL ;

	if(size <= DYN_ALLOC_MAX_BLOCK_SIZE)
		return sys_allocate_user_mem(alloc_block_FF(size),size);

	//==============================================================
	//TODO: [PROJECT'23.MS2 - #09] [2] USER HEAP - malloc() [User Side]

	if(sys_isUHeapPlacementStrategyFIRSTFIT()){

		uint32 pagePtr; // initialize with uhl + PAGE_SIZE
		uint32 totalSize;
		int successfullyAllocatedThread = 0;
		int thrdSize = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
		for (int i=0; i < maxThreads; i++) {

			if (thrd_addr[i] == NULL) {

				if(thrd_pages[i] == 0){

					if(i == 0){

						thrd_addr[i] = pagePtr;
						thrd_pages[i] = thrdSize;
						return sys_allocate_user_mem(thrd_addr[i], size);

					}

					pagePtr = thrd_addr[i-1] + (thrd_pages[i-1] * PAGE_SIZE)
					totalSize = pagePtr + (thrd_pages[i]* PAGE_SIZE);

					if(totalSize > USER_HEAP_MAX)
						return NULL;

					thrd_addr[i] = thrd_addr[i-1] + (thrd_pages[i-1] * PAGE_SIZE);
					thrd_pages[i] = thrdSize;
					return sys_allocate_user_mem(thrd_addr[i], size);
				}

				int totalPages = thrd_pages[i];
				for(int j = i + 1; j < maxThreads; j++){

					if(thrd_addr[j] != NULL)
						break;
					totalPages += thrd_pages[j];
				}
				totalSize = totalPages * PAGE_SIZE;

				if(totalSize >= size){

					thrd_addr[i] = pagePtr;
					thrd_pages[i] = thrdSize;
					return sys_allocate_user_mem(thrd_addr[i], size);

				}
			}
		}
	}
//cases not handled: 1) if double 0s mid function 2)merging pages and allocating 3)freeing and merging
	return NULL;

	//Use sys_isUHeapPlacementStrategyFIRSTFIT() and	sys_isUHeapPlacementStrategyBESTFIT()
	//to check the current strategy

}

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void* virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #11] [2] USER HEAP - free() [User Side]
	// Write your code here, remove the panic and write your code
	panic("free() is not implemented yet...!!");
}


//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0) return NULL ;
	//==============================================================
	panic("smalloc() is not implemented yet...!!");
	return NULL;
}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================
	// Write your code here, remove the panic and write your code
	panic("sget() is not implemented yet...!!");
	return NULL;
}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================

	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}


//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{
	// Write your code here, remove the panic and write your code
	panic("sfree() is not implemented yet...!!");
}


//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize)
{
	panic("Not Implemented");

}
void shrink(uint32 newSize)
{
	panic("Not Implemented");

}
void freeHeap(void* virtual_address)
{
	panic("Not Implemented");

}

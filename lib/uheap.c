#include <inc/lib.h>

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


#define MAX_USER_PAGES (USER_HEAP_MAX - USER_HEAP_START) / PAGE_SIZE

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

uint32 userPages[MAX_USER_PAGES] = {0};

void* malloc(uint32 size)
{
	InitializeUHeap();

	if (size == 0 || size > DYN_ALLOC_MAX_SIZE) return NULL ;

	if(size <= DYN_ALLOC_MAX_BLOCK_SIZE)
	{
		return alloc_block_FF(size);
	}

	uint32 uhl = sys_get_uhl();

	uint32 neededPages = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
	uint32 startIndex, firstIndex;
	uint32 counter, firstTimeFlag;

	counter = firstTimeFlag = 0;
	startIndex = ((uhl  + PAGE_SIZE) - USER_HEAP_START) / PAGE_SIZE;

	while(startIndex < MAX_USER_PAGES){

		if(userPages[startIndex] == 0){

			if(firstTimeFlag == 0){
				firstIndex = startIndex;
				firstTimeFlag = 1;
			}

			counter++;
			if(counter == neededPages)
				break;
		}
		else{
			counter = 0;
			firstTimeFlag = 0;
			startIndex += userPages[startIndex] - 1;
		}

		startIndex++;
	}

	//uint32 startVa = sys_get_alloc_va(size);

	userPages[firstIndex] = neededPages;
	uint32 startVa = (firstIndex * PAGE_SIZE) + USER_HEAP_START;
	sys_allocate_user_mem(startVa, size);


	return (void *)startVa;
}

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void* virtual_address)
{
	uint32 uhl = sys_get_uhl();

	if (virtual_address >= (void*) USER_HEAP_START && virtual_address < (void*)uhl)

		free_block(virtual_address);

	else if(virtual_address >= (void*) (uhl + PAGE_SIZE) && virtual_address < (void*)USER_HEAP_MAX ){

		uint32 numOfPages = userPages[(((uint32) virtual_address) - USER_HEAP_START) / PAGE_SIZE];

		if(numOfPages == 0)

			panic("Invalid Address");

		//sys_get_free_size((uint32)virtual_address);

		sys_free_user_mem((uint32)virtual_address, numOfPages);
		userPages[(((uint32) virtual_address) - USER_HEAP_START) / PAGE_SIZE] = 0;

	}
	else
		panic("Invalid Address");
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

#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

//define the list of processes that will hold each base address and how many pages associated with it


int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	//TODO: [PROJECT'23.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator()


	if(initSizeToAllocate > daLimit)
	{
		return E_NO_MEM;
	}

	kbreak = (initSizeToAllocate % PAGE_SIZE)
		   ?  initSizeToAllocate + PAGE_SIZE - (initSizeToAllocate % PAGE_SIZE)
		   :  initSizeToAllocate ;

	uint32 iteration = kbreak / PAGE_SIZE;

	kstart = daStart;
	kbreak += kstart;
	khl = daLimit;

	struct FrameInfo *ptr_frame_info;

	int mappingDone = 0;
	int ret,fret,mret;
	uint32 pagePtr = kstart;
	uint32 *ptr_page_table = NULL;



	for(int i = 0;i<iteration;i++)
	{

		fret = allocate_frame(&ptr_frame_info);

		if(fret == 0)
		{

			int mret = map_frame(ptr_page_directory, ptr_frame_info, pagePtr, PERM_WRITEABLE);
			if(mret == 0)
			{
				ptr_frame_info->va = pagePtr;
				mappingDone = 1;
			}
			else
			{
				mappingDone = 0;
				break;
			}
		}

		pagePtr += PAGE_SIZE;

	}

	if(mappingDone){

		initialize_dynamic_allocator(kstart, kbreak - kstart);
		return 0;
	}


	return E_NO_MEM;
}

void* sbrk(int increment)
{
	if (increment == 0)
		return (void*)kbreak;

	int numOfPages = increment/PAGE_SIZE;

	if(increment > 0 && (increment + kbreak) < khl)
	{
		uint32 pagePtr,exStart = kbreak = ROUNDUP(kbreak,PAGE_SIZE);
		int fret,mret;

		struct FrameInfo *ptr_frame_info;

		if(increment%PAGE_SIZE != 0)
			numOfPages++;

		kbreak += (numOfPages * PAGE_SIZE);
		pagePtr = exStart;

		for(int i = 0; i < numOfPages; i++){

			fret = allocate_frame(&ptr_frame_info);

			if(fret == 0)
			{
				int mret = map_frame(ptr_page_directory, ptr_frame_info, pagePtr, PERM_WRITEABLE);

				if(mret != 0){
					panic("Unsuccessful Map");
				}

				ptr_frame_info->va = pagePtr;

			}
			pagePtr += PAGE_SIZE;
		}

		return (void*)exStart;

	}

	if(increment < 0)
	{

		//ensure it doesn't go beyond the start downwards
		if((-increment) > (kbreak - kstart))
		{
		    return (void *)-1;
		}


		uint32 exStart = kbreak;
		uint32* ptr_page_table = NULL;

		struct FrameInfo *ptr_frame_info;

		kbreak += increment;
		numOfPages *= -1;

		for(int i = 0;i<numOfPages;i++)
		{
			exStart -= PAGE_SIZE;

			get_page_table(ptr_page_directory, exStart, &ptr_page_table);
			ptr_frame_info = get_frame_info(ptr_page_directory,exStart,&ptr_page_table);

			unmap_frame(ptr_page_directory,exStart);
			free_frame(ptr_frame_info);

		}
		return (void*)kbreak;
	}

	panic("Limit Reached");
}



void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT'23.MS2 - #03] [1] KERNEL HEAP - kmalloc()

	if(size <= 0 || size > DYN_ALLOC_MAX_SIZE)
		return NULL;

	if(size <= DYN_ALLOC_MAX_BLOCK_SIZE)
		return alloc_block_FF(size);


	struct FrameInfo *ptr_frame_info;

	uint32 pagePtr = khl + PAGE_SIZE;
	uint32 startPage,endPage;
	uint32* ptr_page_table = NULL;
	int remainingSize = size;
	int pages = 0;
	int ret,fret,mret;

	startPage = endPage = pagePtr;

	if(isKHeapPlacementStrategyFIRSTFIT()){

		while(remainingSize > 0){

			ret = get_page_table(ptr_page_directory, pagePtr, &ptr_page_table);

			if(ret == TABLE_IN_MEMORY){

				ptr_frame_info = get_frame_info(ptr_page_directory, pagePtr, &ptr_page_table);
				pagePtr += PAGE_SIZE;

				if(ptr_frame_info == 0){

					endPage = pagePtr - PAGE_SIZE;
					remainingSize -= PAGE_SIZE;
					pages++;

				}
				else{

					startPage = endPage = pagePtr;
					remainingSize = size;
					pages = 0;

				}
			}
		}

		uint32 currPage = startPage;

		for(int i = 0; i < pages; i++){

			fret = allocate_frame(&ptr_frame_info);

			if(fret == 0){

				mret = map_frame(ptr_page_directory, ptr_frame_info, currPage, PERM_WRITEABLE);
				ptr_frame_info->va = currPage;
				currPage+= PAGE_SIZE;

			}
		}

		ptr_frame_info = get_frame_info(ptr_page_directory, startPage, &ptr_page_table);
		ptr_frame_info->numOfPages = pages;


		return (void *)startPage;

	}

	return NULL;
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #04] [1] KERNEL HEAP - kfree()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//panic("kfree() is not implemented yet...!!");

	//block allocator area
	if (virtual_address >= (void*) KERNEL_HEAP_START && virtual_address <= (void*)khl ) {
		free_block(virtual_address);
	}

	//page allocator area
	else if (virtual_address >= (void*)khl+PAGE_SIZE && virtual_address <= (void*)KERNEL_HEAP_MAX) {

		struct FrameInfo *ptr_frame_info;
		uint32 pagePtr = khl + PAGE_SIZE;
		uint32* ptr_page_table = NULL;

		int ret = get_page_table(ptr_page_directory, pagePtr, &ptr_page_table);


		if(ret == TABLE_IN_MEMORY){


			ptr_frame_info = get_frame_info(ptr_page_directory, (uint32)virtual_address, &ptr_page_table);
			uint32 pages = ptr_frame_info->numOfPages;


			void* currPageAddr = virtual_address;
			for (int i=0; i < pages; i++) {
				currPageAddr = virtual_address+(i*PAGE_SIZE);

				unmap_frame(ptr_page_directory, (uint32)currPageAddr);
				//free_frame(ptr_frame_info); //freeing is redundant, unmap_frame automatically calls free

			}


		}
	}

	//invalid address
	else {
		panic("Invalid Address passed to kfree()!!");
	}
}



unsigned int kheap_virtual_address(unsigned int physical_address){

	//TODO: [PROJECT'23.MS2 - #05] [1] KERNEL HEAP - kheap_virtual_address()

	struct FrameInfo * ptr ;
	ptr= to_frame_info(physical_address);

	uint32 va = ptr->va;

	if (ptr == NULL || va<KERNEL_HEAP_START || va >KERNEL_HEAP_MAX)

	  return 0;


	uint32 offset = physical_address % PAGE_SIZE;

	return va + offset;

}


unsigned int kheap_physical_address(unsigned int virtual_address){

	uint32 Dir_Entry = ptr_page_directory[PDX(virtual_address)];

	//frame of page table
	int frameNum = Dir_Entry >> 12;

	uint32 *ptr_page_table = NULL;
	int ret = get_page_table(ptr_page_directory, virtual_address, &ptr_page_table);
	if (ret == TABLE_IN_MEMORY)
	{
		uint32 Table_Entry = ptr_page_table [PTX(virtual_address)];
		//frame of page itself
		frameNum = Table_Entry >> 12;
		//calculate physical address within offset
		uint32 physicalAddr=(frameNum*PAGE_SIZE)+(virtual_address%PAGE_SIZE);
		return physicalAddr;

	}
		return 0;
}


void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}




//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'23.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc()
	// Write your code here, remove the panic and write your code

	if (new_size == 0) {
		kfree(virtual_address);
		return NULL;
	}
	if (virtual_address == NULL) {
		return kmalloc(new_size);
	}



	uint32 pagePtr = khl + PAGE_SIZE;
	uint32* ptr_page_table = NULL;
	int ret = get_page_table(ptr_page_directory, pagePtr, &ptr_page_table);
	if(ret != TABLE_IN_MEMORY) return NULL;
	struct FrameInfo *ptr_frame_info;

	//check if after it is free
	ptr_frame_info = get_frame_info(ptr_page_directory, (uint32)virtual_address, &ptr_page_table);

	uint32 oldNumOfPages = ptr_frame_info->numOfPages;
	int newNumOfPages = ROUNDUP(new_size, PAGE_SIZE)/PAGE_SIZE;
	int numDifferences = newNumOfPages - oldNumOfPages;

	uint32 oldLastPage = (uint32)virtual_address + ((oldNumOfPages-1) * PAGE_SIZE);


	//2 cases, if negative then deallocate, if positive then allocate above or free it and recall kmalloc if no space above it


	if (numDifferences >= 0) {


		//is there enough space after it?

		uint32 currPage = oldLastPage;
		for (int i=0; i < numDifferences-1; i++) {
			currPage += PAGE_SIZE;
			ptr_frame_info = get_frame_info(ptr_page_directory, currPage, &ptr_page_table);
			if (currPage >= khl || ptr_frame_info != 0) {
				kfree(virtual_address);
				return kmalloc(new_size);
			}
		}

		currPage = oldLastPage;
		//none of them was taken or didn't hit limit, so allocate the frames


		for (int i=1; i < numDifferences; i++) {
			currPage = oldLastPage + (i*PAGE_SIZE);
			ptr_frame_info = get_frame_info(ptr_page_directory, currPage, &ptr_page_table);


			int fret = allocate_frame(&ptr_frame_info);
			if(fret == 0)
			{
				int mret = map_frame(ptr_page_directory, ptr_frame_info, pagePtr, PERM_WRITEABLE);

				if(mret != 0) {
					panic("Unsuccessful Map");
				}

				ptr_frame_info->va = pagePtr;

			}
		}

	}

	//negative difference between pages (decreasing)
	else {

		uint32 currPage = oldLastPage;

		for (int i = 0; i < (-numDifferences); i++) {
			ptr_frame_info = get_frame_info(ptr_page_directory, currPage, &ptr_page_table);
			unmap_frame(ptr_page_directory, currPage);
			currPage = currPage - PAGE_SIZE;

		}

	}

	ptr_frame_info = get_frame_info(ptr_page_directory, (uint32)virtual_address, &ptr_page_table);
	ptr_frame_info->numOfPages = newNumOfPages;

	return virtual_address;

	//panic("krealloc() is not implemented yet...!!");
}

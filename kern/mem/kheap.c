#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"


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
				mappingDone = 1;
			}else
			{
				mappingDone = 0;
				break;
			}
		}

		pagePtr += PAGE_SIZE;

	}

	if(mappingDone){
		initialize_dynamic_allocator(kstart, kbreak);
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
		uint32 pagePtr,exStart = kbreak;
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
			}
			pagePtr += PAGE_SIZE;
		}

		return (void*)exStart;

	}

	if(increment < 0)
	{
		uint32 exStart = kbreak;
		uint32* ptr_page_table = NULL;

		struct FrameInfo *ptr_frame_info;

		kbreak -= increment;
		numOfPages *= -1;

		if(increment%PAGE_SIZE != 0)
			numOfPages++;

		for(int i = 0;i<numOfPages;i++)
		{
			get_page_table(ptr_page_directory, exStart, &ptr_page_table);
			ptr_frame_info = get_frame_info(ptr_page_directory,exStart,&ptr_page_table);

			unmap_frame(ptr_page_directory,exStart);
			free_frame(ptr_frame_info);

			exStart -= PAGE_SIZE;
		}
		return (void*)kbreak;
	}

	panic("Limit Reached");
	return (void*)-1;
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
	int numOfPages = 0;
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
					numOfPages++;

				}
				else{

					startPage = endPage = pagePtr;
					remainingSize = size;
					numOfPages = 0;

				}
			}
		}

		uint32 iPage = startPage;

		for(int i = 0; i < numOfPages; i++){

			fret = allocate_frame(&ptr_frame_info);

			if(fret == 0){

				mret = map_frame(ptr_page_directory, ptr_frame_info, iPage, PERM_WRITEABLE);
				iPage+= PAGE_SIZE;

			}
		}

		return (void*)startPage;

	}


	if(isKHeapPlacementStrategyBESTFIT()){

		//BONUS

	}

	return NULL;
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #04] [1] KERNEL HEAP - kfree()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	panic("kfree() is not implemented yet...!!");
}

unsigned int kheap_virtual_address(unsigned int physical_address){

	//TODO: [PROJECT'23.MS2 - #05] [1] KERNEL HEAP - kheap_virtual_address()

	struct FrameInfo * ptr ;
	ptr= to_frame_info(physical_address);

	if (ptr == NULL)
	{
	 return 0;
	}

	return ptr->va;

}


unsigned int kheap_physical_address(unsigned int virtual_address){

	//TODO: [PROJECT'23.MS2 - #06] [1] KERNEL HEAP - kheap_physical_address()

	uint32 pdx = PDX(virtual_address);
	uint32 ptx = PTX(virtual_address);

	// Retrieve the page directory entry
	uint32 page_directory_entry = ptr_page_directory[pdx];

	// Check if the page directory entry is present
	if ((page_directory_entry & PERM_PRESENT) != PERM_PRESENT) {
		// No mapping, return 0
		return 0;
	}

	// Get the frame number of the page table from the page directory entry
	uint32 frameOfPageTable = EXTRACT_ADDRESS(page_directory_entry);

	// Retrieve the page table entry
	uint32 *ptr_page_table = (uint32 *) frameOfPageTable;
	uint32 page_table_entry=ptr_page_table[ptx];

	// Check if the page table entry is present
	if ((page_table_entry & PERM_PRESENT) != PERM_PRESENT) {
		// No mapping, return 0
		return 0;
	}

	// Calculate the physical address using the page frame and offset
	uint32 frameOfPage = EXTRACT_ADDRESS(page_table_entry);
	uint32 offset = virtual_address %PAGE_SIZE;

	// Calculate physical address by combining the frame number and offset
	uint32 physical_address = (frameOfPage * PAGE_SIZE) + offset;

	return physical_address;
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
	return NULL;
	panic("krealloc() is not implemented yet...!!");
}

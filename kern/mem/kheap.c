#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"
#include <inc/queue.h>


//define the list of processes that will hold each base address and how many pages associated with it
#define maxProccesses 1024
void *proc_addr[maxProccesses] = {NULL};
int proc_pages[maxProccesses] = {0};


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
		blockBase = kstart;
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

		kbreak += increment;
		numOfPages *= -1;

		if((-increment)%PAGE_SIZE != 0)
			numOfPages++;

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
	return (void*)-1;
}


void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT'23.MS2 - #03] [1] KERNEL HEAP - kmalloc()

	if(size <= 0 || size > DYN_ALLOC_MAX_SIZE)
		return NULL;

	if(size <= DYN_ALLOC_MAX_BLOCK_SIZE) {
		return alloc_block_FF(size);
	}


	struct FrameInfo *ptr_frame_info;

	uint32 pagePtr = khl + PAGE_SIZE;
	uint32 startPage,endPage;
	uint32* ptr_page_table = NULL;
	int remainingSize = size;
	int numOfPages = 0;
	int ret,fret,mret;
	int listSize;

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

		uint32 currPage = startPage;

		for(int i = 0; i < numOfPages; i++){

			fret = allocate_frame(&ptr_frame_info);

			if(fret == 0){

				mret = map_frame(ptr_page_directory, ptr_frame_info, currPage, PERM_WRITEABLE);
				currPage+= PAGE_SIZE;

			}
		}

		for (int i=0; i < maxProccesses; i++) {
			if (proc_addr[i] == NULL) {
				proc_addr[i] = (void*)startPage;
				proc_pages[i] = numOfPages;
				break;
			}
		}


		return (void *)startPage;

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


			uint32 pages = 0;

			int i = 0;
			int numOfPages;
			for (i =0; i < maxProccesses; i++) {
				if (proc_addr[i] == virtual_address) {
					numOfPages = proc_pages[i];
					break;
				}
			}



			void* currPageAddr = virtual_address;
			for (int i=0; i < numOfPages; i++) {
				currPageAddr = virtual_address+(i*PAGE_SIZE);

				unmap_frame(ptr_page_directory, (uint32)currPageAddr);
				//free_frame(ptr_frame_info); //freeing is redundant, unmap_frame automatically calls free

			}

			//set it to NULL so it can be reused
			proc_addr[i] = NULL;

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

	if (ptr == NULL)
	{
	 return 0;
	}

	return ptr->va;

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
	return NULL;
	panic("krealloc() is not implemented yet...!!");
}

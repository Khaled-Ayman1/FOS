/*
 * chunk_operations.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include <kern/trap/fault_handler.h>
#include <kern/disk/pagefile_manager.h>
#include "kheap.h"
#include "memory_manager.h"
#include <inc/queue.h>
#include <kern/tests/utilities.h>

//extern void inctst();

/******************************/
/*[1] RAM CHUNKS MANIPULATION */
/******************************/

//===============================
// 1) CUT-PASTE PAGES IN RAM:
//===============================
//This function should cut-paste the given number of pages from source_va to dest_va on the given page_directory
//	If the page table at any destination page in the range is not exist, it should create it
//	If ANY of the destination pages exists, deny the entire process and return -1. Otherwise, cut-paste the number of pages and return 0
//	ALL 12 permission bits of the destination should be TYPICAL to those of the source
//	The given addresses may be not aligned on 4 KB
int cut_paste_pages(uint32* page_directory, uint32 source_va, uint32 dest_va, uint32 num_of_pages)
{
	panic("cut_paste_pages() is not implemented yet...!!");

	return 0 ;
}

//===============================
// 2) COPY-PASTE RANGE IN RAM:
//===============================
//This function should copy-paste the given size from source_va to dest_va on the given page_directory
//	Ranges DO NOT overlapped.
//	If ANY of the destination pages exists with READ ONLY permission, deny the entire process and return -1.
//	If the page table at any destination page in the range is not exist, it should create it
//	If ANY of the destination pages doesn't exist, create it with the following permissions then copy.
//	Otherwise, just copy!
//		1. WRITABLE permission
//		2. USER/SUPERVISOR permission must be SAME as the one of the source
//	The given range(s) may be not aligned on 4 KB
int copy_paste_chunk(uint32* page_directory, uint32 source_va, uint32 dest_va, uint32 size)
{
	panic("copy_paste_chunk() is not implemented yet...!!");
	return 0;
}

//===============================
// 3) SHARE RANGE IN RAM:
//===============================
//This function should copy-paste the given size from source_va to dest_va on the given page_directory
//	Ranges DO NOT overlapped.
//	It should set the permissions of the second range by the given perms
//	If ANY of the destination pages exists, deny the entire process and return -1. Otherwise, share the required range and return 0
//	If the page table at any destination page in the range is not exist, it should create it
//	The given range(s) may be not aligned on 4 KB
int share_chunk(uint32* page_directory, uint32 source_va,uint32 dest_va, uint32 size, uint32 perms)
{
	panic("share_chunk() is not implemented yet...!!");
	return 0;
}

//===============================
// 4) ALLOCATE CHUNK IN RAM:
//===============================
//This function should allocate the given virtual range [<va>, <va> + <size>) in the given address space  <page_directory> with the given permissions <perms>.
//	If ANY of the destination pages exists, deny the entire process and return -1. Otherwise, allocate the required range and return 0
//	If the page table at any destination page in the range is not exist, it should create it
//	Allocation should be aligned on page boundary. However, the given range may be not aligned.
int allocate_chunk(uint32* page_directory, uint32 va, uint32 size, uint32 perms)
{
	panic("allocate_chunk() is not implemented yet...!!");
	return 0;
}

//=====================================
// 5) CALCULATE ALLOCATED SPACE IN RAM:
//=====================================
void calculate_allocated_space(uint32* page_directory, uint32 sva, uint32 eva, uint32 *num_tables, uint32 *num_pages)
{
	panic("calculate_allocated_space() is not implemented yet...!!");
}


//=====================================
// 6) CALCULATE REQUIRED FRAMES IN RAM:
//=====================================
//This function should calculate the required number of pages for allocating and mapping the given range [start va, start va + size) (either for the pages themselves or for the page tables required for mapping)
//	Pages and/or page tables that are already exist in the range SHOULD NOT be counted.
//	The given range(s) may be not aligned on 4 KB
uint32 calculate_required_frames(uint32* page_directory, uint32 sva, uint32 size)
{
	panic("calculate_required_frames() is not implemented yet...!!");
	return 0;
}

//=================================================================================//
//===========================END RAM CHUNKS MANIPULATION ==========================//
//=================================================================================//

/*******************************/
/*[2] USER CHUNKS MANIPULATION */
/*******************************/

//======================================================
/// functions used for USER HEAP (malloc, free, ...)
//======================================================

//=====================================
// 1) ALLOCATE USER MEMORY:
//=====================================
void allocate_user_mem(struct Env* e, uint32 virtual_address, uint32 size)
{
	uint32 *ptr_page_table = NULL;
	uint32 numOfPages = ROUNDUP(size, PAGE_SIZE)/ PAGE_SIZE;

	uint32 pagePtr = virtual_address;
	int ret;
	for(int i = 0; i < numOfPages; i++){

		ret = get_page_table(e->env_page_directory, pagePtr, &ptr_page_table);

		if(ret == TABLE_NOT_EXIST)
			ptr_page_table = create_page_table(e->env_page_directory, pagePtr);


		pt_set_page_permissions(e->env_page_directory, pagePtr, PERM_WRITEABLE | PERM_USER | PERM_MARKED, 0);

		pagePtr+=PAGE_SIZE;

	}

	//panic("allocate_user_mem() is not implemented yet...!!");
}

//=====================================
// 2) FREE USER MEMORY:
//=====================================
void free_user_mem(struct Env* e, uint32 virtual_address, uint32 size)
{

	uint32 pagePtr = virtual_address;
	uint32 perm;
	uint32 numOfPages = size;



	while(numOfPages > 0){

		perm = pt_get_page_permissions(e->env_page_directory,pagePtr);

		if((perm & PERM_PRESENT) == 0){
			pagePtr += PAGE_SIZE;
			numOfPages--;
			continue;
		}


		//remove it from the fifo list page_WS_list
		if(isPageReplacmentAlgorithmFIFO() && e->page_last_WS_element != NULL) {
			struct WorkingSetElement* element;
			LIST_FOREACH(element, &(e->page_WS_list)) {
				if (ROUNDDOWN(element->virtual_address, PAGE_SIZE) == ROUNDDOWN(pagePtr, PAGE_SIZE))
					break;
			}
			if (element != NULL) {
				//make sure that the element about to be removed wasn't the page_last_WS_element
				//if it was, make the page_last_WS_element the one after it
				//if the one after it was NULL then the adjust piece of code below will still perform as expected
				if (element->virtual_address == e->page_last_WS_element->virtual_address)
					e->page_last_WS_element = LIST_NEXT(element);

				LIST_REMOVE(&(e->page_WS_list), element);
			}
		}


		//important, check that it exists and has a page table before freeing
		//note that it could be in the page_WS_list but doesn't have a page table
		//so removing it from list first then checking is the correct approach
		//discovered in FIFO test "run tffo2 11"
		uint32* ptr_page_table ;
		int ret = get_page_table(e->env_page_directory, pagePtr, &ptr_page_table);
		if (ptr_page_table == NULL) {
			pagePtr += PAGE_SIZE;
			numOfPages--;
			continue;
		}


		pt_set_page_permissions(e->env_page_directory, pagePtr, 0, PERM_WRITEABLE | PERM_MARKED);

		env_page_ws_invalidate(e, pagePtr);

		pf_remove_env_page(e, pagePtr);

		unmap_frame(e->env_page_directory, pagePtr);

		pt_clear_page_table_entry(e->env_page_directory, pagePtr);

		pagePtr += PAGE_SIZE;
		numOfPages--;
	}


	//adjust the page_WS_list in FIFO
	if(isPageReplacmentAlgorithmFIFO() && e->page_last_WS_element != NULL) {

		struct WorkingSetElement* curr_element;
		if (e->page_last_WS_element != NULL) {

			//re-sort the FIFO, make the page_last_WS_element as the head with the others following it
			//example: {a,b,c,d,NULL} if c is the head then the result should be {c,d,a,b,NULL}
			//then c is no longer marked as page_last_WS_element, as the list is not full and so that placement (if any) can place at the tail
			LIST_FOREACH(curr_element, &(e->page_WS_list)) {
				if (curr_element->virtual_address == e->page_last_WS_element->virtual_address)
					break;

				struct WorkingSetElement* new_element = curr_element;
				LIST_REMOVE(&(e->page_WS_list), curr_element);
				LIST_INSERT_TAIL(&(e->page_WS_list), new_element);
			}
		}

		e->page_last_WS_element = NULL;

	}


}

//=====================================
// 2) FREE USER MEMORY (BUFFERING):
//=====================================
void __free_user_mem_with_buffering(struct Env* e, uint32 virtual_address, uint32 size)
{
	// your code is here, remove the panic and write your code
	panic("__free_user_mem_with_buffering() is not implemented yet...!!");
}

//=====================================
// 3) MOVE USER MEMORY:
//=====================================
void move_user_mem(struct Env* e, uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
{
	//your code is here, remove the panic and write your code
	panic("move_user_mem() is not implemented yet...!!");

	// This function should move all pages from "src_virtual_address" to "dst_virtual_address"
	// with the given size
	// After finished, the src_virtual_address must no longer be accessed/exist in either page file
	// or main memory
}

//=================================================================================//
//========================== END USER CHUNKS MANIPULATION =========================//
//=================================================================================//


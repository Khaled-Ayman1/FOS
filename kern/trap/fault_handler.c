/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{

	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//Handle the page fault

void page_fault_handler(struct Env * curenv, uint32 fault_va)
{
	fault_va = ROUNDDOWN(fault_va, PAGE_SIZE);
#if USE_KHEAP
		struct WorkingSetElement *victimWSElement = NULL;
		uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
#else
		int iWS =curenv->page_last_WS_index;
		uint32 wsSize = env_page_ws_get_size(curenv);
#endif


	if(isPageReplacmentAlgorithmFIFO()) {

		//cprintf("REPLACEMENT=========================WS Size = %d\n", wsSize );
		//refer to the project presentation and documentation for details
		//placement from MS2 [heavily modified for FIFO]

		if(wsSize < (curenv->page_WS_max_size))
		{
			//TODO: [PROJECT'23.MS2 - #15] [3] PAGE FAULT HANDLER - Placement

			placement(curenv, fault_va);

		}


		//replacement, MS3
		else {

			//TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement

			struct WorkingSetElement* element;
			int found = 0;
			LIST_FOREACH(element, &curenv->page_WS_list) {
				if (ROUNDDOWN(element->virtual_address, PAGE_SIZE) == ROUNDDOWN(fault_va, PAGE_SIZE)) {
					found = 1;
					break;
				}
			}

			if (!found) {

				//remove victim, unmap it and remove from list and write it to disk
				uint32 *ptr_page_table = NULL;
				struct WorkingSetElement* victim = curenv->page_last_WS_element;
				uint32 victim_perm = pt_get_page_permissions(curenv->env_page_directory, ROUNDDOWN(victim->virtual_address, PAGE_SIZE));
				struct FrameInfo* victim_frame = get_frame_info(curenv->env_page_directory, ROUNDDOWN(victim->virtual_address, PAGE_SIZE), &ptr_page_table);

				//if modified, write to disk
				if ((victim_perm & PERM_MODIFIED) == PERM_MODIFIED) {
					int ret = pf_update_env_page(curenv, ROUNDDOWN(victim->virtual_address, PAGE_SIZE), victim_frame);
					if (ret == E_NO_PAGE_FILE_SPACE)
						panic("ERROR: No enough virtual space on the page file");
				}

				if (curenv->page_last_WS_element == victim)
				{
					curenv->page_last_WS_element = LIST_NEXT(victim);
				}

				//unmap victim and remove it from list
				unmap_frame(curenv->env_page_directory, ROUNDDOWN(victim->virtual_address,PAGE_SIZE));

				LIST_REMOVE(&(curenv->page_WS_list), victim);


				placement(curenv, fault_va);


			}

		}
	}


	if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX))
	{
		cprintf("\n in lru\n");
		//TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER - LRU Replacement
		// Write your code here, remove the panic and write your code
		//panic("page_fault_handler() LRU Replacement is not implemented yet...!!");
		uint32 ALSize = LIST_SIZE(&(curenv->ActiveList));
		uint32 SLSize = LIST_SIZE(&(curenv->SecondList));
		struct FrameInfo* new_frame;
		struct FrameInfo* checked_frame;
		struct FrameInfo* pushed_frame;
		uint32* ptr_page_table1 = NULL;
		cprintf("\n before placement\n");
		env_page_ws_print(curenv);
		cprintf("\n ALSize=%d\n",ALSize);
		cprintf("\n SLSize=%d\n",SLSize);
		cprintf("\n (curenv->page_WS_max_size)=%d\n",(curenv->page_WS_max_size));
		cprintf("\n fault_va=%x\n",fault_va);

		if((ALSize + SLSize) < (curenv->page_WS_max_size))//one or both of the lists are not full
		{
			//TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER – LRU Placement

			if(ALSize < (curenv->ActiveListSize))
			{
				//active list empty, add element to it
				//check if element is in active list
				struct WorkingSetElement* element_to_find;
				struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv,fault_va);
				int flag = 0;
				checked_frame = get_frame_info(curenv->env_page_directory,fault_va,&ptr_page_table1);
				if(checked_frame != NULL){
					if(checked_frame->element->currentList == 0){
						flag = 1;
					}
				}
				//element already in list
				if(flag)
				{
					//remove element from the place its is already in then add it at the head
					LIST_REMOVE(&(curenv->ActiveList),checked_frame->element);
					checked_frame->element = new_element;
					checked_frame->element->currentList = 0;
					LIST_INSERT_HEAD(&(curenv->ActiveList),checked_frame->element);

				}else
				{
					allocate_frame(&new_frame);
					map_frame(curenv->env_page_directory,new_frame,fault_va,PERM_WRITEABLE | PERM_USER);
					new_frame->va = fault_va;
					new_frame->element = new_element;
					new_frame->element->currentList = 0;
					LIST_INSERT_HEAD(&(curenv->ActiveList),new_frame->element);
					uint32 page = pf_read_env_page(curenv, (void*) fault_va);
					if(page == E_PAGE_NOT_EXIST_IN_PF && !(((fault_va <= USTACKTOP && fault_va >= USTACKBOTTOM) ||
							 (fault_va <= USER_HEAP_MAX && fault_va >= USER_HEAP_START))))
					{
						cprintf("\n wrong kill 1\n");
						sched_kill_env(curenv->env_id);
					}

				}

			}else
			{
				//active list is full,add to second list, mark it as NOT PRESENT in second list
				//check if element is in second list
				struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv,fault_va);
				struct WorkingSetElement* pushed_element = LIST_LAST(&(curenv->ActiveList));
				int flag = 0;
				checked_frame = get_frame_info(curenv->env_page_directory,fault_va,&ptr_page_table1);
				if(checked_frame != NULL){
					if(checked_frame->element->currentList == 1){
						flag = 1;
					}
				}
				//element already in list
				if(flag)
				{
					//remove element from the place its is already in then add it at the head
					//remove found element from second list
					LIST_REMOVE(&(curenv->SecondList),checked_frame->element);

					//remove last element from active list
					LIST_REMOVE(&(curenv->ActiveList), pushed_element);

					//insert last element from active list into second list
					pushed_frame = get_frame_info(curenv->env_page_directory,pushed_element->virtual_address,&ptr_page_table1);
					pushed_frame->element = pushed_element;
					pushed_frame->element->currentList = 1;
					LIST_INSERT_HEAD(&(curenv->SecondList),pushed_frame->element);
					pt_set_page_permissions(curenv->env_page_directory,pushed_element->virtual_address,0,PERM_PRESENT);

					//insert found element into active list
					checked_frame->element = new_element;
					checked_frame->element->currentList = 0;
					LIST_INSERT_HEAD(&(curenv->ActiveList),checked_frame->element);
					pt_set_page_permissions(curenv->env_page_directory,fault_va,PERM_PRESENT,0);
				}
				else //new element
				{
					//remove last element in active list
					LIST_REMOVE(&(curenv->ActiveList), pushed_element);

					//insert the last element in active list in second list
					pushed_frame = get_frame_info(curenv->env_page_directory,pushed_element->virtual_address,&ptr_page_table1);
					pushed_frame->element = pushed_element;
					pushed_frame->element->currentList = 1;
					LIST_INSERT_HEAD(&(curenv->SecondList),pushed_frame->element);
					pt_set_page_permissions(curenv->env_page_directory,pushed_element->virtual_address,0,PERM_PRESENT);

					//insert new element in active list
					allocate_frame(&new_frame);
					map_frame(curenv->env_page_directory,new_frame,fault_va,PERM_WRITEABLE | PERM_USER);
					new_frame->va = fault_va;
					new_frame->element = new_element;
					new_frame->element->currentList = 0;
					LIST_INSERT_HEAD(&(curenv->ActiveList),new_frame->element);
					uint32 page = pf_read_env_page(curenv, (void*) fault_va);
					if(page == E_PAGE_NOT_EXIST_IN_PF && !(((fault_va <= USTACKTOP && fault_va >= USTACKBOTTOM) ||
							 (fault_va <= USER_HEAP_MAX && fault_va >= USER_HEAP_START))))
					{
						cprintf("\n wrong kill 2\n");
						sched_kill_env(curenv->env_id);
					}
				}


			}


		}
		else //both lists are full
		{

			cprintf("\n replacement\n");
			cprintf("\n fault_va=%x\n",fault_va);
			//TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - LRU Replacement
			struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv,fault_va);
			struct WorkingSetElement* pushed_element = LIST_LAST(&(curenv->ActiveList));
			int flagAL = 0;
			int flagSL = 0;

			//check if element in active list
			cprintf("\n is in active list? \n");
			checked_frame = get_frame_info(curenv->env_page_directory,fault_va,&ptr_page_table1);
			if(checked_frame != NULL){
				cprintf("\n right frame\n");
				/*
				 * problem here frame element = 0
				 * for some reason element is not added to frame
				 */
				cprintf("\n frame element=%x\n",checked_frame->element);
				cprintf("\n currentList=%d\n",checked_frame->element->currentList);
				if(checked_frame->element->currentList == 0){
					cprintf("\n right list\n");
					flagAL = 1;
				}
			}
			cprintf("\n outside if\n");
			//element already in list
			if(flagAL)
			{
				cprintf("\n found element in active list\n");
				//remove element from the place its is already in then add it at the head
				LIST_REMOVE(&(curenv->ActiveList),checked_frame->element);
				checked_frame->element = new_element;
				checked_frame->element->currentList = 0;
				LIST_INSERT_HEAD(&(curenv->ActiveList),checked_frame->element);

			}

			//check if element in second list
			if(flagAL == 0)
			{
				cprintf("\n is element in second list? \n");
				checked_frame = get_frame_info(curenv->env_page_directory,fault_va,&ptr_page_table1);
				if(checked_frame != NULL){
					cprintf("\n right frame\n");
					if(checked_frame->element->currentList == 1){
						cprintf("\n right list\n");
						flagSL = 1;
					}
				}
				//element already in list
				if(flagSL)
				{
					cprintf("\n element already exists\n");
					//remove element from the place its is already in then add it at the head
					//remove found element from second list
					LIST_REMOVE(&(curenv->SecondList),checked_frame->element);

					//remove last element from active list
					LIST_REMOVE(&(curenv->ActiveList), pushed_element);

					//insert last element from active list into second list
					pushed_frame = get_frame_info(curenv->env_page_directory,pushed_element->virtual_address,&ptr_page_table1);
					pushed_frame->element = pushed_element;
					pushed_frame->element->currentList = 1;
					LIST_INSERT_HEAD(&(curenv->SecondList),pushed_frame->element);
					pt_set_page_permissions(curenv->env_page_directory,pushed_element->virtual_address,0,PERM_PRESENT);

					//insert found element into active list
					checked_frame->element = new_element;
					checked_frame->element->currentList = 0;
					LIST_INSERT_HEAD(&(curenv->ActiveList),checked_frame->element);
					pt_set_page_permissions(curenv->env_page_directory,fault_va,PERM_PRESENT,0);
				}
				else //new element added
				{
					cprintf("\n new element\n");
					//have to eliminate last element in second list and do the replacement
					struct WorkingSetElement* element_to_eliminate = LIST_LAST(&(curenv->SecondList));
					uint32 perm = pt_get_page_permissions(curenv->env_page_directory,element_to_eliminate->virtual_address);
					if(perm & PERM_MODIFIED){

						uint32* ptr_page_table = NULL;
						struct FrameInfo* ptr_frame_info = get_frame_info(curenv->env_page_directory,element_to_eliminate->virtual_address,&ptr_page_table);
						pf_update_env_page(curenv,element_to_eliminate->virtual_address,ptr_frame_info);

					}
					LIST_REMOVE(&(curenv->SecondList),element_to_eliminate);
					unmap_frame(curenv->env_page_directory,element_to_eliminate->virtual_address);
					//env_page_ws_invalidate(curenv,element_to_eliminate->virtual_address);

					//remove last element in active list
					LIST_REMOVE(&(curenv->ActiveList),pushed_element);

					//add last element in active list into second list
					pushed_frame = get_frame_info(curenv->env_page_directory,pushed_element->virtual_address,&ptr_page_table1);
					pushed_frame->element = pushed_element;
					pushed_frame->element->currentList = 1;
					LIST_INSERT_HEAD(&(curenv->SecondList),pushed_frame->element);
					pt_set_page_permissions(curenv->env_page_directory,pushed_element->virtual_address,0,PERM_PRESENT);

					//add new element into active list
					allocate_frame(&new_frame);
					map_frame(curenv->env_page_directory,new_frame,fault_va,PERM_WRITEABLE | PERM_USER);
					new_frame->va = fault_va;
					new_frame->element = new_element;
					new_frame->element->currentList = 0;
					LIST_INSERT_HEAD(&(curenv->ActiveList),new_frame->element);
					uint32 page = pf_read_env_page(curenv, (void*) fault_va);
					if(page == E_PAGE_NOT_EXIST_IN_PF && !(((fault_va <= USTACKTOP && fault_va >= USTACKBOTTOM) ||
							 (fault_va <= USER_HEAP_MAX && fault_va >= USER_HEAP_START))))
					{
						cprintf("\n wrong kill 3\n");
						sched_kill_env(curenv->env_id);
					}


				}

			}

		}


	}
		cprintf("\n after placement\n");
		env_page_ws_print(curenv);

		//TODO: [PROJECT'23.MS3 - BONUS] [1] PAGE FAULT HANDLER - O(1) implementation of LRU replacement
}


void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}



void placement(struct Env * curenv, uint32 fault_va) {


	struct WorkingSetElement *victimWSElement = NULL;
	uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));

	//cprintf("PLACEMENT=========================WS Size = %d\n", wsSize );


	struct WorkingSetElement* element;
	int found = 0;
	LIST_FOREACH(element, &curenv->page_WS_list) {
		if (ROUNDDOWN(element->virtual_address, PAGE_SIZE) == ROUNDDOWN(fault_va, PAGE_SIZE)) {
			found = 1;
			break;
		}
	}

	//if not found then insert it, don't put repititions because this is FIFO
	if (!found) {

		//allocate the new one
		struct FrameInfo *newFrame;
		uint32 fret  = allocate_frame(&newFrame);
		map_frame(curenv->env_page_directory, newFrame, fault_va, PERM_WRITEABLE | PERM_USER);
		newFrame->va = fault_va;

		uint32 page_from_disk = pf_read_env_page(curenv, (void*)fault_va);
		//if it is not found in disk, then make sure it exists within user stack and heap
		 if(page_from_disk == E_PAGE_NOT_EXIST_IN_PF) {
			 if (!(((fault_va <= USTACKTOP && fault_va >= USTACKBOTTOM) ||
				 (fault_va <= USER_HEAP_MAX && fault_va >= USER_HEAP_START)))) {
					sched_kill_env(curenv->env_id);
			 }
		}


		//new element to insert to list
		struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv,fault_va);


		if (curenv->page_last_WS_element == NULL) {
			LIST_INSERT_TAIL(&(curenv->page_WS_list), new_element);

			//after insertion, update last elem
			//maybe list became full now and maybe not, if not full leave page_last_WS_element as NULL
			//if it just became full (wasn't at replacement before this) then make page_last_WS_element point back to first element
			if (LIST_SIZE(&(curenv->page_WS_list)) == (curenv->page_WS_max_size)) {
				curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
			}

		}

		//coming from replacement because last isn't null
		//replacement already updates last element so no need to do it
		else {
			LIST_INSERT_BEFORE(&(curenv->page_WS_list), curenv->page_last_WS_element, new_element);
		}


	}


}



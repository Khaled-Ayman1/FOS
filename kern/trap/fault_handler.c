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
#if USE_KHEAP
		struct WorkingSetElement *victimWSElement = NULL;
		uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
#else
		int iWS =curenv->page_last_WS_index;
		uint32 wsSize = env_page_ws_get_size(curenv);
#endif

		if(wsSize < (curenv->page_WS_max_size))
		{
			//cprintf("PLACEMENT=========================WS Size = %d\n", wsSize );
			//TODO: [PROJECT'23.MS2 - #15] [3] PAGE FAULT HANDLER - Placement
			// Write your code here, remove the panic and write your code
			//panic("page_fault_handler().PLACEMENT is not implemented yet...!!");
			 struct FrameInfo *f;
			 uint32 faulted_page  = allocate_frame(&f);
			 uint32 v_add = ROUNDDOWN(fault_va, PAGE_SIZE);
			 uint32 perm = pt_get_page_permissions(curenv->env_page_directory, fault_va);
			 uint32 page = pf_read_env_page(curenv, (void*) fault_va);
			 if(page != E_PAGE_NOT_EXIST_IN_PF)
			 {
				 map_frame(curenv->env_page_directory, f, v_add, PERM_WRITEABLE | PERM_USER);
				 f->va = v_add;
			 }
			 else{
				 if (((fault_va <= USTACKTOP && fault_va >= USTACKBOTTOM) ||
					 (fault_va <= USER_HEAP_MAX && fault_va >= USER_HEAP_START)))
				 {
						map_frame(curenv->env_page_directory, f, v_add, PERM_WRITEABLE | PERM_USER);
						f->va = v_add;

				 }else{
					sched_kill_env(curenv->env_id);
				 }

			 }
			  struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv,fault_va);
				if (new_element != NULL) {
					if (curenv->page_last_WS_element == NULL) {
						curenv->page_last_WS_element= new_element;
					} else {
						curenv->page_last_WS_element->prev_next_info.le_next = new_element;
						curenv->page_last_WS_element = new_element;
					}
				}
				LIST_INSERT_TAIL(&(curenv->page_WS_list), new_element);
				if (LIST_SIZE(&(curenv->page_WS_list)) == curenv->page_WS_max_size)
				{
					curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
				}
				else
				{
					curenv->page_last_WS_element = NULL;
				}
		}
	else
	{
		//cprintf("REPLACEMENT=========================WS Size = %d\n", wsSize );
		//refer to the project presentation and documentation for details
		if(isPageReplacmentAlgorithmFIFO())
		{
			//TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement
			// Write your code here, remove the panic and write your code
			panic("page_fault_handler() FIFO Replacement is not implemented yet...!!");
		}
		if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX))
		{
			//TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER - LRU Replacement
			// Write your code here, remove the panic and write your code
			//panic("page_fault_handler() LRU Replacement is not implemented yet...!!");
			uint32 ALSize = LIST_SIZE(&(curenv->ActiveList));
			uint32 SLSize = LIST_SIZE(&(curenv->ActiveList));
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
					LIST_FOREACH(element_to_find,&(curenv->ActiveList))
					{
						if(element_to_find == new_element)
						{
							flag = 1;
							break;
						}
					}
					//element already in list
					if(flag)
					{
						//remove element from the place its is already in then add it at the head
						if(ALSize > 1)
						{
							LIST_REMOVE(&(curenv->ActiveList),new_element);
						}
					}
					LIST_INSERT_HEAD(&(curenv->ActiveList),new_element);

				}else
				{
					//active list is full,add to second list, mark it as NOT PRESENT in second list
					//check if element is in second list
					struct WorkingSetElement* element_to_find;
					struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv,fault_va);
					struct WorkingSetElement* pushed_element = LIST_LAST(&(curenv->ActiveList));
					int flag = 0;
					LIST_FOREACH(element_to_find,&(curenv->SecondList))
					{
						if(element_to_find == new_element)
						{
							flag = 1;
							break;
						}
					}
					//element already in list
					if(flag)
					{
						//remove element from the place its is already in then add it at the head
						//remove found element from second list
						LIST_REMOVE(&(curenv->SecondList),new_element);

						//remove last element from active list
						LIST_REMOVE(&(curenv->ActiveList), pushed_element);

						//insert last element from active list into second list
						LIST_INSERT_HEAD(&(curenv->SecondList),pushed_element);
						pt_set_page_permissions(curenv->env_page_directory,pushed_element->virtual_address,0,PERM_PRESENT);

						//insert found element into active list
						LIST_INSERT_HEAD(&(curenv->ActiveList),new_element);
						pt_set_page_permissions(curenv->env_page_directory,fault_va,PERM_PRESENT,0);
					}
					else //new element
					{
						//remove last element in active list
						LIST_REMOVE(&(curenv->ActiveList), pushed_element);

						//insert the last element in active list in second list
						LIST_INSERT_HEAD(&(curenv->SecondList),pushed_element);
						pt_set_page_permissions(curenv->env_page_directory,pushed_element->virtual_address,0,PERM_PRESENT);

						//insert new element in active list
						LIST_INSERT_HEAD(&(curenv->ActiveList),new_element);
					}


				}

			}
			else //both lists are full
			{
				//TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - LRU Replacement
				struct WorkingSetElement* element_to_find;
				struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv,fault_va);
				struct WorkingSetElement* pushed_element = LIST_LAST(&(curenv->ActiveList));
				int flagAL = 0;
				int flagSL = 0;

				//check if element in active list
				LIST_FOREACH(element_to_find,&(curenv->ActiveList))
				{
					if(element_to_find == new_element)
					{
						flagAL = 1;
						break;
					}
				}
				//element already in list
				if(flagAL)
				{
					//remove element from the place its is already in then add it at the head
					if(ALSize > 1)
					{
						LIST_REMOVE(&(curenv->ActiveList),new_element);
						LIST_INSERT_HEAD(&(curenv->ActiveList),new_element);
					}
				}

				//check if element in second list
				if(flagAL == 0)
				{
					LIST_FOREACH(element_to_find,&(curenv->SecondList))
					{
						if(element_to_find == new_element)
						{
							flagSL = 1;
							break;
						}
					}
					//element already in list
					if(flagSL)
					{
						//remove element from the place its is already in then add it at the head
						//remove found element from second list
						LIST_REMOVE(&(curenv->SecondList),new_element);

						//remove last element from active list
						LIST_REMOVE(&(curenv->ActiveList), pushed_element);

						//insert last element from active list into second list
						LIST_INSERT_HEAD(&(curenv->SecondList),pushed_element);
						pt_set_page_permissions(curenv->env_page_directory,pushed_element->virtual_address,0,PERM_PRESENT);

						//insert found element into active list
						LIST_INSERT_HEAD(&(curenv->ActiveList),new_element);
						pt_set_page_permissions(curenv->env_page_directory,fault_va,PERM_PRESENT,0);
					}
					else //new element added
					{
						//have to eliminate last element in second list and do the replacement
						struct WorkingSetElement* element_to_eliminate = LIST_LAST(&(curenv->SecondList));
						uint32 perm = pt_get_page_permissions(curenv->env_page_directory,pushed_element->virtual_address);
						if(perm & PERM_MODIFIED){
							/*
							 * disk writing missing
							 *
							 */
							uint32* ptr_page_table = NULL;
							struct FrameInfo* ptr_frame_info = get_frame_info(curenv->env_page_directory,pushed_element->virtual_address,&ptr_page_table);
							pf_update_env_page(curenv,pushed_element->virtual_address,ptr_frame_info);
						}
						LIST_REMOVE(&(curenv->SecondList),element_to_eliminate);

						//add last element in active list into second list
						LIST_INSERT_HEAD(&(curenv->SecondList),pushed_element);
						pt_set_page_permissions(curenv->env_page_directory,pushed_element->virtual_address,0,PERM_PRESENT);

						//add new element into active list
						LIST_INSERT_HEAD(&(curenv->ActiveList),new_element);
					}

				}


			}

			//TODO: [PROJECT'23.MS3 - BONUS] [1] PAGE FAULT HANDLER - O(1) implementation of LRU replacement
		}
	}
}

void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}




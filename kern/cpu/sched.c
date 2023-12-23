#include "sched.h"

#include <inc/assert.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/trap.h>
#include <kern/mem/kheap.h>
#include <kern/mem/memory_manager.h>
#include <kern/tests/utilities.h>
#include <kern/cmd/command_prompt.h>
#include <inc/queue.h>

uint32 isSchedMethodRR(){if(scheduler_method == SCH_RR) return 1; return 0;}
uint32 isSchedMethodMLFQ(){if(scheduler_method == SCH_MLFQ) return 1; return 0;}
uint32 isSchedMethodBSD(){if(scheduler_method == SCH_BSD) return 1; return 0;}

//===================================================================================//
//============================ SCHEDULER FUNCTIONS ==================================//
//===================================================================================//

//===================================
// [1] Default Scheduler Initializer:
//===================================
void sched_init()
{
	old_pf_counter = 0;

	sched_init_RR(INIT_QUANTUM_IN_MS);

	init_queue(&env_new_queue);
	init_queue(&env_exit_queue);
	scheduler_status = SCH_STOPPED;
}

//=========================
// [2] Main FOS Scheduler:
//=========================
void
fos_scheduler(void)
{
	//	cprintf("inside scheduler\n");

	chk1();
	scheduler_status = SCH_STARTED;

	//This variable should be set to the next environment to be run (if any)
	struct Env* next_env = NULL;

	if (scheduler_method == SCH_RR)
	{
		// Implement simple round-robin scheduling.
		// Pick next environment from the ready queue,
		// and switch to such environment if found.
		// It's OK to choose the previously running env if no other env
		// is runnable.

		//If the curenv is still exist, then insert it again in the ready queue
		if (curenv != NULL)
		{
			enqueue(&(env_ready_queues[0]), curenv);
		}

		//Pick the next environment from the ready queue
		next_env = dequeue(&(env_ready_queues[0]));

		//Reset the quantum
		//2017: Reset the value of CNT0 for the next clock interval
		kclock_set_quantum(quantums[0]);
		//uint16 cnt0 = kclock_read_cnt0_latch() ;
		//cprintf("CLOCK INTERRUPT AFTER RESET: Counter0 Value = %d\n", cnt0 );

	}
	else if (scheduler_method == SCH_MLFQ)
	{
		next_env = fos_scheduler_MLFQ();
	}
	else if (scheduler_method == SCH_BSD)
	{
		next_env = fos_scheduler_BSD();
	}
	//temporarily set the curenv by the next env JUST for checking the scheduler
	//Then: reset it again
	struct Env* old_curenv = curenv;
	curenv = next_env ;
	chk2(next_env) ;
	curenv = old_curenv;

	//sched_print_all();

	if(next_env != NULL)
	{
		//		cprintf("\nScheduler select program '%s' [%d]... counter = %d\n", next_env->prog_name, next_env->env_id, kclock_read_cnt0());
		//		cprintf("Q0 = %d, Q1 = %d, Q2 = %d, Q3 = %d\n", queue_size(&(env_ready_queues[0])), queue_size(&(env_ready_queues[1])), queue_size(&(env_ready_queues[2])), queue_size(&(env_ready_queues[3])));
		env_run(next_env);
	}
	else
	{
		/*2015*///No more envs... curenv doesn't exist any more! return back to command prompt
		curenv = NULL;
		//lcr3(K_PHYSICAL_ADDRESS(ptr_page_directory));
		lcr3(phys_page_directory);

		//cprintf("SP = %x\n", read_esp());

		scheduler_status = SCH_STOPPED;
		//cprintf("[sched] no envs - nothing more to do!\n");
		while (1)
			run_command_prompt(NULL);

	}
}

//=============================
// [3] Initialize RR Scheduler:
//=============================
void sched_init_RR(uint8 quantum)
{

	// Create 1 ready queue for the RR
	num_of_ready_queues = 1;
#if USE_KHEAP
	sched_delete_ready_queues();
	env_ready_queues = kmalloc(sizeof(struct Env_Queue));
	quantums = kmalloc(num_of_ready_queues * sizeof(uint8)) ;
#endif
	quantums[0] = quantum;
	kclock_set_quantum(quantums[0]);
	init_queue(&(env_ready_queues[0]));

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_RR;
	//=========================================
	//=========================================
}

//===============================
// [4] Initialize MLFQ Scheduler:
//===============================
void sched_init_MLFQ(uint8 numOfLevels, uint8 *quantumOfEachLevel)
{
#if USE_KHEAP
	//=========================================
	//DON'T CHANGE THESE LINES=================
	sched_delete_ready_queues();
	//=========================================
	//=========================================

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_MLFQ;
	//=========================================
	//=========================================
#endif
}

//===============================
// [5] Initialize BSD Scheduler:
//===============================
void sched_init_BSD(uint8 numOfLevels, uint8 quantum)
{
#if USE_KHEAP

	//TODO: [PROJECT'23.MS3 - #4] [2] BSD SCHEDULER - sched_init_BSD

	sched_delete_ready_queues();

	num_of_ready_queues = numOfLevels;

	load_avg = fix_int(0);
	ticks = 0;

	quantums = kmalloc(sizeof(uint8));
	env_ready_queues = kmalloc(sizeof(struct Env_Queue) * num_of_ready_queues);

	quantums[0] = quantum;

	for(int i = 0; i < num_of_ready_queues; i++){

		init_queue(&env_ready_queues[i]);
	}
	for(int i = 0; i < 1000; i++){
		removed_envs[i] = NULL;
	}
	kclock_set_quantum(quantums[0]);

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_BSD;
	//=========================================
	//=========================================
#endif
}


//=========================
// [6] MLFQ Scheduler:
//=========================
struct Env* fos_scheduler_MLFQ()
{
	panic("not implemented");
	return NULL;
}

//=========================
// [7] BSD Scheduler:
//=========================
struct Env* fos_scheduler_BSD()
{
	//TODO: [PROJECT'23.MS3 - #5] [2] BSD SCHEDULER - fos_scheduler_BSD

	if(curenv != NULL){

		fixed_point_t  div = fix_frac((PRI_MAX + 1), num_of_ready_queues);
		uint32 index = (PRI_MAX - curenv->priority) / fix_round(div);

		//cprintf("\nRunning Env Stopped, Will Place at Q#%d\n", index);

		enqueue(&env_ready_queues[index], curenv);
		curenv->env_status = ENV_READY;
	}

	for(int i = 0; i < num_of_ready_queues; i++){

		if(queue_size(&env_ready_queues[i]) != 0){

			//cprintf("\n\nScheduling from READY Q#%d\n", i);
			//cprintf("\n---------------------------------------------------------------------------------\n");

			kclock_set_quantum(quantums[0]);

			return dequeue(&env_ready_queues[i]);

		}
	}

	load_avg = fix_int(0);
	return NULL;
}

//========================================
// [8] Clock Interrupt Handler
//	  (Automatically Called Every Quantum)
//========================================
void clock_interrupt_handler()
{
	//TODO: [PROJECT'23.MS3 - #5] [2] BSD SCHEDULER - Your code is here

	//Running Environment will increment recent cpu
	num_of_ready_processes = 0;
	if(curenv != NULL){

		curenv->recent_cpu = fix_add(curenv->recent_cpu, fix_int(1));

		//cprintf("\n---------INCREMENTING RUNNING RECENT ENV [%d] = %d---------\n",curenv->env_id, env_get_recent_cpu(curenv));

		num_of_ready_processes++;

	}

	struct Env *ptr_env = NULL;
	if (((timer_ticks() * quantums[0]) % 1000) <= (quantums[0] - 1) && timer_ticks() > 0){

		//cprintf("\n----------------------------------SECOND PASSED----------------------------------\n");

		for(int i = 0; i < num_of_ready_queues; i++)
			num_of_ready_processes += queue_size(&env_ready_queues[i]);

		//cprintf("\nProcesses:%d\n", num_of_ready_processes);

		fixed_point_t fraction1 = fix_frac(59,60);
		fixed_point_t fraction2 = fix_frac(1,60);
		fixed_point_t old_load_avg = load_avg;
		fixed_point_t dec_ready_processes = fix_int(num_of_ready_processes);

		fixed_point_t div_1 = fix_mul(fraction1, old_load_avg);
		fixed_point_t div_2 = fix_mul(fraction2, dec_ready_processes);

		load_avg = fix_add(div_1, div_2);

		//cprintf("\nLoad Avg: %d\n", get_load_average());

		fixed_point_t coeff_recent, dec_nice;

		fixed_point_t mult_load = fix_scale(load_avg, 2);
		fixed_point_t mult_add_load = fix_add(mult_load, fix_int(1));
		fixed_point_t coefficient = fix_div(mult_load, mult_add_load);

		int j;
		uint8 new_queue_flag = 1;
		struct Env_Queue *queue_type;

		for(int i = 0; i <= num_of_ready_queues; i++){

			if(new_queue_flag){
				queue_type = &env_new_queue;
				new_queue_flag = 0;
				j = i;
			}
			else{
				queue_type = env_ready_queues;
				j = i - 1;
			}
			if(queue_size(&queue_type[j]) == 0)
				continue;

			LIST_FOREACH(ptr_env, &queue_type[j]){

				coeff_recent = fix_mul(coefficient, ptr_env->recent_cpu);
				dec_nice = fix_int(ptr_env->nice);
				ptr_env->recent_cpu = fix_add(coeff_recent, dec_nice);

				//cprintf("\nEnv: %d\nRecent Cpu:%d", ptr_env->env_id, env_get_recent_cpu(ptr_env));
			}
		}

		coeff_recent = fix_mul(coefficient, curenv->recent_cpu);
		dec_nice = fix_int(curenv->nice);
		curenv->recent_cpu = fix_add(coeff_recent, dec_nice);

		//cprintf("\nUpdating Running Env: %d\nRecent Cpu:%d\n", curenv->env_id, env_get_recent_cpu(curenv));

		//cprintf("\n---------------------------------------------------------------------------------\n");
	}

	if((timer_ticks() + 1) % 4 == 0){

		//cprintf("\n----------------------------\tFOURTH TICK\t----------------------------\n");

		fixed_point_t div, dec_pri, dec_nice, calc_pri;
		uint32 trunc_priority;

		uint8 new_queue_flag = 1;
		int j;

		struct Env_Queue *queue_type;

		int index = 0;
		for(int i = 0; i <= num_of_ready_queues; i++){

			if(new_queue_flag){

				queue_type = &env_new_queue;
				new_queue_flag = 0;
				j = i;
			}
			else{
				queue_type = env_ready_queues;
				j = i - 1;
			}

			if(queue_size(&queue_type[j]) == 0)
				continue;

			LIST_REVERSE(ptr_env, &queue_type[j]){

				dec_pri = fix_int(PRI_MAX);
				div = fix_unscale(ptr_env->recent_cpu, 4);
				calc_pri = fix_sub(dec_pri, div);
				dec_nice = fix_int(ptr_env->nice * 2);
				calc_pri = fix_sub(calc_pri, dec_nice);

				trunc_priority = fix_trunc(calc_pri);

				if(trunc_priority > PRI_MAX)
					ptr_env->priority = PRI_MAX;

				else if(trunc_priority < PRI_MIN)
					ptr_env->priority = PRI_MIN;

				else
					ptr_env->priority = trunc_priority;


				//cprintf("Env-> %d\nPriority: %d\n", ptr_env->env_id, ptr_env->priority);

				//Remove from ready queue for enqueue
				if(queue_type == env_ready_queues){

					removed_envs[index] = ptr_env;

					LIST_REMOVE(&(queue_type[j]), ptr_env);

					index++;
				}
			}
		}

		//Renqueue
		for(int i = 0; i < 1000; i++){

			if(removed_envs[i] == NULL)
				continue;

			//cprintf("\nRemoved Array at INDEX %d --- Env ID: %d\n", i, removed_envs[i]->env_id);

			div = fix_frac((PRI_MAX + 1), num_of_ready_queues);
			uint32 assignIndex = (PRI_MAX - removed_envs[i]->priority) / fix_round(div);

			//cprintf("div = %d\npriority: %d\nWill be reassigned at %d", fix_round(div), removed_envs[i]->priority, assignIndex);

			enqueue(&env_ready_queues[assignIndex], removed_envs[i]);
			removed_envs[i]->env_status = ENV_READY;

			removed_envs[i] = NULL;

		}

		//Updating Running Env

		dec_pri = fix_int(PRI_MAX);
		div = fix_unscale(curenv->recent_cpu, 4);
		calc_pri = fix_sub(dec_pri, div);
		dec_nice = fix_int(curenv->nice * 2);
		calc_pri = fix_sub(calc_pri, dec_nice);

		trunc_priority = fix_trunc(calc_pri);

		if(trunc_priority > PRI_MAX)
			curenv->priority = PRI_MAX;

		else if(trunc_priority < PRI_MIN)
			curenv->priority = PRI_MIN;

		else
			curenv->priority = trunc_priority;

		//cprintf("\nRunning Env-> %d --- New Priority: %d\n\n", curenv->env_id, curenv->priority);

		//cprintf("\n--------------------------------------------------------\n");
	}

	/********DON'T CHANGE THIS LINE***********/
	ticks++ ;
	if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_TIME_APPROX))
	{
		update_WS_time_stamps();
	}
	//cprintf("Clock Handler\n") ;
	fos_scheduler();
	/*****************************************/
}

//===================================================================
// [9] Update LRU Timestamp of WS Elements
//	  (Automatically Called Every Quantum in case of LRU Time Approx)
//===================================================================
void update_WS_time_stamps()
{
	struct Env *curr_env_ptr = curenv;

	if(curr_env_ptr != NULL)
	{
		struct WorkingSetElement* wse ;
		{
			int i ;
#if USE_KHEAP
			LIST_FOREACH(wse, &(curr_env_ptr->page_WS_list))
			{
#else
			for (i = 0 ; i < (curr_env_ptr->page_WS_max_size); i++)
			{
				wse = &(curr_env_ptr->ptr_pageWorkingSet[i]);
				if( wse->empty == 1)
					continue;
#endif
				//update the time if the page was referenced
				uint32 page_va = wse->virtual_address ;
				uint32 perm = pt_get_page_permissions(curr_env_ptr->env_page_directory, page_va) ;
				uint32 oldTimeStamp = wse->time_stamp;

				if (perm & PERM_USED)
				{
					wse->time_stamp = (oldTimeStamp>>2) | 0x80000000;
					pt_set_page_permissions(curr_env_ptr->env_page_directory, page_va, 0 , PERM_USED) ;
				}
				else
				{
					wse->time_stamp = (oldTimeStamp>>2);
				}
			}
		}

		{
			int t ;
			for (t = 0 ; t < __TWS_MAX_SIZE; t++)
			{
				if( curr_env_ptr->__ptr_tws[t].empty != 1)
				{
					//update the time if the page was referenced
					uint32 table_va = curr_env_ptr->__ptr_tws[t].virtual_address;
					uint32 oldTimeStamp = curr_env_ptr->__ptr_tws[t].time_stamp;

					if (pd_is_table_used(curr_env_ptr->env_page_directory, table_va))
					{
						curr_env_ptr->__ptr_tws[t].time_stamp = (oldTimeStamp>>2) | 0x80000000;
						pd_set_table_unused(curr_env_ptr->env_page_directory, table_va);
					}
					else
					{
						curr_env_ptr->__ptr_tws[t].time_stamp = (oldTimeStamp>>2);
					}
				}
			}
		}
	}
}


#include "kernel.h"
#include "hardware.h"

extern void (*cpu_idle)(void);

struct task_struct *current;
static struct task_struct *idle_task;

static struct task_struct task[NUM_TASKS];

static struct list_head freequeue;
static struct list_head readyqueue;

static uint8_t global_pid;
static uint8_t global_quantum;

static inline struct task_struct *list_pop_front_task_struct(struct list_head *l)
{
	return list_entry(list_pop_front(l), struct task_struct, list);
}

syscall_value_t sys_fork(void)
{
	struct task_struct *new;

	if (list_empty(&freequeue))
		return -1;

	new = list_pop_front_task_struct(&freequeue);

	/* Copy current task_struct to the new one */
	memcpy(new, current, sizeof(struct task_struct));

	new->reg.r1 = 0;
	new->pid = sched_get_free_pid();

	/* Place new to the readyqueue */
	list_add_tail(&new->list, &readyqueue);

	return new->pid;
}

syscall_value_t sys_getpid(void)
{
	return current->pid;
}

syscall_value_t sys_getticks(void)
{
	return hw_getticks();
}

syscall_value_t sys_readkey(void)
{
	return hw_getkey();
}

static int sched_needs_switch(void)
{
	if (current->pid == 0) {
		/* If executing idle_task and there are other tasks waiting
		 * switch to them, else do nothing.
		 */
		if (!list_empty(&readyqueue))
			return 1;
		else
			return 0;
	} else {
		/* If not executing idle_task, switch only if quantum expired
		 * Does not mind if only there is one user task
		 */
		global_quantum--;
		if (global_quantum == 0)
			return 1;
		else
			return 0;
	}
}

static void sched_task_switch(struct task_struct *next)
{
	global_quantum = next->quantum;
	current = next;
}

void sched_schedule(struct list_head *queue)
{
	struct task_struct *next;

	if (current->pid != 0) {
		/* Save current to the appropriate queue */
		list_add_tail(&current->list, queue);
		next = list_pop_front_task_struct(&readyqueue);
	} else {
		/* If there's no task in the readyqueue, switch to idle */
		if (list_empty(&readyqueue))
			next = idle_task;
		else
			next = list_pop_front_task_struct(&readyqueue);
	}

	/* Task switch to next */
	sched_task_switch(next);
}

void sched_run(void)
{
	/* Update task sched attrib. and check if switch needed */
	if (sched_needs_switch()) {
		sched_schedule(&readyqueue);
	}
}

uint8_t sched_get_free_pid(void)
{
	return ++global_pid;
}

static void sched_init_queues(void)
{
	int i;

	INIT_LIST_HEAD(&freequeue);
	INIT_LIST_HEAD(&readyqueue);

	for (i = 0; i < NUM_TASKS; i++)
		list_add_tail(&(&task[i])->list, &freequeue);
}

static void sched_init_idle(void)
{
	idle_task = list_pop_front_task_struct(&freequeue);

	idle_task->pid = 0;
	// Idle task does not have quantum since we task_switch to
	// any another user task if exists. Anyway we set up to default
	idle_task->quantum = SCHED_DEFAULT_QUANTUM;
	idle_task->reg.pc = (uintptr_t)&cpu_idle;
	/* Interrupts enabled, kernel mode */
	idle_task->reg.psw = PSW_IE | PSW_KERNEL_MODE;
}

static void sched_init_task1(void)
{
	struct task_struct *task1 = list_pop_front_task_struct(&freequeue);

	task1->pid = 1;
	task1->quantum = SCHED_DEFAULT_QUANTUM;
	task1->reg.pc = (unsigned int)&_user_code_start;
	/* Interrups enabled, user mode */
	task1->reg.psw = PSW_IE | PSW_USER_MODE;

	/* Sched starts with quantum from task1 */
	global_quantum = task1->quantum;

	current = task1;
}

void sched_init(void)
{
	int i;

	for (i = 0; i < NUM_TASKS; i++) {
		task[i].pid = -1;
		memset(&(&task[i])->regs, 0, sizeof(task[i].regs));
	}

	sched_init_queues();
	sched_init_idle();
	sched_init_task1();

	/* Skip idle and task1 processes */
	global_pid = 1;
}

int kernel_main(void)
{
	hw_init();
	sched_init();

	void (*user_entry)(void) = (void (*)(void))(&_user_code_start);

	/* Enable interrupts, user mode and jump to the user code */
	__asm__(
		"wrs s0, %0\n\t"
		"wrs s1, %1\n\t"
		"reti"
		: : "r"(PSW_USER_MODE | PSW_IE), "r"(user_entry)
	);

	return 0;
}

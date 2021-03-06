#include <stdint.h>
#include "task.h"
#include "schedule.h"
#include "uart.h"
#include "irq.h"
#include "string_util.h"
#include "syscall.h"
#include "sys.h"
#include "signal.h"
#include "mmu.h"
#include "slab.h"

struct task_struct * kernel_task_pool;
uint16_t * task_kernel_stack_pool;

void task_init()
{
  uart_puts("Task init started\n");
  kernel_task_pool = (struct task_struct *)slab_malloc(sizeof(struct task_struct) * TASK_POOL_SIZE);
  for(unsigned idx = 0; idx < TASK_POOL_SIZE; ++idx)
  {
    kernel_task_pool[idx].id = 0;
  }
  task_kernel_stack_pool = (uint16_t *)slab_malloc(sizeof(uint16_t) * TASK_POOL_SIZE * TASK_KERNEL_STACK_SIZE);
  uart_puts("Task init complete\n");
  return;
}

uint64_t task_privilege_task_create(void(*start_func)(), unsigned priority)
{
  TASK_GUARD();
  unsigned new_id = 0;

  /* find usable position in task_pool */
  for(unsigned idx = 0; idx < TASK_POOL_SIZE; ++idx)
  {
    if(kernel_task_pool[idx].id != idx + 1)
    {
      new_id = idx + 1;
      break;
    }
  }
  if(new_id == 0)
  {
    /* failed to create new task */
    return 0;
  }

  /* assign id */
  kernel_task_pool[TASK_ID_TO_IDX(new_id)].id = new_id;

  /* init quantum_count to 0 */
  kernel_task_pool[TASK_ID_TO_IDX(new_id)].quantum_count = 0;

  /* reset flag */
  kernel_task_pool[TASK_ID_TO_IDX(new_id)].flag = 0;

  /* reset signal */
  kernel_task_pool[TASK_ID_TO_IDX(new_id)].signal = 0;

  /* set priority */
  kernel_task_pool[TASK_ID_TO_IDX(new_id)].priority = (uint64_t)priority;

  /* Enable interrupt by default */
  SET_BIT(kernel_task_pool[TASK_ID_TO_IDX(new_id)].cpu_context.spsr_el1, 7);

  /* assign context */
  kernel_task_pool[TASK_ID_TO_IDX(new_id)].cpu_context.lr = (uint64_t)start_func;

  /* clear fd table */
  for(int i = 0; i < TASK_MAX_FD; ++i)
  {
    kernel_task_pool[TASK_ID_TO_IDX(new_id)].fd[i] = 0;
  }

  /* set default working dir to / */
  kernel_task_pool[TASK_ID_TO_IDX(new_id)].current_dir_vnode = vfs_get_root_vnode();

  /* stack grow from high to low */
  kernel_task_pool[TASK_ID_TO_IDX(new_id)].cpu_context.fp = (uint64_t)(task_kernel_stack_pool + new_id * TASK_KERNEL_STACK_SIZE);
  kernel_task_pool[TASK_ID_TO_IDX(new_id)].cpu_context.sp = (uint64_t)(task_kernel_stack_pool + new_id * TASK_KERNEL_STACK_SIZE);

  /* push into queue */
  schedule_enqueue(new_id, (unsigned)priority);
  TASK_UNGUARD();
  return new_id;
}

uint64_t task_get_current_task_id(void)
{
  uint64_t current_task_id;
  asm volatile("mrs %0, tpidr_el1\n":
               "=r"(current_task_id));
  return current_task_id;
}

void task_test(void)
{
  char ann[] = ANSI_BG_GREEN ANSI_BLACK"[Kernel mode test]"ANSI_RESET" ";
  char string_buff[0x80];
  uint64_t current_task_id = task_get_current_task_id();
  irq_int_enable();

  uart_puts(ann);
  uart_puts("ID: ");
  string_longlong_to_char(string_buff, (int64_t)current_task_id);
  uart_puts(string_buff);
  uart_puts(" Entering user mode test\n");
  uart_puts(ann);
  task_do_exec((uint64_t *)&_binary_test_bin_start, (uint64_t)&_binary_test_bin_end - (uint64_t)&_binary_test_bin_start);
}

void task_do_exec(uint64_t * start, uint64_t size)
{
  char string_buff[0x40];
  uart_puts("Start: ");
  string_ulonglong_to_hex_char(string_buff, (unsigned long long)&start);
  uart_puts(string_buff);
  uart_puts("\tsize: ");
  string_ulonglong_to_hex_char(string_buff, (unsigned long long)size);
  uart_puts(string_buff);
  uart_puts("\n");
  /* CAUTION: kernel stack may explode if you keep doing exec */

  uint64_t current_task_id = task_get_current_task_id();
  struct user_space_mm_struct * current_user_mm_struct = &(kernel_task_pool[TASK_ID_TO_IDX(current_task_id)].user_space_mm);

  /* Ask scheduler to switch pmd because it use user space */
  SET_BIT(kernel_task_pool[TASK_ID_TO_IDX(current_task_id)].flag, TASK_STATE_USER_SPACE);

  /* create user space va */
  mmu_create_user_pmd_pte(current_user_mm_struct);

  /* setup pmc */
  mmu_user_task_set_pmd(current_user_mm_struct);

  /* copy memory to physical page frame */
  mmu_copy_user_to_text((char *) start, current_user_mm_struct, (unsigned)size);

  /* setup register and eret */
  asm volatile(
    "mov x0, %0\n"
    "msr sp_el0, x0\n"
    : : "r"(USER_STACK_VA)); /* stack grows from high to low */

  asm volatile(
    "mov x0, %0\n"
    "msr elr_el1, x0\n"
    : : "r"(0x0));

  asm volatile(
    "eor x0, x0, x0\n"
    "msr spsr_el1, x0\n"
    "eret");
}

uint64_t task_get_current_task_signal(void)
{
  uint64_t current_task_id = task_get_current_task_id();
  return kernel_task_pool[TASK_ID_TO_IDX(current_task_id)].signal;
}

void task_start_waiting(void)
{
  uint64_t current_task_id = task_get_current_task_id();

  TASK_GUARD();

  SET_BIT(kernel_task_pool[TASK_ID_TO_IDX(current_task_id)].flag, 2);
  schedule_enqueue_wait(current_task_id);
  TASK_UNGUARD();

  schedule_yield();
  return;
}

void task_end_waiting(void)
{
  TASK_GUARD();
  /* put the first task in the wait queue back to running queue */
  uint64_t task_id = schedule_dequeue_wait();
  if(task_id == 0)
  {
    return;
  }

  /* Some task in wait queue might be zombie */
  while(CHECK_BIT(kernel_task_pool[TASK_ID_TO_IDX(task_id)].flag, TASK_STATE_ZOMBIE))
  {
    task_id = schedule_dequeue_wait();
  }

  CLEAR_BIT(kernel_task_pool[TASK_ID_TO_IDX(task_id)].flag, 2);
  schedule_enqueue(task_id, (unsigned)kernel_task_pool[TASK_ID_TO_IDX(task_id)].priority);
  TASK_UNGUARD();
  return;
}

int task_guard_section(void)
{
  if(!scheduler_is_init())
  {
    /* make sure task_pool is set */
    return 0;
  }
  /* return 1 if already guard, return 0 if not guarded yet and guarded */
  uint64_t current_task_id = task_get_current_task_id();
  if(CHECK_BIT(kernel_task_pool[TASK_ID_TO_IDX(current_task_id)].flag, TASK_STATE_GUARD))
  {
    return 1;
  }
  SET_BIT(kernel_task_pool[TASK_ID_TO_IDX(current_task_id)].flag, TASK_STATE_GUARD);
  return 0;
}

int task_unguard_section(void)
{
  if(!scheduler_is_init())
  {
    /* make sure task_pool is set */
    return 0;
  }
  /* return 1 if already unguard, return 0 if guarded and unguard */
  uint64_t current_task_id = task_get_current_task_id();
  if(CHECK_BIT(kernel_task_pool[TASK_ID_TO_IDX(current_task_id)].flag, TASK_STATE_GUARD) == 0)
  {
    return 1;
  }
  CLEAR_BIT(kernel_task_pool[TASK_ID_TO_IDX(current_task_id)].flag, TASK_STATE_GUARD);
  return 0;
}

int task_is_guarded(void)
{
  uint64_t current_task_id = task_get_current_task_id();
  return CHECK_BIT(kernel_task_pool[TASK_ID_TO_IDX(current_task_id)].flag, TASK_STATE_GUARD);
}

int task_set_fd(struct vfs_file_struct * file)
{
  uint64_t current_task_id = task_get_current_task_id();
  for(int idx = 0; idx < TASK_MAX_FD; ++idx)
  {
    if(kernel_task_pool[TASK_ID_TO_IDX(current_task_id)].fd[idx] == 0)
    {
      kernel_task_pool[TASK_ID_TO_IDX(current_task_id)].fd[idx] = file;
      return idx;
    }
  }
  uart_puts("Exceed maximum fd. Entering busy loop\n");
  while(1);
  return 0;
}

struct vfs_file_struct * task_get_vfs_file(int fd)
{
  /* TODO: check fd */
  uint64_t current_task_id = task_get_current_task_id();
  return kernel_task_pool[TASK_ID_TO_IDX(current_task_id)].fd[fd];
}

void task_unset_fd(int fd)
{
  uint64_t current_task_id = task_get_current_task_id();
  kernel_task_pool[TASK_ID_TO_IDX(current_task_id)].fd[fd] = 0;
  return;
}

struct vfs_vnode_struct * task_get_current_vnode(void)
{
  return kernel_task_pool[TASK_ID_TO_IDX(task_get_current_task_id())].current_dir_vnode;
}

void task_set_current_vnode(struct vfs_vnode_struct * vnode)
{
  kernel_task_pool[TASK_ID_TO_IDX(task_get_current_task_id())].current_dir_vnode = vnode;
  return;
}


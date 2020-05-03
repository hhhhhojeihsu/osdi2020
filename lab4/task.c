#include <stdint.h>
#include "task.h"
#include "queue.h"
#include "schedule.h"
#include "uart.h"
#include "irq.h"
#include "string_util.h"
#include "syscall.h"
#include "sys.h"

struct task_struct kernel_task_pool[TASK_POOL_SIZE];
uint16_t task_kernel_stack_pool[TASK_POOL_SIZE][TASK_KERNEL_STACK_SIZE];
uint16_t task_user_stack_pool[TASK_POOL_SIZE][TASK_USER_STACK_SIZE];

uint64_t task_privilege_task_create(void(*start_func)())
{
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

  /* assign context */
  kernel_task_pool[TASK_ID_TO_IDX(new_id)].cpu_context.lr = (uint64_t)start_func;

  /* stack grow from high to low */
  kernel_task_pool[TASK_ID_TO_IDX(new_id)].cpu_context.fp = (uint64_t)(task_kernel_stack_pool[new_id]);
  kernel_task_pool[TASK_ID_TO_IDX(new_id)].cpu_context.sp = (uint64_t)(task_kernel_stack_pool[new_id]);

  /* push into queue */
  schedule_enqueue(new_id);
  return new_id;
}

uint64_t task_get_current_task_id(void)
{
  uint64_t current_task_id;
  asm volatile("mrs %0, tpidr_el1\n":
               "=r"(current_task_id));
  return current_task_id;
}

void task_idle(void)
{
  while(1)
  {
    while(schedule_check_queue_empty())
    {
      asm volatile ("wfi");
      /* keep waiting for new task even wake from timer interrupt */
    }
    scheduler();
  }
}

void task_privilege_demo(void)
{
  int current_reschedule_count = 0;
  int max_reschedule_time = 3;
  while(1)
  {
    char string_buff[0x10];
    uint64_t current_task_id = task_get_current_task_id();
    uint64_t current_quantum_count;
    uart_puts("Hi I'm privilege task id ");
    string_longlong_to_char(string_buff, (int64_t)current_task_id);
    uart_puts(string_buff);
    uart_putc('\n');
    irq_int_enable();
    uart_puts("Enabling timer interrupt\n");

    uart_puts("Waiting to reschedule\n");
    /* if quantum_count get from task_pool is less than previous one, the reschedule occurred */
    do
    {
      current_quantum_count = kernel_task_pool[TASK_ID_TO_IDX(current_task_id)].quantum_count;
      asm volatile("wfi");
    }
    while(current_quantum_count <= kernel_task_pool[TASK_ID_TO_IDX(current_task_id)].quantum_count);
    ++current_reschedule_count;
    if(current_reschedule_count >= max_reschedule_time)
    {
      uart_puts("Privilege task will exit\n");
      sys_exit(0);
    }
  }
}

void task_do_exec(void(*start_func)())
{
  /* CAUTION: kernel stack may explode if you keep doing exec */
  uint64_t current_task_id = task_get_current_task_id();
  /* setup register and eret */
  asm volatile(
    "mov x0, %0\n"
    "msr sp_el0, x0\n"
    : : "r"((uint64_t)(task_user_stack_pool[current_task_id]))); /* stack grows from high to low */

  asm volatile(
    "mov x0, %0\n"
    "msr elr_el1, x0\n"
    : : "r"(start_func));

  asm volatile(
    "eor x0, x0, x0\n"
    "msr spsr_el1, x0\n"
    "eret");
}

void task_user_demo(void)
{
  char string_buff[0x10];
  uint64_t current_task_id = task_get_current_task_id();

  irq_int_enable();
  uart_puts("task_user_demo in kernel mode with irq enable id ");
  string_longlong_to_char(string_buff, (int64_t)current_task_id);
  uart_puts(string_buff);
  uart_putc('\n');

  task_do_exec(task_user_context1_demo);
}

void task_user_context1_demo(void)
{
  syscall_uart_puts("task_user_demo in user mode.\n");
  syscall_uart_puts("Let's exec in user mode\n");
  syscall_exec(task_user_context2_demo);
}

void task_user_context2_demo(void)
{
  char input_string[0x20];
  int new_task_id;

  syscall_uart_puts("I'm Mr.Meeseeks. Look at me. Let's call another meeseeks.\n");
  new_task_id = syscall_fork();
  if(new_task_id == 0)
  {
    syscall_uart_puts("I'm the new meeseeks\n");
    syscall_uart_gets(input_string, '\n', 0x20 - 2);
    uart_puts("Owee new meeseeks' mission accomplished. [Poof]\n");
    syscall_exit(0);
  }
  else
  {
    syscall_uart_puts("New meeseeks has id of ");
    string_longlong_to_char(input_string, new_task_id);
    syscall_uart_puts(input_string);
    syscall_uart_puts("\n");

    while(1)
    {
      syscall_uart_puts("I'm the original meeseeks\n");
      syscall_uart_gets(input_string, '\n', 0x20 - 2);
    }
  }

}


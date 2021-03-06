#include <stdint.h>
#include "meta_macro.h"
#include "string_util.h"
#include "irq.h"
#include "uart.h"
#include "timer.h"
#include "queue.h"

int defer_mode = 0;
int isr_int_enable = 0;

void irq_int_enable(void)
{
  // Clear interrupt mask for d, a, (i), f
  asm volatile("msr daifclr, #0x2");
}

void irq_int_disable(void)
{
  // Set interrupt mask for d, a, (i), f
  asm volatile("msr daifset, #0x2");
}

void irq_el1_handler(void)
{
  char string_buff[0x100];
  static int core_timer_count = 0;
  static int local_timer_count = 0;
  if(CHECK_BIT(*LOCAL_TIMER_CONTROL_REG, 31))
  {
    ++local_timer_count;
    uart_puts("ARM local time interrupt \"");
    string_longlong_to_char(string_buff, local_timer_count);
    uart_puts(string_buff);
    uart_puts("\" received\n");
    timer_clear_local_timer_int_and_reload();
    if(local_timer_count == DISABLE_TIMER_COUNT)
    {
      timer_disable_local_timer();
      local_timer_count = 0;
    }
  }
  else if(CHECK_BIT(*INT_BASIC_PENDING, 19))
  {
    static int tx_bug_fix = 0;
    // UART interrupt
    // [19] is GPU IRQ 59 which is uart_int (59)
    /* TX int: availible to write*/
    if(CHECK_BIT(*UART_MIS, 5) & CHECK_BIT(*UART_RIS, 5))
    {
      if(!tx_bug_fix)
      {
        tx_bug_fix = 1;
        /* clear intended interrupt */
        *UART_ICR = 0x5;
        return;
      }

      if(!QUEUE_EMPTY(tx_queue))
      {
        while(!CHECK_BIT(*UART_FR, 5)) /* while fifo is not full */
        {
          /* dump data */
          if(!QUEUE_EMPTY(tx_queue))
          {
            *UART_DR = (uint32_t)QUEUE_POP(tx_queue);
          }
          else
          {
            break;
          }
        }
      }
      /* Nothing to do so clear tx interrupt */
      *UART_ICR = 0x20;
      return;
    }
    /* RX int */
    else if(CHECK_BIT(*UART_MIS, 4) & CHECK_BIT(*UART_RIS, 4))
    {
      while(!CHECK_BIT(*UART_FR, 4)) /* rxfe not set -> fifo is not empty -> move data into queue */
      {
        if(!QUEUE_FULL(rx_queue))
        {
          QUEUE_PUSH(rx_queue, (char)*UART_DR);
        }
        else
        {
          while(1);
          /* There's nothing we can do for now, the program will hang. Try enlarge the queue */
        }
      }
      /* Cleart interrupt for safety */
      *UART_ICR = 0x10;
      return;
    }
  }
  else
  {
    ++core_timer_count;
    if(core_timer_count == DISABLE_TIMER_COUNT)
    {
      timer_expire_core_timer();
      core_timer_count = 0;
    }
    else
    {
      timer_set_core_timer(CORE_TIMER_SECS);
    }
    /* Start of bottom half */
    if(isr_int_enable)
    {
      irq_int_enable();
    }

    string_buff[0] = '\0';
    string_concat(string_buff, "ARM core time interrupt \"");
    {
      char id[4];
      if(core_timer_count == 0)
      {
        string_longlong_to_char(id, DISABLE_TIMER_COUNT);
      }
      else
      {
        string_longlong_to_char(id, core_timer_count);
      }
      string_concat(string_buff, id);
    }
    string_concat(string_buff, "\" received\n");
    for(int i = 0; i < string_length(string_buff); ++i)
    {
      uart_putc(string_buff[i]);
      if(defer_mode)
      {
        /* busy waiting */
        for(int defer_cycle = 0; defer_cycle < PI_DEFER_CYCLE; ++defer_cycle){char c = 0; ++c;}
      }
    }
    if(isr_int_enable)
    {
      irq_int_disable();
    }
  }
  return;
}


#include "shell.h"
#include "uart.h"
#include "meta_macro.h"
#include "string_util.h"
#include "mailbox.h"
#include "pm.h"
#include "timer.h"
#include "irq.h"
#include "exc.h"

const char * shell_command_list[] = {
  "hello",
  "help",
  "timestamp",
  "reboot",
  "bd",
  "vcmem",
  "txt",
  "exc",
  "irq",
  "defer",
  "nodefer",
  "isrirqint",
  "noisrirqint",
  0x0
};

const char * shell_command_descriptions[] = {
  "Print Hello World!",
  "Help",
  "Get current timestamp",
  "Reboot rpi3",
  "Show board revision",
  "Show vc memory address",
  "Show .text location",
  "Issue same EL synchronous exception",
  "Enable timer interrupt",
  "Start deferring core timer",
  "Stop deferring core timer",
  "Enable IRQ interrupt in core timer interrupt handler",
  "Disable IRQ interrupt in core timer interrupt handler",
  0x0
};

int (*shell_command_function_ptr[])(char *) = {
  shell_hello,
  shell_help,
  shell_timestamp,
  shell_reboot,
  shell_show_board_revision,
  shell_show_vc_memory,
  shell_show_text_location,
  shell_exc,
  shell_irq,
  shell_defer,
  shell_nodefer,
  shell_isrirqint,
  shell_noisrirqint,
  0x0
};

char pikachu0[] =
  ""ANSI_BG_GREEN".......***.................................................."ANSI_RESET"\n"
  ""ANSI_BG_GREEN".......****................................................."ANSI_RESET"\n"
  ""ANSI_BG_GREEN".......***."ANSI_BG_YELLOW" "ANSI_BG_GREEN"................................................"ANSI_RESET"\n"
  ""ANSI_BG_GREEN"......****."ANSI_BG_YELLOW"   "ANSI_BG_GREEN".............................................."ANSI_RESET"\n"
  ""ANSI_BG_GREEN".........*."ANSI_BG_YELLOW"     "ANSI_BG_GREEN"............................................"ANSI_RESET"\n"
  ""ANSI_BG_GREEN".........*."ANSI_BG_YELLOW"       "ANSI_BG_GREEN"............................***********.."ANSI_BG_YELLOW" "ANSI_RESET"\n"
  ""ANSI_BG_GREEN"..........*"ANSI_BG_YELLOW"        "ANSI_BG_GREEN"...........................********.."ANSI_BG_YELLOW"    "ANSI_RESET"\n"
  ""ANSI_BG_GREEN"..........*."ANSI_BG_YELLOW"         "ANSI_BG_GREEN".........................******.."ANSI_BG_YELLOW"      "ANSI_RESET"\n"
  ""ANSI_BG_GREEN"............"ANSI_BG_YELLOW"          "ANSI_BG_GREEN".........................***."ANSI_BG_YELLOW"         "ANSI_RESET"\n"
  ""ANSI_BG_GREEN"............"ANSI_BG_YELLOW"           "ANSI_BG_GREEN".........................."ANSI_BG_YELLOW"           "ANSI_RESET"\n"
  ""ANSI_BG_GREEN"............."ANSI_BG_YELLOW"            "ANSI_BG_GREEN"......................"ANSI_BG_YELLOW"             "ANSI_RESET"\n"
  ""ANSI_BG_GREEN"............*."ANSI_BG_YELLOW"            "ANSI_BG_GREEN"..................."ANSI_BG_YELLOW"              "ANSI_BG_GREEN"."ANSI_RESET"\n"
  ""ANSI_BG_GREEN"............*."ANSI_BG_YELLOW"            "ANSI_BG_GREEN".....           ."ANSI_BG_YELLOW"                "ANSI_BG_GREEN"*"ANSI_RESET"\n"
  ""ANSI_BG_GREEN"............**."ANSI_BG_YELLOW"                                           "ANSI_BG_GREEN".*"ANSI_RESET"\n"
  ""ANSI_BG_GREEN"...........****."ANSI_BG_YELLOW"                                         "ANSI_BG_GREEN"..."ANSI_RESET"\n"
  ""ANSI_BG_GREEN"...........*****."ANSI_BG_YELLOW"                                        "ANSI_BG_GREEN"..."ANSI_RESET"\n"
  ""ANSI_BG_GREEN"........*********."ANSI_BG_YELLOW"  .                                   "ANSI_BG_GREEN"...."ANSI_RESET"\n"
  ""ANSI_BG_GREEN"..****************..."ANSI_BG_YELLOW"                                 "ANSI_BG_GREEN"......"ANSI_RESET"\n"
  ""ANSI_BG_GREEN"..*****************."ANSI_BG_YELLOW"                              "ANSI_BG_GREEN".. ......."ANSI_RESET"\n"
  ""ANSI_BG_GREEN".*****************."ANSI_BG_YELLOW"                                "ANSI_BG_GREEN"........."ANSI_RESET"\n"
  ""ANSI_BG_GREEN".*****************."ANSI_BG_YELLOW"                                 "ANSI_BG_GREEN"........"ANSI_RESET"\n"
  ""ANSI_BG_GREEN".****************."ANSI_BG_YELLOW"      "ANSI_RESET"..."ANSI_BG_YELLOW"               "ANSI_RESET"..."ANSI_BG_YELLOW"       "ANSI_BG_GREEN"........"ANSI_RESET"\n"
  ""ANSI_BG_GREEN"****************.."ANSI_BG_YELLOW"     "ANSI_RESET"."ANSI_BG_WHITE" "ANSI_RESET".*."ANSI_BG_YELLOW"             "ANSI_RESET"."ANSI_BG_WHITE" "ANSI_RESET".*."ANSI_BG_YELLOW"       "ANSI_BG_GREEN"......."ANSI_RESET"\n"
  ""ANSI_BG_GREEN"***************.*."ANSI_BG_YELLOW"     "ANSI_RESET". .*."ANSI_BG_YELLOW"             "ANSI_RESET". .*."ANSI_BG_YELLOW"       "ANSI_BG_GREEN"......."ANSI_RESET"\n"
  ""ANSI_BG_GREEN"**************..."ANSI_BG_YELLOW"      "ANSI_RESET".***."ANSI_BG_YELLOW"             "ANSI_RESET".***."ANSI_BG_YELLOW"       "ANSI_BG_GREEN".*....."ANSI_RESET"\n"
  ""ANSI_BG_GREEN"**************..."ANSI_BG_YELLOW"       "ANSI_RESET"***"ANSI_BG_YELLOW"               "ANSI_RESET"***"ANSI_BG_YELLOW"         "ANSI_BG_GREEN"...***"ANSI_RESET"\n"
  ""ANSI_BG_GREEN"**************.*."ANSI_BG_YELLOW"                                     "ANSI_BG_GREEN".*****"ANSI_RESET"\n";
char pikachu1[] =
  ""ANSI_BG_GREEN"**************.."ANSI_BG_YELLOW"               ...                    "ANSI_BG_GREEN".*****"ANSI_RESET"\n"
  ""ANSI_BG_GREEN"**************.."ANSI_BG_YELLOW" "ANSI_BG_RED"..."ANSI_BG_YELLOW"           "ANSI_RESET"...."ANSI_BG_YELLOW"            "ANSI_BG_RED".."ANSI_BG_YELLOW"      "ANSI_BG_GREEN"*****"ANSI_RESET"\n"
  ""ANSI_BG_GREEN"*************..."ANSI_BG_RED"....."ANSI_BG_YELLOW"                         "ANSI_BG_RED"....."ANSI_BG_YELLOW"    "ANSI_BG_GREEN".****"ANSI_RESET"\n"
  ""ANSI_BG_GREEN"************...."ANSI_BG_RED"......"ANSI_BG_YELLOW"                       "ANSI_BG_RED"......."ANSI_BG_YELLOW"   "ANSI_BG_GREEN"....."ANSI_RESET"\n"
  ""ANSI_BG_GREEN"**********......"ANSI_BG_RED"......"ANSI_BG_YELLOW"                       "ANSI_BG_RED"......."ANSI_BG_YELLOW"   "ANSI_BG_GREEN"....."ANSI_RESET"\n"
  ""ANSI_BG_GREEN"*.............. "ANSI_BG_RED"....."ANSI_BG_YELLOW"          "ANSI_BG_RED"......"ANSI_BG_YELLOW"        "ANSI_BG_RED"......."ANSI_BG_YELLOW"    "ANSI_BG_GREEN"...."ANSI_RESET"\n"
  ""ANSI_BG_GREEN"................"ANSI_BG_YELLOW" "ANSI_BG_RED"..."ANSI_BG_YELLOW"          "ANSI_BG_RED"........"ANSI_BG_YELLOW"       "ANSI_BG_RED"......."ANSI_BG_YELLOW"    "ANSI_BG_GREEN"...."ANSI_RESET"\n"
  ""ANSI_BG_GREEN"................"ANSI_BG_YELLOW"              "ANSI_BG_RED"........"ANSI_BG_YELLOW"        "ANSI_BG_RED"....."ANSI_BG_YELLOW"     "ANSI_BG_GREEN"...."ANSI_RESET"\n"
  ""ANSI_BG_GREEN"................"ANSI_BG_YELLOW"              "ANSI_BG_RED"........"ANSI_BG_YELLOW"                  "ANSI_BG_GREEN"...."ANSI_RESET"\n"
  ""ANSI_BG_GREEN"................"ANSI_BG_YELLOW"               "ANSI_BG_RED"......."ANSI_BG_YELLOW"                   "ANSI_BG_GREEN"..."ANSI_RESET"\n"
  ""ANSI_BG_GREEN"................."ANSI_BG_YELLOW"               "ANSI_BG_RED"....."ANSI_BG_YELLOW"                    "ANSI_BG_GREEN"..."ANSI_RESET"\n"
  ""ANSI_BG_GREEN"................. "ANSI_BG_YELLOW"                                       "ANSI_BG_GREEN"..."ANSI_RESET"\n"
  ""ANSI_BG_GREEN".................."ANSI_BG_YELLOW"                                       "ANSI_BG_GREEN"..."ANSI_RESET"\n"
  ""ANSI_BG_GREEN".................."ANSI_BG_YELLOW"                                       "ANSI_BG_GREEN"..."ANSI_RESET"\n"
  ""ANSI_BG_GREEN"..................."ANSI_BG_YELLOW"                                      "ANSI_BG_GREEN"..."ANSI_RESET"\n"
  ""ANSI_BG_GREEN"..................."ANSI_BG_YELLOW"                                       "ANSI_BG_GREEN".."ANSI_RESET"\n"
  ""ANSI_BG_GREEN"..................."ANSI_BG_YELLOW"                                       "ANSI_BG_GREEN".."ANSI_RESET"\n"
  ""ANSI_BG_GREEN".................*"ANSI_BG_YELLOW"                                        "ANSI_BG_GREEN".."ANSI_RESET"\n";

void shell(void)
{
  char string_buffer[0x1000];

  uart_init();

	/* Show boot message */
  uart_puts(pikachu0);
  uart_puts(pikachu1);
  shell_show_board_revision(string_buffer);
  shell_show_vc_memory(string_buffer);
  shell_show_text_location(string_buffer);

  while(1)
  {
    uart_puts(ANSI_RED "# " ANSI_RESET);
    uart_gets(string_buffer, '\n', 0x1000 - 1);
    _shell_parser(string_buffer);
  }

  return;
}

void _shell_parser(char * string_buffer)
{
  /* remove newline */
  string_strip(string_buffer, '\n');

  if(string_length(string_buffer) == 0)
  {
    return;
  }

  /* Check commands */
  int command_idx = 0;
  for(; shell_command_list[command_idx] != 0x0; ++command_idx)
  {
    if(string_cmp(string_buffer, shell_command_list[command_idx], 999999))
    {
      (*shell_command_function_ptr[command_idx])(string_buffer);
      break;
    }
  }
  if(shell_command_list[command_idx] == 0x0)
  {
    uart_puts("Err: command ");
    uart_puts(string_buffer);
    uart_puts(" not found, try <help>\n");
  }
}

int shell_hello(char * string_buffer)
{
  UNUSED(string_buffer);
  uart_puts("Hello World!\n");
  return 0;
}

int shell_help(char * string_buffer)
{
  string_buffer[0] = '\0';
  for(int command_idx = 0; shell_command_list[command_idx] != 0x0; ++command_idx)
  {
    string_concat(string_buffer, shell_command_list[command_idx]);
    string_concat(string_buffer, ": ");
    string_concat(string_buffer, shell_command_descriptions[command_idx]);
    string_concat(string_buffer, "\n");
  }
  uart_puts(string_buffer);
  return 0;
}

int shell_timestamp(char * string_buffer)
{
  unsigned long long time_frequency = get_cntfrq_el0();
  unsigned long long time_counter = get_cntpct_el0();
  float current_time = (float)((double)time_counter / (double)time_frequency);
  string_float_to_char(string_buffer, current_time);
  uart_putc('[');
  uart_puts(string_buffer);
  uart_puts("]\n");
  return 0;
}

int shell_reboot(char * string_buffer)
{
  UNUSED(string_buffer);

  uart_puts("Reboot...");
  /* Full reset */
  *PM_RSTC = PM_PASSWORD | 0x20;
  /* Reset in 10 tick */
  *PM_WDOG = PM_PASSWORD | 10;
  /* Stuck */
  while(1);
  return 0;
}

int shell_show_board_revision(char * string_buffer)
{
  uart_puts("Board revision: ");
  string_ulonglong_to_hex_char(string_buffer, mailbox_get_board_revision());
  uart_puts(string_buffer);
  uart_putc('\n');
  return 1;
}

int shell_show_vc_memory(char * string_buffer)
{
  uint32_t vc_memory_base;
  uint32_t vc_memory_size;

  if(!mailbox_get_vc_memory())
  {
    uart_puts("Unable to get vc memory\n");
    return 0;
  }

  vc_memory_base = __mailbox_buffer[5];
  vc_memory_size = __mailbox_buffer[6];

  uart_puts("VC core base address: ");
  string_ulonglong_to_hex_char(string_buffer, vc_memory_base);
  uart_puts(string_buffer);
  uart_puts(" size: ");
  string_ulonglong_to_hex_char(string_buffer, vc_memory_size);
  uart_puts(string_buffer);
  uart_putc('\n');
  return 1;
}

int shell_show_text_location(char * string_buffer)
{
  uart_puts("Start of .text: ");
  string_ulonglong_to_hex_char(string_buffer, (unsigned long long)&__executable_start);
  uart_puts(string_buffer);
  uart_puts(" end of .text: ");
  string_ulonglong_to_hex_char(string_buffer, (unsigned long long)&__etext);
  uart_puts(string_buffer);
  uart_putc('\n');
  return 1;
}

int shell_exc(char * string_buffer)
{
  UNUSED(string_buffer);
  asm volatile("svc #1");

  return 0;
}

int shell_irq(char * string_buffer)
{
  UNUSED(string_buffer);
  asm volatile("svc #2");
  return 0;
}

int shell_defer(char* string_buffer)
{
  UNUSED(string_buffer);
  defer_mode = 1;
  return 0;
}

int shell_nodefer(char* string_buffer)
{
  UNUSED(string_buffer);
  defer_mode = 0;
  return 0;
}

int shell_isrirqint(char * string_buffer)
{
  UNUSED(string_buffer);
  isr_int_enable = 1;
  return 0;
}

int shell_noisrirqint(char * string_buffer)
{
  UNUSED(string_buffer);
  isr_int_enable = 0;
  return 0;
}


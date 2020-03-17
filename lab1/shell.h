#ifndef __SHELL_H__
#define __SHELL_H__

void shell(void);
void _shell_parser();
int shell_hello(char * string_buffer);
int shell_help(char * string_buffer);
int shell_timestamp(char * string_buffer);
int shell_reboot(char * string_buffer);

extern int get_cntfrq_el0(void);
extern int get_cntpct_el0(void);

#endif


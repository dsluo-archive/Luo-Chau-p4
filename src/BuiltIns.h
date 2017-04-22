#ifndef BUILTINS_H
#define BUILTINS_H

#include <sys/types.h>
typedef int jid_t;

int bg(jid_t jid);
int fg(jid_t jid);

int cd(const char * path);

void exit_command(int status);
void exit_command();

int help();

int jobs();

int kill(pid_t pid);
int kill(int signal, pid_t pid);
#endif

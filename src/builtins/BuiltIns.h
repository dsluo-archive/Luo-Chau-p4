#ifndef BUILTINS_H
#define BUILTINS_H

#include <sys/types.h>
#include <signal.h>
typedef int jid_t;

int bg(jid_t jid);
int fg(jid_t jid);

int cd(const char * path);

void exit(int status);
void exit();

int help();

int jobs();

int kill(pid_t pid);
int kill(int signal=SIGTERM, pid_t pid);
#endif

#ifndef BUILTINS_H
#define BUILTINS_H

#include <sys/types.h>
#include <vector>
#include <string>

using namespace std;

/* typedef struct entry { */
/*     pid_t pid; */
/*     string status; */
/*     string name; */

/*     entry(pid_t pid, string status, string name) : */
/*         pid(pid), status(status), name(name) {} */ 
/* } entry; */

/* static vector<entry> jobtable; */

int cd(const char * path);

void exit_command(int status);
void exit_command();

int bg(pid_t jid);
int fg(pid_t jid);

void jobs();
void addtojobtable(pid_t jid);

int kill(pid_t pid);
int kill(int signal, pid_t pid);

#endif

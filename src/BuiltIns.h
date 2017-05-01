#ifndef BUILTINS_H
#define BUILTINS_H

#include <sys/types.h>
#include <vector>
#include <string>

using namespace std;

int cd_cmd(int argc, char **argv);

void jobs();
void addtojobtable(pid_t jid);

int kill_cmd(int argc, char **argv);

int export_cmd(int argc, char **argv);
#endif

#include "BuiltIns.h"
#include <unistd.h>
#include <cstdio>

int cd(const char * path) {
    int status = chdir(path);
    if (status == -1) {
        perror("cd");
    }

    return status;
}

void addtojobtable(pid_t jid){
     
}

void jobs(){

}


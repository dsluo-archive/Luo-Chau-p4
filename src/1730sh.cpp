#include <array>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <iterator>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <map>
#include <stdexcept>
#include <readline/readline.h>
#include <readline/history.h>

#include "BuiltIns.h"

using namespace std;

typedef struct job_entry {
    pid_t pgid;
    string status;
    string cmd; 
} job_entry;

static vector<job_entry> jobtable;

vector<string> tokenize(string str);
char * get_prompt();
void close_pipe(int pipefd[2]);
void addtotable(pid_t pgid, string cmd);

void handle_stop(pid_t pgid, string cmd); // ctrl-z
void handle_kill(int signum); // ctrl-c

void handle_bg(int signum);
void update_status(pid_t pgid, string status);

int main() {
    const char *HELP_MESSAGE =
        "bg JID               – Resume the stopped job JID in the background, as if it had been started with &.\n"
        "cd [PATH]            – Change the current directory to PATH. The environmental variable HOME is the default PATH.\n"
        "exit [N]             – Cause the shell to exit with a status of N. If N is omitted, the exit status is that of the last job executed.\n"
        "export NAME[=WORD]   – NAME is automatically included in the environment of subsequently executed jobs.\n" 
        "fg JID               – Resume job JID in the foreground, and make it the current job.\n"
        "help                 – Display this help message.\n"
        "jobs                 – List current jobs.\n"
        "kill [-s SIGNAL] PID – The kill utility sends the specified signal to the specified process or process group PID\n"
        "                       If no signal is specified, the SIGTERM signal is sent.\n";
    string line;
    int *last_wstatus = nullptr;
    int status;
    
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGCHLD, handle_bg);
    
    int dfl_in = dup(STDIN_FILENO);
    if (dfl_in == -1) { perror("dup"); exit(EXIT_FAILURE); }
    int dfl_out = dup(STDOUT_FILENO);
    if (dfl_out == -1) { perror("dup"); exit(EXIT_FAILURE); }
    int dfl_err = dup(STDERR_FILENO);
    if (dfl_err == -1) { perror("dup"); exit(EXIT_FAILURE); }
    char * line_read;

    // Make the shell its own process leader
    if (setpgid(getpid(), getpid()) < 0){
        perror("setpgid");
        exit(EXIT_FAILURE); 
    }
    
    // get terminal control from the shell that started this shell
    if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0){
        perror("tcsetpgrp");
        exit(EXIT_FAILURE); 
    }

    const char * prompt = get_prompt();
    while ((line_read = readline(prompt))) {
        line = string(line_read);
        add_history(line_read);
        free(line_read);
        delete [] prompt;

        string in = "";
        string out = "";
        string err = "";

        int infd = STDIN_FILENO;
        int outfd = STDOUT_FILENO;
        int errfd = STDERR_FILENO;

        bool out_append = false;
        bool err_append = false;

        bool bg = false;

        vector<string> tokens = tokenize(line);
        vector<vector<string>> procs; 
        vector<string> proc;
        
        // Reap children, pick up background process updates, etc. 
        pid_t chld_pid; 
        while ((chld_pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0){
            // print status info out of handler. save this data in a global structure of sorts 
            if (WIFEXITED(status)){
                // update pid status
                update_status(chld_pid, "exited"); 
            } else if (WIFSTOPPED(status)){
                // update pid status
                update_status(chld_pid, "stopped"); 
            } else if (WIFSIGNALED(status)){
                // update pid status
                update_status(chld_pid, "killed"); 
            } else {
                perror("waitpid");
            }
        }
        
        // TODO: Need to handle output and input redirection anywhere. 
        for (auto it = tokens.begin(); it != tokens.end(); it++) {
            if ((it + 1) != tokens.end()) {
                if (*it == "e>") { // TODO: Handling error redirection
                    err_append = false;
                    err = *(it + 1);
                    it++;
                } else if (*it == "e>>") { // TODO: Handling error redirection
                    err_append = true;
                    err = *(it + 1);
                    it++;
                } else if (*it == ">") { // TODO: Handling output redirection
                    out_append = false;
                    out = *(it + 1);
                    it++;
                } else if (*it == ">>") { // TODO: Handling output redirection
                    out_append = true;
                    out = *(it + 1);
                    it++;
                } else if (*it == "<") { // TODO: Handling input redirection
                    in = *(it + 1);
                    it++;
                } else if (*it == "|") {
                    if (!proc.empty()) {
                        procs.push_back(proc);
                        proc.clear();
                    }
                } else {
                    proc.push_back(*it);
                }
               
                if (it == tokens.end() - 1) {
                    procs.push_back(proc);
                }
            } else {
                if (*it == "&" && it == tokens.end() - 1)
                    bg = true;
                else
                    proc.push_back(*it);
                procs.push_back(proc);
            }
        }

        pid_t pid;
        pid_t bg_pid = 0;
        vector<array<int, 2>> pipefds;
        vector<pid_t> pids;
        
        // for each process in procs,
        for (size_t i = 0; i < procs.size(); i++) {
            // create pipe for command
            if (i != procs.size() - 1){
                int pipefd[2];
                if (pipe(pipefd) == -1){
                    perror("pipe");
                    exit(EXIT_FAILURE); 
                }
                pipefds.push_back({pipefd[0], pipefd[1]});
            }

            // Construct argv
            size_t size = procs[i].size();
            char **argv = new char*[size];
            size_t k;
            for (k = 0; k < size; k++) {
                argv[k] = (char *) procs[i][k].c_str();
            }
            // argv must be null-terminated
            argv[size] = nullptr;

            if (strcmp(argv[0], "cd") == 0) {
                cd(size, argv);
            } else if (strcmp(argv[0], "exit") == 0) {
                if (argv[1] != nullptr) {
                    exit(atoi(argv[1])); 
                } else {
                    int exit_status;
                    if (last_wstatus != nullptr)
                        exit_status = WIFEXITED(*last_wstatus) 
                            ? WEXITSTATUS(*last_wstatus)
                            : EXIT_FAILURE;
                    else
                        exit_status = EXIT_SUCCESS;

                    exit(exit_status);
                }
            } else if (strcmp(argv[0], "help") == 0) {
                printf(HELP_MESSAGE);

            } else if (strcmp(argv[0], "bg") == 0) {    // TODO: need to write bg [jid]
                pid_t ret_pgid = atoi(argv[1]);       
                
                update_status(ret_pgid, "running");
                
                //waitpid(ret_pgid, &status, 0);
                
                // give terminal control back to the foreground process group
                if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0){
                    perror("tcsetpgrp");
                    exit(EXIT_FAILURE); 
                }
            } else if (strcmp(argv[0], "fg") == 0) {    
                pid_t ret_pgid = atoi(argv[1]);       
                
                // give terminal control to the background process group
                if (tcsetpgrp(STDIN_FILENO, ret_pgid) < 0){
                    perror("tcsetpgrp");
                }
                
                // continue stopped processes
                if (kill(ret_pgid, SIGCONT) < 0){
                    perror("kill");
                }

                update_status(ret_pgid, "continued");
                
                waitpid(ret_pgid, &status, 0);
                
                // give terminal control back to the foreground process group
                if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0){
                    perror("tcsetpgrp");
                    exit(EXIT_FAILURE); 
                }
            } else if (strcmp(argv[0], "jobs") == 0) { 
                if (!jobtable.empty()){
                    printf("JID\tSTAT\tCOMMAND\n"); 
                    for (auto val : jobtable){
                        printf("[%d] %s\t%s\n", val.pgid, val.status.c_str(), val.cmd.c_str()); 
                    }
                }
            } else if (strcmp(argv[0], "export") == 0){ 
                export_env(size, argv);
            } else if (strcmp(argv[0], "kill") == 0) { 
                kill_cmd(size, argv);
            } else {
                if (i == 0){
                    if (in != "") {
                        if ((infd = open(in.c_str(), O_RDONLY)) == -1){
                            perror("open");
                            exit(EXIT_FAILURE);
                        }
                    }
                }

                if (i == procs.size() - 1){
                    if (out != "") {
                        int mode = O_WRONLY | O_CREAT | (out_append ? O_APPEND : O_TRUNC);
                        if ((outfd = open(out.c_str(), mode, 0644)) == -1){
                            perror("open");
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (err != "") {
                        int mode = O_WRONLY | O_CREAT | (err_append ? O_APPEND : O_TRUNC);
                        if ((errfd = open(err.c_str(), mode)) == -1){
                            perror("open");
                            exit(EXIT_FAILURE);
                        }
                    }
                }

                pid = fork(); 

                if (pid > 0){
                    if (i == 0){
                        if (setpgid(pid, pid) == -1){ 
                            perror("setpgid"); 
                        }
                    } else {
                        if (setpgid(pid, bg_pid) == -1){
                            perror("setpgid"); 
                        }
                    }
                    
                    if (!bg){
                        if (tcsetpgrp(STDIN_FILENO, pid) < 0){
                            perror("tcsetpgrp");
                            exit(EXIT_FAILURE); 
                        }
                    } else {
                        if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0){
                            perror("tcsetpgrp");
                            exit(EXIT_FAILURE); 
                        }
                    }
                }
                
                if (pid == 0){
                    signal(SIGTTIN, SIG_DFL); 
                    signal(SIGTTOU, SIG_DFL); 
                    signal(SIGTSTP, SIG_DFL); 
                    signal(SIGINT, handle_kill); // kill process, burn it with fire
            
                    // if not the first command 
                    if (i != 0){ 
                        if (dup2(pipefds.at(i - 1)[0], STDIN_FILENO) == -1){
                            perror("dup2");
                            exit(EXIT_FAILURE); 
                        }
                    } else {
                        if (setpgid(0, 0) == -1){
                            perror("setpgid"); 
                        } 

                        if (dup2(infd, STDIN_FILENO) == -1) {
                            perror("dup2");
                            exit(EXIT_FAILURE);
                        }
                    }
                    
                    // if not the last command
                    if (i != procs.size() - 1){ 
                        if (dup2(pipefds.at(i)[1], STDOUT_FILENO) == -1){
                            perror("dup2");
                            exit(EXIT_FAILURE); 
                        }
                    } else {
                        if (dup2(outfd, STDOUT_FILENO) == -1){
                            perror("dup2");
                            exit(EXIT_FAILURE);
                        }
                        
                        if (dup2(errfd, STDERR_FILENO) == -1){ 
                            perror("dup2");
                            exit(EXIT_FAILURE);
                        }
                    }


                    for (unsigned int i = 0; i < pipefds.size(); i++){
                        close_pipe(pipefds.at(i).data()); 
                    }
                    
                    // populate arguments with argv
                    int status = execvp(procs[i][0].c_str(), argv);
                    delete[] argv;
                    if (status == -1){
                        perror(procs[i][0].c_str()); 
                    }

                    if (status == -1)
                        exit(EXIT_FAILURE);
                    else
                        exit(EXIT_SUCCESS);

                } else if (pid < 0){
                    perror("fork"); 
                    exit(EXIT_FAILURE);
                } else {
                    /* if (bg && (i == 0)){ */
                    /*     if (setpgid(pid, pid) == -1){ */
                    /*         perror("setpgid"); */ 
                    /*     } */

                    /*     bg_pid = pid; */ 
                    /*     addtotable(bg_pid, line); */ 
                    /* } */ 
                    
                    if (bg && (i == 0)){
                        bg_pid = pid;
                        addtotable(bg_pid, line);
                    } 
                    
                }
            }
        }

        // close all pipes when done
        for (unsigned int i = 0; i < pipefds.size(); i++){
            close_pipe(pipefds.at(i).data()); 
        }

        if (!bg){  
            for (unsigned int i = 0; i < procs.size(); i++){ 
                waitpid(pid, &status, WUNTRACED); 
                if (WIFSTOPPED(status)){
                    if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0){
                        perror("tcsetpgrp");
                        exit(EXIT_FAILURE); 
                    }

                    handle_stop(pid, line); 
                }
            }
        }
        last_wstatus = &status;
        
        // Restore file descriptors 
        if (dup2(STDIN_FILENO, dfl_in) == -1)
            perror("dup2");
        if (dup2(STDOUT_FILENO, dfl_out) == -1)
            perror("dup2");
        if (dup2(STDERR_FILENO, dfl_err) == -1)
            perror("dup2");

        if (infd != STDIN_FILENO && close(infd) != 0)
            perror("close");
        if (outfd != STDOUT_FILENO && close(outfd) != 0)
            perror("close");
        if (errfd != STDERR_FILENO && close(errfd) != 0)
            perror("close");
        
        prompt = get_prompt();
        
        if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0){
            perror("tcsetpgrp");
            exit(EXIT_FAILURE); 
        }
    }

    printf("\n");
    return EXIT_SUCCESS;
}

char * get_prompt(){
    char * prompt;
    // Figure out what the current working directory is. 
    long maxlen = pathconf(".", _PC_PATH_MAX);
    char *buf = nullptr;
    char *p; 
    if ((p = getcwd(buf, maxlen)) == NULL)
        perror("getcwd");

    // If the user is above their home directory, use a tilde.
    // If not, print the absolute path instead.
    const char *homedir = getenv("HOME");
    if (homedir == NULL)
        perror("getenv");     

    // Check the current working directory versus the home directory.
    if (strlen(p) >= strlen(homedir)){
        char *rpath = p + strlen(homedir);
        prompt = new char[strlen(rpath) + strlen("1730sh: $ ")];
        sprintf(prompt, "1730sh: ~%s$ ", rpath);   
    } else {
        prompt = new char[strlen(p) + strlen("1730sh: $ ")];
        sprintf(prompt, "1730sh: %s$ ", p); 
    }

    return prompt;
}

vector<string> tokenize(string str) {
    vector<string> output;

    char prev = '\0';

    for (auto it = str.begin(); it != str.end(); it++) {
        // Skip over excessive spaces
        if (*it == ' ' && prev != '\0' && prev != ' ') {
            if (it + 1 != str.end())
                output.push_back({""});
        } 
        // Handle quotes
        else if (*it == '"') {
            while (++it != str.end()) {
                // Handle escapes
                if (*it == '\\') {
                    if (*(it + 1) == '"') {
                        output.back().push_back('"');
                        it++;
                    } else if (*(it + 1) == '\\' || *(it + 1) == '\n') {
                        output.back().push_back('\\');
                        it++;
                    }
                } else if (*it == '"') {
                    break;
                } else {
                    output.back().push_back(*it);
                }
            }
            // Handle everything else
        } else if (*it != ' ') {
            if (output.empty())
                output.push_back("");
            output.back().push_back(*it);
        }
        prev = *it;
    }

    return output;
}

void close_pipe(int pipefd[2]){
    if (close(pipefd[0]) == -1){
        perror("close");
        exit(EXIT_FAILURE); 
    }

    if (close(pipefd[1]) == -1){
        perror("close");
        exit(EXIT_FAILURE); 
    }
}

void handle_bg(int signum){
    pid_t chld_pid;
    int status;

    while ((chld_pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0){
        // print status info out of handler. save this data in a global structure of sorts 
        if (WIFEXITED(status)){
            // update pid status
            update_status(chld_pid, "exited"); 
        } else if (WIFSTOPPED(status)){
            // update pid status
            update_status(chld_pid, "stopped"); 
        } else if (WIFSIGNALED(status)){
            // update pid status
            update_status(chld_pid, "killed"); 
        } else {
            perror("waitpid");
        }
    }

}

void handle_stop(pid_t pgid, string cmd){
    if (jobtable.empty()){
        addtotable(pgid, cmd); 
    } else {
        for (auto entry : jobtable){ // really just a hacked solution. You'll understand if you read the code. 
            if (entry.pgid == pgid)
                break;
            else
                addtotable(pgid, cmd); 
        }
    }

    update_status(pgid, "stopped");
}

void handle_kill(int signum){
    update_status(getpid(), "terminated");

    // send the kill signal (SIGKILL) to the pid (only the child should be calling this)
    kill(getpid(), SIGKILL);
}

void update_status(pid_t pgid, string status){
    for (auto entry = jobtable.begin(); entry != jobtable.end(); entry++) {
        job_entry &job = *entry; 
    
        if (job.pgid != pgid){
            continue; 
        } else {
            job.status = status; 
            printf("UPDATE: [%d] %s %s\n", job.pgid, job.cmd.c_str(), job.status.c_str());
        }
    }
}

void addtotable(pid_t pgid, string cmd){
    job_entry job; 
    
    job.pgid = pgid;
    job.cmd = cmd;
    job.status = "running";

    jobtable.push_back(job);     
    
    printf("\nADD: [%d] %s %s\n", job.pgid, job.cmd.c_str(), job.status.c_str());
}

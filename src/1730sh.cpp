#include <string>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <iterator>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#include "BuiltIns.h"

using namespace std;

vector<string> tokenize(string str);
void printprompt();

int main() {
    string line;
    int *pipefds;

    signal(SIGINT, SIG_IGN);
   
    printprompt();
    while (getline(cin, line)) {
        int pipe_count = 0;

        bool append = false;

        string in = "STDIN_FILENO";
        string out = "STDOUT_FILENO";
        string err = "STDERR_FILENO";

        vector<string> tokens = tokenize(line);
        vector<vector<string>> procs; // TODO: Crashes right here after doing piped commands
        vector<string> proc;

        for (auto it = tokens.begin(); it != tokens.end(); it++) {
            if ((it + 1) != tokens.end()) {
                if (*it == "e>") {
                    err = *(it + 1);
                    append = false; 
                    it++;
                } else if (*it == "e>>") {
                    err = *(it + 1);
                    append = true; 
                    it++;
                } else if (*it == ">") {
                    out = *(it + 1);
                    append = false; 
                    it++;
                } else if (*it == ">>") {
                    out = *(it + 1);
                    append = true; 
                    it++;
                } else if (*it == "<") {
                    in = *(it + 1);
                    it++;
                } else if (*it == "|") {
                    if (!proc.empty()) {
                        procs.push_back(proc);
                        proc.clear();
                    }
                    pipe_count++;
                } else {
                    proc.push_back(*it);
                }
            } else {
                proc.push_back(*it);
                procs.push_back(proc);
            }
        }

        // Create all pipes needed
        // Should look like:
        // cmd0   cmd1   cmd2   cmd3
        //    pipe1  pipe2  pipe3
        //    [0,1]  [1,2]  [2,3]
        if (procs.size() > 1){ 
            pipefds = new int[(procs.size() - 1) * 2]; 
            for (unsigned int i = 0; i < ((procs.size() - 1) * 2); i++){
                if (pipe(pipefds + i * 2) < 0){
                    perror("pipe");
                }
            }
        }

        // for each process in procs,
        int status;
        for (size_t i = 0, j = 0; i < procs.size(); i++, j += 2) {
            // Construct argv
            char **argv = new char*[procs[i].size()];
            for (size_t k = 0; k < procs[i].size(); k++) {
                argv[k] = (char *) procs[i][k].c_str();
            }
            // argv must be null-terminated
            argv[procs[i].size()] = nullptr;

            if (strcmp(argv[0], "cd") == 0) {
                cd(argv[1]);
            } else if (strcmp(argv[0], "exit") == 0) {
                if (argv[1] != nullptr) {
                    exit(atoi(argv[1])); 
                } else {
                    // TODO: change this so that it exits with status code of last command
                    exit(EXIT_SUCCESS);
                }
            } else if (strcmp(argv[0], "help") == 0) {
                // TODO: Move this somewhere else, don't need to shove this into a variable everytime 
                const char *help =
                    "bg JID               – Resume the stopped job JID in the background, as if it had been started with &.\n"
                    "cd [PATH]            – Change the current directory to PATH. The environmental variable HOME is the default PATH.\n"
                    "exit [N]             – Cause the shell to exit with a status of N. If N is omitted, the exit status is that of the last job executed.\n"
                    "export NAME[=WORD]   – NAME is automatically included in the environment of subsequently executed jobs.\n" "fg JID               – Resume job JID in the foreground, and make it the current job.\n"
                    "help                 – Display this help message.\n"
                    "jobs                 – List current jobs.\n"
                    "kill [-s SIGNAL] PID – The kill utility sends the specified signal to the specified process or process group PID\n"
                    "                       If no signal is specified, the SIGTERM signal is sent.\n";

                printf(help);
            } else if (strcmp(argv[0], "fg") == 0) {    // TODO: Write foreground cmd
            } else if (strcmp(argv[0], "bg") == 0) {    // TODO: Write background cmd
            } else if (strcmp(argv[0], "jobs") == 0) {  // TODO: Write jobs cmd
            } else if (strcmp(argv[0], "kill") == 0) {  // TODO: Write kill cmd
            } else {
                pid_t pid = fork();
                
                if (pid == 0){
                    // if not the first command 
                    if (j != 0){
                        if (dup2(pipefds[j - 2], STDIN_FILENO) < 0){
                             perror("dup2");
                             // TODO: stop processing
                        }
                    }
                    
                    // if not the last command
                    if (i + 1 < procs.size()){
                        if (dup2(pipefds[j + 1], STDOUT_FILENO) < 0){
                            perror("dup2");
                            // TODO: stop processing 
                        }
                    }
                    
                    // close all pipefds in child 
                    for (unsigned int i = 0; i < ((procs.size() - 1) * 2); i++){
                        close(pipefds[i]); 
                    }

                    // populate arguments with argv
                    if (execvp(procs[i][0].c_str(), argv) == -1){
                        perror(procs[i][0].c_str()); 
                    }
                    delete[] argv;
                    
                    break; // stop loop only in child
                } else if (pid < 0){
                    perror("fork"); 
                } 
            }
        }
        
        // close all pipes when done 
        for (unsigned int i = 0; i < ((procs.size() - 1) * 2); i++){
            close(pipefds[i]); 
        }

        for (unsigned int i = 0; i < procs.size(); i++){
            wait(&status); 
        }

        printprompt();
    }
    delete[] pipefds;
   
    printf("\n");
    return EXIT_SUCCESS;
} 
void printprompt(){
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
    printf("1730sh: ");
    if (strlen(p) >= strlen(homedir)){
        char *rpath = p + strlen(homedir);
        printf("~%s$ ", rpath);   
    } else {
        printf("%s$ ", p); 
    }
}

vector<string> tokenize(string str) {
    vector<string> output;
    
    char prev = '\0';
    
    for (auto it = str.begin(); it != str.end(); it++) {
        // Skip over excessive spaces
        if (*it == ' ' && prev != '\0' && prev != ' ') {
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


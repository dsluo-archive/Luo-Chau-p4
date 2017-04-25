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

#include "BuiltIns.h"

using namespace std;

vector<string> tokenize(string str);
void printprompt();
void close_pipe(int pipefd[2]);

int main() {
    string line;

    signal(SIGINT, SIG_IGN);
   
    printprompt();
    while (getline(cin, line)) {
        string in = "STDIN_FILENO";
        string out = "STDOUT_FILENO";
        string err = "STDERR_FILENO";

        int infd = STDIN_FILENO;
        int outfd = STDOUT_FILENO;
        int errfd = STDERR_FILENO;

        vector<string> tokens = tokenize(line);
        vector<vector<string>> procs;
        vector<string> proc;
        
        for (auto it = tokens.begin(); it != tokens.end(); it++) {
            if ((it + 1) != tokens.end()) {
                if (*it == "e>") { // TODO: Handling error redirection
                    err = *(it + 1);
                     
                    /* if ((errfd = open(err.c_str(), O_WRONLY)) == -1){ */
                    /*     perror("open"); */
                    /*     exit(EXIT_FAILURE); */ 
                    /* } */ 
                    
                    it++;
                } else if (*it == "e>>") { // TODO: Handling error redirection
                    err = *(it + 1);

                    /* if ((errfd = open(err.c_str(), O_WRONLY | O_APPEND)) == -1){ */
                    /*     perror("open"); */
                    /*     exit(EXIT_FAILURE); */ 
                    /* } */ 

                    it++;
                } else if (*it == ">") { // TODO: Handling output redirection
                    out = *(it + 1);
                    
                    /* if ((outfd = open(out.c_str(), O_WRONLY)) == -1){ */
                    /*     perror("open"); */
                    /*     exit(EXIT_FAILURE); */ 
                    /* } */ 
                    
                    it++;
                } else if (*it == ">>") { // TODO: Handling output redirection
                    out = *(it + 1);
                   
                    /* if ((outfd = open(out.c_str(), O_WRONLY | O_APPEND)) == -1){ */
                    /*     perror("open"); */
                    /*     exit(EXIT_FAILURE); */ 
                    /* } */ 
                   
                    it++;
                } else if (*it == "<") { // TODO: Handling input redirection
                    in = *(it + 1);
                  
                    /* if ((infd = open(in.c_str(), O_WRONLY)) == -1){ */
                    /*     perror("open"); */
                    /*     exit(EXIT_FAILURE); */ 
                    /* } */ 
                   
                    it++;
                } else if (*it == "|") {
                    if (!proc.empty()) {
                        procs.push_back(proc);
                        proc.clear();
                    }
                } else {
                    proc.push_back(*it);
                }
            } else {
                proc.push_back(*it);
                procs.push_back(proc);
            }
        }

        pid_t pid;
        vector<array<int, 2>> pipefds;

        // for each process in procs,
        for (size_t i = 0; i < procs.size(); i++) {
            // Construct argv
            char **argv = new char*[procs[i].size()];
            for (size_t k = 0; k < procs[i].size(); k++) {
                argv[k] = (char *) procs[i][k].c_str();
            }
            // argv must be null-terminated
            argv[procs[i].size()] = nullptr;

            // create pipe for command
            if (i != procs.size() - 1){
                int pipefd[2];
                if (pipe(pipefd) == -1){
                    perror("pipe");
                    exit(EXIT_FAILURE); 
                }
                pipefds.push_back({pipefd[0], pipefd[1]});
            } 

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
                // if the first command 
                if (i == 0){
                    // TODO: Handling stdin redirection 
                }
                
                // if the last command
                if (i == procs.size() - 1){
                    // TODO: Handle error redirection 
                    // TODO: Handling stdout redirection
                }
                
                pid = fork();
                
                if (pid == 0){
                    // if not the first command 
                    if (i != 0){
                        if (dup2(pipefds.at(i - 1)[0], STDIN_FILENO) == -1){
                            perror("dup2");
                            exit(EXIT_FAILURE); 
                        }
                    }
                    
                    // if not the last command 
                    if (i != procs.size() -1){
                        if (dup2(pipefds.at(i)[1], STDOUT_FILENO) == -1){
                            perror("dup2");
                            exit(EXIT_FAILURE); 
                        }
                    }

                    for (unsigned int i = 0; i < pipefds.size(); i++){
                        close_pipe(pipefds.at(i).data()); 
                    }

                    // populate arguments with argv
                    if (execvp(procs[i][0].c_str(), argv) == -1){
                        perror(procs[i][0].c_str()); 
                        delete[] argv;
                        
                        exit(EXIT_FAILURE);
                    }
                } else if (pid < 0){
                    perror("fork"); 
                    exit(EXIT_FAILURE);
                } 
            }
        }
        
        /* // close all pipes when done */ 
        for (unsigned int i = 0; i < pipefds.size(); i++){
            close_pipe(pipefds.at(i).data()); 
        }
        
        waitpid(pid, nullptr, 0);

        printprompt();
    }
    
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


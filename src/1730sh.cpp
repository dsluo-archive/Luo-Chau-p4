#include <string>
#include <cstdio>
#include <iostream>
#include <vector>
#include <iterator>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

using namespace std;

char* trimwhitespace(char *str);
vector<string> tokenize(string str);
void printprompt();

int main() {

    string line;
   
    printprompt();
    while (getline(cin, line)) {
        int pipe_count = 0;

        bool append = false;

        string in = "STDIN_FILENO";
        string out = "STDOUT_FILENO";
        string err = "STDERR_FILENO";

        vector<string> tokens = tokenize(line);
        vector<vector<string>> procs;
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
                    pipe_count++;
                } else {
                    proc.push_back(*it);
                }
            } else {
                proc.push_back(*it);
                procs.push_back(proc);
            }
        }
       
        printprompt();
        

        int pid = fork();
        int status;
        if (pid == 0){
            char **args = new char*[procs[0].size()];
            int i = 0;
            for (auto it = procs[0].begin(); it != procs[0].end(); it++){
                args[i] = (char *) (*it).c_str(); 
                i++;
            }
            args[i] = nullptr;
            
            // populate arguments with char **
            if (execvp(procs[0][0].c_str(), args) == -1){
                perror(procs[0][0].c_str()); 
            }
            
            delete[] args;
        } else {
            while (wait(&status) != pid){
                // wait for child to complete 
            }
        }
    }
    printf("\n");
    return EXIT_SUCCESS;
} 
void printprompt(){
    // figure out what the current working directory is. 
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
    if (strlen(p) > strlen(homedir)){
        char *rpath = p + strlen(homedir);
        printf("~%s> ", rpath);   
    } else {
        printf("%s> ", p); 
    }
}

vector<string> tokenize(string str) {
    vector<string> output;
    
    char prev = '\0';

    for (auto it = str.begin(); it != str.end(); it++) {
        if (*it == ' ' && prev != '\0' && prev != ' ') {
            output.push_back({""});
        } else if (*it == '"') {
            while (++it != str.end()) {
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
                //it++;
            }
                 
        } else if (*it != ' ') {
            if (output.empty())
                output.push_back("");
            output.back().push_back(*it);
        }
        prev = *it;
    }

    return output;
}

/**
 * trimwhitespace 
 * @param str string literal to be cleaned of white spaces.
 * @return a string without trailing whitespace (on either end). 
 * Taken from here -> http://gist.github.com/alan-mushi/c8a6f34d1df18574f643/
 */
char* trimwhitespace(char *str){
    char *end;
    while (isspace(*str))
        str++;

    if (*str == 0)
        return str;

    end = str + strnlen(str, 128) - 1;
    while (end > str && isspace(*end))
        end--; 

    *(end + 1) = '\0';
    return str;
}


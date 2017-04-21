#include <string>
#include <cstdio>
#include <iostream>
#include <vector>
#include <iterator>

using namespace std;

vector<string> tokenize(string str) {
    vector<string> output;
    
    char prev = '\0';

    for (auto it = str.begin(); it != str.end(); it++) {
        if (*it == ' ' && prev != '\0' && prev != ' ') {
            output.push_back({""});
        } else if (*it == '"') {
            while (++it != str.end()) {
                if (*it == '\\' && *(it + 1) == '"') {
                    output.back().push_back('"');
                    it++;
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

int main() {

    string line;
    printf("repl$ ");
    while (getline(cin, line)) {

        int pipe_count = 0;

        string in = "STDIN_FILENO";
        string out = "STDOUT_FILENO";
        string err = "STDERR_FILENO";

        vector<string> tokens = tokenize(line);
        vector<vector<string>> procs;
        vector<string> proc;

        for (auto it = tokens.begin(); it != tokens.end(); it++) {
            if ((it + 1) != tokens.end()) {
                if (*it == "e>") {
                 // if (!proc.empty()) {
                 //     procs.push_back(proc);
                 //     proc.clear();
                 // }
                    err = *(it + 1);
                    err += " (truncate)";
                    it++;
                } else if (*it == "e>>") {
                 // if (!proc.empty()) {
                 //     procs.push_back(proc);
                 //     proc.clear();
                 // }
                    err = *(it + 1);
                    err += " (append)";
                    it++;
                } else if (*it == ">") {
                 // if (!proc.empty()) {
                 //     procs.push_back(proc);
                 //     proc.clear();
                 // }
                    out = *(it + 1);
                    out += " (truncate)";
                    it++;
                } else if (*it == ">>") {
                 // if (!proc.empty()) {
                 //     procs.push_back(proc);
                 //     proc.clear();
                 // }
                    out = *(it + 1);
                    out += " (append)";
                    it++;
                } else if (*it == "<") {
                 // if (!proc.empty()) {
                 //     procs.push_back(proc);
                 //     proc.clear();
                 // }
                    in = *(it + 1);
                    it++;
                } else if (*it == "|") {
                 // if (!proc.empty()) {
                 //     procs.push_back(proc);
                 //     proc.clear();
                 // }
                    pipe_count++;
                    //it++;
                } else {
                    proc.push_back(*it);
                }
            } else {
                proc.push_back(*it);
                procs.push_back(proc);
            }
        }

        printf("\n");
        
        printf("Job STDIN  = %s\n", in.c_str());
        printf("Job STDOUT = %s\n", out.c_str());
        printf("Job STDERR = %s\n", err.c_str());

        printf("\n");

        printf("%d pipe(s)\n", pipe_count);
        printf("%lu process(es)\n", procs.size());

        printf("\n");

        for (unsigned long i = 0; i < procs.size(); i++) {
            printf("Process %lu argv:\n", i);
            for (unsigned long j = 0; j < procs[i].size(); j++) {
                printf("%lu: %s\n", j, procs[i][j].c_str());
            }
            if (i + 1 != procs.size())
                printf("\n");
        }

        printf("\n");

        printf("repl$ ");
    }
    printf("\n");
    return EXIT_SUCCESS;
}

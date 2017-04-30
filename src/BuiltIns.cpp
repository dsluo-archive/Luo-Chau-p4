#include <unistd.h>
#include <cstdio>
#include <map>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

#include "BuiltIns.h"

int cd(const char * path) {
    int status = chdir(path);
    if (status == -1) {
        perror("cd");
    }

    return status;
}

int kill(int argc, char **argv) {
    map<string, int> signal_map = {
        {"SIGHUP",    SIGHUP},
        {"SIGINT",    SIGINT},
        {"SIGQUIT",   SIGQUIT},
        {"SIGILL",    SIGILL},
        {"SIGABRT",   SIGABRT},
        {"SIGFPE",    SIGFPE},
        {"SIGKILL",   SIGKILL},
        {"SIGSEGV",   SIGSEGV},
        {"SIGPIPE",   SIGPIPE},
        {"SIGALRM",   SIGALRM},
        {"SIGTERM",   SIGTERM},
        {"SIGUSR1",   SIGUSR1},
        {"SIGUSR2",   SIGUSR2},
        {"SIGCHLD",   SIGCHLD},
        {"SIGCONT",   SIGCONT},
        {"SIGSTOP",   SIGSTOP},
        {"SIGTSTP",   SIGTSTP},
        {"SIGTTIN",   SIGTTIN},
        {"SIGTTOU",   SIGTTOU}, 
        {"SIGBUS",    SIGBUS},
        {"SIGPOLL",   SIGPOLL},
        {"SIGPROF",   SIGPROF},
        {"SIGSYS",    SIGSYS},
        {"SIGTRAP",   SIGTRAP},
        {"SIGURG",    SIGURG},
        {"SIGVTALRM", SIGVTALRM},
        {"SIGXCPU",   SIGXCPU},
        {"SIGXFSZ",   SIGXFSZ},
        {"SIGIOT",    SIGIOT},
        // {"SIGEMT",    SIGEMT},
        {"SIGSTKFLT", SIGSTKFLT},
        {"SIGIO",     SIGIO},
        {"SIGCLD",    SIGCLD},
        {"SIGPWR",    SIGPWR},
        // {"SIGINFO",   SIGINFO},
        // {"SIGLOST",   SIGLOST},
        {"SIGWINCH",  SIGWINCH},
        {"SIGUNUSED", SIGUNUSED}
    };

    int signal = -1;
    pid_t pid;

    char opt;
    while((opt = getopt(argc, argv, "s:")) != -1) {
        switch (opt) {
            case 's':
                try {
                    signal = stoi(optarg);
                } catch (invalid_argument) {
                    signal = signal_map[string(optarg)];
                }
                break;
        }
    }
    
    if (signal == -1)
        signal = SIGTERM;
    
    pid = stoul(argv[optind]);

    return kill(pid, signal);
}

int export_env(int argc, char** argv) {
    if (argc < 2) {
        perror(argv[0]);
        return EXIT_FAILURE;
    }
    const char * name = strtok(argv[1], "=");
    const char * word = strtok(NULL, "=");
    if (name == nullptr || word == nullptr) {
        printf("Usage: export NAME[=WORD]");
        return EXIT_FAILURE;
    }
    if (setenv(name, word, 1) == -1) {
        perror(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;

}

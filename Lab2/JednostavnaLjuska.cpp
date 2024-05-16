#include <iostream>
#include <vector>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

void primanje_sigint(int sig) {
    cout << "\n";
}

void primanje_sigint2(int sig) {
    exit(0);
}

void split_line(vector<string> *args, string line) {
    char *ptr;
    char *dup = strdup(line.c_str());
    ptr = strtok(dup, " ");

    while (ptr != NULL) 
    {
        args->push_back(ptr);
        ptr = strtok(NULL, " ");
    }

    free(ptr);
    free(dup);
    
}

int execute(vector<string> args, string line) {
    if(line == "exit" || line == "") {
        exit(0);
    }
    
    if(args[0] == "cd") { 
        if (args.size() == 1) {
            chdir(getenv("HOME"));
        } else {
            int rc = chdir(args[1].c_str());
            if(rc < 0) {
                stringstream errorMsg;
                errorMsg << "Error: cd to \"" << args[1] << "\" failed";
                perror(errorMsg.str().c_str());
            } 
        }
        return 1;
    }

    pid_t pid = fork();
    
    if (pid == -1) {
        cout << "Pogreška pri kreiranju novog procesa.\n";
        return 1;
    } else {
        if (pid == 0) {

        signal(SIGINT, primanje_sigint2);

        vector<char *> argv;
        for(auto &arg : args) {
            argv.push_back(&arg[0]);
        }

        argv.push_back(nullptr);

        char *envp[] = {nullptr};

        execve(argv[0], argv.data(), envp);
        
        string path = getenv("PATH");
        char *path_str = new char[path.size() + 1];
        strcpy(path_str, path.c_str());
        char *ptr = strtok(path_str, ":");
        while (ptr != NULL) {
            string pom = ptr;
            pom += "/";
            pom += args[0];
            if(execve(pom.c_str(), &argv[0], NULL) < 0 ) {
                ptr = strtok(NULL, ":");
            } else {
                break;
            }
        }
        if(ptr == NULL) {
            stringstream errorMsg;
            errorMsg << "Error";
            perror(errorMsg.str().c_str());
        }

        delete[] path_str;
        }

        int status;
        waitpid(pid, &status, 0);
        return 1;
    }
    return -1;
}

void shell() {
    int status;

    do {

        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        string path(cwd);
        cout << "fsh> " << path << ": ";

        signal(SIGINT, primanje_sigint);

        string line;
        vector<string> args;

        getline(cin, line);
        split_line(&args, line);
        status = execute(args, line);

    } while(status != -1);
}

int main() {

    shell();
    return 0;
}
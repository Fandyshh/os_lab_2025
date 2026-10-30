#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        printf("[CHILD]  Process PID = %d is exiting now...\n", getpid());
        exit(0);
    } else {
        printf("[PARENT] Process PID = %d created child PID = %d\n", getpid(), pid);
        printf("[PARENT] Sleeping for 30 seconds without wait()...\n");
        sleep(30);
        printf("[PARENT] Exiting.\n");
    }

    return 0;
}
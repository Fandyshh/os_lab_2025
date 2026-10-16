#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        // Дочерний процесс замещает свой образ программой sequential_min_max
        char *args[] = {"./sequential_min_max", "42", "100", NULL};
        execv("./sequential_min_max", args);

        // Если execv вернул управление, значит произошла ошибка
        perror("execv failed");
        exit(1);
    } else {
        // Родительский процесс ожидает завершения дочернего
        wait(NULL);
        printf("Child process completed execution.\n");
    }

    return 0;
}
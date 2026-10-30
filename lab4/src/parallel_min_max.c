#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

// Глобальные переменные для обработки сигнала по таймауту
static volatile sig_atomic_t timeout_happened = 0;
static pid_t *child_pids = NULL;
static int total_pnum = 0;

void handle_alarm(int sig) {
  (void)sig;
  timeout_happened = 1;
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = -1; // -1 означает, что таймаут не задан
  bool with_files = false;

  while (true) {
    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"by_files", no_argument, 0, 'f'},
        {"timeout", required_argument, 0, 0},
        {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("seed must be a positive number\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("array_size must be a positive number\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              printf("pnum must be a positive number\n");
              return 1;
            }
            break;
          case 3:
            with_files = true;
            break;
          case 4:
            timeout = atoi(optarg);
            if (timeout <= 0) {
              printf("timeout must be a positive number\n");
              return 1;
            }
            break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;
      case '?':
        break;
      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--by_files] [--timeout \"num\"]\n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  total_pnum = pnum;
  child_pids = malloc(sizeof(pid_t) * pnum);

  int (*pipefd)[2] = NULL;
  if (!with_files) {
    pipefd = malloc(sizeof(int[2]) * pnum);
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipefd[i]) == -1) {
        perror("pipe failed");
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  int chunk_size = array_size / pnum;

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      if (child_pid == 0) {
        // Дочерний процесс
        unsigned int begin = i * chunk_size;
        unsigned int end = (i == pnum - 1) ? array_size : (i + 1) * chunk_size;

        struct MinMax local_min_max = GetMinMax(array, begin, end);

        if (with_files) {
          char filename[64];
          sprintf(filename, "result_%d.txt", i);
          FILE *fp = fopen(filename, "w");
          if (fp != NULL) {
            fprintf(fp, "%d %d\n", local_min_max.min, local_min_max.max);
            fclose(fp);
          } else {
            perror("fopen failed in child");
            return 1;
          }
        } else {
          close(pipefd[i][0]);
          write(pipefd[i][1], &local_min_max.min, sizeof(int));
          write(pipefd[i][1], &local_min_max.max, sizeof(int));
          close(pipefd[i][1]);
        }

        free(array);
        free(child_pids);
        if (!with_files) free(pipefd);
        return 0;
      } else {
        // Родительский процесс запоминает PID потомка
        child_pids[i] = child_pid;
      }
    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // Настройка таймера при наличии timeout
  if (timeout > 0) {
    signal(SIGALRM, handle_alarm);
    alarm(timeout);
  }

  // Ожидание завершения дочерних процессов
  int active_children = pnum;
  while (active_children > 0) {
    if (timeout_happened) {
      printf("Timeout reached! Killing remaining child processes...\n");
      for (int i = 0; i < pnum; i++) {
        if (child_pids[i] > 0) {
          kill(child_pids[i], SIGKILL);
        }
      }
      break;
    }

    if (timeout > 0) {
      // Неблокирующий опрос процессов
      for (int i = 0; i < pnum; i++) {
        if (child_pids[i] > 0) {
          int status;
          pid_t result = waitpid(child_pids[i], &status, WNOHANG);
          if (result > 0) {
            child_pids[i] = 0; // процесс успешно завершился
            active_children--;
          }
        }
      }
      usleep(1000); // пауза 1 мс, чтобы не нагружать процессор постоянным опросом
    } else {
      // Стандартный блокирующий режим, если таймаут не задан
      wait(NULL);
      active_children--;
    }
  }

  // Если сработал таймаут, собираем зомби-процессы после kill и выходим с ошибкой
  if (timeout_happened) {
    while (wait(NULL) > 0);
    free(array);
    free(child_pids);
    if (!with_files) free(pipefd);
    printf("Process execution was aborted due to timeout.\n");
    return 1;
  }

  // Агрегация результатов родителем
  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      char filename[64];
      sprintf(filename, "result_%d.txt", i);
      FILE *fp = fopen(filename, "r");
      if (fp != NULL) {
        fscanf(fp, "%d %d", &min, &max);
        fclose(fp);
        remove(filename);
      } else {
        perror("fopen failed in parent");
      }
    } else {
      close(pipefd[i][1]);
      read(pipefd[i][0], &min, sizeof(int));
      read(pipefd[i][0], &max, sizeof(int));
      close(pipefd[i][0]);
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  free(child_pids);
  if (!with_files) free(pipefd);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
} 
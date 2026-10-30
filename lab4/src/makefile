CC = gcc
CFLAGS = -I. -Wall
LDFLAGS = -pthread

all: parallel_min_max process_memory zombie psum

# Задание 1
parallel_min_max: utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

# Задание 2
zombie: zombie.c
	$(CC) -o zombie zombie.c $(CFLAGS)

# Задание 3
process_memory: process_memory.c
	$(CC) -o process_memory process_memory.c $(CFLAGS)

# Задание 5: библиотека и программа суммирования
sum_lib.o: sum_lib.c sum_lib.h
	$(CC) -o sum_lib.o -c sum_lib.c $(CFLAGS)

psum: parallel_sum.c sum_lib.o utils.o
	$(CC) -o psum parallel_sum.c sum_lib.o utils.o $(CFLAGS) $(LDFLAGS)

# Вспомогательные модули
utils.o: utils.c utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

find_min_max.o: find_min_max.c find_min_max.h utils.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

clean:
	rm -f utils.o find_min_max.o sum_lib.o parallel_min_max process_memory zombie psum result_*.txt


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <string.h>
#include <iostream>
#include <fstream> 

#define SHM_SIZE 512

union semun {
  int val;                    
  struct semid_ds *buf;       
  unsigned short int *array;  
  struct seminfo *__buf;      
};

int shmid, semid;
char *adrshm;

// вывод ошибок
void sce(char *msg)
{
  perror(msg);
  exit(EXIT_FAILURE);
}

// получение ресурсов от семафора
void P(int semid, int index)
{
  int ret;
  struct sembuf op;

  op.sem_num = index;
  op.sem_op = -1;
  op.sem_flg = 0;
  if ((ret = semop(semid, &op, 1)) == -1) {
    sce("semop(), P()");
  }
}

// возврат ресурсов семафору
void V(int semid, int index)
{
  int ret;
  struct sembuf op;

  op.sem_num = index;
  op.sem_op = +1;
  op.sem_flg = 0;
  if ((ret = semop(semid, &op, 1)) == -1) {
    sce("semop(), V()");
  }
}

// инициализация семафора
void Init(int semid, int index, int value)
{
  int ret;
  union semun arg;

  arg.val = 1;
  if ((ret = semctl(semid, index, SETVAL, value)) == -1) {
    sce("semctl(), SETVAL");
  }
}

// клиент
void client(char* file_old, char* file_new)
{
  /* запись имени нового файла в область разделяемой памяти */
  P(semid, 0);
  sprintf((char*) adrshm, "%s", file_new);
  V(semid, 0);
  sleep(10);

  /* открытие исходного файла */
  std::fstream fRead;
  fRead.open(file_old, std::fstream::in);

  /* обработка файла */
  if (fRead.fail()) {
    fRead.close();
  } else {
    /* буффер для чтения данных */
    char character[SHM_SIZE];
      
    while(1) {
      /* чтение сегмента файла в буффер */
      fRead.read(character, SHM_SIZE);
      P(semid, 0);
      /* запись данных в сегмент разделяемой памяти */
      sprintf((char*) adrshm, "%s", character);
      V(semid, 0);
      sleep(1);
      /* обработка "end-of-file" */
      if (fRead.eof()) {
        //sprintf((char*) adrshm, "%s", file_new);
	/* запись сообщения о конце файла с сегмент разделяемой памяти */
        char* end = "end";
        P(semid, 0);
        sprintf((char*) adrshm, "%s", end);
        V(semid, 0);
        sleep(1);
        break;
      }
    }
    /* закрытие файла */
    fRead.close();
  }
  _exit(0);
}

// сервер
void server(void)
{
  /* создание нового файла */
  P(semid, 0);
  std::ofstream outfile (adrshm);
  V(semid, 0);
  /* запись данных из сегмента разделяемой памяти в файл (до получения сообщения о конце файла) */
  while(!strcmp(adrshm, "end")) {
    P(semid, 0);
    outfile << adrshm;
    V(semid, 0);
    sleep(1);
  }
  /* закрытие файла */
  outfile.close();
  _exit(0);
}


int main(int argc, char *argv[])
{
  int ret;
  pid_t son1, son2;

  /* проверка параметров */
  if (argc != 3) {
    fprintf(stderr, "Arguments error.\n");
    _exit(1);
  }

  /* создание семафора */
  if ((semid = semget(IPC_PRIVATE, 1, IPC_CREAT|0600)) == -1) {
    sce("semget");
  } 
  /* инициализация семафора */
  Init(semid, 0, 1);

  /* создание сегмента разделяемой памяти */
  if ((shmid = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT|0600)) == -1) {
    sce("shmget");
  }
  /* создание указателя на сегмент разделяемой памяти в виртуальном адресном пространстве */
  if ((adrshm = (char*) shmat(shmid, NULL, 0)) == (void*) -1) {
    sce("shmat");
  }

  /* создание клиентского и серверного потоков */

  /* создание потока */
  if (( son1 = fork()) == -1) {
    sce("1st fork()");
  }
  if (son1 == 0) {
    client(argv[1], argv[2]);
  }

  if (( son2 = fork()) == -1) {
    sce("2nd fork()");
  }
  if (son2 == 0) {
    server();
  }

  /* выход из приложения по нажатию клавиши 'q' */
  while( (ret = getchar()) != 'q')
    ;

  /* очистка памяти */

  /* удаление дочерних процессов */
  if ((ret = kill(son1, SIGKILL)) == -1) {
    perror("kill() 1st son");
  }
  if ((ret = kill(son2, SIGKILL)) == -1) {
    perror("kill() 2nd son");
  }
  /* удаление ссылки и сегмента разделяемой памяти */
  if ((ret = shmdt(adrshm)) == -1) {
    perror("shmdt()");
  }
  if ((ret = shmctl(shmid, IPC_RMID, NULL)) == -1) {
    perror("shmctl(), IPC_RMID");
  }
  /* удаление семафора */
  if ((ret = semctl(semid, 0, IPC_RMID)) == -1) {
    perror("semctl(), IPC_RMID");
  }

  printf("Father ends.\n");
  exit(EXIT_SUCCESS);
}




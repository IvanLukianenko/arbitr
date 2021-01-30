/* 
Клиент должен:
1. зарегистрироваться в сети через очередь для регистрации формат #ИМЯ
2. посылать сообщения в формате ИМЯ#CООБЩЕНИЕ
3. выйти из сети командй ##
*/
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#define MESSAGE_LENGTH 255
#define ARBITR_TYPE 21

char self_name[20];
char message[MESSAGE_LENGTH];
int msqid, msqid1, msqid2, msqid3;

key_t  key, key1, key2, key3;

struct mymsgbuf
{
  long mtype;
  char mtext[MESSAGE_LENGTH];
} mybuf3;


struct mymsgbuf mybuf;
struct mymsgbuf mybuf1;
struct mymsgbuf mybuf2;

int self_id;
int temp_id;
int len;


pthread_t thr1, thr2, thr3;

pid_t chpid;

key = 4;
key1 = 1;
key2 = 2;
key3 = 3;

void* thread1(void * thread_data){
  while(1){
      if ((len = msgrcv(msqid, (struct msgbuf *) &mybuf, MESSAGE_LENGTH, self_id, 0 )) < 0){
        printf("Can\'t receive message from queue\n");
      }
      else{
        printf("Вам пришло сообщение: %s\n", mybuf.mtext);
      }
    }
}

void* thread2(void * thread_data){
  mybuf3.mtype = ARBITR_TYPE;
  sprintf(mybuf3.mtext, "%d", self_id);
  while(1){
      if (scanf("%s", message)){
        if (strcmp(message, "##") == 0){
          break;
        }
        if (msgsnd(msqid3, (struct msgbuf *) &mybuf3, MESSAGE_LENGTH, 0) < 0){
          printf("Can\'t send message to queue");
        }
        mybuf.mtype = ARBITR_TYPE;
        strcpy(mybuf.mtext, message);
        if (msgsnd(msqid, (struct msgbuf *) &mybuf, MESSAGE_LENGTH, 0) < 0){
          printf("Can\'t send message to queue");
        }
      }
  }
}


void* thread3(void * thread_data){
  mybuf3.mtype = ARBITR_TYPE;
  sprintf(mybuf3.mtext, "%d", self_id);
  while (1){
    if (msgsnd(msqid3, (struct msgbuf *) &mybuf3, MESSAGE_LENGTH, 0) < 0){
      printf("Can\'t send message to queue");
    }
    sleep(10);
  };
}
int main(){
  // подключение к очереди для сообщений
  if ((msqid = msgget(key, 0666 | IPC_CREAT)) < 0)
  {
    printf("msget error");
    return -1;
  }
  // подключение к очереди для регистрации
  if ((msqid1 = msgget(key1, 0666 | IPC_CREAT)) < 0)
  {
    printf("msget error");
    return -1;
  }
  // id 
  if ((msqid2 = msgget(key2, 0666 | IPC_CREAT)) < 0)
  {
    printf("msget error");
    return -1;
  }
  // online
  if ((msqid3 = msgget(key3, 0666 | IPC_CREAT)) < 0)
  {
    printf("msget error");
    return -1;
  }
  
  printf("Введите имя в формате #NAME для регистрации в сети: ");
  scanf("%s", self_name);

  mybuf1.mtype = ARBITR_TYPE;
  strcpy(mybuf1.mtext, self_name);

  if (msgsnd(msqid1, (struct msgbuf *) &mybuf1, MESSAGE_LENGTH, 0) < 0){
    printf("Can\'t send message to queue");
  }

  temp_id = strlen(self_name);

  if ((len = msgrcv(msqid2, (struct msgbuf *) &mybuf2, MESSAGE_LENGTH, temp_id, 0 )) < 0){
    printf("Can\'t receive message from queue\n");
  }

  self_id = atoi(mybuf2.mtext);
  printf("Ваш идентификатор в сети: %d\n", self_id);
  
  pthread_create(&thr1, NULL, &thread1, NULL);
  pthread_create(&thr2, NULL, &thread2, NULL);
  pthread_create(&thr3, NULL, &thread3, NULL);

  pthread_join(thr2, NULL);
  
  
  return 0;
}

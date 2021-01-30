/*
Это арбитр очереди он:
1. запустит потоки для обработки пользователей сети.
2. будет регистрировать каждого подключившегося абонента к очереди:
  2.1. Пользователь присылает свое имя, по этому имени вычисляется индентификатор, который записывается в табличку
3. При передачи сообщения, пользователь указывает имя абонента и посылаемое ему сообщение, арбитр вычисляет его идентификтор и если этот пользователь в сети, то он получит идентификатор и отправит адресованное ему сообщение.
4. Проверяет онлайн, каждый клиент присылает каждые 10 секунд сообщение, арбитр обновляет таблицу времени в сети.
5. Если пользователь дольше 20 секунд не присылал уведомление об онлайне, то он удаляется из таблицы пользователей онлайн.
*/
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>


#define MESSAGE_LENGTH 255
#define ARBITR_TYPE 21

char* message1;
char* name1;
int id_to_send;

key_t  key, key1, key2, key3;
int maxlen;
pid_t chpid;
char list_of_clients[10][20];
char name[20];
char name2[20];
char message[MESSAGE_LENGTH];
key = 4;
key1 = 1;
key2 = 2;
key3 = 3;
int clients;
clients = 1;
struct mymsgbuf
{
  long mtype;
  char mtext[MESSAGE_LENGTH];
} mybuf;
struct mymsgbuf mybuf1;
struct mymsgbuf mybuf2;
struct mymsgbuf mybuf3;
int msqid, msqid1, msqid2, msqid3;
int temp_id;
char num[3];

char list_of_clients[10][20];
int list_of_clients_online[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int len;
char sep[3] = "#";

struct sembuf mybuf_for_take = {0, -1, 0};	// для захвата семафора
struct sembuf mybuf_for_giveup = {0, 1, 0}; // для отпускание семафора

union semun {
	int val;
	struct semid_ds * buf;
	unsigned short *array;

	struct seminfo *_buf;
};

int semid;
union semun arg;

void squeeze (char s[]) {
  int k;
  int n = strlen(s);
  for(k = 0; k < n-1; k++){
    s[k] = s[k+1];
  }
  s[n-1] = '\0';
}
int findId(char name[]){
  int k; 
  for (k = 0; k < 10; k++){
    if(strcmp(name, list_of_clients[k]) == 0){
      return k;
    }
  }
  return -1;
}
void parseName(char s[]){
  int k = 0;
  strcpy(name1, " ");
  while (s[k] != '#'){
    name1[k] = s[k];
    k++;
  }
  name1[k]='\0';
}
void parseMessage(char s[]){
  int k = 0;
  strcpy(message1, " ");
  while(s[k] != '#'){
    k++;
  }
  k++;
  int i = 0;
  while(s[k]!='\0'){
    message1[i] = s[k];
    i++;
    k++;
  }
  message[k] = '\0';
}
int id_to_give;

int find_free_id(){
  for(int j = 1; j < 10; j++){
    if(strcmp(list_of_clients[j], " ") == 0)
    return j;
  }
};


void * thread1(void* thread_data){
    
    /*Поток, который обрабатывает регистрации*/
    while(1){
      if ((len = msgrcv(msqid1, (struct msgbuf *) &mybuf1, MESSAGE_LENGTH, ARBITR_TYPE, 0 )) < 0){
        printf("Can\'t receive message from queue\n");
        break;
      }
      else{
        strcpy(name, mybuf1.mtext);
        if(name[0] == '#'){
          semop(semid, &mybuf_for_take, 1);
          if (clients == 1){
            printf("А у нас тут первый клиент, добро пожаловать, ");
          }
          else
            printf("Ура! У нас новый клиент ");
          squeeze(name);
          id_to_give = find_free_id();
          strcpy(list_of_clients[id_to_give], name);
          //sleep(1); 
          printf("%s\n", list_of_clients[id_to_give]);
          temp_id = strlen(name)+1;
          printf("Выданный идентификатор: %d\n", id_to_give);
          clients++;
          list_of_clients_online[id_to_give] = time(NULL);
          semop(semid, &mybuf_for_giveup, 1);
          mybuf2.mtype = temp_id;
          sprintf(mybuf2.mtext, "%d", id_to_give);
        
          if (msgsnd(msqid2, (struct msgbuf *) &mybuf2, MESSAGE_LENGTH, 0) < 0){
            printf("Can\'t send message to reg queue\n");
          }
        }
      }
    }
}

void *thread2(void * thread_data){
    
    /* Поток, который обрабатывает передачи сообщений */
    
    while(1){

      if ((len = msgrcv(msqid, (struct msgbuf *) &mybuf, MESSAGE_LENGTH, ARBITR_TYPE, 0 )) < 0){

        printf("Can\'t receive message from queue\n");
        break;
      }
      else{
        name1 = strtok(mybuf.mtext, sep);
        message1 = strtok(NULL, sep);
        id_to_send = findId(name1);
        mybuf.mtype = id_to_send;
        strcpy(mybuf.mtext, message1);
        if(id_to_send >= 0){
          if (msgsnd(msqid, (struct msgbuf *) &mybuf, MESSAGE_LENGTH, 0) < 0){
            printf("Can\'t send message to message queue\n");
          }
        }
        else{
          printf("Данный абонент не в сети\n");
        }
      }
    }

}

int online_id;

void * thread3(void* thread_data){
  
  /* Поток для проверки онлайна */
  
  while(1){
    if((len = msgrcv(msqid3, (struct msgbuf *) &mybuf3, MESSAGE_LENGTH, ARBITR_TYPE, 0 )) < 0){
      printf("Can\'t receive message from queue\n");
      break;
    }
    else{
      semop(semid, &mybuf_for_take, 1);
      online_id = atoi(mybuf3.mtext);
      list_of_clients_online[online_id] = time(NULL);
      for (int i = 0; i < 10; i++){
        if (time(NULL) - list_of_clients_online[i] > 20 ){
          strcpy(list_of_clients[i], " ");
          clients--;
        }
      }
      semop(semid, &mybuf_for_giveup, 1);
    }
  }  
}

int main(){
  pthread_t thr1, thr2, thr3;
  // Cоздание очереди для обработки сообщений
  if ((msqid = msgget(key, 0666 | IPC_CREAT)) < 0)
  {
    printf("msget error");
    return -1;
  }
  // Создание очереди для обработки регистраций
  if ((msqid1 = msgget(key1, 0666 | IPC_CREAT)) < 0)
  {
    printf("msget error");
    return -1;
  }

  // Создание очереди для рассылки идентификаторов
  if ((msqid2 = msgget(key2, 0666 | IPC_CREAT)) < 0)
  {
    printf("msget error");
  }
  // Очередь для проверки онлайна
  if ((msqid3 = msgget(key3, 0666 | IPC_CREAT)) < 0)
  {
    printf("msget error");
    return -1;
  }
  for(int j=0;j<10;j++){
    strcpy(list_of_clients[j], " ");
  } 
  
  semid = semget(key, 1, 0666 | IPC_CREAT);  
  arg.val = 1;
  
  semctl(semid, 0, SETVAL, arg);
  printf("Анонимная социальная сеть \'Anon.Net\' запущена! \n");
  pthread_create(&thr1, NULL, &thread1, NULL);
  pthread_create(&thr2, NULL, &thread2, NULL);
  pthread_create(&thr3, NULL, &thread3, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);
  
  return 0;
}


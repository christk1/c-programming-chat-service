// Client side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#define PORT 8888
void sig_handler(int signo);//catch ctrl-c
void *writefunc(void *ptr);
void *readfunc(void *ptr);

int main(int argc, char const *argv[])
{
    struct sockaddr_in address;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char *hello = "Hello from client";
    char buffer[1024] = {0};
    char message[1024] = {0};
    char username[256];
    char ubuf[256];
    char email[256];
    char password[256];
    char passrepeat[256];
    char groupname[256];
    char gid[10];
    char option[10];
    bzero(option,10);
    int pnum=8888;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    valread = read( sock , buffer, 1023);
    printf("%s\n",buffer );
    while (1) {

      //signal handling
      if (signal(SIGINT, sig_handler) == SIG_ERR){
         printf("\ncan't catch SIGINT\n");
      }
      if (signal(SIGUSR1, sig_handler) == SIG_ERR){
         printf("\ncan't catch SIGUSR1\n");
      }
      if (signal(SIGTSTP, sig_handler) == SIG_ERR){
         printf("\ncan't catch SIGTSTP\n");
      }
      //signal handling_end
      do {
        printf("1. Login\n2. Sign up\n");
        scanf("%s",option);
        while ((getchar()) != '\n');
        if (strcmp(option,"exit***")==0) {
           valread = write(sock, option, strlen(option));
           exit(1);
        }
      } while(strcmp(option,"1")!=0 && strcmp(option,"2")!=0);
      if (strcmp(option,"1")==0) {//Login
        do {
          printf("give username:\n");
          scanf("%s",username );
          while ((getchar()) != '\n');
        } while(strchr(username,'-')!=NULL);
        if (strcmp(username,"exit***")==0) {
           valread = write(sock, username, strlen(username));
           exit(1);
        }
        do {
          printf("give password:\n");
          scanf("%s",password );
          while ((getchar()) != '\n');
        } while(strchr(password,'-')!=NULL);
        if (strcmp(password,"exit***")==0) {
           valread = write(sock, password, strlen(password));
           exit(1);
        }
        bzero(buffer,1024);
        strcpy(buffer,"1");
        strcat(buffer,"-");
        strcat(buffer,username);
        strcat(buffer,"-");
        strcat(buffer,password);

        send(sock , buffer , strlen(buffer) , 0 );
        bzero(buffer,1024);
        valread = read( sock , buffer, 1023);
        if (strcmp(buffer,"continue##")==0) {
          break;
        }
        printf("User doesnt exist or already logged in.\n");
        bzero(option,10);
      }//end login
      if (strcmp(option,"2")==0) {//sign up
        do {
          do {
            printf("give username:\n");
            scanf("%s",username );
            while ((getchar()) != '\n');
          } while(strchr(username,'-')!=NULL);
          if (strcmp(username,"exit***")==0) {
             valread = write(sock, username, strlen(username));
             exit(1);
          }
          do {
            printf("give email:\n");
            scanf("%s",email );
            while ((getchar()) != '\n');
          } while(strchr(email,'-')!=NULL);
          if (strcmp(email,"exit***")==0) {
             valread = write(sock, email, strlen(email));
             exit(1);
          }
          do {
            printf("give password:\n");
            scanf("%s",password );
          } while(strchr(password,'-')!=NULL);
          if (strcmp(password,"exit***")==0) {
             valread = write(sock, password, strlen(password));
             exit(1);
          }
          printf("give password again:\n");
          scanf("%s",passrepeat );
          while ((getchar()) != '\n');
          if (strcmp(passrepeat,"exit***")==0) {
             valread = write(sock, passrepeat, strlen(passrepeat));
             exit(1);
          }
        } while(strcmp(password,passrepeat)!=0);

        bzero(buffer,1024);
        strcpy(buffer,"2");
        strcat(buffer,"-");
        strcat(buffer,username);
        strcat(buffer,"-");
        strcat(buffer,password);
        strcat(buffer,"-");
        strcat(buffer,email);
        printf("%s\n",buffer );

        send(sock , buffer , strlen(buffer) , 0 );
        bzero(buffer,1024);
        valread = read( sock , buffer, 1023);
        if (strcmp(buffer,"continue##")==0) {
          break;
        }
        else if (strcmp(buffer,"repeat###")==0) {
          printf("User already logged in.\n");
        }
        else{
          printf("User exists.\n");
        }
        bzero(option,10);
      }//end_sign up
    }
    printf("user logged in\n");
    do {
      send(sock , "3-k", 3 , 0 );
      bzero(buffer,1024);
      valread = read( sock , buffer, 1023);
      printf("%s\n",buffer );
      do {
        bzero(option,10);
        scanf("%s",option );
        while ((getchar()) != '\n');
        if (strstr(option,"exit***")!=NULL) {
           bzero(message,1024);
           strcpy(message,username);
           strcat(message,"-");
           strcat(message,"exit***");
           valread = write(sock, message, strlen(message));
           exit(1);
        }
      } while(strcmp(option,"1")!=0 && strcmp(option,"2")!=0 && strcmp(option,"3")!=0 && strcmp(option,"4")!=0 && strcmp(option,"5")!=0);
      if (strcmp(option,"1")==0) {//create group
        bzero(option,10);
        strcpy(option,"4");
        do {
          printf("Give group name:\n");
          scanf("%s",groupname );
          while ((getchar()) != '\n');
        } while(strchr(groupname,'-')!=NULL);
        if (strstr(groupname,"exit***")!=NULL) {
           bzero(message,1024);
           strcpy(message,username);
           strcat(message,"-");
           strcat(message,"exit***");
           valread = write(sock, message, strlen(message));
           exit(1);
        }
        bzero(buffer,1024);
        strcpy(buffer,option);
        strcat(buffer,"-");
        strcat(buffer,groupname);
        strcat(buffer,"-");
        strcat(buffer,username);

        send(sock , buffer, strlen(buffer) , 0 );
        bzero(buffer,1024);
        valread = read( sock , buffer, 1023);
        printf("%s\n",buffer );
        bzero(option,10);
      }//create group_end
      if(strcmp(option,"2")==0){//join group
        bzero(option,10);
        strcpy(option,"5");
        printf("Give group id:\n");
        scanf("%s",groupname );
        while ((getchar()) != '\n');
        if (strstr(groupname,"exit***")!=NULL) {
           bzero(message,1024);
           strcpy(message,username);
           strcat(message,"-");
           strcat(message,"exit***");
           valread = write(sock, message, strlen(message));
           exit(1);
        }
        bzero(buffer,1024);
        strcpy(buffer,option);
        strcat(buffer,"-");
        strcat(buffer,groupname);
        strcat(buffer,"-");
        strcat(buffer,username);

        send(sock , buffer, strlen(buffer) , 0 );
        bzero(buffer,1024);
        valread = read( sock , buffer, 1023);
        printf("%s\n",buffer );
        bzero(option,10);
      }//join group_end
      if (strcmp(option,"3")==0) {//delete group
        bzero(option,10);
        strcpy(option,"6");
        bzero(buffer,1024);
        strcpy(buffer,option);
        do {
          printf("Give group id:\n");
          scanf("%s",groupname );
          while ((getchar()) != '\n');
        } while(strchr(groupname,'-')!=NULL);
        if (strstr(groupname,"exit***")!=NULL) {
           bzero(message,1024);
           strcpy(message,username);
           strcat(message,"-");
           strcat(message,"exit***");
           valread = write(sock, message, strlen(message));
           exit(1);
        }
        strcat(buffer,"-");
        strcat(buffer,groupname);
        strcat(buffer,"-");
        strcat(buffer,username);
        send(sock , buffer, strlen(buffer) , 0 );
        bzero(buffer,1024);
        valread = read( sock , buffer, 1023);
        printf("%s\n",buffer );
        bzero(option,10);
      }//delete group_end
      if (strcmp(option,"5")==0) {//join a server
        bzero(option,10);
        strcpy(option,"8");
        bzero(buffer,1024);
        strcpy(buffer,option);
        strcat(buffer,"-");
        strcat(buffer,username);
        send(sock , buffer, strlen(buffer) , 0 );
        bzero(buffer,1024);
        valread = read( sock , buffer, 1023);
        printf("Group IDs in which user is enrolled:\n%s\n",buffer );
        do {
          printf("give group id to connect to:\n");
          scanf("%s",gid );
          while ((getchar()) != '\n');
        } while(strchr(gid,'-')!=NULL);
        if (strstr(gid,"exit***")!=NULL) {
           bzero(message,1024);
           strcpy(message,username);
           strcat(message,"-");
           strcat(message,"exit***");
           valread = write(sock, message, strlen(message));
           exit(1);
        }
        bzero(buffer,1024);
        strcpy(buffer,"9");
        strcat(buffer,"-");
        strcat(buffer,gid);
        strcat(buffer,"-");
        strcat(buffer,username);
        send(sock , buffer, strlen(buffer) , 0 );
        bzero(buffer,1024);
        valread = read( sock , buffer, 1023);
        //printf("%s\n",buffer );
        bzero(option,10);
        if (strcmp(buffer,"User enrolled please join!")==0) {
          valread = write(sock, "exit***", 7);//exit from main server
          close(sock);
          break;
        }
      }//join a server_end
    } while(strcmp(option,"4")!=0);
    if (strcmp(option,"4")==0) {//delete account
      bzero(option,10);
      strcpy(option,"7");
      do {
        printf("Are you sure you want to delete this account?(YES/NO)\n");
        bzero(buffer,1024);
        scanf("%s",buffer );
        while ((getchar()) != '\n');
      } while(strcmp(buffer,"YES")!=0 && strcmp(buffer,"NO")!=0);
      if (strcmp(buffer,"YES")==0) {
        bzero(buffer,1024);
        strcpy(buffer,option);
        strcat(buffer,"-");
        strcat(buffer,username);
        send(sock , buffer, strlen(buffer) , 0 );
        bzero(buffer,1024);
        valread = read( sock , buffer, 1023);
        printf("%s\n",buffer );
      }
      bzero(message,1024);
      strcat(message,username);
      strcat(message,"-");
      strcat(message,"exit***");
      valread = write(sock, message, strlen(message));
      exit(1);
    }//delete account_end
    while(1){
      if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      {
          printf("\n Socket creation error \n");
          return -1;
      }

      memset(&serv_addr, '0', sizeof(serv_addr));
      pnum++;
      //printf("pnum:%d\n",pnum);

      serv_addr.sin_family = AF_INET;
      serv_addr.sin_port = htons(pnum);

      // Convert IPv4 and IPv6 addresses from text to binary form
      if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
      {
          printf("\nInvalid address/ Address not supported \n");
          return -1;
      }

      if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
      {
          printf("\nConnection Failed \n");
          return -1;
      }
      bzero(buffer,1024);
      valread = read( sock , buffer, 1024);
      printf("%s\n",buffer );
      bzero(buffer,1024);
      strcpy(buffer,username);
      strcat(buffer,"-");
      strcat(buffer,gid);
      //printf("buffer:%s\n",buffer);
      send(sock ,buffer , strlen(buffer) , 0 );//send info
      bzero(buffer,1024);
      valread = read( sock , buffer, 1024);//wait for join
      //printf("%s\n",buffer );
      if (strcmp(buffer,"join!")==0) {
        break;
      }
      else{
        valread = write(sock, "exit***", 7);
      }
    }
    //code

    pthread_t wrth,reth;
    int ret1,ret2;
    bzero(buffer,1024);

    sprintf(buffer,"%d",sock);
    strcat(buffer,"-");
    strcat(buffer,username);
    strcat(buffer,"-");
    strcat(buffer,gid);
    char ttt[20],ttt1[20];
    strcpy(ttt,buffer);
    strcpy(ttt1,buffer);
    char* ddd = &ttt[0];
    char* ddd1 = &ttt1[0];
    ret1=pthread_create(&wrth,NULL,writefunc,(void*)ddd);

    if(ret1)
    {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",ret1);
        exit(EXIT_FAILURE);
    }
    ret2=pthread_create(&reth,NULL,readfunc,(void*)ddd1);
    if(ret2)
    {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",ret2);
        exit(EXIT_FAILURE);
    }
    pthread_join(wrth,NULL);
    pthread_join(reth,NULL);
    return 0;
}

void sig_handler(int signo)//catch ctrl-c
{
  if (signo == SIGINT){
    printf("\nPlease type: exit***\n");
    printf("write something:\n");
  }
  else if (signo == SIGUSR1){
    //printf("received SIGUSR1\n");
    printf("\nPlease type: exit***\n");
    printf("write something:\n");
  }
  else if (signo == SIGTSTP){
    //printf("\nreceived SIGTSTP\n");
    printf("\nPlease type: exit***\n");
    printf("write something:\n");
  }
}

void *writefunc(void *ptr){
  char message[500];
  char username[256];
  char sendtousr[256];
  char gid[10];
  char socke[10];
  int sock;
  const char s[2] = "-";
  char *token;
  char *junk;
  char *ret;
  int valread;
  char buffer[1024];
  bzero(buffer,1024);
  char * buff;
  buff = (char *) ptr;
  printf("here i stack:%s\n",buff );
  token=strtok(buff,s);
  strcpy(socke,token);
  token=strtok(NULL,s);
  strcpy(username,token);
  token=strtok(NULL,s);
  strcpy(gid,token);

  sock=strtol(socke,&junk,10);//convert string to int
  printf("start conversation:\n");
  while (1) {
    scanf(" %[^\n]s", message);
    while ((getchar()) != '\n');
    //printf("sent:%s\n",message );-------------------------------
    if (strchr(message,'-')!=NULL) {
      printf("'-' character not permitted\n");
      continue;
    }
    if (strlen(message)==0) {
      continue;
    }
    if (message[0]=='@' && strchr(message,' ')!=NULL) {
      char mesbup[500];
      bzero(mesbup,500);
      strcpy(mesbup,message);
      token=strtok(message," ");
      strcpy(sendtousr,token);
      ret=strchr(mesbup,' ');
      int idxToDel = 0;
      memmove(&ret[idxToDel], &ret[idxToDel + 1], strlen(ret) - idxToDel);
      printf("ret:%s\n",ret );
      strcpy(message,ret);
      printf("user:%s\n",sendtousr );
      printf("message:%s\n",message );
      idxToDel = 0;
      memmove(&sendtousr[idxToDel], &sendtousr[idxToDel + 1], strlen(sendtousr) - idxToDel);
      bzero(buffer,1024);
      strcpy(buffer,"@");
      strcat(buffer,sendtousr);
      strcat(buffer,"-");
      strcat(buffer,username);
      strcat(buffer,"-");
      strcat(buffer,message);
      send(sock , buffer , strlen(buffer) , 0 );
    }
    else{
      bzero(buffer,1024);
      strcpy(buffer,username);
      strcat(buffer,"-");
      strcat(buffer,gid);
      strcat(buffer,"-");
      strcat(buffer,message);
      if (strstr(message,"exit***")!=NULL) {
         valread = write(sock, buffer, strlen(buffer));
         exit(1);
      }
      send(sock , buffer , strlen(buffer) , 0 );
    }
  }
}
void *readfunc(void *ptr){
  char *buff;
  buff = (char *) ptr;
  int valread;
  char buffer[1024];
  int sock;
  char socke[10];
  const char s[2] = "-";
  char *token;
  char *junk;
  token=strtok(buff,s);
  strcpy(socke,token);
  sock=strtol(socke,&junk,10);//convert string to int

  while (1) {
    valread = read( sock , buffer, 1024);
    printf("%s\n",buffer );
    bzero(buffer,1024);
  }
}

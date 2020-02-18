#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>   //strlen
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <my_global.h>
#include <mysql.h>
#include <stdbool.h>

#define TRUE1   1
#define FALSE1  0
#define servers 8
#define groupn 2

struct node{
  int groupID;
  char *username;
  int sockfiled;
  struct node *next;
};

void *MainServer( void *ptr );
void *Secserver( void *ptr );
void finish_with_error(MYSQL *con);
void pushnode(struct node** head_ref, char* new_username,int new_groupID,int sockfd);
void printList(struct node *node);
void update(struct node* head, char* new_username,int new_groupID,int sockfd);
void deleteConnectedUser(struct node **head_ref,char *uname);
bool search(struct node* head, char *x);
void sendMess(struct node* head, int gid,char *message);
void sendToUser(struct node* head, char *tousr,char *message);

struct node* headOfConnected = NULL;

int main(int argc , char *argv[])
{
     pthread_t s1, s2[20];
     int  iret1, iret2,i;
     int a=8888;


     int *port = malloc(sizeof(*port));
     *port=a;
    /* Create independent threads each of which will execute function */
     iret1 = pthread_create( &s1, NULL, MainServer,port);
     if(iret1)
     {
         fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
         exit(EXIT_FAILURE);
     }
     for(i=0;i<servers;i++){
       int *port = malloc(sizeof(*port));
       a++;
       *port=a;
       iret2 = pthread_create( &s2[i], NULL, Secserver,port);
       if(iret2)
       {
           fprintf(stderr,"Error - pthread_create() return code: %d\n",iret2);
           exit(EXIT_FAILURE);
       }
     }


     /* Wait till threads are complete before main continues. Unless we  */
     /* wait we run the risk of executing an exit which will terminate   */
     /* the process and all threads before the threads have completed.   */

     pthread_join( s1, NULL);
     for(i=0;i<servers;i++){
       pthread_join( s2[i], NULL);
     }

     exit(EXIT_SUCCESS);
}

void *MainServer( void *ptr )
{
     int pn = *((int *) ptr);
     char sqlQuery[256];
     char username[256];
     char password[256];
     char repeatedPassword[256];
     char email[256];
     char groupname[256];
     int ret2;
     free(ptr);
     const char s[2] = "-";
     char *token;
     char option[10];
     int num_fields;//user fields in db
     int countRows=0;//if user wxists in db
     char userID[256];//user id in db(for search of existance of the user)
     char groupID[256];//group id in db
     bzero(userID,256);
     char *junk;
     int gid;
     char admin[256];//admin of a group in db

     while (1) {
       int n;
       int opt = TRUE1;
       int master_socket , addrlen , new_socket , client_socket[30] ,
             max_clients = 30 , activity ,i, valread , sd;
       int max_sd;
       struct sockaddr_in address;
       char buffer[1024];  //data buffer of 1024


       //set of socket descriptors
       fd_set readfds;

       //a message
       char *message = "WELCOME!";

       //initialise all client_socket[] to 0 so not checked
       for (i = 0; i < max_clients; i++)
       {
           client_socket[i] = 0;
       }

       //create a master socket
       if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
       {
           perror("socket failed");
           exit(EXIT_FAILURE);
       }

       //set master socket to allow multiple connections ,
       //this is just a good habit, it will work without this
       if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
             sizeof(opt)) < 0 )
       {
           perror("setsockopt");
           exit(EXIT_FAILURE);
       }

       //type of socket created
       address.sin_family = AF_INET;
       address.sin_addr.s_addr = INADDR_ANY;
       address.sin_port = htons( pn );
       //bind the socket to localhost port 8888
       if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)
       {
           perror("bind failed");
           exit(EXIT_FAILURE);
       }
       printf("Listener on master port %d \n", pn);

       //try to specify maximum of 3 pending connections for the master socket
       if (listen(master_socket, 3) < 0)
       {
           perror("listen");
           exit(EXIT_FAILURE);
       }

       //accept the incoming connection
       addrlen = sizeof(address);
       puts("Waiting for connections ...");
       while(TRUE1)
       {
           //clear the socket set
           FD_ZERO(&readfds);

           //add master socket to set
           FD_SET(master_socket, &readfds);
           max_sd = master_socket;

           //add child sockets to set
           for ( i = 0 ; i < max_clients ; i++)
           {
               //socket descriptor
               sd = client_socket[i];

               //if valid socket descriptor then add to read list
               if(sd > 0)
                   FD_SET( sd , &readfds);

               //highest file descriptor number, need it for the select function
               if(sd > max_sd)
                   max_sd = sd;
           }
           //wait for an activity on one of the sockets , timeout is NULL ,
           //so wait indefinitely
           activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
           if ((activity < 0) && (errno!=EINTR))
           {
               printf("select error");
           }
           //If something happened on the master socket ,
           //then its an incoming connection

           if (FD_ISSET(master_socket, &readfds))
           {
               if ((new_socket = accept(master_socket,
                       (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
               {
                   perror("accept");
                   exit(EXIT_FAILURE);
               }

               //inform user of socket number - used in send and receive commands
               printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

               //send new connection greeting message
               if( send(new_socket, message, strlen(message), 0) != strlen(message) )
               {
                   perror("send");
               }

               puts("Welcome message sent successfully");

               //add new socket to array of sockets
               for (i = 0; i < max_clients; i++)
               {
                   //if position is empty
                   if( client_socket[i] == 0 )
                   {
                       client_socket[i] = new_socket;
                       printf("Adding to list of sockets as %d\n" , i);

                       break;
                   }
               }

           }
           //else its some IO operation on some other socket
           for (i = 0; i < max_clients; i++)
           {
               sd = client_socket[i];
               if (FD_ISSET( sd , &readfds))
               {
                 bzero(buffer,1024);
                 valread = read( sd , buffer, 1023);
                 if (strstr(buffer,"exit***")!=NULL)
                 {
                   if (strcmp(buffer,"exit***")==0) {
                     //Somebody disconnected , get his details and print
                     getpeername(sd , (struct sockaddr*)&address , \
                         (socklen_t*)&addrlen);
                     printf("Host disconnected , ip %s , port %d \n" ,
                           inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                     //Close the socket and mark as 0 in list for reuse
                     close( sd );
                     client_socket[i] = 0;
                   }
                   else{
                     printf("received:%s\n",buffer );
                     bzero(username,256);
                     token = strtok(buffer, s);
                     strcpy(username,token);

                     deleteConnectedUser(&headOfConnected,username);//remove from active users
                     //Somebody disconnected , get his details and print
                     getpeername(sd , (struct sockaddr*)&address , \
                         (socklen_t*)&addrlen);
                     printf("Host disconnected , ip %s , port %d \n" ,
                           inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                     //Close the socket and mark as 0 in list for reuse
                     printf("\n Active users:\n");
                     printList(headOfConnected);
                     printf("\n");
                     close( sd );
                     client_socket[i] = 0;
                   }
                 }

                   //Echo back the message that came in
                   else
                   {
                       //set the string terminating NULL byte on the end
                       //of the data read
                       token = strtok(buffer, s);
                       strcpy(option,token);
                       if (strcmp(option,"1")==0) {//login
                         token = strtok(NULL, s);
                         strcpy(username,token);
                         printf("%s\n",username );
                         token = strtok(NULL, s);
                         bzero(password,256);
                         strcpy(password,token);
                         printf("%s\n",password );

                         bool v;
                         v=search(headOfConnected,username);//if already connected relogin
                         if (v==true) {
                           send(sd , "repeat##", 8 , 0 );
                         }
                         else{
                           MYSQL *con = mysql_init(NULL);

                           if (con == NULL)
                           {
                               fprintf(stderr, "%s\n", mysql_error(con));
                               exit(1);
                           }
                           if (mysql_real_connect(con, "localhost", "root", "pass",
                             "cdb", 0, NULL, 0) == NULL)
                           {
                               finish_with_error(con);
                               send(sd,"exit***",7,0);
                               continue;
                           }
                           bzero(sqlQuery,256);
                             sprintf(sqlQuery,"SELECT COUNT(*) FROM chat_user WHERE UName='%s' AND UPass='%s'",username,password);
                             if (mysql_query(con,sqlQuery))
                             {
                                 finish_with_error(con);
                                 send(sd,"exit***",7,0);
                                 continue;
                             }
                             MYSQL_RES *result = mysql_store_result(con);

                             if (result == NULL)
                               {
                                   finish_with_error(con);
                                   send(sd,"exit***",7,0);
                                   continue;
                               }

                             int num_fields = mysql_num_fields(result);
                             MYSQL_ROW row;
                             row = mysql_fetch_row(result);
                             bzero(buffer,1024);
                             sprintf(buffer,"%s",row[0]);
                             if (strcmp(buffer,"0")!=0) {
                               countRows++;
                             }
                             printf("user:%d\n",countRows );
                             mysql_free_result(result);
                             mysql_close(con);
                             if (countRows==0) {
                               printf("User doesnt exist!\n");
                               countRows=0;//reinitialize
                               send(sd , "repeat###", 8 , 0 );
                             }
                             else{
                               printf("User exist!\n");
                               pushnode(&headOfConnected,username,0,sd);
                               printf("\n Active Users:\n");
                               printList(headOfConnected);
                               countRows=0;//reinitialize
                               send(sd , "continue##", 10 , 0 );
                             }
                         }
                       }//end_login
                       if (strcmp(option,"2")==0) {//sign up
                         token = strtok(NULL, s);
                         strcpy(username,token);
                         printf("%s\n",username );
                         token = strtok(NULL, s);
                         strcpy(password,token);
                         printf("%s\n",password );
                         token = strtok(NULL, s);
                         strcpy(email,token);
                         printf("%s\n",email );

                         MYSQL *con = mysql_init(NULL);

                         if (con == NULL)
                         {
                             fprintf(stderr, "%s\n", mysql_error(con));
                             exit(1);
                         }
                         if (mysql_real_connect(con, "localhost", "root", "pass",
                           "cdb", 0, NULL, 0) == NULL)
                         {
                             finish_with_error(con);
                             send(sd,"exit***",7,0);
                             continue;
                         }
                         bzero(sqlQuery,256);
                         sprintf(sqlQuery,"SELECT COUNT(*) FROM chat_user WHERE UName='%s'",username);
                         if (mysql_query(con,sqlQuery))
                         {
                             finish_with_error(con);
                             send(sd,"exit***",7,0);
                             continue;
                         }
                         MYSQL_RES *result = mysql_store_result(con);

                         if (result == NULL)
                           {
                               finish_with_error(con);
                               send(sd,"exit***",7,0);
                               continue;
                           }

                         int num_fields = mysql_num_fields(result);
                         MYSQL_ROW row;
                         row = mysql_fetch_row(result);
                         bzero(buffer,1024);
                         sprintf(buffer,"%s",row[0]);
                         if (strcmp(buffer,"0")!=0) {
                           countRows++;
                         }
                         printf("user:%d\n",countRows );
                         mysql_free_result(result);
                         if (countRows==0) {
                           printf("User doesnt exist!\n");

                           bzero(sqlQuery,256);
                           sprintf(sqlQuery,"INSERT INTO chat_user(UName,UPass,email) VALUES('%s','%s','%s')",username,password,email);
                           if (mysql_query(con,sqlQuery))
                           {
                               finish_with_error(con);
                               send(sd,"exit***",7,0);
                               continue;
                           }

                           //select user rows to display
                           if (mysql_query(con, "SELECT * FROM chat_user"))
                            {
                                finish_with_error(con);
                                send(sd,"exit***",7,0);
                                continue;
                            }

                            MYSQL_RES *result = mysql_store_result(con);

                            if (result == NULL)
                            {
                                finish_with_error(con);
                                send(sd,"exit***",7,0);
                                continue;
                            }

                            int num_fields = mysql_num_fields(result);

                            MYSQL_ROW row;

                            while ((row = mysql_fetch_row(result)))
                            {
                                for(int i = 0; i < num_fields; i++)
                                {
                                    printf("%s ", row[i] ? row[i] : "NULL");
                                }
                                    printf("\n");
                            }

                            mysql_free_result(result);
                            mysql_close(con);
                            countRows=0;
                            pushnode(&headOfConnected,username,0,sd);
                            printf("\n Active Users:\n");
                            printList(headOfConnected);
                           send(sd , "continue##", 10 , 0 );
                         }
                         else{
                           printf("User exists!\n");
                           countRows=0;
                           send(sd , "repeat##", 8 , 0 );
                         }


                       }//end_sign up
                       if (strcmp(option,"3")==0){//provide groups
                         MYSQL *con = mysql_init(NULL);

                         if (con == NULL)
                         {
                             fprintf(stderr, "%s\n", mysql_error(con));
                             exit(1);
                         }
                         if (mysql_real_connect(con, "localhost", "root", "pass",
                           "cdb", 0, NULL, 0) == NULL)
                         {
                             finish_with_error(con);
                             send(sd,"exit***",7,0);
                             continue;
                         }
                             bzero(sqlQuery,256);
                             sprintf(sqlQuery,"SELECT * FROM chat_group");
                             if (mysql_query(con,sqlQuery))
                             {
                                 finish_with_error(con);
                                 send(sd,"exit***",7,0);
                                 continue;
                             }
                             MYSQL_RES *result = mysql_store_result(con);

                             if (result == NULL)
                               {
                                   finish_with_error(con);
                                   send(sd,"exit***",7,0);
                                   continue;
                               }

                             num_fields = mysql_num_fields(result);
                             MYSQL_ROW row;
                             countRows=0;
                             bzero(buffer,1024);
                             sprintf(buffer + strlen(buffer),"**********Groups**********\n");
                             printf("%s\n",buffer );
                             while ((row = mysql_fetch_row(result)))
                             {
                                 for(int i = 0; i < num_fields; i++)
                                 {
                                     printf("%s ", row[i] ? row[i] : "NULL");
                                     sprintf(buffer + strlen(buffer),"%s ", row[i] ? row[i] : "NULL");

                                 }
                                     sprintf(buffer + strlen(buffer),"\n");
                                     printf("\n");
                             }

                             sprintf(buffer + strlen(buffer),"**********MENU**********\n");
                             sprintf(buffer + strlen(buffer),"1. Create Group\n");
                             sprintf(buffer + strlen(buffer),"2. Enroll to a Group\n");
                             sprintf(buffer + strlen(buffer),"3. Delete Group\n");
                             sprintf(buffer + strlen(buffer),"4. Delete account\n");
                             sprintf(buffer + strlen(buffer),"5. Join group conversation");
                             mysql_free_result(result);
                             mysql_close(con);
                             send(sd , buffer,strlen(buffer) , 0 );
                       }//provide groups_end
                       if (strcmp(option,"4")==0) {//create group
                         token = strtok(NULL, s);
                         strcpy(groupname,token);
                         bzero(username,256);
                         token = strtok(NULL, s);
                         strcpy(username,token);
                         bzero(userID,256);
                         MYSQL *con = mysql_init(NULL);//connection to add a group

                         if (con == NULL)
                         {
                             fprintf(stderr, "%s\n", mysql_error(con));
                             exit(1);
                         }
                         if (mysql_real_connect(con, "localhost", "root", "pass",
                           "cdb", 0, NULL, 0) == NULL)
                         {
                             finish_with_error(con);
                             send(sd,"exit***",7,0);
                             continue;
                         }
                         bzero(sqlQuery,256);
                         sprintf(sqlQuery,"INSERT INTO chat_group (groupName,admin) VALUES('%s','%s')",groupname,username);
                         if (mysql_query(con,sqlQuery))
                         {
                           finish_with_error(con);
                           send(sd,"exit***",7,0);
                           continue;
                         }
                         printf("%s created group:%s\n",username,groupname );
                         mysql_close(con);//connection to add a group_end

                         con = mysql_init(NULL);//new connection to obtain groupid
                         if (con == NULL)
                         {
                             fprintf(stderr, "%s\n", mysql_error(con));
                             exit(1);
                         }
                         if (mysql_real_connect(con, "localhost", "root", "pass",
                           "cdb", 0, NULL, 0) == NULL)
                         {
                             finish_with_error(con);
                             send(sd,"exit***",7,0);
                             continue;
                         }
                         bzero(sqlQuery,256);
                         sprintf(sqlQuery,"SELECT groupID FROM chat_group WHERE groupName='%s'",groupname);
                         if (mysql_query(con,sqlQuery ))
                          {
                              finish_with_error(con);
                              send(sd,"exit***",7,0);
                              continue;
                          }
                          MYSQL_RES *result = mysql_store_result(con);
                          if (result == NULL)
                          {
                              finish_with_error(con);
                              send(sd,"exit***",7,0);
                              continue;
                          }
                          int num_fields = mysql_num_fields(result);
                          MYSQL_ROW row;
                          row = mysql_fetch_row(result);
                          bzero(groupID,256);
                          sprintf(groupID,"%s",row[0]);
                          mysql_free_result(result);
                          mysql_close(con);//new connection to obtain groupid_end

                         con = mysql_init(NULL);//new connection to obtain userid
                         if (con == NULL)
                         {
                             fprintf(stderr, "%s\n", mysql_error(con));
                             exit(1);
                         }
                         if (mysql_real_connect(con, "localhost", "root", "pass",
                           "cdb", 0, NULL, 0) == NULL)
                         {
                             finish_with_error(con);
                             send(sd,"exit***",7,0);
                             continue;
                         }
                         bzero(sqlQuery,256);
                         sprintf(sqlQuery,"SELECT usersID FROM chat_user WHERE UName='%s'",username);
                         if (mysql_query(con,sqlQuery ))
                          {
                              finish_with_error(con);
                              send(sd,"exit***",7,0);
                              continue;
                          }
                          result = mysql_store_result(con);
                          if (result == NULL)
                          {
                              finish_with_error(con);
                              send(sd,"exit***",7,0);
                              continue;
                          }
                          num_fields = mysql_num_fields(result);
                          row = mysql_fetch_row(result);
                          bzero(userID,256);
                          sprintf(userID,"%s",row[0]);
                          mysql_free_result(result);
                          mysql_close(con);//new connection to obtain userid_end
                          // printf("%s has user ID:%s\n",username,userID );
                          con = mysql_init(NULL);//connection to add group creator to group
                          if (con == NULL)
                          {
                              fprintf(stderr, "%s\n", mysql_error(con));
                              exit(1);
                          }
                          if (mysql_real_connect(con, "localhost", "root", "pass",
                            "cdb", 0, NULL, 0) == NULL)
                          {
                              finish_with_error(con);
                              send(sd,"exit***",7,0);
                              continue;
                          }
                          bzero(sqlQuery,256);
                          sprintf(sqlQuery,"INSERT INTO chat_user_has_chat_group(userIDbelongs,TOgroupID) VALUES('%s','%s')",userID,groupID);
                          if (mysql_query(con,sqlQuery ))
                           {
                               finish_with_error(con);
                               send(sd,"exit***",7,0);
                               continue;
                           }
                           mysql_close(con);//connection to add group creator to group_end
                         send(sd , "group created successfully!",27 , 0 );

                       }
                       if (strcmp(option,"5")==0) {//Enroll to a group
                         token = strtok(NULL, s);
                         bzero(groupID,256);
                         strcpy(groupID,token);
                         printf("%s\n",groupID );
                         token = strtok(NULL, s);
                         bzero(username,256);
                         strcpy(username,token);
                         printf("%s\n",username );

                          MYSQL *con = mysql_init(NULL);//new connection to obtain userid
                          if (con == NULL)
                          {
                              fprintf(stderr, "%s\n", mysql_error(con));
                              exit(1);
                          }
                          if (mysql_real_connect(con, "localhost", "root", "pass",
                            "cdb", 0, NULL, 0) == NULL)
                          {
                              finish_with_error(con);
                              send(sd,"exit***",7,0);
                              continue;
                          }
                          bzero(sqlQuery,256);
                          sprintf(sqlQuery,"SELECT usersID FROM chat_user WHERE UName='%s'",username);
                          if (mysql_query(con,sqlQuery ))
                           {
                               finish_with_error(con);
                               send(sd,"exit***",7,0);
                               continue;
                           }
                           MYSQL_RES *result = mysql_store_result(con);
                           if (result == NULL)
                           {
                               finish_with_error(con);
                               send(sd,"exit***",7,0);
                               continue;
                           }
                           num_fields = mysql_num_fields(result);
                           MYSQL_ROW row;
                           row = mysql_fetch_row(result);
                           bzero(userID,256);
                           sprintf(userID,"%s",row[0]);
                           mysql_free_result(result);
                           mysql_close(con);//new connection to obtain userid_end

                           con = mysql_init(NULL);//connection to add user to group
                           if (con == NULL)
                           {
                               fprintf(stderr, "%s\n", mysql_error(con));
                               exit(1);
                           }
                           if (mysql_real_connect(con, "localhost", "root", "pass",
                             "cdb", 0, NULL, 0) == NULL)
                           {
                               finish_with_error(con);
                               send(sd,"exit***",7,0);
                               continue;
                           }
                           bzero(sqlQuery,256);
                           sprintf(sqlQuery,"INSERT INTO chat_user_has_chat_group(userIDbelongs,TOgroupID) VALUES('%s','%s')",userID,groupID);
                           if (mysql_query(con,sqlQuery ))
                            {
                                finish_with_error(con);
                                send(sd,"exit***",7,0);
                                continue;
                            }
                            mysql_close(con);//connection to add user to group_end

                         send(sd , "Enrolled to group! You can now join.",36 , 0 );
                       }//enroll to a group_end
                       if (strcmp(option,"6")==0) {//delete group
                         token = strtok(NULL, s);
                         bzero(groupID,256);
                         strcpy(groupID,token);
                         token = strtok(NULL, s);
                         bzero(username,256);
                         strcpy(username,token);


                         MYSQL *con = mysql_init(NULL);//new connection to obtain rows
                         if (con == NULL)
                         {
                             fprintf(stderr, "%s\n", mysql_error(con));
                             exit(1);
                         }
                         if (mysql_real_connect(con, "localhost", "root", "pass",
                           "cdb", 0, NULL, 0) == NULL)
                         {
                             finish_with_error(con);
                             send(sd,"exit***",7,0);
                             continue;
                         }
                         bzero(sqlQuery,256);
                         sprintf(sqlQuery,"SELECT COUNT(*) FROM chat_group WHERE groupID='%s'",groupID);
                         if (mysql_query(con,sqlQuery))
                         {
                             finish_with_error(con);
                             send(sd,"exit***",7,0);
                             continue;
                         }
                         MYSQL_RES *result = mysql_store_result(con);

                         if (result == NULL)
                           {
                               finish_with_error(con);
                               send(sd,"exit***",7,0);
                               continue;
                           }
                           int num_fields = mysql_num_fields(result);
                           MYSQL_ROW row;
                           row = mysql_fetch_row(result);
                           bzero(buffer,1024);
                           sprintf(buffer,"%s",row[0]);
                           if (strcmp(buffer,"0")==0) {
                             send(sd,"exit***",7,0);
                             continue;
                           }
                           mysql_free_result(result);
                           mysql_close(con);//new connection to obtain rows_end
                           con = mysql_init(NULL);//new connection to obtain admin
                           if (con == NULL)
                           {
                               fprintf(stderr, "%s\n", mysql_error(con));
                               exit(1);
                           }
                           if (mysql_real_connect(con, "localhost", "root", "pass",
                             "cdb", 0, NULL, 0) == NULL)
                           {
                               finish_with_error(con);
                               send(sd,"exit***",7,0);
                               continue;
                           }
                         bzero(sqlQuery,256);
                         sprintf(sqlQuery,"SELECT admin FROM chat_group WHERE groupID='%s'",groupID);
                         if (mysql_query(con,sqlQuery ))
                          {
                              finish_with_error(con);
                              send(sd,"exit***",7,0);
                              continue;
                          }
                          result = mysql_store_result(con);
                          if (result == NULL)
                          {
                              finish_with_error(con);
                              send(sd,"exit***",7,0);
                              continue;
                          }

                          row = mysql_fetch_row(result);
                          bzero(admin,256);
                          sprintf(admin,"%s",row[0]);
                          printf("admin:%s\n",admin );
                          mysql_free_result(result);
                          mysql_close(con);//new connection to obtain admin_end

                          con = mysql_init(NULL);//new connection to delete group
                          if (con == NULL)
                          {
                              fprintf(stderr, "%s\n", mysql_error(con));
                              exit(1);
                          }
                          if (mysql_real_connect(con, "localhost", "root", "pass",
                            "cdb", 0, NULL, 0) == NULL)
                          {
                              finish_with_error(con);
                              send(sd,"exit***",7,0);
                              continue;
                          }
                          printf("uname:%s\n",username );
                          if (strcmp(admin,username)==0) {
                            bzero(sqlQuery,256);
                            sprintf(sqlQuery,"DELETE FROM chat_group WHERE groupID='%s'",groupID);
                            if (mysql_query(con,sqlQuery ))
                             {
                                 finish_with_error(con);
                                 send(sd,"exit***",7,0);
                                 continue;
                             }
                             send(sd,"group deleted successfully!",27,0);
                          }else{
                            send(sd,"User doesn't have privilleges to delete this group!",51,0);
                          }
                          mysql_close(con);//new connection to delete group_end

                       }//delete group_end
                       if (strcmp(option,"7")==0) {//delete account
                         token = strtok(NULL, s);
                         bzero(username,256);
                         strcpy(username,token);

                         MYSQL *con = mysql_init(NULL);//new connection to obtain rows
                         if (con == NULL)
                         {
                             fprintf(stderr, "%s\n", mysql_error(con));
                             exit(1);
                         }
                         if (mysql_real_connect(con, "localhost", "root", "pass",
                           "cdb", 0, NULL, 0) == NULL)
                         {
                             finish_with_error(con);
                             send(sd,"exit***",7,0);
                             continue;
                         }
                         bzero(sqlQuery,256);
                         sprintf(sqlQuery,"DELETE FROM chat_user WHERE UName='%s'",username);
                         if (mysql_query(con, sqlQuery))
                         {
                             finish_with_error(con);
                         }
                         mysql_close(con);
                         send(sd,"account deleted successfully!",29,0);

                       }//delete account_end

                       if (strcmp(option,"8")==0){//return groupids
                         token = strtok(NULL, s);
                         bzero(username,256);
                         strcpy(username,token);
                         MYSQL *con = mysql_init(NULL);//new connection to obtain userid
                         if (con == NULL)
                         {
                             fprintf(stderr, "%s\n", mysql_error(con));
                             exit(1);
                         }
                         if (mysql_real_connect(con, "localhost", "root", "pass",
                           "cdb", 0, NULL, 0) == NULL)
                         {
                             finish_with_error(con);
                             send(sd,"exit***",7,0);
                             continue;
                         }
                         bzero(sqlQuery,256);
                         sprintf(sqlQuery,"SELECT usersID FROM chat_user WHERE UName='%s'",username);
                         if (mysql_query(con,sqlQuery ))
                          {
                              finish_with_error(con);
                              send(sd,"exit***",7,0);
                              continue;
                          }
                          MYSQL_RES *result = mysql_store_result(con);
                          if (result == NULL)
                          {
                              finish_with_error(con);
                              send(sd,"exit***",7,0);
                              continue;
                          }
                          num_fields = mysql_num_fields(result);
                          MYSQL_ROW row;
                          row = mysql_fetch_row(result);
                          bzero(userID,256);
                          sprintf(userID,"%s",row[0]);
                          mysql_free_result(result);
                          mysql_close(con);//new connection to obtain userid_end

                          con = mysql_init(NULL);//new connection to obtain groupIDs in which user is enrolled
                          if (con == NULL)
                          {
                              fprintf(stderr, "%s\n", mysql_error(con));
                              exit(1);
                          }
                          if (mysql_real_connect(con, "localhost", "root", "pass",
                            "cdb", 0, NULL, 0) == NULL)
                          {
                              finish_with_error(con);
                              send(sd,"exit***",7,0);
                              continue;
                          }
                          bzero(sqlQuery,256);
                          sprintf(sqlQuery,"SELECT TOgroupID FROM chat_user_has_chat_group WHERE userIDbelongs='%s'",userID);
                          if (mysql_query(con,sqlQuery ))
                           {
                               finish_with_error(con);
                               send(sd,"exit***",7,0);
                               continue;
                           }
                           result = mysql_store_result(con);
                           if (result == NULL)
                           {
                               finish_with_error(con);
                               send(sd,"exit***",7,0);
                               continue;
                           }
                           countRows=0;
                           num_fields = mysql_num_fields(result);
                           bzero(buffer,1024);
                           while ((row = mysql_fetch_row(result)))
                           {
                             countRows++;
                               for(int i = 0; i < num_fields; i++)
                               {
                                   printf("%s ", row[i] ? row[i] : "NULL");
                                   sprintf(buffer + strlen(buffer),"%s ", row[i] ? row[i] : "NULL");

                               }
                                   sprintf(buffer + strlen(buffer),"\n");
                                   printf("\n");
                           }
                           mysql_free_result(result);
                           mysql_close(con);
                           if (countRows==0) {
                             send(sd , "no groups available!", 20, 0 );
                           }else{
                             countRows=0;
                             send(sd , buffer,strlen(buffer) , 0 );
                           }
                       }//return groupids_end
                       if (strcmp(option,"9")==0){//check if gid exists
                         token = strtok(NULL, s);
                         bzero(groupID,256);
                         strcpy(groupID,token);
                         token = strtok(NULL, s);
                         bzero(username,256);
                         strcpy(username,token);
                         MYSQL *con = mysql_init(NULL);//new connection to obtain userid
                         if (con == NULL)
                         {
                             fprintf(stderr, "%s\n", mysql_error(con));
                             exit(1);
                         }
                         if (mysql_real_connect(con, "localhost", "root", "pass",
                           "cdb", 0, NULL, 0) == NULL)
                         {
                             finish_with_error(con);
                             send(sd,"exit***",7,0);
                             continue;
                         }
                         bzero(sqlQuery,256);
                         sprintf(sqlQuery,"SELECT usersID FROM chat_user WHERE UName='%s'",username);
                         if (mysql_query(con,sqlQuery ))
                          {
                              finish_with_error(con);
                              send(sd,"exit***",7,0);
                              continue;
                          }
                          MYSQL_RES *result = mysql_store_result(con);
                          if (result == NULL)
                          {
                              finish_with_error(con);
                              send(sd,"exit***",7,0);
                              continue;
                          }
                          int num_fields = mysql_num_fields(result);
                          MYSQL_ROW row;
                          row = mysql_fetch_row(result);
                          bzero(userID,256);
                          sprintf(userID,"%s",row[0]);
                          printf("user id:%s\n",userID );
                         bzero(sqlQuery,256);
                         sprintf(sqlQuery,"SELECT COUNT(*) FROM chat_user_has_chat_group WHERE userIDbelongs='%s' AND TOgroupID='%s'",userID,groupID);
                         if (mysql_query(con,sqlQuery ))
                          {
                              finish_with_error(con);
                              send(sd,"exit***",7,0);
                              continue;
                          }
                          result = mysql_store_result(con);

                          if (result == NULL)
                          {
                              finish_with_error(con);
                              send(sd,"exit***",7,0);
                              continue;
                          }
                          num_fields = mysql_num_fields(result);
                          row = mysql_fetch_row(result);
                          bzero(buffer,1024);
                          sprintf(buffer,"%s",row[0]);
                          if (strcmp(buffer,"0")!=0) {
                            countRows++;
                          }
                          printf("user:%d\n",countRows );
                          mysql_free_result(result);
                          mysql_close(con);
                          if (countRows==0) {
                            printf("User not enrolled to that group!\n");
                            countRows=0;//reinitialize
                            send(sd , "User not enrolled to that group!", 32 , 0 );
                          }
                          else{
                            printf("User enrolled please join!\n");
                            countRows=0;//reinitialize
                            send(sd , "User enrolled please join!", 26 , 0 );
                          }

                       }//check if gid exists_end

                       bzero(buffer,1024);

                   }
               }
           }
       }
     }
}

void *Secserver( void *ptr ){

    int pn = *((int *) ptr);
    free(ptr);
    int groupIDs[groupn];//groups of the server
    int gid=0;
    char *junk;
    char sqlQuery[256];
    char groupid[256];
    int join=0;
    char username[256];
    char tousr[256];
    char mesag[500];
    int l;
    const char s[2] = "-";
    char *token;
    for(l=0;l<groupn;l++){
      groupIDs[l]=0;
    }
    int opt = TRUE1;
    int master_socket , addrlen , new_socket , client_socket[30] ,
          max_clients = 30 , activity, i , valread , sd;
    int max_sd;
    struct sockaddr_in address;

    char buffer[1024];  //data buffer of 1K

    //set of socket descriptors
    fd_set readfds;

    //a message
    char *message = "WELCOME!";

      //initialise all client_socket[] to 0 so not checked
      for (i = 0; i < max_clients; i++)
      {
          client_socket[i] = 0;
      }

      //create a master socket
      if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
      {
          perror("socket failed");
          exit(EXIT_FAILURE);
      }

      //set master socket to allow multiple connections ,
      //this is just a good habit, it will work without this
      if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
            sizeof(opt)) < 0 )
      {
          perror("setsockopt");
          exit(EXIT_FAILURE);
      }

      //type of socket created
      address.sin_family = AF_INET;
      address.sin_addr.s_addr = INADDR_ANY;
      address.sin_port = htons( pn );

      //bind the socket to localhost pn 8888
      if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)
      {
          perror("bind failed");
          exit(EXIT_FAILURE);
      }
      printf("Listener on port %d \n", pn);

      //try to specify maximum of 3 pending connections for the master socket
      if (listen(master_socket, 3) < 0)
      {
          perror("listen");
          exit(EXIT_FAILURE);
      }

      //accept the incoming connection
      addrlen = sizeof(address);
      puts("Waiting for connections ...");

      while(TRUE1)
      {
          //clear the socket set
          FD_ZERO(&readfds);

          //add master socket to set
          FD_SET(master_socket, &readfds);
          max_sd = master_socket;

          //add child sockets to set
          for ( i = 0 ; i < max_clients ; i++)
          {
              //socket descriptor
              sd = client_socket[i];

              //if valid socket descriptor then add to read list
              if(sd > 0)
                  FD_SET( sd , &readfds);

              //highest file descriptor number, need it for the select function
              if(sd > max_sd)
                  max_sd = sd;
          }

          //wait for an activity on one of the sockets , timeout is NULL ,
          //so wait indefinitely
          activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
          if ((activity < 0) && (errno!=EINTR))
          {
              printf("**********select error***********\n");
          }

          //If something happened on the master socket ,
          //then its an incoming connection
          if (FD_ISSET(master_socket, &readfds))
          {
              if ((new_socket = accept(master_socket,
                      (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
              {
                  perror("accept");
                  exit(EXIT_FAILURE);
              }

              //inform user of socket number - used in send and receive commands
              printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

              //send new connection greeting message
              if( send(new_socket, message, strlen(message), 0) != strlen(message) )
              {
                  perror("send");
              }

              puts("Welcome message sent successfully");

              //code
              bzero(buffer,1024);
              valread = read( new_socket , buffer, 1024);
              printf("received:%s\n",buffer );
              bzero(username,256);
              token = strtok(buffer, s);
              strcpy(username,token);
              token = strtok(NULL, s);
              bzero(groupid,10);
              strcpy(groupid,token);
              gid=strtol(groupid,&junk,10);//convert string to int

              printf("username:%s\n",username );
              printf("groupID:%d\n",gid );

              join=0;
              for(l=0;l<groupn;l++){
                if (groupIDs[l]==0) {
                  update(headOfConnected,username,gid,new_socket);
                  printf("\n Active users:\n");
                  printList(headOfConnected);
                  printf("\n");
                  groupIDs[l]=gid;
                  join++;
                  break;
                }
                if (groupIDs[l]==gid) {
                  update(headOfConnected,username,gid,new_socket);
                  printf("\n Active users:\n");
                  printList(headOfConnected);
                  printf("\n");
                  join++;
                  break;
                }
              }
              if (join>0) {
                for(l=0;l<groupn;l++){
                  printf("g1:%d\n",groupIDs[l] );
                }
                send(new_socket,"join!",5,0);
                char prevchat[1024];
                bzero(prevchat,1024);


                MYSQL *con = mysql_init(NULL);//connection to add private_message

                if (con == NULL)
                {
                    fprintf(stderr, "%s\n", mysql_error(con));
                    exit(1);
                }
                if (mysql_real_connect(con, "localhost", "root", "pass",
                  "cdb", 0, NULL, 0) == NULL)
                {
                    finish_with_error(con);
                    //send(sd,"exit***",7,0);
                    continue;
                }
                bzero(sqlQuery,256);
                sprintf(sqlQuery,"SELECT chat_user.UName,GroupMessage.message FROM GroupMessage,chat_user WHERE senderGroup='%s' AND chat_user.usersID=GroupMessage.sender",groupid);
                if (mysql_query(con,sqlQuery))
                {
                    finish_with_error(con);
                    //send(sd,"exit***",7,0);
                    continue;
                }
                ////////
                MYSQL_RES *result = mysql_store_result(con);

                if (result == NULL)
                  {
                      finish_with_error(con);
                      //send(sd,"exit***",7,0);
                      continue;
                  }
                int num_fields = mysql_num_fields(result);
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(result)))
                {
                    for(int i = 0; i < num_fields; i++)
                    {
                        printf("%s ", row[i] ? row[i] : "NULL");
                        sprintf(prevchat + strlen(prevchat),"%s ", row[i] ? row[i] : "NULL");

                    }
                        sprintf(prevchat + strlen(prevchat),"\n");
                        printf("\n");
                }
                mysql_free_result(result);
                ////////
                mysql_close(con);
                send(new_socket,prevchat,strlen(prevchat),0);

              }
              else {
                send(new_socket,"full!",5,0);
              }

              //add new socket to array of sockets
              for (i = 0; i < max_clients; i++)
              {
                  //if position is empty
                  if( client_socket[i] == 0 )
                  {
                      client_socket[i] = new_socket;
                      printf("Adding to list of sockets as %d\n" , i);

                      break;
                  }
              }
          }
          //else its some IO operation on some other socket
          for (i = 0; i < max_clients; i++)
          {
              sd = client_socket[i];

              if (FD_ISSET( sd , &readfds))
              {
                  //Check if it was for closing , and also read the
                  //incoming message
                  bzero(buffer,1024);
                  valread = read( sd , buffer, 1023);
                  if (strstr(buffer,"exit***")!=NULL)
                  {
                    if (strcmp(buffer,"exit***")==0) {
                      //Somebody disconnected , get his details and print
                      getpeername(sd , (struct sockaddr*)&address , \
                          (socklen_t*)&addrlen);
                      printf("Host disconnected , ip %s , port %d \n" ,
                            inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                      //Close the socket and mark as 0 in list for reuse
                      close( sd );
                      client_socket[i] = 0;
                    }
                    else{
                      printf("received:%s\n",buffer );
                      bzero(username,256);
                      token = strtok(buffer, s);
                      strcpy(username,token);
                      token = strtok(NULL, s);
                      bzero(groupid,10);
                      strcpy(groupid,token);
                      gid=strtol(groupid,&junk,10);//convert string to int
                      deleteConnectedUser(&headOfConnected,username);
                      //Somebody disconnected , get his details and print
                      getpeername(sd , (struct sockaddr*)&address , \
                          (socklen_t*)&addrlen);
                      printf("Host disconnected , ip %s , port %d \n" ,
                            inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                      //Close the socket and mark as 0 in list for reuse
                      printf("\n Active users:\n");
                      printList(headOfConnected);
                      printf("\n");
                      close( sd );
                      client_socket[i] = 0;
                    }
                  }

                  //Echo back the message that came in
                  else
                  {
                    if (buffer[0]=='@') {
                      printf("received:%s\n",buffer );
                      int idxToDel = 0;
                      memmove(&buffer[idxToDel], &buffer[idxToDel + 1], strlen(buffer) - idxToDel);
                      token=strtok(buffer,s);
                      bzero(tousr,256);
                      strcpy(tousr,token);
                      token = strtok(NULL, s);
                      bzero(username,256);
                      strcpy(username,token);
                      token = strtok(NULL, s);
                      bzero(mesag,500);
                      strcpy(mesag,token);
                      bzero(buffer,1024);
                      strcpy(buffer,username);
                      strcat(buffer,":");
                      strcat(buffer,mesag);
                      buffer[valread] = '\0';
                      sendToUser(headOfConnected,tousr,buffer);

                      MYSQL *con = mysql_init(NULL);//connection to add private_message

                      if (con == NULL)
                      {
                          fprintf(stderr, "%s\n", mysql_error(con));
                          exit(1);
                      }
                      if (mysql_real_connect(con, "localhost", "root", "pass",
                        "cdb", 0, NULL, 0) == NULL)
                      {
                          finish_with_error(con);
                          //send(sd,"exit***",7,0);
                          continue;
                      }

                      //////////////
                      bzero(sqlQuery,256);
                        sprintf(sqlQuery,"SELECT COUNT(*) FROM chat_user WHERE UName='%s'",tousr);
                        if (mysql_query(con,sqlQuery))
                        {
                            finish_with_error(con);
                            //send(sd,"exit***",7,0);
                            continue;
                        }
                        MYSQL_RES *result = mysql_store_result(con);

                        if (result == NULL)
                          {
                              finish_with_error(con);
                              //send(sd,"exit***",7,0);
                              continue;
                          }
                          int exist1=0;
                        MYSQL_ROW row;
                        row = mysql_fetch_row(result);
                        bzero(buffer,1024);
                        sprintf(buffer,"%s",row[0]);
                        if (strcmp(buffer,"0")!=0) {
                          exist1++;
                        }
                        else{
                          continue;
                        }
                        mysql_free_result(result);
                      ////////////
                      bzero(sqlQuery,256);
                      sprintf(sqlQuery,"SELECT usersID FROM chat_user WHERE UName='%s';",username);//sender_ID

                      if (mysql_query(con,sqlQuery))
                      {
                        finish_with_error(con);
                        //send(sd,"exit***",7,0);
                        continue;
                      }
                      result = mysql_store_result(con);

                      if (result == NULL)
                      {
                          finish_with_error(con);
                          //send(sd,"exit***",7,0);
                          continue;
                      }
                      row = mysql_fetch_row(result);
                      char senderid[256];
                      bzero(senderid,256);
                      sprintf(senderid,"%s",row[0]);//sender ID
                      mysql_free_result(result);
                      bzero(sqlQuery,256);
                      sprintf(sqlQuery,"SELECT usersID FROM chat_user WHERE UName='%s';",tousr);//receiver_ID
                      if (mysql_query(con,sqlQuery))
                      {
                        finish_with_error(con);
                        //send(sd,"exit***",7,0);
                        continue;
                      }
                      result = mysql_store_result(con);
                      if (result == NULL)
                      {
                          finish_with_error(con);
                          //send(sd,"exit***",7,0);
                          continue;
                      }
                      row = mysql_fetch_row(result);
                      char receiverid[256];
                      bzero(receiverid,256);
                      sprintf(receiverid,"%s",row[0]);//receiver ID
                      mysql_free_result(result);


                      bzero(sqlQuery,256);
                      sprintf(sqlQuery,"INSERT INTO PrivateMessage(senderID,receiverID,message) VALUES('%s','%s','%s');",senderid,receiverid,mesag);
                      if (mysql_query(con,sqlQuery))
                      {
                        finish_with_error(con);
                        //send(sd,"exit***",7,0);
                        continue;
                      }
                      mysql_close(con);//connection to add private_message_end

                    }
                    else{
                      printf("received:%s\n",buffer );
                      bzero(username,256);
                      token = strtok(buffer, s);
                      strcpy(username,token);
                      token = strtok(NULL, s);
                      bzero(groupid,10);
                      strcpy(groupid,token);
                      gid=strtol(groupid,&junk,10);//convert string to int
                      token = strtok(NULL, s);
                      bzero(mesag,500);
                      strcpy(mesag,token);
                      bzero(buffer,1024);
                      strcpy(buffer,username);
                      strcat(buffer,":");
                      strcat(buffer,mesag);
                      buffer[valread] = '\0';
                      sendMess(headOfConnected,gid,buffer);

                      MYSQL *con = mysql_init(NULL);//connection to add group_message

                      if (con == NULL)
                      {
                          fprintf(stderr, "%s\n", mysql_error(con));
                          exit(1);
                      }
                      if (mysql_real_connect(con, "localhost", "root", "pass",
                        "cdb", 0, NULL, 0) == NULL)
                      {
                          finish_with_error(con);
                          //send(sd,"exit***",7,0);
                          continue;
                      }
                      bzero(sqlQuery,256);
                      sprintf(sqlQuery,"SELECT usersID FROM chat_user WHERE UName='%s';",username);//sender_ID

                      if (mysql_query(con,sqlQuery))
                      {
                        finish_with_error(con);
                        //send(sd,"exit***",7,0);
                        continue;
                      }
                      MYSQL_RES *result = mysql_store_result(con);

                      if (result == NULL)
                      {
                          finish_with_error(con);
                          //send(sd,"exit***",7,0);
                          continue;
                      }
                      MYSQL_ROW row;
                      row = mysql_fetch_row(result);
                      char senderid[256];
                      bzero(senderid,256);
                      sprintf(senderid,"%s",row[0]);//sender ID
                      mysql_free_result(result);

                      bzero(sqlQuery,256);
                      sprintf(sqlQuery,"INSERT INTO GroupMessage (sender,senderGroup,message) VALUES('%s','%s','%s')",senderid,groupid,mesag);
                      if (mysql_query(con,sqlQuery))
                      {
                        finish_with_error(con);
                        //send(sd,"exit***",7,0);
                        continue;
                      }
                      mysql_close(con);//connection to add group_message_end

                    }
                  }
              }
          }
      }


}


void pushnode(struct node** head_ref, char* new_username,int new_groupID,int sockfd){
  /* 1. allocate node */
    struct node* new_node = (struct node*) malloc(sizeof(struct node));

    /* 2. put in the data  */
    new_node->groupID  = new_groupID;
    new_node->sockfiled = sockfd;
    new_node->username = (char*)malloc(sizeof(new_username));
    strcpy(new_node->username,new_username);
    /* 3. Make next of new node as head */
    new_node->next = (*head_ref);

    /* 4. move the head to point to the new node */
    (*head_ref)    = new_node;
}

void update(struct node* head, char* new_username,int new_groupID,int sockfd){
  struct node* current = head;  // Initialize current
  while (current != NULL)
  {
      if (strcmp(current->username,new_username) == 0){
        current->sockfiled=sockfd;
        current->groupID=new_groupID;
        return;
      }

      current = current->next;
  }
}

void printList(struct node *node)
{
  while (node != NULL)
  {
     printf("username:%s ", node->username);
     printf("Group ID:%d ", node->groupID);
     printf("File desc:%d",node->sockfiled );
     printf("\n");
     node = node->next;
  }
}

void deleteConnectedUser(struct node **head_ref,char *uname)
{
    // Store head node
    struct node* temp = *head_ref, *prev;
    // If head node itself holds the key to be deleted
    if (temp != NULL && strcmp(temp->username,uname)==0)
    {
        *head_ref = temp->next;   // Changed head
        free(temp);               // free old head
        return;
    }
    // Search for the key to be deleted, keep track of the
    // previous node as we need to change 'prev->next'
    while (temp != NULL && strcmp(temp->username,uname)!=0)
    {
        prev = temp;
        temp = temp->next;
    }
    // If key was not present in linked list
    if (temp == NULL) return;
    // Unlink the node from linked list
    prev->next = temp->next;
    free(temp);  // Free memory
}

/* Checks whether the value x is present in linked list */
bool search(struct node* head, char *x)
{
    struct node* current = head;  // Initialize current
    while (current != NULL)
    {
        if (strcmp(current->username,x) == 0)
            return true;
        current = current->next;
    }
    return false;
}

/* send message to group */
void sendMess(struct node* head, int gid,char *message)
{
    struct node* current = head;  // Initialize current
    while (current != NULL)
    {
        if (current->groupID == gid){
          send(current->sockfiled , message , strlen(message) , 0 );
        }

        current = current->next;
    }
}

void sendToUser(struct node* head, char *tousr,char *message){
  struct node* current = head;  // Initialize current
  while (current != NULL)
  {
      if (strcmp(current->username,tousr)==0 ){
        send(current->sockfiled , message , strlen(message) , 0 );
        break;
      }

      current = current->next;
  }
}

void finish_with_error(MYSQL *con)
{
  fprintf(stderr, "%s\n", mysql_error(con));
  mysql_close(con);

}

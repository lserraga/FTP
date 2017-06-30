#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/errno.h>
#define MAXLINE 4096
#define SA struct sockaddr
#define LISTENQ 1024
#define LENGTH_COMMAND 200
#define maxThreads 100

//Definition of boolean value
typedef int bool;
enum {
   false,
   true
};
//Error function
void error(char * msg) {
   perror(msg);
   exit(0);
}

bool checkMaincommand(const char* command){
  if (strcmp(command,"quit")==0 || strcmp(command,"ls")==0 || strcmp(command,"get")==0)
    return true;
  if (strcmp(command,"put")==0)
    return true;
  return false;
}

//Functions that sends a request to the server (after already checking that the 
//parameters are correct). It returns true if it was correct and false elsewhere
bool sendRequest(const char* param1, const char* param2, int controlfd){
  FILE *desiredFile;
  char aux[200];

  //Sends the command to the server
   // if(send(controlfd , client_command , LENGTH_COMMAND, 0)==-1){
     // printf("Error sending the following command: %s\n",client_command);
      //continue;
    //}

  if (strcmp(param1,"quit")==0){
    send(controlfd,"QUIT", LENGTH_COMMAND , 0);
  }

  else if (strcmp(param1,"put")==0){
    desiredFile = fopen(param2, "r");
    if (desiredFile==NULL){
      printf("Unable to open %s\n",param2);
      fclose(desiredFile);
      return false;
    }
    fclose(desiredFile);
    strcpy(aux,"STOR ");
    strcat(aux,param2);
    send(controlfd,aux, LENGTH_COMMAND , 0);
  }

  else if (strcmp(param1,"ls")==0){
    strcpy(aux,"LIST ");
    strcat(aux,param2);
    send(controlfd,aux, LENGTH_COMMAND , 0);
  }
  else{
    strcpy(aux,"RETR ");
    strcat(aux,param2);
    send(controlfd,aux, LENGTH_COMMAND , 0);
  }
  return true;
}

//Functions that reads a charr array (input), returns true or false
//depending if the command structure is valid. It also writes the first 
//parameter in param1 and the second in param2
//bool getCommandParameters(const char* input,char *param1,char *param2){
bool checkCommandStruct(const char* input,int controlfd){
  char aux1[100],aux2[100],aux3[100];
  bzero(aux1,100);
  bzero(aux2,100);
  bzero(aux3,100);

  sscanf(input,"%s %s %s",aux1,aux2,aux3);

  //printf("Param1: %s  size:%i \nParam2: %s  size:%i\nParam3: %s  size:%i\n",aux1,strlen(aux1),aux2,strlen(aux2),aux3,strlen(aux3));

  if (!checkMaincommand(aux1)){
    printf("Not a valid command\n");
    return false;
  }

  if (strlen(aux3)!=0){
    printf("Too many arguments for %s\n",aux1);
    return false;
  }

  if (strlen(aux2)!=0 && strcmp(aux1,"quit")==0){
    printf("Too many arguments for %s\n",aux1);
    return false;
  }

  if (strlen(aux2)==0 && strcmp(aux1,"quit")!=0 && strcmp(aux1,"ls")!=0){
    printf("Not enough arguments for %s\n",aux1);
    return false;
  }
  return sendRequest(aux1,aux2,controlfd);
}





int main(int argc, char **argv){
  int controlfd;
  struct sockaddr_in  servaddr;

  //Checks if the usage is correct
  if (argc != 3){
    printf("usage: ./ftpclient <Server-IP> <Listening-Port>");
    exit(1);
  }

  //socket creates a TCP socket (returns socket identifier)
  controlfd = socket(AF_INET, SOCK_STREAM, 0);
  if (controlfd < 0)
    error("Error initializing socket (socket())\n");

  //Set struct to 0
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port   = htons(atoi(argv[2])); /* daytime server */
  // Htons converts to the binary port number
  //inet_pton converts ASCII argument to the proper format ("presentation to numeric")
  if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr)!=1)
    error("Error getting the address of the FTP server");


  //conection between sockets, 3rd argument is the length of the socket adress
  //SA is a generic sockaddr struct
  if(connect(controlfd, (SA *) &servaddr, sizeof(servaddr))==-1)
    error("Error connecting to the FTP server");


  int dataConnectionID=0;
  char client_command[LENGTH_COMMAND+1],server_response[LENGTH_COMMAND+1];
  while(1){
    bzero(client_command,LENGTH_COMMAND+1);
    bzero(server_response,LENGTH_COMMAND+1);
    printf("ftp>");
    fgets(client_command,LENGTH_COMMAND,stdin);
    //Taken the /n character off
    client_command[strlen(client_command)-1]='\0';



    send(controlfd,client_command, LENGTH_COMMAND , 0);

    //if we don receive a response the server must have been disconnected
    if(recv(controlfd,server_response, LENGTH_COMMAND, 0)<=0 || strcmp(client_command, "quit") == 0){
      printf("%s\n",server_response);
      printf("Disconnected from the server\n");
      break;
    }

    printf("%s\n",server_response);
  }
  close(controlfd);
  exit(0);
}

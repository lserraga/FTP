#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
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

void Send(int fd , char* message ,int messageLength){

  if(send(fd , message , messageLength, 0)==-1){
    printf("Error sending the following command: %s\n",messageLength);
    exit(0);
  }
}

//Functions that sends a request to the server (after already checking that the 
//parameters are correct). It returns true if it was correct and false elsewhere
bool translateRequest(const char* param1, const char* param2, int controlfd){
  FILE *desiredFile;
  char aux[200];

  if (strcmp(param1,"quit")==0){
    Send(controlfd,"QUIT", LENGTH_COMMAND);
  }
  //checks if the file exists
  else if (strcmp(param1,"put")==0){
    desiredFile = fopen(param2, "r");
    if (desiredFile==NULL){
      printf("Unable to open %s\n",param2);
      return false;
    }
    fclose(desiredFile);
    strcpy(aux,"STOR ");
    strcat(aux,param2);
    Send(controlfd,aux, LENGTH_COMMAND);
  }

  else if (strcmp(param1,"ls")==0){
    strcpy(aux,"LIST ");
    strcat(aux,param2);
    Send(controlfd,aux, LENGTH_COMMAND);
  }
  else{
    strcpy(aux,"RETR ");
    strcat(aux,param2);
    Send(controlfd,aux, LENGTH_COMMAND);
  }
  return true;
}

//Functions that reads a charr array (input), returns true or false
//depending if the command structure is valid. It also writes the first 
//parameter in param1 and the second in param2
bool sendRequest(const char* input,int controlfd){
  char aux1[100],aux2[100],aux3[100];
  bzero(aux1,100);
  bzero(aux2,100);
  bzero(aux3,100);

  sscanf(input,"%s %s %s",aux1,aux2,aux3);

  //printf("Param1: %s  size:%i \nParam2: %s  size:%i\nParam3: %s  size:%i\n",aux1,strlen(aux1),aux2,strlen(aux2),aux3,strlen(aux3));

  if (checkMaincommand(aux1)==false){
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
  return translateRequest(aux1,aux2,controlfd);
}

//Initialize the socket, bind it and make it a listener. It sends the address
//to the server and returns the id of the listener socket
int sendPortCommand(const struct sockaddr_in newAddress, int controlfd){
  char ipAdress[INET_ADDRSTRLEN];
  int port,portaux;
  int datafd;
  struct sockaddr_in auxaddress;
  socklen_t auxlen = sizeof(auxaddress);
  char * temp = NULL;
  char message[200],aux[10];
  strcpy(message,"PORT ");


  datafd=socket(AF_INET, SOCK_STREAM, 0);


  bind(datafd, (SA * ) & newAddress, sizeof(newAddress));
  listen(datafd, LISTENQ);

  ////////inet_ntop(AF_INET, & (newAddress.sin_addr.s_addr), ipAdress, INET_ADDRSTRLEN);
  //Getting the proxy addres for the logging 
  char serevraddres[INET_ADDRSTRLEN];
  getsockname(datafd, (SA * ) & auxaddress, & auxlen);
  inet_ntop(AF_INET, & (auxaddress), serevraddres, INET_ADDRSTRLEN);
  ///////port=ntohs(newAddress.sin_port);
  portaux=ntohs(auxaddress.sin_port);
  //printf("Client IP='%s', data PORT='%i'\n",serevraddres,portaux);

  temp = strtok(serevraddres, ".");
  while(temp){
    strcat(message,temp);
    strcat(message,",");
    bzero(temp, strlen(temp));
    temp = strtok(NULL, ".");
  }

  sprintf(aux,"%i",portaux/256);
  strcat(message,aux);
  strcat(message,",");
  bzero(aux,10);
  sprintf(aux,"%i",portaux%256);
  strcat(message,aux);

  //printf("Message: %s\n",message);

  send(controlfd,message, LENGTH_COMMAND , 0); 

  return datafd; 
}

//Function that waits for an incoming connection and handles the transfer of data to or from the server
bool transferingData(const char* param1, const char* param2,int listenfd,int controlfd){
  char dataRespone[MAXLINE+1], server_response[LENGTH_COMMAND+1];
  int datafd,n,sizeRecived=0;
  struct sockaddr_in servaddrData;
  int serverlen = sizeof(servaddrData);
  FILE* desiredFile;
  int fd;
  struct stat file_stat;

  //Accepting the server connection
  datafd = accept(listenfd, (SA * ) &servaddrData, &serverlen);
  bzero(server_response,LENGTH_COMMAND+1);
  if(recv(controlfd,server_response, LENGTH_COMMAND, 0)<0){
    printf("Error sending data port to server (no message back)\n");
    close(datafd);
    return false;
  }
  //We have to recieve a 201 port ok from the server
  //printf("Answer after port command: %s\n",server_response,datafd);
  if(strncmp(server_response,"201",3)!=0){
    printf("Error sending data port to server (received error message)\n");
    close(datafd);
    return false;
  }

  bzero(dataRespone,MAXLINE+1);

  //Receiving a file
  if (strcmp(param1,"get")==0){
    desiredFile = fopen(param2, "w");
    while((n=recv(datafd,dataRespone, MAXLINE, 0))>0){
      dataRespone[n]='\0';
      sizeRecived+=n;
      fwrite(dataRespone, sizeof(char), n, desiredFile);
    }
    printf("Received %i bytes\n",sizeRecived);
    fclose(desiredFile);
  }

  //Recieveing a listing
  else if(strcmp(param1,"ls")==0){
    while((n=recv(datafd,dataRespone, MAXLINE, 0))>0){
      dataRespone[n]='\0';
      //printf("Received %i bytes\n",n);
      printf("%s\n",dataRespone);
    }
  }
  //Sending a file
  else if(strcmp(param1,"put")==0){
    fd = open(param2, O_RDONLY);
    fstat(fd, &file_stat);
    sendfile (datafd, fd, NULL, file_stat.st_size);
    printf("Sent %i Bytes\n",file_stat.st_size);
    close(fd);
  }
  
  close(datafd);
  return true;
}




int main(int argc, char **argv){
  int controlfd,listenfd;
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

  char client_command[LENGTH_COMMAND+1],server_response[LENGTH_COMMAND+1];
  char param1[100],param2[100];
  struct sockaddr_in dataAddress;
  

  while(1){
    bzero(client_command,LENGTH_COMMAND+1);
    bzero(server_response,LENGTH_COMMAND+1);
    bzero(param1,100);
    bzero(param2,100);

    //Getting the command from the user
    printf("ftp>");
    fgets(client_command,LENGTH_COMMAND,stdin);
    //Taken the /n character off
    client_command[strlen(client_command)-1]='\0';
    sscanf(client_command,"%s %s",param1,param2);

    //SendRequest checks if the command is correct before sending anything
    if(sendRequest(client_command,controlfd)==false)
      continue;

    //if we don receive a response the server must have been disconnected
    if(recv(controlfd,server_response, LENGTH_COMMAND, 0)<=0 || strcmp(client_command, "quit") == 0){
      //printf("%s\n",server_response);
      printf("Disconnected from the server\n");
      break;
    }

    //printf("%s\n",server_response);

    //The server response must have been a 200 command OK to continue
    if(strncmp(server_response,"200",3)==0){

      //Setting up the new data connection
      bzero( & dataAddress, sizeof(dataAddress));
      dataAddress.sin_family = AF_INET;
      dataAddress.sin_addr.s_addr = htonl(INADDR_ANY);
      dataAddress.sin_port = 0;
      
      //Setting up the listenting port and sending the address to the server
      listenfd=sendPortCommand(dataAddress,controlfd);
      //Accepting the connection and sening the data
      transferingData(param1,param2,listenfd,controlfd);
    }

  }
  close(controlfd);
  exit(0);
}

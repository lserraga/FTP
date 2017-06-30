#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/sendfile.h>
#define MAXLINE 4096
#define SA struct sockaddr
#define LISTENQ 1024
#define LENGTH_COMMAND 200


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

//Function that checks if a Server Command is valid
bool checkMaincommand(const char* command){
  if (strcmp(command,"QUIT")==0 || strcmp(command,"ABOR")==0 || strcmp(command,"LIST")==0)
    return true;
  if (strcmp(command,"PORT")==0 || strcmp(command,"RETR")==0 || strcmp(command,"STOR")==0)
    return true;
  return false;
}

//Functions that reads a charr array (input), returns true or false
//depending if the command structure is valid. It also returns the first 
//parameter in param1 and the second in param2
bool checkCommandStruct(const char* input,char *param1,char *param2,int controlfd){
  char aux3[100];
  bzero(param1,strlen(param1));
  bzero(param2,strlen(param2));
  bzero(aux3,100);

  //Getting command and parameters
  sscanf(input,"%s %s %s",param1,param2,aux3);

  printf(" Received a message: Command: %s Parameter: %s\n",param1,param2);

  if (checkMaincommand(param1)==false){
    send(controlfd,"500 Syntax error (unrecognized command)", LENGTH_COMMAND , 0);
    return false;
  }

  //if it has 3 arguments its wrong
  if (strlen(aux3)!=0 || (strlen(param2)!=0 && (strcmp(param1,"QUIT")==0 || strcmp(param1,"ABOR")==0))){
    send(controlfd,"501 Syntax error (Too many arguments)", LENGTH_COMMAND , 0);
    return false;
  }

  if (strlen(param2)==0){
    if (strcmp(param1,"QUIT")!=0 && strcmp(param1,"ABOR")!=0 && strcmp(param1,"LIST")!=0){
      send(controlfd,"501 Syntax error (Not enough arguments)", LENGTH_COMMAND , 0);
      return false;
    }
  }
  return true;
}

//Functions that sends an answer to the client (after already checking that the 
//parameters are correct). It returns true if you have to wait for the port
//for the data connection and false otherwise
bool sendResponse(const char *input,int controlfd,bool sendIfOk,bool expectedPort){
    FILE *desiredFile;
    DIR* desiredDir;
    char param1[100];
    char param2[100];
    char *temp=NULL;
    int i=0;
    
    //Checking the structure of the command
    if (checkCommandStruct(input, param1,param2,controlfd)==false)
        return false;

    //Checking whether we were expecting a PORT command
    if (expectedPort && strcmp(param1,"PORT")!=0){
        send(controlfd,"502 Expected port command", LENGTH_COMMAND , 0);
        return false;
    }
    if (!expectedPort && strcmp(param1,"PORT")==0){
        send(controlfd,"502 Port command not expected", LENGTH_COMMAND , 0);
        return false;
    }

    //strtok modifies the char array so we have to make a copy
    char *aux=malloc(strlen(param2));
    strcpy(aux,param2);

    //User decided to quit
    if (strcmp(param1,"QUIT")==0){
        send(controlfd,"221 Goodbye", LENGTH_COMMAND , 0);
        return false;
    }

    //Checking if the file to Retrieve exists
    if (strcmp(param1,"RETR")==0){
        desiredFile = fopen(param2, "r");
        if (desiredFile==NULL){
            send(controlfd,"550 File Not Found", LENGTH_COMMAND , 0);
            return false;
        }
        fclose(desiredFile);
    }

    //Checking if the ls will return something
    else if (strcmp(param1,"LIST")==0 && strlen(param2)>0){
        desiredDir = opendir(param2);
        if (desiredDir==NULL && access( param2, F_OK ) == -1){
            send(controlfd,"550 Directory/File Not Found", LENGTH_COMMAND , 0);
            return false;
        }
        if(desiredDir!=NULL)
            closedir(desiredDir);
    }

    //Checking validity of port command 
    else if (strcmp(param1,"PORT")==0){
        temp = strtok(aux, ",");
        //All the numbers must be between 0 and 256
        while(temp){
            if(atoi(temp)<0 || atoi(temp)>256){
                send(controlfd,"501 Syntax error (PORT: not a valid address)", LENGTH_COMMAND , 0);
                free(aux);
                return false;
            }
            bzero(temp, strlen(temp));
            temp = strtok(NULL, ",");
            i++;
        }
        if(i!=6){
            send(controlfd,"501 Syntax error (PORT: not a valid address)", LENGTH_COMMAND , 0);
            free(aux);
            return false;
        }
        free(aux);
    }
    //We dont always want to asnwer right away. Like with port, we must try to make the connection first
    else if(sendIfOk==true && strcmp(param1,"ABOR")==0){
        send(controlfd,"200 Command OK", LENGTH_COMMAND , 0);
        return false;
    }
    if(sendIfOk==true)
        send(controlfd,"200 Command OK", LENGTH_COMMAND , 0);
    return true;
}
   
//Gets the IP and the port in char arrays from a PORT message. Has to be called after cchecking that the
//Port message structure is correct
void getAddressStruct(const char* portMessage, char* addressIP, int *port){
    bzero(addressIP,strlen(addressIP));
    char param1[100];
    char param2[100];
    char *temp;
    int i=0;

    sscanf(portMessage,"%s %s",param1,param2);

    temp = strtok(param2, ",");
    while(temp){
        if(i==0){
            strcpy(addressIP,temp);
            strcat(addressIP,".");
        }
        else if(i<4){
            strcat(addressIP,temp);
            strcat(addressIP,".");
        }
        else if(i==4)
            *port=atoi(temp)*256;
        else
            *port=*port+atoi(temp);
        bzero(temp, strlen(temp));
        temp = strtok(NULL, ",");
        i++;
    }
    addressIP[strlen(addressIP)-1]='\0';

    return;
}

//Send error message
void send425(int controlfd,int datafd){
    printf("Error making the data connection\n");
    send(controlfd,"425 Can't open data connection", LENGTH_COMMAND , 0);
    close(datafd);
}

//Given a data connection and 2 parameters this function will handle
// the transfer of the data to the client or from the client
void transferingData(const char* param1, const char* param2,int datafd){
    int fd,sizeReceived=0,n;
    DIR *dp;
    struct dirent *ep;
    struct stat file_stat;
    char dataResponse[MAXLINE+1], aux[200],path[100];   

    //Send the listing
    //I know I did in a really weird way but I didnt have time to change it to a popen()
    if (strcmp(param1,"LIST")==0){
        strcpy(path,"./");
        strcat(path,param2);
        dp = opendir (path);
        if (access( param2, F_OK )!=-1 && dp==NULL){
            send (datafd, param2, MAXLINE, 0);
        }
        else{
            sprintf(dataResponse,"File listing for the directory %s\n",param2);
            if (dp){
                while ((ep = readdir(dp)) != NULL){
                    bzero(aux,200);
                    sprintf(aux,"%s\t\t",ep->d_name);
                    if(ep->d_type==DT_DIR)
                        strcat(aux,"DIR\n");
                    else
                        strcat(aux,"FILE\n");
                    strcat(dataResponse,aux);
                }
                send (datafd, dataResponse, MAXLINE, 0);
                printf("Sent %i Bytes\n",strlen(dataResponse));
                closedir(dp);
            }
        }
    }
    //Send the file
    else if(strcmp(param1,"RETR")==0){
        fd = open(param2, O_RDONLY);
        fstat(fd, &file_stat);
        sendfile (datafd, fd, NULL, file_stat.st_size);
        printf("Sent %i Bytes\n",file_stat.st_size);
        close(fd);
        
    }
    //Receive the file
    else if(strcmp(param1,"STOR")==0){
        fd = open(param2, O_CREAT|O_WRONLY, S_IRUSR | S_IWUSR);
        while((n=recv(datafd,dataResponse, MAXLINE, 0))>0){
          dataResponse[n]='\0';
          sizeReceived+=n;
          write(fd, dataResponse, n);
        }
        printf("Received %i bytes\n",sizeReceived);
        close(fd);        
    }

    printf("Data connection terminated\n");
    close(datafd);
    return;
}

//This function will try to connect to the clients data connection included in portMessag
//,if it fails to connect, it will send a 425 message to the conrol coneection and a 201 
//otherwise. It will return the data file descriptor connection, negative if something went wrong
int dataConnection(const char* portMessage,int controlfd,const char* param1, const char* param2){
    char addressIP[100];
    int port;
    int datafd;
    struct sockaddr_in  clientaddres;

    getAddressStruct(portMessage,addressIP,&port);

    //socket creates a TCP socket (returns socket identifier)
    datafd = socket(AF_INET, SOCK_STREAM, 0);
    if(datafd<0){
        printf("Error making the data connection");
        return datafd;
    }
    //Set struct to 0
    bzero(&clientaddres, sizeof(clientaddres));
    clientaddres.sin_family = AF_INET;
    clientaddres.sin_port   = htons(port);
    if(inet_pton(AF_INET, "127.0.0.1", &clientaddres.sin_addr)!=1){
        send425(controlfd,datafd);
    }
    else if(connect(datafd, (SA *) &clientaddres, sizeof(clientaddres))==-1){
        send425(controlfd,datafd);
    }   
    else{
        printf("Data connection, client IP: %s Port: %i\n",addressIP,port);
        send(controlfd,"201 Data connection OK", LENGTH_COMMAND , 0);
    }

    return datafd;
}


int main(int argc, char * * argv){
    int listenfd;
    struct sockaddr_in servaddr;
    
    //Check if the arguments are okey
    if (argc != 2) {
        printf("Usage: <Listening-port>");
        exit(1);
    }

    //Creation of the TCP socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
        error("Error initializing socket (socket())\n");
    bzero( & servaddr, sizeof(servaddr));
    //Creating the address struck
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));
    
    //Bind the socket to the address
    if (bind(listenfd, (SA * ) & servaddr, sizeof(servaddr)) < 0)
      error("Error binding socket (bind())\n");

    //Socket converted into a listening socket that will accept incoming connections
    //ListenQ defines the maximum number of connection s in line

    if (listen(listenfd, LISTENQ) < 0)
        error("Error on listening");

    printf("**** FTP server up and running**** \n\nCreated by Luis Serra Garcia\n");
    printf("Network Programming-University of California, Santa Cruz\n\n\n");
    

    //Infinite loop waiting for connection
    for (;;) {
        char client_command[LENGTH_COMMAND+1];
        int controlfd;
        int datafd;
        char param1[100],param2[100];
        struct sockaddr_in cli_addr,cli_addrData;
        int clilen = sizeof(cli_addr), clilenData = sizeof(cli_addrData);
        pid_t pid;

        //Accept the connection from the client
        //Accepting the connection request from the client
        controlfd = accept(listenfd, (SA * ) &cli_addr, &clilen);
        if (controlfd < 0) {
            printf("Error accepting connection\n");
            continue;
        }

        //Forking the client connection for server concurrency
        pid = fork();
        if (pid==0){
            char clientaddres[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, & (cli_addr), clientaddres, INET_ADDRSTRLEN);
            printf("Client connected: ip='%s'\n",clientaddres);
            

            bzero(client_command,LENGTH_COMMAND+1);
            //Waits for commands from the client. It it says quit or disconnects
            //suddenly, it closes the connection
            while(recv(controlfd,client_command, LENGTH_COMMAND, 0)>0){
                bzero(param1,100);
                bzero(param2,100);
                sscanf(client_command,"%s %s",param1,param2);

                //With sendResponse, the structure of the client command is also checked
                //It also send the response to the control connection
                if (sendResponse(client_command,controlfd,true,false)==true){
                    //We have to receive a PORT command
                    printf("We have to receive a port command\n");
                    bzero(client_command,LENGTH_COMMAND+1);
                    recv(controlfd,client_command, LENGTH_COMMAND, 0);
                    //If sendResponse returns true, we have received a correct port command
                    if (sendResponse(client_command,controlfd,false,true)==1){
                        //Setting up data connection
                        if((datafd=dataConnection(client_command,controlfd,param1,param2))>=0)
                            transferingData(param1,param2,datafd);
                    }
                }

                bzero(client_command,LENGTH_COMMAND+1);
            }
            close(controlfd);
            printf("Client with ip='%s' port='%i' disconnected\n",clientaddres,ntohs(cli_addr.sin_port));
            //Exiting child
            exit(0);
        }
        else{
            //Parent
            close(controlfd);
        }
    }
    close(listenfd);
}

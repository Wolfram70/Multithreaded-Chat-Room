#define _XOPEN_SOURCE_EXTENDED 1

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <thread>
#include <stdlib.h>

#define PORT 5000
#define MAX_CLIENTS 5

int* connectedClientSockets;
bool* joinedClients;
bool chatRunning = true;
char closedMessage[64];
char** clientNames;

void moderator();
void acceptNewConnections(int);
void communicateWithClient(int, int);

void acceptNewConnections(int listeningSocket)
{
  int clientID = 0;
  sockaddr_in clientAddress;
  bzero(&clientAddress, sizeof(clientAddress));
  socklen_t clientAddressLength = sizeof(clientAddress);

  std::thread clientThreads[MAX_CLIENTS];

  while(chatRunning)
  {
    connectedClientSockets[clientID] = accept(listeningSocket, (sockaddr*)&clientAddress, &clientAddressLength);
    clientThreads[clientID] = std::thread(communicateWithClient, connectedClientSockets[clientID], clientID);
    clientThreads[clientID].detach();
    clientID++;
  }
}

void moderator()
{
  char input[256];
  char inputCopy[256];
  char moderatorMessage[256];
  char leave[] = "!leavechatroom";
  while(chatRunning)
  {
    std::cin.getline(input, 256);
    strcpy(inputCopy, input);

    char* keyword = strtok(inputCopy, " ");

    sprintf(moderatorMessage,"\x1b[31mModerator:\x1b[0m %s",input);

    if(!strcmp(input, "close"))
    {
      chatRunning = false;
      for(int i = 0; i < MAX_CLIENTS; i++)
      {
        send(connectedClientSockets[i], closedMessage, sizeof(closedMessage), 0);
      }
      break;
    }
    else if(!strcmp(input, "list"))
    {
      for(int i = 0; i < MAX_CLIENTS; i++)
      {
        if(joinedClients[i])
        {
          std::cout << "\x1b[31m" <<clientNames[i] << "\x1b[0m" << std::endl;
        }
      }
    }
    else if(!strcmp(keyword, "ban"))
    {
      char* bannedClient = strtok(NULL, "\0");
      for(int i = 0; i < MAX_CLIENTS; i++)
      {
        if(!strcmp(bannedClient, clientNames[i]))
        {
          send(connectedClientSockets[i], leave, sizeof(leave), 0);
          std::cout << "\x1b[31m" << bannedClient << " has been banned.\x1b[0m" << std::endl;
          joinedClients[i] = false;
          break;
        }
      }
    }
    else if(!strcmp(keyword, "timeout"))
    {
      char *timeoutClient = strtok(NULL, " ");
      char *timeoutTime = strtok(NULL, "\0");
      int timeoutTimeInt = atoi(timeoutTime);

      char timeoutMessage[256] = "!timeout ";
      strcat(timeoutMessage, timeoutTime);

      for(int i = 0; i < MAX_CLIENTS; i++)
      {
        if(!strcmp(timeoutClient, clientNames[i]))
        {
          send(connectedClientSockets[i], timeoutMessage, sizeof(timeoutMessage), 0);
          std::cout << "\x1b[31m" << timeoutClient << " has been timed out for " << timeoutTime << " seconds.\x1b[0m" << std::endl;
          break;
        }
      }
    }
    else
    {
      for(int i = 0; i < MAX_CLIENTS; i++)
      {
        if(joinedClients[i])
        {
          send(connectedClientSockets[i], moderatorMessage, sizeof(moderatorMessage), 0);
        }
      }
    }
  }
}

bool isPrivate(char* message, int& privateClientID, char* trueMessage)
{
  int i, j;
  for(i = 0; i < 256; i++)
  {
    if(message[i] == '-') break;
    trueMessage[i] = message[i];
  }
  trueMessage[i] = '\0';
  i++;

  if(!strcmp(trueMessage, "private"))
  {
    for(j = 0;i < 256; i++, j++)
    {
      if(message[i] == ':') break;
      trueMessage[j] = message[i];
    }
    trueMessage[j] = '\0';

    for(int k = 0; k < MAX_CLIENTS; k++)
    {
      if(joinedClients[k] && !strcmp(trueMessage, clientNames[k]))
      {
        privateClientID = k;
        for(j = 0; i < 256; i++, j++)
        {
          trueMessage[j] = message[i];
        }
        trueMessage[j] = '\0';
        return true;
      }
    }
    privateClientID = -2;
    return true;
  }

  privateClientID = -1;
  return false;
}

void communicateWithClient(int clientSocket, int clientID) 
{
  char recieveBuffer[256];
  char sendBuffer[256];

  char clientName[256];

  char trueMessageBuffer[256];

  sprintf(sendBuffer, "\x1b[36mPlease enter your name: \x1b[0m");
  send(clientSocket, sendBuffer, sizeof(sendBuffer), 0);

  while(recv(clientSocket, recieveBuffer, sizeof(recieveBuffer), 0) <= 0);
  strcpy(clientName, recieveBuffer);

  recieveBuffer[0] = '\0'; 

  sprintf(sendBuffer,"\x1b[32mYour name has been set to\x1b[0m \x1b[33m%s\x1b[0m\x1b[32m. You can start conversing!!\x1b[0m",clientName);
  send(clientSocket, sendBuffer, sizeof(sendBuffer), 0);

  sendBuffer[0] = '\0';

  joinedClients[clientID] = true;

  printf("\x1b[32mClient %d joined as \x1b[0m",clientID + 1);
  printf("\x1b[33m%s\x1b[0m",clientName);
  std::cout<<std::endl;

  clientNames[clientID] = clientName;

  sprintf(sendBuffer, "\x1b[33m%s\x1b[0m \x1b[32mhas joined the chat.\x1b[0m", clientName);

  for(int i = 0; i < MAX_CLIENTS; i++)
  {
    if((i != clientID) && joinedClients[i])
    {
      send(connectedClientSockets[i], sendBuffer, sizeof(sendBuffer), 0);
    }
  }

  char clientNameCopy[256]; 
  strcpy(clientNameCopy,clientName);

  char leaveChat[] = "!leavechatroom";

  int privateClientID = -1;

  while(chatRunning)
  {
    while(recv(clientSocket, recieveBuffer, sizeof(recieveBuffer), 0) <= 0);

    if(isPrivate(recieveBuffer, privateClientID, trueMessageBuffer))
    {
      if(privateClientID == -2)
      {
        sprintf(sendBuffer, "\x1b[31mClient not found.\x1b[0m");
        send(clientSocket, sendBuffer, sizeof(sendBuffer), 0);
      }
      else
      {
        sprintf(sendBuffer, "\x1b[35mPRIVATE MESSAGE FROM %s: \x1b[0m%s", clientName, trueMessageBuffer+1);
        send(connectedClientSockets[privateClientID], sendBuffer, sizeof(sendBuffer), 0);
      }
      privateClientID = -1;
      continue;
    }

    if(!strcmp(recieveBuffer, leaveChat))
    {
      joinedClients[clientID] = false;
      sprintf(sendBuffer, "\x1b[31mYou have left the chat.\x1b[0m");
      send(clientSocket, sendBuffer, sizeof(sendBuffer), 0);
      sprintf(sendBuffer, "\x1b[33m%s\x1b[0m \x1b[31mhas left the chat.\x1b[0m", clientName);
      for(int i = 0; i < MAX_CLIENTS; i++)
      {
        if((i != clientID) && joinedClients[i])
        {
          send(connectedClientSockets[i], sendBuffer, sizeof(sendBuffer), 0);
        }
      }
      return;
    }

    sprintf(sendBuffer, "\x1b[33m%s:\x1b[0m\x1b[0m %s",clientName, recieveBuffer);
    std::cout<<sendBuffer<<std::endl;

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
      if((i != clientID) && joinedClients[i])
      {
        send(connectedClientSockets[i], sendBuffer, sizeof(sendBuffer), 0);
      }
    }

    sendBuffer[0] = '\0';
    recieveBuffer[0] = '\0';
  }
}

int main()
{ 
  sprintf(closedMessage,"\x1b[31mATTENTION: Chatroom is closed \x1b[0m");
  std::cout<<"Initialising server..."<<std::endl;

  //creating the socket that listens for connections
  int listeningSocket;
  listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
  
  //defining the server address
  sockaddr_in serverAddress;
  bzero(&serverAddress, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(PORT);

  //binding the listening socket to the server address
  bind(listeningSocket, (sockaddr*)&serverAddress, sizeof(serverAddress));

  //listening for connections
  listen(listeningSocket, MAX_CLIENTS);

  connectedClientSockets = new int[MAX_CLIENTS];
  joinedClients = new bool[MAX_CLIENTS];
  clientNames = new char*[MAX_CLIENTS];

  for(int i = 0; i < MAX_CLIENTS; i++)
  {
    joinedClients[i] = false;
  }

  std::cout<<"Server initialised"<<std::endl;

  std::thread acceptorThread(acceptNewConnections, listeningSocket);
  std::thread moderatorThread(moderator);

  while(chatRunning);

  acceptorThread.detach();
  moderatorThread.detach();

  //sleep for 2000 milliseconds
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));

  close(listeningSocket);
  /*for(int i = 0; i < MAX_CLIENTS; i++)
  {
    if(joinedClients[i])
    {
      close(connectedClientSockets[i]);
    }
  }*/

  return 0;
}

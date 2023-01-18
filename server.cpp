#define _XOPEN_SOURCE_EXTENDED 1

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <thread>

#define PORT 5000
#define MAX_CLIENTS 10

int* connectedClientSockets;
bool chatRunning = true;
char closedMessage[] = "ATTENTION: Chatroom is closed.";

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
    std::cout << "Client " << clientID << " connected." << std::endl;
    clientThreads[clientID] = std::thread(communicateWithClient, connectedClientSockets[clientID], clientID);
    clientThreads[clientID].detach();
    clientID++;
  }
}

void moderator()
{
  char input[256];
  char moderatorMessage[256];
  while(chatRunning)
  {
    std::cin.getline(input, 256);

    sprintf(moderatorMessage, "Moderator: %s", input);

    if(!strcmp(input, "close"))
    {
      chatRunning = false;
      for(int i = 0; i < MAX_CLIENTS; i++)
      {
        send(connectedClientSockets[i], closedMessage, sizeof(closedMessage), 0);
      }
      break;
    }
    else
    {
      for(int i = 0; i < MAX_CLIENTS; i++)
      {
        send(connectedClientSockets[i], moderatorMessage, sizeof(moderatorMessage), 0);
      }
    }
  }
}

void communicateWithClient(int clientSocket, int clientID) 
{
  char recieveBuffer[256];
  char sendBuffer[256];

  while(chatRunning)
  {
    while(recv(clientSocket, recieveBuffer, sizeof(recieveBuffer), 0) <= 0);
    std::cout << "Client " << clientID << " sent: " << recieveBuffer << std::endl;

    sprintf(sendBuffer, "Client %d: %s", clientID, recieveBuffer);
    
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
      if(i != clientID)
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

  std::thread acceptorThread(acceptNewConnections, listeningSocket);
  std::thread moderatorThread(moderator);

  while(chatRunning);

  acceptorThread.detach();
  moderatorThread.detach();

  //sleep for 2000 milliseconds
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));

  close(listeningSocket);

  return 0;
}

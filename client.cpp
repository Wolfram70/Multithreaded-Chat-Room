#define _XOPEN_SOURCE_EXTENDED 1

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <thread>

#define PORT 5000

bool chatRunning = true;
char closed[] = "ATTENTION: Chatroom is closed.";

void sendServer(int clientSocket)
{
  char sendBuffer[256];
  
  while(chatRunning)
  {
    std::cin.getline(sendBuffer, 256);
    send(clientSocket, sendBuffer, sizeof(sendBuffer), 0);
    sendBuffer[0] = '\0';
    if(!chatRunning)
    {
      break;
    }
  }
}

void recieveFromServer(int clientSocket)
{
  char recieveBuffer[256];
  
  while(chatRunning)
  {
    while(recv(clientSocket, recieveBuffer, sizeof(recieveBuffer), 0) < 0);
    std::cout << recieveBuffer << std::endl;
    if(strcmp(recieveBuffer, closed) == 0)
    {
      chatRunning = false;
      break;
    }
    recieveBuffer[0] = '\0';
  }
}

int main()
{
  //creating the socket to communicate with the server
  int clientSocket;
  clientSocket = socket(AF_INET, SOCK_STREAM, 0);

  //defining the server address
  sockaddr_in serverAddress;
  bzero(&serverAddress, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(PORT);
  serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

  //connect to the server
  connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress));

  std::thread sendThread = std::thread(sendServer, clientSocket);
  std::thread recieveThread = std::thread(recieveFromServer, clientSocket);

  while(chatRunning);

  //terminate the threads forcefully
  sendThread.detach();
  recieveThread.detach();

  //sleep for 200 milliseconds
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  close(clientSocket);

  return 0;
}

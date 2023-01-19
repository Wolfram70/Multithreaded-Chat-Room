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
char* image;
char smallImage[256] = "image";
char largeImage[256] = "IMAGE";

void sendImage(char* filename, int clientSocket)
{
  int size = 0;
  char sendBuffer[256];

  sendBuffer[0] = '\0';
  strcpy(sendBuffer, "IMAGE");

  FILE* imageFile = fopen(filename, "rb");

  fread(image, 1, sizeof(image) - 1, imageFile);
  fseek(imageFile, 0, SEEK_END);
  size = ftell(imageFile);
  std::cout << size << " " << filename << " end" << sizeof(filename) << std::endl;
  fclose(imageFile);

  send(clientSocket, sendBuffer, sizeof(sendBuffer), 0);
  //sleep for 150ms to allow server to receive message
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  send(clientSocket, filename, 256, 0);
  //sleep for 30ms to allow server to receive message
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  send(clientSocket, (void *)&size, sizeof(int), 0);
  //sleep for 30ms to allow server to receive message
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  send(clientSocket, image, 300000, 0);
}

void recieveImage(int clientSocket)
{
  char fileName[256];
  int size;

  //sleep for 10ms to allow server to send message
  std::this_thread::sleep_for(std::chrono::milliseconds(90));
  while(recv(clientSocket, fileName, sizeof(fileName), 0) <= 0);
  //sleep for 10ms to allow server to send message
  std::this_thread::sleep_for(std::chrono::milliseconds(90));
  while(recv(clientSocket, (void *)&size, sizeof(int), 0) <= 0);
  //sleep for 10ms to allow server to send message
  std::this_thread::sleep_for(std::chrono::milliseconds(90));
  while(recv(clientSocket, image, 300000, 0) <= 0);
  std::cout << size << " " << fileName << " end" << std::endl;
  FILE* imageFile = fopen(fileName, "wb");
  fwrite(image, 1, size, imageFile);
  fclose(imageFile);
}

void sendServer(int clientSocket)
{
  char sendBuffer[256];
  char fileName[256];
  
  while(chatRunning)
  {
    std::cin.getline(sendBuffer, 256);
    if(!strcmp(sendBuffer, smallImage))
    {
      std::cin.getline(fileName, 256);
      sendImage(fileName, clientSocket);
      fileName[0] = '\0';
      continue;
    }
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
    while(recv(clientSocket, recieveBuffer, sizeof(recieveBuffer), 0) <= 0);
    if(strcmp(recieveBuffer, largeImage) == 0)
    {
      recieveImage(clientSocket);
      recieveBuffer[0] = '\0';
      continue;
    }
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

  image = new char[300000];
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

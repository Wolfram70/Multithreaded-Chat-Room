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
bool timedOut = false;
int timedoutTime = 0;
char closed[] = "\x1b[31mATTENTION: Chatroom is closed \x1b[0m";

void sendServer(int clientSocket)
{
  char sendBuffer[256];
  char leave[] = "!leavechatroom";
  
  while(chatRunning)
  {
    if(timedOut)
    {
      std::this_thread::sleep_for(std::chrono::seconds(timedoutTime));
      //sleep for timedOutTime seconds
      usleep(timedoutTime * 1000000);
      timedOut = false;
      std::cout << "\x1b[31mATTENTION: You can send messages.\x1b[0m"<< std::endl;
      fflush(stdin);
    }
    fflush(stdin);
    std::cin.getline(sendBuffer, 256);
    send(clientSocket, sendBuffer, sizeof(sendBuffer), 0);
    if(!strcmp(sendBuffer, leave))
    {
      //sleep for 1s
      std::this_thread::sleep_for(std::chrono::seconds(1));
      chatRunning = false;
    }
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
  char recieveBufferCopy[256];
  char leave[] = "!leavechatroom";
  
  while(chatRunning)
  {
    while(recv(clientSocket, recieveBuffer, sizeof(recieveBuffer), 0) < 0);
    strcpy(recieveBufferCopy, recieveBuffer);
    if(!strcmp(recieveBuffer, leave))
    {
      chatRunning = false;
      std::cout << "\x1b[31mATTENTION: You have been banned from the chatroom \x1b[0m" << std::endl;
      return;
    }
    if(!strcmp(strtok(recieveBufferCopy, " "), "!timeout"))
    {
      timedOut = true;
      timedoutTime = atoi(strtok(NULL, "\0"));
      std::cout << recieveBuffer << timedoutTime << std::endl;
      std::cout << "\x1b[31mATTENTION: You have been timed out for " << timedoutTime << " seconds \x1b[0m" << std::endl;
      continue;
    }
    std::cout << recieveBuffer << std::endl;
    if(strcmp(recieveBuffer, closed) == 0)
    {
      //std::cout << "Chatroom is closed" << std::endl;
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
  int connection_id = connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress));

  if(connection_id == -1){
    printf("\x1b[31mConnection to the server couldn't be established\x1b[0m");
    return -1;
  }

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
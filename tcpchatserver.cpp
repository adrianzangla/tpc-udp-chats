// g++ your_file.c -lws2_32
#include <iostream>
#include <string>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>

#define USERNAME_LENGTH 8
#define PORT 60000

bool exitChat = false;

void sendData(SOCKET sock, std::string username, std::string ipAddress, int bufferSize)
{
    std::string header = username + " (" + ipAddress + ") dice: ";
    std::string message;
    std::string messageToSend;
    do
    {
        std::cout << "Mensaje: ";
        std::getline(std::cin, message);
        if (exitChat)
        {
            return;
        }
        exitChat = message == "exit";
        message = header + message;
        if (message.length() > bufferSize)
        {
            std::cerr << "El mensaje es demasiado largo" << std::endl;
            continue;
        }
        if (send(sock, message.c_str(), message.length(), 0) == SOCKET_ERROR)
        {
            if (!exitChat)
            {
                std::cerr << "send falló. Error: " << WSAGetLastError() << std::endl;
                closesocket(sock);
            }
            return;
        }
    } while (!exitChat);
    closesocket(sock);
}

void recvData(SOCKET sock, int bufferSize)
{
    char buffer[bufferSize];
    std::string message;
    std::string tail = ") dice: ";
    int bytesReceived;
    do
    {
        bytesReceived = recv(sock, buffer, bufferSize, 0);
        if (bytesReceived == SOCKET_ERROR)
        {
            if (!exitChat)
            {
                std::cerr << "recv falló Error: " << WSAGetLastError() << std::endl;
                closesocket(sock);
            }
            return;
        }
        message.assign(buffer, bytesReceived);
        std::cout << message << std::endl;
        exitChat = message.substr(message.find(tail) + tail.length()) == "exit";
    } while (!exitChat);
    std::cout << "El usuario " << message.substr(0, message.find(" dice: ")) << " ha abandonado la conversación" << std::endl;
    closesocket(sock);
}

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        std::cerr << "Uso: udpchat.exe <Nombre de usuario>" << std::endl;
        return 1;
    }

    std::string username(argv[1]);
    if (username.length() > USERNAME_LENGTH)
    {
        std::cerr << "El nombre de usuario es demasiado largo" << std::endl;
        return 1;
    }

    WSADATA wsaData;
    int iResult;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        std::cerr << "WSAStartup falló. Error: " << iResult << std::endl;
        return 1;
    }

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET)
    {
        std::cerr << "socket falló. Error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    int bufferSize;
    int bufferSizeSize = sizeof(bufferSize);
    if (getsockopt(server, SOL_SOCKET, SO_RCVBUF, (char *)&bufferSize, &bufferSizeSize) == SOCKET_ERROR)
    {
        std::cout << "getsockopt falló. Error: " << WSAGetLastError() << std::endl;
        closesocket(server);
        WSACleanup();
        return 1;
    }

    char hostName[256];
    if (gethostname(hostName, sizeof(hostName)) == SOCKET_ERROR)
    {
        std::cerr << "gethostname falló Error: " << WSAGetLastError() << std::endl;
        closesocket(server);
        WSACleanup();
        return 1;
    }

    std::string server_ip_address;
    hostent *host = gethostbyname(hostName);
    if (host != nullptr && host->h_addr_list[0] != nullptr)
    {
        server_ip_address = inet_ntoa(*(in_addr *)host->h_addr_list[0]);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip_address.c_str());
    server_addr.sin_port = htons(PORT);

    if (bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        std::cerr << "bind falló. Error: " << WSAGetLastError() << std::endl;
        closesocket(server);
        WSACleanup();
        return 1;
    }

    std::cout << "socket vinculado en " << server_ip_address << std::endl;

    if (listen(server, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "listen falló. Error: " << WSAGetLastError() << std::endl;
        closesocket(server);
        WSACleanup();
        return 1;
    }

    struct sockaddr client_addr;
    int client_addrlen = sizeof(client_addr);
    SOCKET client = accept(server, &client_addr, &client_addrlen);
    if (client == INVALID_SOCKET)
    {
        std::cerr << "accept falló. Error: " << WSAGetLastError() << std::endl;
    }

    std::thread sender(sendData, client, username, server_ip_address, bufferSize);
    std::thread receiver(recvData, client, bufferSize);

    if (sender.joinable())
    {
        sender.join();
    }
    if (receiver.joinable())
    {
        receiver.join();
    }

    closesocket(server);
    WSACleanup();
    return 0;
}
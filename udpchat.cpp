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

void sendData(SOCKET sock, const sockaddr *addr, std::string username, std::string ipAddress, int bufferSize)
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
        if (sendto(sock, message.c_str(), message.length(), 0, addr, sizeof(*addr)) == SOCKET_ERROR)
        {
            if (!exitChat)
            {
                std::cerr << "sendto falló. Error: " << WSAGetLastError() << std::endl;
                closesocket(sock);
                WSACleanup();
            }
            return;
        }
    } while (!exitChat);
    closesocket(sock);
    WSACleanup();
}

void recvData(SOCKET sock, sockaddr *addr, int *addr_len, int bufferSize)
{
    char buffer[bufferSize];
    std::string message;
    std::string tail = ") dice: ";
    int bytesReceived;
    do
    {
        bytesReceived = recvfrom(sock, buffer, bufferSize, 0, addr, addr_len);
        if (bytesReceived == SOCKET_ERROR)
        {
            if (!exitChat)
            {
                std::cerr << "recvfrom falló Error: " << WSAGetLastError() << std::endl;
                closesocket(sock);
                WSACleanup();
            }
            return;
        }
        message.assign(buffer, bytesReceived);
        std::cout << message << std::endl;
        exitChat = message.substr(message.find(tail) + tail.length()) == "exit";
    } while (!exitChat);
    std::cout << "El usuario " << message.substr(0, message.find(" dice: ")) << " ha abandonado la conversación" << std::endl;
    closesocket(sock);
    WSACleanup();
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

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "socket falló. Error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    int bufferSize;
    int bufferSizeSize = sizeof(bufferSize);
    if (getsockopt(sock, SOL_SOCKET, SO_MAX_MSG_SIZE, (char *)&bufferSize, &bufferSizeSize) == SOCKET_ERROR)
    {
        std::cerr << "setockopt falló. Error: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    char broadcastOption = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastOption, sizeof(broadcastOption)) == SOCKET_ERROR)
    {
        std::cerr << "setockopt falló. Error: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    char hostName[256];
    if (gethostname(hostName, sizeof(hostName)) == SOCKET_ERROR)
    {
        std::cerr << "gethostname falló Error: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::string sock_ip_address;
    hostent *host = gethostbyname(hostName);
    if (host != nullptr && host->h_addr_list[0] != nullptr)
    {
        sock_ip_address = inet_ntoa(*(in_addr *)host->h_addr_list[0]);
    }

    struct sockaddr_in sock_addr;
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = inet_addr(sock_ip_address.c_str());
    sock_addr.sin_port = htons(PORT);

    if (bind(sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == SOCKET_ERROR)
    {
        std::cerr << "bind falló. Error: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "socket enlazado en " << sock_ip_address << std::endl;

    sockaddr_in broadcast_addr;
    ZeroMemory(&broadcast_addr, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(PORT);
    broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    std::string message = "El usuario " + username + "(" + sock_ip_address + ")" + "se ha unido a la conversación";
    if (sendto(sock, message.c_str(), message.length(), 0, (const sockaddr *)&broadcast_addr, sizeof(broadcast_addr)) == SOCKET_ERROR)
    {
        std::cerr << "sendto falló. Error: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::thread sender(sendData, sock, (const sockaddr *)&broadcast_addr, username, sock_ip_address, bufferSize);
    int broadcast_addr_len = sizeof(broadcast_addr);
    std::thread receiver(recvData, sock, (sockaddr *)&broadcast_addr, &broadcast_addr_len, bufferSize);

    if (sender.joinable())
    {
        sender.join();
    }
    if (receiver.joinable())
    {
        receiver.join();
    }

    return 0;
}
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
                std::cerr << "sendto falló. Error: " << WSAGetLastError() << std::endl;
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
                std::cerr << "recvfrom falló Error: " << WSAGetLastError() << std::endl;
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

    if (argc < 3)
    {
        std::cerr << "Uso: udpchat.exe <Nombre de usuario> <Dirección IP del servidor>" << std::endl;
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

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "socket falló. Error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    int bufferSize;
    int bufferSizeSize = sizeof(bufferSize);
    if (getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&bufferSize, &bufferSizeSize) == SOCKET_ERROR)
    {
        std::cout << "getsockopt falló. Error: " << WSAGetLastError() << std::endl;
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

    std::cout << "socket vinculado en " << sock_ip_address << std::endl;

    sockaddr_in server_addr;
    ZeroMemory(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(argv[2]);

    if (connect(sock, (const sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        std::cerr << "connect falló. Error: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::string message = "El usuario " + username + "(" + sock_ip_address + ")" + "se ha unido a la conversación";

    if (send(sock, message.c_str(), message.length(), 0) == SOCKET_ERROR)
    {
        std::cerr << "send falló. Error: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::thread sender(sendData, sock, username, sock_ip_address, bufferSize);
    std::thread receiver(recvData, sock, bufferSize);

    if (sender.joinable())
    {
        sender.join();
    }
    if (receiver.joinable())
    {
        receiver.join();
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
// g++ your_file.c -lws2_32
#include <iostream>
#include <string>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <fstream>

#define USERNAME_LENGTH 8
#define PORT 60000

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        std::cerr << "Uso: udpchat.exe <Dirección IP del servidor>" << std::endl;
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
    sock_addr.sin_port = htons(60001);

    if (bind(sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == SOCKET_ERROR)
    {
        std::cerr << "bind falló. Error: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr;
    ZeroMemory(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(sock, (const sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        std::cerr << "connect falló. Error: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    char buffer[bufferSize];

    int bytesReceived = recv(sock, buffer, bufferSize, 0);
    if (bytesReceived == SOCKET_ERROR)
    {
        std::cerr << "recv falló. Error: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::string fileExtension(buffer, bytesReceived);

    std::ofstream file("received_file"+fileExtension, std::ifstream::binary);
    if (!file)
    {
        std::cerr << "Error creando el archivo" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    bytesReceived = recv(sock, buffer, bufferSize, 0);
    if (bytesReceived == SOCKET_ERROR)
    {
        std::cerr << "recv falló. Error: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    file.write(buffer, bytesReceived);
    file.close();

    closesocket(sock);
    WSACleanup();
    return 0;
}
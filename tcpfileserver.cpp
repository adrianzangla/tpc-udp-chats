// g++ your_file.c -lws2_32
#include <iostream>
#include <string>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <fstream>

#define PORT 60000

bool exitChat = false;

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        std::cerr << "Uso: udpchat.exe <Ruta del archivo>" << std::endl;
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

    std::cout << "socket enlazado en " << server_ip_address << std::endl;

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

    std::string fileName = argv[1];
    std::string fileExtension = fileName.substr(fileName.find("."));

    if (send(client, fileExtension.c_str(), fileExtension.length(), 0) == SOCKET_ERROR)
    {
        std::cerr << "send falló. Error: " << WSAGetLastError() << std::endl;
        closesocket(server);
        closesocket(client);
        WSACleanup();
        return 1;
    }

    std::ifstream file(fileName, std::ifstream::binary);
    if (!file)
    {
        std::cerr << "Error abriendo el archivo" << std::endl;
        closesocket(server);
        closesocket(client);
        WSACleanup();
        return 1;
    }

    file.seekg(0, std::ios::end);
    int fileSize = file.tellg();

    if (bufferSize < fileSize)
    {
        std::cerr << "El archivo es demasiado grande" << std::endl;
        closesocket(server);
        closesocket(client);
        WSACleanup();
        return 1;
    }

    char buffer[fileSize];

    file.seekg(0, std::ios::beg);
    file.read(buffer, fileSize);
    file.close();

    if (send(client, buffer, fileSize, 0) == SOCKET_ERROR)
    {
        std::cerr << "send falló. Error: " << WSAGetLastError() << std::endl;
        closesocket(server);
        closesocket(client);
        WSACleanup();
        return 1;
    }

    closesocket(server);
    closesocket(client);
    WSACleanup();
    return 0;
}
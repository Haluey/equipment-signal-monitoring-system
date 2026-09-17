#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <string>
#include <random>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "ws2_32.lib")

int main()
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cout << "WSAStartup failed" << std::endl;
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == INVALID_SOCKET)
    {
        std::cout << "Socket creation failed" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9000);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(
        serverSocket,
        reinterpret_cast<sockaddr*>(&serverAddr),
        sizeof(serverAddr)
    ) == SOCKET_ERROR)
    {
        std::cout << "Bind failed" << std::endl;

        closesocket(serverSocket);
        WSACleanup();

        return 1;
    }

    if (listen(serverSocket, 1) == SOCKET_ERROR)
    {
        std::cout << "Listen failed" << std::endl;

        closesocket(serverSocket);
        WSACleanup();

        return 1;
    }

    std::cout << "Equipment Simulator Server" << std::endl;
    std::cout << "Waiting for client on port 9000..." << std::endl;

    SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);

    if (clientSocket == INVALID_SOCKET)
    {
        std::cout << "Accept failed" << std::endl;

        closesocket(serverSocket);
        WSACleanup();

        return 1;
    }

    std::cout << "Client connected!" << std::endl;

    // 가상 장비 데이터 생성기
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<double> signalDist(0.0, 1.0);
    std::uniform_int_distribution<int> frequencyDist(1000, 1500);
    std::uniform_real_distribution<double> temperatureDist(25.0, 40.0);

    // 처음에는 정지 상태
    bool isRunning = false;

    while (true)
    {
        // 클라이언트 명령이 들어왔는지 확인
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(clientSocket, &readSet);

        timeval timeout{};
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int selectResult = select(
            0,
            &readSet,
            nullptr,
            nullptr,
            &timeout
        );

        if (selectResult == SOCKET_ERROR)
        {
            std::cout << "select() failed." << std::endl;
            break;
        }

        // 명령 수신
        if (selectResult > 0 && FD_ISSET(clientSocket, &readSet))
        {
            char commandBuffer[128]{};

            int received = recv(
                clientSocket,
                commandBuffer,
                sizeof(commandBuffer) - 1,
                0
            );

            if (received <= 0)
            {
                std::cout << "Client disconnected." << std::endl;
                break;
            }

            commandBuffer[received] = '\0';

            std::string command(commandBuffer);

            if (command.find("START") != std::string::npos)
            {
                isRunning = true;

                std::cout << "RX COMMAND: START" << std::endl;
            }
            else if (command.find("STOP") != std::string::npos)
            {
                isRunning = false;

                std::cout << "RX COMMAND: STOP" << std::endl;
            }
        }

        // START 상태일 때만 데이터 생성 및 전송
        if (isRunning)
        {
            double signal = signalDist(gen);
            int frequency = frequencyDist(gen);
            double temperature = temperatureDist(gen);

            std::ostringstream oss;

            oss << std::fixed << std::setprecision(2)
                << "Signal=" << signal
                << ";Frequency=" << frequency
                << ";Temperature=" << temperature
                << "\n";

            std::string message = oss.str();

            int sendResult = send(
                clientSocket,
                message.c_str(),
                static_cast<int>(message.size()),
                0
            );

            if (sendResult == SOCKET_ERROR)
            {
                std::cout << "Send failed." << std::endl;
                break;
            }

            std::cout << "TX: " << message;
        }
    }

    closesocket(clientSocket);
    closesocket(serverSocket);

    WSACleanup();

    return 0;
}
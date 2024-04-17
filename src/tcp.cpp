#include "tcp.h"
#include "D6T.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

void TCPServer::startServer()
{
    // Creazione del socket del server e configurazione dell'indirizzo
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        logError("Error: Failed to create socket.");
        return;
    }
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Utilizza INADDR_ANY per ascoltare su tutte le interfacce di rete

    // Binding del socket del server
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        logError("Error: Failed to bind socket.");
        close(serverSocket);
        return;
    }

    // Inizio dell'ascolto per le connessioni in arrivo
    if (listen(serverSocket, 5) == -1)
    {
        logError("Error: Failed to listen on socket.");
        close(serverSocket);
        return;
    }

    // Imposta il socket del server su non bloccante
    fcntl(serverSocket, F_SETFL, O_NONBLOCK);

    logMessage("Server started on port " + std::to_string(port));
}

void TCPServer::handleConnections()
{
    // Accettazione delle connessioni in arrivo e aggiunta delle socket dei client connessi alla lista
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (clientSocket == -1)
    {
        if (errno != EWOULDBLOCK)
        {
            logError("Error: Failed to accept client connection.");
        }
        return;
    }
    clientSockets.push_back(clientSocket);
    logMessage("Client connected: " + std::string(inet_ntoa(clientAddr.sin_addr)));

    // Invia dati ai client connessi
    sendTemperatureDataToAllClients();
}

void TCPServer::sendTemperatureDataToAllClients()
{
    // Copia la lista dei client connessi per evitare modifiche durante l'iterazione
    std::vector<int> connectedClients = clientSockets;

    // Invia dati di temperatura ai client connessi
    for (int clientSocket : connectedClients)
    {
        // Controlla se il socket del client è ancora valido
        if (std::find(clientSockets.begin(), clientSockets.end(), clientSocket) == clientSockets.end())
        {
            // Il client è stato chiuso, passa al prossimo client
            continue;
        }

        int bytesSent = send(clientSocket, reinterpret_cast<const char *>(pixData), N_PIXEL * sizeof(double), 0);
        if (bytesSent == -1)
        {
            logError("Error: Failed to send temperature data to client.");
            // Gestisci la chiusura del socket
            close(clientSocket);
            // Rimuovi il client dalla lista
            clientSockets.erase(std::remove(clientSockets.begin(), clientSockets.end(), clientSocket), clientSockets.end());
        }
        else
        {
            tcpMessageCounter++;
        }
    }
}

void TCPServer::sendTemperatureString(std::string temperatureString)
{
    // Copia la lista dei client connessi per evitare modifiche durante l'iterazione
    std::vector<int> connectedClients = clientSockets;

    // Invia dati di temperatura ai client connessi
    for (int clientSocket : connectedClients)
    {
        // Controlla se il socket del client è ancora valido
        if (std::find(clientSockets.begin(), clientSockets.end(), clientSocket) == clientSockets.end())
        {
            // Il client è stato chiuso, passa al prossimo client
            continue;
        }

        int bytesSent = send(clientSocket, temperatureString.c_str(), temperatureString.length(), 0);
        if (bytesSent == -1)
        {
            logError("Error: Failed to send temperature data to client.");
            // Gestisci la chiusura del socket
            close(clientSocket);
            // Rimuovi il client dalla lista
            clientSockets.erase(std::remove(clientSockets.begin(), clientSockets.end(), clientSocket), clientSockets.end());
        }
        else
        {
            tcpMessageCounter++;
        }
    }
}

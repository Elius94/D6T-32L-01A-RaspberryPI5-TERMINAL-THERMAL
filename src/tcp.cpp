#include "tcp.h"
#include "D6T.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <zlib.h> // For CRC32 calculation

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
    // Header
    uint32_t header = 0xaabbccdd;
    header = htonl(header);

    // Numero di pixel presenti
    uint32_t numPixels = N_PIXEL;
    numPixels = htonl(numPixels);

    // Numero di righe
    uint32_t numRows = N_ROW;
    numRows = htonl(numRows);

    // Dati di temperatura
    std::vector<uint32_t> temperatureData;
    // Popola temperatureData con i dati di temperatura
    for (int i = 0; i < N_PIXEL; i++)
    {
        uint32_t temp = htonl(static_cast<uint32_t>(pixData[i] * 100));
        temperatureData.push_back(temp);
    }

    // Calcola CRC32 dei dati
    unsigned long crc = crc32(0L, Z_NULL, 0); // Inizializza CRC32
    for (uint32_t temp : temperatureData)
    {
        crc = crc32(crc, reinterpret_cast<const Bytef *>(&temp), sizeof(temp));
    }
    crc = htonl(crc);

    // Creazione del buffer per l'invio
    std::vector<char> buffer(sizeof(header) + sizeof(numPixels) + sizeof(numRows) + temperatureData.size() * sizeof(uint32_t) + sizeof(crc));
    char *ptr = buffer.data();

    // Copia dei dati nel buffer
    memcpy(ptr, &header, sizeof(header));
    ptr += sizeof(header);

    memcpy(ptr, &numPixels, sizeof(numPixels));
    ptr += sizeof(numPixels);

    memcpy(ptr, &numRows, sizeof(numRows));
    ptr += sizeof(numRows);

    memcpy(ptr, temperatureData.data(), temperatureData.size() * sizeof(uint32_t));
    ptr += temperatureData.size() * sizeof(uint32_t);

    memcpy(ptr, &crc, sizeof(crc));

    // Invio dei dati a tutti i client connessi
    for (int clientSocket : clientSockets)
    {
        ssize_t bytesSent = send(clientSocket, buffer.data(), buffer.size(), 0);
        if (bytesSent == -1)
        {
            logError("Error: Failed to send temperature data to client.");
        }
        else if (static_cast<size_t>(bytesSent) != buffer.size())
        {
            logError("Error: Incomplete temperature data sent to client.");
        }
        else
        {
            // logMessage("Sent temperature data to client.");
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

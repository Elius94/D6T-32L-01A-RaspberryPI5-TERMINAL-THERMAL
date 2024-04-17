#ifndef TCP_H
#define TCP_H

#include <iostream>
#include <string>
#include <vector>
#include <unistd.h> // Includi questo header per la funzione close
#include <sys/socket.h>

class TCPServer
{
public:
    TCPServer(const int &port, const double *pixData)
        : port(port), pixData(pixData), serverSocket(-1){};

    ~TCPServer()
    {
        stopServer();
    }

    // Funzione per inizializzare il server TCP
    void startServer();

    inline void stopServer()
    {
        for (int clientSocket : clientSockets)
        {
            // send shutdown before closing the socket
            shutdown(clientSocket, SHUT_RDWR);
            sleep(1);
            close(clientSocket);
        }

        // Svuota il vettore delle connessioni dei client
        clientSockets.clear();

        // Chiudi il socket del server
        close(serverSocket);

        logMessage("Server stopped.");
    }

    // Funzione per gestire le connessioni dei client e l'invio dei dati
    void handleConnections();
    // Funzione per inviare i dati di temperatura ai client connessi
    void sendTemperatureDataToAllClients();
    void sendTemperatureString(std::string temperatureString);

    void getLogs(std::vector<std::string> &errors, std::vector<std::string> &messages)
    {
        errors = this->errors;
        messages = this->messages;
    }

    bool getIsRunning() const
    {
        return isRunning;
    }

    void logError(const std::string &error)
    {
        errors.push_back(error);
    }

    void logMessage(const std::string &message)
    {
        messages.push_back(message);
    }

    uint64_t getTcpMessageCounter() const
    {
        return tcpMessageCounter;
    }

    int getConnectedClients() const
    {
        return clientSockets.size();
    }

private:
    int port;
    const double *pixData;
    int serverSocket;
    std::vector<int> clientSockets;

    bool isRunning = true;
    uint64_t tcpMessageCounter = 0;
    std::vector<std::string> errors;
    std::vector<std::string> messages;
};

#endif // TCP_H
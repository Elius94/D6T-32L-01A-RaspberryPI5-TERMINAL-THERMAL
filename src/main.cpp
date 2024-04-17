#include <iostream>
#include <csignal> // for signal handling
#include <cstdlib> // for std::system
#include <chrono>
#include <thread>
#include <iomanip>
#include "main.h"

// Global flag to indicate if the program should continue running
bool running = true;

// Signal handler function to handle termination signals
void signalHandler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM)
    {
        running = false;
    }
}

int main(int argc, char *argv[])
{
    // Register signal handler
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    TempSensor *sensor = new TempSensor();

    int port = 3000;
    for (int i = 1; i < argc - 1; i++)
    {
        std::string arg = argv[i];
        if (arg == "--port")
        {
            port = std::stoi(argv[i + 1]);
            break;
        }
    }

    bool gui = false;
    // controlla se tra gli argomenti è presente la stringa "gui"
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--gui")
        {
            gui = true;
            break;
        }
    }

    // controlla se tra gli argomenti è presente la stringa "tcpImage"
    bool tcpImage = false;
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--tcpImage")
        {
            tcpImage = true;
            break;
        }
    }

    TCPServer server(port, sensor->getPixData());
    server.startServer();

    auto nextMeasurementTime = std::chrono::steady_clock::now();
    auto nextConnectionHandlingTime = std::chrono::steady_clock::now();
    auto drawGuiTime = std::chrono::steady_clock::now();
    std::string img = "";

    while (running)
    {
        auto currentTime = std::chrono::steady_clock::now();

        if (currentTime >= nextMeasurementTime)
        {
            nextMeasurementTime = currentTime + std::chrono::milliseconds(200);

            sensor->getMeasure();
            if (gui || tcpImage)
                img = sensor->getTemperatureImageString();

            // Invia i dati di temperatura a tutti i client
            if (tcpImage)
                server.sendTemperatureString(img);
            else
                server.sendTemperatureDataToAllClients();
        }

        if (currentTime >= nextConnectionHandlingTime)
        {
            server.handleConnections();
            nextConnectionHandlingTime = currentTime + std::chrono::milliseconds(1);
        }

        if (gui && currentTime >= drawGuiTime)
        {
            drawGuiTime = currentTime + std::chrono::milliseconds(200);
            // Ottieni gli errori e i messaggi dal server
            std::vector<std::string> errors, messages;
            server.getLogs(errors, messages);

            // Pulisce lo schermo
            std::system("clear");

            // Stampa l'immagine termica
            std::cout << img << std::endl;

            // stampa runningstatus, client connected e tcp message counter
            std::cout << "Running: " << (server.getIsRunning() ? "\033[32mYes\033[0m" : "\033[31mNo\033[0m") << " | "
                      << "Connected Clients: " << server.getConnectedClients() << " | "
                      << "TCP Message Counter: " << server.getTcpMessageCounter() << std::endl;

            // add space
            std::cout << std::endl;

            // Stampa gli ultimi 20 errori e messaggi accanto all'immagine
            std::cout << "\033[33m"
                      << "Server Logs:"
                      << "\033[0m" << std::endl;

            // Stampa "Errors" e "Messages" come intestazioni
            std::cout << "\033[31m"
                      << "Errors"
                      << "\033[0m";
            for (size_t i = 0; i < 50; ++i)
            {
                std::cout << " ";
            }
            std::cout << "\033[32m"
                      << "Messages"
                      << "\033[0m" << std::endl;

            // Stampa fino a 20 errori e messaggi
            size_t maxMessagesToShow = 20;
            size_t maxErrorsToShow = 20;
            size_t maxLinesToShow = std::max(maxMessagesToShow, maxErrorsToShow);

            for (size_t i = 0; i < maxLinesToShow; ++i)
            {
                // Stampare errore se disponibile
                if (i < errors.size())
                {
                    int errorIndex = errors.size() - i - 1; // Correggi l'indice
                    std::cout << "\033[31m" << errors[errorIndex] << "\033[0m";
                    // Calcolare la lunghezza dell'errore e riempire con spazi fino a 60 caratteri
                    std::string &error = errors[errorIndex];
                    size_t errorLength = error.length();
                    for (size_t j = errorLength; j < 60; ++j)
                    {
                        std::cout << " ";
                    }
                }
                else
                {
                    // Se non ci sono più errori, riempire la colonna con spazi
                    for (size_t j = 0; j < 60; ++j)
                    {
                        std::cout << " ";
                    }
                }

                // Stampare messaggio se disponibile
                if (i < messages.size())
                {
                    int messageIndex = messages.size() - i - 1;                     // Correggi l'indice
                    std::cout << "\033[32m" << messages[messageIndex] << "\033[0m"; // Correggi l'indice
                }

                std::cout << std::endl;
            }
        }

        // Verifica se è stato ricevuto un segnale di interruzione
        if (!running)
        {
            break;
        }
    }
    std::cout << "Exiting program..." << std::endl;
    server.stopServer();

    delete sensor;
    return 0;
}

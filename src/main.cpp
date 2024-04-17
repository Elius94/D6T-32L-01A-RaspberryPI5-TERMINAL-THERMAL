#include <iostream>
#include <csignal> // for signal handling
#include "main.h"
#include <cstdlib> // for std::system

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

    std::cout << "Reading Temperature From D6T I2C Sensor" << std::endl;
    TempSensor sensor = TempSensor();

    while (running)
    {
        double temperature = sensor.getTemperature();
        sensor.printTemperatureImageInTerminal();
    }

    return 0;
}

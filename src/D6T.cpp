#include "D6T.h"
#include <unistd.h>
#include <iostream>

// Costruttore
TempSensor::TempSensor()
{
    delay(350);
    printf("D6T-32L-01A I2C example\n");
    // 1. Initialize
    initialSetting();
    printf("Initialization done\n");
    delay(390);
}

// Distruttore
TempSensor::~TempSensor()
{
}

double TempSensor::getTemperature()
{
    // printf("Reading data\n");
    int i;
    int16_t itemp;
    // Read data via I2C
    memset(rbuf, 0, N_READ);
    for (i = 0; i < 10; i++)
    {
        uint32_t ret = i2c_read_reg8(D6T_ADDR, D6T_CMD, rbuf, N_READ);
        if (ret == 0)
        {
            break;
        }
        else if (ret == 23)
        { // write error
            delay(60);
        }
        else if (ret == 24)
        { // read error
            delay(60);
        }
    }
    // printf("Read done\n");
    checkPEC(rbuf, N_READ - 1);
    // printf("PEC check done\n");

    // Convert to temperature data (degC)
    ptat = (double)conv8us_s16_le(rbuf, 0) / 10.0;
    for (i = 0; i < N_PIXEL; i++)
    {
        itemp = conv8us_s16_le(rbuf, 2 + 2 * i);
        pix_data[i] = (double)itemp / 10.0;
    }

    // Output results
    // printf("PTAT: %4.1f [degC], Temperature: ", ptat);
    // for (i = 0; i < N_PIXEL; i++)
    // {
    //     printf("%4.1f, ", pix_data[i]);
    // }
    // printf("[degC]\n");

    delay(200);
    return ptat;
}

void TempSensor::printTemperatureImageInTerminal() {
    // Clear the shell
    system("clear");

    // Define the temperature range
    const double minTemp = 0.0;
    const double maxTemp = 200.0;

    // Print the temperature image
    for (int i = 0; i < N_PIXEL; i++) {
        // Normalize the temperature value to the range [0, 1]
        double normalizedTemp = 1.0 - ((pix_data[i] - minTemp) / (maxTemp - minTemp));

        // Map the normalized temperature value to the ANSI color range [231, 16]
        int colorIndex = 231 - round(normalizedTemp * 215);

        // Set the color of the terminal output using ANSI escape sequences
        std::cout << "\033[48;5;" << colorIndex << "m  "; // Background color

        // If we reached the end of a row, move to the next line
        if ((i + 1) % 32 == 0) {
            std::cout << "\033[0m\n"; // Reset color and move to next line
        }
    }

    // dopo il disegno, stampa il valore della temperatura PTAT e il max e min dei pixel

    double max = *std::max_element(pix_data, pix_data + N_PIXEL);
    double min = *std::min_element(pix_data, pix_data + N_PIXEL);
    std::cout << "PTAT: " << ptat << " [degC]";
    std::cout << " Max: " << max << " [degC] Min: " << min << " [degC]";
    std::cout << std::endl;
}

uint32_t TempSensor::i2c_read_reg8(uint8_t devAddr, uint8_t regAddr,
                                   uint8_t *data, short unsigned int length)
{
    int fd = open(I2CDEV, O_RDWR);
    if (fd < 0)
    {
        fprintf(stderr, "Failed to open device: %s\n", strerror(errno));
        return 21;
    }
    int err = 0;
    do
    {
        struct i2c_msg messages[] = {
            {devAddr, 0, 1, &regAddr},
            {devAddr, I2C_M_RD, length, data},
        };
        struct i2c_rdwr_ioctl_data ioctl_data = {messages, 2};
        if (ioctl(fd, I2C_RDWR, &ioctl_data) != 2)
        {
            fprintf(stderr, "i2c_read: failed to ioctl: %s\n", strerror(errno));
        }

    } while (false);
    close(fd); // change
    return err;
}

uint32_t TempSensor::i2c_write_reg8(uint8_t devAddr,
                                    uint8_t *data, int length)
{
    int fd = open(I2CDEV, O_RDWR);
    if (fd < 0)
    {
        fprintf(stderr, "Failed to open device: %s\n", strerror(errno));
        return 21;
    }
    int err = 0;
    do
    {
        if (ioctl(fd, I2C_SLAVE, devAddr) < 0)
        {
            fprintf(stderr, "Failed to select device: %s\n", strerror(errno));
            err = 22;
            break;
        }
        if (write(fd, data, length) != length)
        {
            fprintf(stderr, "Failed to write reg: %s\n", strerror(errno));
            err = 23;
            break;
        }
    } while (false);
    close(fd);
    return err;
}

uint8_t TempSensor::calc_crc(uint8_t data)
{
    int index;
    uint8_t temp;
    for (index = 0; index < 8; index++)
    {
        temp = data;
        data <<= 1;
        if (temp & 0x80)
        {
            data ^= 0x07;
        }
    }
    return data;
}

/** <!-- D6T_checkPEC {{{ 1--> D6T PEC(Packet Error Check) calculation.
 * calculate the data sequence,
 * from an I2C Read client address (8bit) to thermal data end.
 */
bool TempSensor::checkPEC(uint8_t buf[], int n)
{
    int i;
    uint8_t crc = calc_crc((D6T_ADDR << 1) | 1); // I2C Read address (8bit)
    for (i = 0; i < n; i++)
    {
        crc = calc_crc(buf[i] ^ crc);
    }
    bool ret = crc != buf[n];
    if (ret)
    {
        fprintf(stderr,
                "PEC check failed: %02X(cal)-%02X(get)\n", crc, buf[n]);
    }
    return ret;
}

/** <!-- conv8us_s16_le {{{1 --> convert a 16bit data from the byte stream.
 */
int16_t TempSensor::conv8us_s16_le(uint8_t *buf, int n)
{
    uint16_t ret;
    ret = (uint16_t)buf[n];
    ret += ((uint16_t)buf[n + 1]) << 8;
    return (int16_t)ret; // and convert negative.
}

void TempSensor::delay(int msec)
{
    struct timespec ts = {.tv_sec = msec / 1000,
                          .tv_nsec = (msec % 1000) * 1000000};
    nanosleep(&ts, NULL);
}

void TempSensor::initialSetting(void)
{
    uint8_t dat1[] = {D6T_SET_ADD, (((uint8_t)D6T_IIR << 4) && 0xF0) | (0x0F && (uint8_t)D6T_AVERAGE)};
    i2c_write_reg8(D6T_ADDR, dat1, sizeof(dat1));
}
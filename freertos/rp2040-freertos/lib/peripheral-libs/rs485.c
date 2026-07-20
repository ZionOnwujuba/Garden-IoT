#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "rs485.h"
#include "constants.h"


/*
1. The Request Packet Sequence (Pico W → Sensor)
The 8 bytes you send (Com[8]) ask the sensor to deliver the data:
Byte Index | Hex Value | Name | Description
Com[0] | 0x01 | Device Address | Targets the sensor matching ID #1.
Com[1] | 0x03 | Function Code | 0x03 instructs the sensor to "Read Holding Registers".
Com[2] | 0x00 | Starting Address (High) | High byte of the target memory register (0x00).
Com[3] | 0x00 | Starting Address (Low) | Low byte of the target memory register (0x00).
Com[4] | 0x00 | Quantity (High) | High byte of how many registers to read (0x00).
Com[5] | 0x04 | Quantity (Low) | Low byte asking for 4 registers total (8 bytes of data).
Com[6] | 0x44 | CRC Checksum (Low) | Error checking code byte 1.
Com[7] | 0x09 | CRC Checksum (High) | Error checking code byte 2.2. 

The Response Packet Sequence (Sensor → Pico W)
Your nested if statements act as a literal scanner, checking off each step of the incoming 
13-byte response sequence:[0x01] -> [0x03] -> [0x08] -> [HUM_H] [HUM_L] -> [TEM_H] [TEM_L] -> 
    [RAW_H] [RAW_L] -> [PH_H] [PH_L] -> [CRC_L] [CRC_H]
Here is exactly how those 13 bytes map to your Data array:
Data Index | Expected Byte / Value | Name | Description
Data[0] | 0x01 | Slave ID | The sensor confirms: "Yes, I am device ID #1 speaking."
Data[1] | 0x03 | Function Code | Confirms it is replying to your "Read" command.
Data[2] | 0x08 | Byte Count | Tells the Pico: "I am handing you 8 bytes of data next."
Data[3] | Variable (High) | Humidity High | Combined with the low byte to build the humidity integer.
Data[4] | Variable (Low) | Humidity Low | Final half of the humidity value.
Data[5] | Variable (High) | Temperature High | Combined with the low byte to build the temperature integer.
Data[6] | Variable (Low) | Temperature Low | Final half of the temperature value.
Data[7] | Variable (High) | Reserved High | Register #3 data payload (often ignored or internal calibration).
Data[8] | Variable (Low) | Reserved Low | Register #3 data payload.
Data[9] | Variable (High) | pH High | Combined with the low byte to build the pH integer.
Data[10] | Variable (Low) | pH Low | Final half of the pH value.
Data[11] | Checksum Byte 1 | CRC Low | Error validation signature.
Data[12] | Checksum Byte 2 | CRC High | Error validation signature.

How Your Code Validates This Sequence
    Your nested if structure matches this sequence byte-by-byte like a combination lock:
        It waits for 0x01. If it matches, it saves it to Data[0].
        It looks for 0x03. If it matches, it saves it to Data[1].
        It looks for 0x08 (the data payload size indicator). If it matches, it saves it to Data[2].
        Knowing 8 bytes of data plus 2 bytes of CRC are left, it safely grabs all 
            remaining 10 bytes at once (readN(&Data[3], 10)).
        It runs the complete 13-byte package through your CRC16_2 math function to ensure the 
            wire line didn't introduce static or corrupted data before saving the final readings.
*/
// Global Variables
uint8_t Com[8] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x04, 0x44, 0x09 };



// Forward Declarations

/*
int main() {
    // Initialize standard I/O (allows printf over USB serial to your PC)
    stdio_init_all();

    // Initialize UART0 at 9600 baud
    uart_init(UART_ID_RS485, BAUDRATE_RS485);
    gpio_set_function(UART_TX_PIN_RS485, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN_RS485, GPIO_FUNC_UART);
    uart_set_format(UART_ID_RS485, 8, 1, UART_PARITY_NONE); // 8N1 standard Modbus format

    // Pause briefly to give the PC serial monitor time to catch up on boot
    sleep_ms(2000);

    // Application Loop
    while (true) {
        rs485_data rsdata;
        readHumiturePH(&rsdata);
        float fahrenheit = (rsdata.tem * 9 / 5) + 32;
        printf("TEM = %.1f °C (%.1f °F)  HUM = %.1f %%RH  PH = %.1f\n", rsdata.tem, fahrenheit, rsdata.hem, rsdata.ph);
        sleep_ms(1000); // delay(1000)
    }
}
*/

void rs485_init(void){
    // Initialize UART0 at 9600 baud
    uart_init(UART_ID_RS485, BAUDRATE_RS485);
    gpio_set_function(UART_TX_PIN_RS485, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN_RS485, GPIO_FUNC_UART);
    uart_set_format(UART_ID_RS485, 8, 1, UART_PARITY_NONE); // 8N1 standard Modbus format
}

void readHumiturePH(rs485_data *result) {
    uint8_t Data[16] = { 0 }; // Sized with safety padding
    uint8_t ch = 0;
    bool flag = true;

    while (flag) {
        sleep_ms(100); // delay(100)

        // Clear any stale remnants out of the RX hardware buffer before sending
        while (uart_is_readable(UART_ID_RS485)) {
            uart_getc(UART_ID_RS485); 
        }

        // Send out the 8-byte Modbus command
        // The hardware shield handles the auto-switching transmission lines instantly
        for (int i = 0; i < 8; i++) {
            uart_putc(UART_ID_RS485, Com[i]);
        }
        
        // Block until all characters leave the Pico's internal hardware transmission FIFO
        uart_tx_wait_blocking(UART_ID_RS485);

        // Give the sensor a small window to receive the command and begin talking back
        sleep_ms(20); 

        // Read and match the incoming packet structural sequence
        if (readN(&ch, 1) == 1) {
            if (ch == 0x01) {
                Data[0] = ch;
                if (readN(&ch, 1) == 1) {
                    if (ch == 0x03) {
                        Data[1] = ch;
                        if (readN(&ch, 1) == 1) {
                            if (ch == 0x08) { // Expecting 8 data bytes payload
                                Data[2] = ch;
                                if (readN(&Data[3], 10) == 10) { // 8 data bytes + 2 CRC bytes
                                    if (CRC16_2(Data, 11) == (Data[11] * 256 + Data[12])) {
                                        result->hem = (Data[3] * 256 + Data[4]) / 10.00f;
                                        result->tem = (Data[5] * 256 + Data[6]) / 10.00f;
                                        result->ph  = (Data[9] * 256 + Data[10]) / 10.00f;
                                        flag = false; // Valid packet parsed, break out of loop
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

uint8_t readN(uint8_t *buf, size_t len) {
    size_t offset = 0;
    size_t left = len;
    int32_t Timeout = 500;
    uint8_t *buffer = buf;
    
    // Track execution duration using the Pico SDK high-resolution timer
    absolute_time_t start_time = get_absolute_time();

    while (left) {
        if (uart_is_readable(UART_ID_RS485)) {
            buffer[offset] = uart_getc(UART_ID_RS485); 
            offset++;
            left--;
        }
        
        // Calculate timeout window in milliseconds
        if (absolute_time_diff_us(start_time, get_absolute_time()) / 1000 > Timeout) {
            break;
        }
    }
    return offset;
}
/*
A 12-bit Cyclic Redundancy Check (CRC) is an error-detecting code used to verify 
    that data has not been corrupted during transmission or storage. It works by 
    treating the data as a massive binary number, dividing it by a predefined 
    13-bit binary key (the generator polynomial), and appending the resulting 
    12-bit remainder to the end of the data. The receiver repeats this division; 
    if the final remainder is zero, the data is assumed error-free.
    
The Step-by-Step Process
    A 12-bit CRC uses a 12-bit remainder, which means the divisor (the generator 
    polynomial) is exactly 13 bits long (degree 12).
    
        1. Preparation at the Sender
            Pad the Data: The sender takes the original data stream and appends 
                exactly 12 zeros to the end of it.
            Binary Division: The sender performs modulo-2 division (using XOR 
                operations) on this padded data, dividing it by the agreed-upon 13-bit divisor.
            Get the Checksum: The remainder of this division process is exactly 12 bits long. 
                This 12-bit remainder is the CRC checksum.
            Transmit: The sender replaces the appended zeros with the 12-bit CRC checksum and 
                sends the combined message to the receiver.
        2. Verification at the Receiver
            Recalculation: The receiver takes the entire received message (original data + 12-bit CRC) 
                and divides it by the exact same 13-bit divisor using modulo-2 division.
            Check the Remainder: If the remainder is all zeros, the data is accepted as error-free. 
                If the remainder has any ones in it, the receiver knows a corruption occurred and 
                can request the data to be resent.
*/
unsigned int CRC16_2(unsigned char *buf, int len) {
    unsigned int crc = 0xFFFF; // Initialize with 0xFFFF
    for (int pos = 0; pos < len; pos++) {
        crc ^= (unsigned int)buf[pos]; // XOR byte into least significant byte of crc
        for (int i = 8; i != 0; i--) { // Loop over each bit
            if ((crc & 0x0001) != 0) { // If the LSB is set
                crc >>= 1;
                crc ^= 0xA001; // Shift right and XOR with reversed polynomial
            } else {
                crc >>= 1; // Just shift right
            }
        }
    }

    crc = ((crc & 0x00ff) << 8) | ((crc & 0xff00) >> 8);
    return crc;
}
